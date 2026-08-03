# Система ошибок в TriangulationLab

Документ фиксирует текущее состояние обработки ошибок в проекте и предлагает первую целевую модель, которую можно постепенно внедрить без резкого рефакторинга.[file:13][file:14]

## Зачем нужен единый подход

Сейчас ошибки проходят через несколько уровней в виде строковых сообщений: в ядре алгоритмов через `result.error_message`, а во facade-слое через Qt-сигнал `errorOccurred(...)`.[file:11][file:12][file:14]
Такой подход работает для первых итераций, но плохо масштабируется: строковые сообщения трудно стабильно использовать в UI, тестах и логировании, а `CLAUDE.md` прямо отмечает необходимость разделить ошибки ввода, алгоритмические сбои и нарушения внутренних инвариантов.[file:13]

## Текущее состояние

### Источники ошибок

| Слой | Текущее поведение | Примечание |
|---|---|---|
| `TriangulationFacade` | Вызывает `emit errorOccurred("...")` для ошибок пользовательского сценария и пробрасывает `result.error_message` из ядра.[file:14] | UI получает только текст, без машинного кода ошибки.[file:14] |
| `EarClipping` | Возвращает `TriangulationResult` с `error_message`.[file:11] | Ошибка описывается только строкой.[file:11] |
| `MonotoneTriangulation` | Возвращает `TriangulationResult` с `error_message`.[file:12] | Ошибка описывается только строкой.[file:12] |
| Внутренние проверки | Используются `GEO_ASSERT_INVARIANT` и `assert(...)` для проверки внутренних предположений алгоритма.[file:12][file:13] | Это уже не пользовательские ошибки, а признаки бага или нарушения инварианта.[file:13] |

### Уже существующие сообщения

| Где возникает | Текущий текст | Файл |
|---|---|---|
| Закрытие полигона при числе точек < 3 | `Need at least 3 points to close polygon` [file:14] | `TriangulationFacade.cpp` |
| После удаления коллинеарных точек осталось < 3 вершин | `Polygon is degenerate after cleaning` [file:14] | `TriangulationFacade.cpp` |
| Запуск триангуляции без замкнутого корректного полигона | `Polygon must be closed and have at least 3 vertices` [file:14] | `TriangulationFacade.cpp` |
| Входной полигон содержит < 3 вершин | `Polygon must have at least 3 vertices` [file:11][file:12] | `ear_clipping.hpp`, `monotone_triangulation.hpp` |
| Входной полигон самопересекается | `Polygon is self-intersecting` [file:11][file:12] | `ear_clipping.hpp`, `monotone_triangulation.hpp` |
| В ear clipping не удалось найти ухо даже после ослабления epsilon | `No ear found in polygon (possibly degenerate or self-intersecting)` [file:11] | `ear_clipping.hpp` |
| В monotone-разбиении не удалось получить y-монотонные части | `Failed to decompose polygon into monotone pieces` [file:12] | `monotone_triangulation.hpp` |
| Не удалась триангуляция y-монотонного куска | `Monotone triangulation failed` [file:12] | `monotone_triangulation.hpp` |

## Категории ошибок

Для проекта естественно выделить три категории ошибок, и это совпадает с TODO в `CLAUDE.md`.[file:13]

| Категория | Смысл | Что делать в UI | Что делать в логике |
|---|---|---|---|
| `Input` | Пользователь или вызывающий код передал некорректные данные, например слишком мало точек или самопересекающийся полигон.[file:11][file:12][file:14] | Показать понятное сообщение пользователю.[file:14] | Не считать это багом, не падать, дать исправить входные данные. |
| `Algorithm` | Вход формально допустим, но конкретный алгоритм не смог завершить обработку или столкнулся с вырожденным случаем.[file:11][file:12] | Показать нейтральную ошибку с возможностью попробовать другой алгоритм или скорректировать полигон. | Сохранять диагностический контекст: алгоритм, число вершин, epsilon, этап обработки. |
| `Internal` | Нарушен внутренний инвариант или предположение реализации, то есть это уже дефект кода, а не ошибка пользователя.[file:12][file:13] | Пользователю не показывать технические детали; вместо этого выдавать безопасное сообщение. | Логировать максимально подробно, в debug-сборке можно падать через assert. |

## Предлагаемый контракт ошибки

Первый практичный шаг — ввести единый тип ошибки, который смогут использовать и core, и facade, а UI уже будет решать, как его отобразить.[file:13][file:14]

```cpp
#pragma once

#include <string>
#include <optional>

namespace geo {

enum class ErrorCategory {
    Input,
    Algorithm,
    Internal
};

enum class ErrorCode {
    NotEnoughVertices,
    PolygonNotClosed,
    DegeneratePolygon,
    SelfIntersectingPolygon,
    EarNotFound,
    MonotoneDecompositionFailed,
    MonotonePieceTriangulationFailed,
    InvariantViolation,
    Unknown
};

struct TriangulationError {
    ErrorCategory category = ErrorCategory::Unknown; // см. замечание ниже
    ErrorCode code = ErrorCode::Unknown;
    std::string user_message;
    std::string debug_message;
    std::string algorithm;
};

} // namespace geo
```

В этой заготовке есть одна маленькая неточность: если использовать только три категории `Input`, `Algorithm`, `Internal`, то для значения по умолчанию лучше либо убрать инициализацию, либо добавить `Unknown` и в `ErrorCategory` тоже. Это хороший пример того, почему сам контракт нужно сначала описать документом, а потом уже аккуратно переносить в код.

Более аккуратный вариант:

```cpp
enum class ErrorCategory {
    Input,
    Algorithm,
    Internal,
    Unknown
};
```

## Предлагаемая таблица кодов ошибок

| Код | Категория | Где сейчас возникает | Сообщение для пользователя | Сообщение для отладки |
|---|---|---|---|---|
| `NotEnoughVertices` | `Input` | Закрытие полигона и запуск алгоритма при числе вершин < 3.[file:14][file:11][file:12] | Полигон должен содержать минимум 3 вершины. | Недостаточно вершин для операции triangulation/close polygon. |
| `PolygonNotClosed` | `Input` | Попытка триангуляции до замыкания полигона.[file:14] | Сначала нужно замкнуть полигон. | Facade received triangulation request for non-closed polygon. |
| `DegeneratePolygon` | `Input` | После `removeCollinearPoints` полигон стал вырожденным.[file:14] | Полигон вырожден после удаления коллинеарных точек. | Polygon became degenerate after preprocessing. |
| `SelfIntersectingPolygon` | `Input` | Проверка `isSimplePolygon(...)` в обоих алгоритмах.[file:11][file:12] | Полигон не должен иметь самопересечений. | Input polygon is not simple. |
| `EarNotFound` | `Algorithm` | Ear clipping не смог найти ухо даже после повторной попытки с ослабленным `epsilon`.[file:11][file:13] | Не удалось триангулировать полигон методом ear clipping. | No ear found; polygon may be degenerate or numerically unstable. |
| `MonotoneDecompositionFailed` | `Algorithm` | `makeMonotone(...)` вернул `false`.[file:12] | Не удалось разбить полигон на y-монотонные части. | Monotone decomposition failed; left edge/helper not found or degenerate sweep case. |
| `MonotonePieceTriangulationFailed` | `Algorithm` | `triangulateMonotonePiece(...)` вернул `false`.[file:12] | Не удалось завершить триангуляцию монотонного куска. | Monotone piece triangulation failed. |
| `InvariantViolation` | `Internal` | `GEO_ASSERT_INVARIANT`, `assert(status.empty())`, `assert(helpers.empty())`.[file:12][file:13] | Внутренняя ошибка алгоритма. | Invariant violated; likely implementation bug. |

## Правила по слоям

### Core

Ядро не должно зависеть от Qt и не должно решать, как показывать ошибку пользователю, что соответствует текущей архитектуре проекта.[file:13]
Поэтому core-слой должен возвращать не просто строку, а структурированную ошибку, например через `std::optional<TriangulationError>` внутри `TriangulationResult` или через `expected`-подобный тип, если позже захочется сделать контракт строже.[file:11][file:12][file:13]

Пример направления:

```cpp
struct TriangulationResult {
    bool success = false;
    std::vector<Triangle2> triangles;
    std::optional<TriangulationError> error;
};
```

### Facade

Facade-слой должен быть местом, где ошибка переводится из формата ядра в формат UI, потому что именно он уже отвечает за переключение алгоритмов, состояние полигона и проброс данных в интерфейс.[file:13][file:14]
На этом уровне можно либо оставить старый сигнал `errorOccurred(QString)` для обратной совместимости, либо добавить новый сигнал `errorOccurred(TriangulationErrorView)` и временно поддерживать оба варианта.[file:14]

Пример переходного шага:

```cpp
struct TriangulationErrorView {
    ErrorCode code;
    QString userMessage;
    QString debugMessage;
    QString algorithm;
};
```

### UI

UI должен показывать только `userMessage` и, возможно, код ошибки в статус-баре или debug-панели, но не сырые технические сообщения из ядра.[file:13][file:14]
Технические детали полезнее направлять в лог или в отладочную панель, особенно для категории `Internal`, где пользователю важен факт сбоя, а не детали реализации sweep-line или монотонных кусков.[file:12][file:13]

## Стратегия внедрения по шагам

### Этап 1. Зафиксировать словарь ошибок

Сначала стоит оставить текущую реализацию почти без изменений и просто завести единый список кодов ошибок и таблицу соответствия текущих строк этим кодам.[file:11][file:12][file:14]
Это самый безопасный шаг, потому что он не ломает UI и тесты, но уже убирает хаос в терминологии.[file:13]

### Этап 2. Расширить `TriangulationResult`

Дальше имеет смысл добавить в `TriangulationResult` поле `error`, а `error_message` временно оставить только для обратной совместимости.[file:11][file:12]
После этого алгоритмы смогут возвращать и машинный код, и человекочитаемое сообщение, и диагностический контекст.[file:13]

### Этап 3. Адаптировать facade

Затем `TriangulationFacade` должен перестать зависеть от свободного текста из ядра и начать работать через `error.code`, `error.category` и `error.user_message`.[file:14]
На этом этапе удобно централизовать перевод ошибок в сообщения UI и решить, какие ошибки показываются пользователю, а какие только логируются.[file:13]

### Этап 4. Доработать тесты

После появления кодов ошибок тесты смогут проверять не конкретные строки, а устойчивые `ErrorCode`, что надёжнее и проще в сопровождении.[file:13]
Особенно полезно добавить тесты на различение `Input`, `Algorithm` и `Internal`, потому что это прямо указано как желаемое направление развития проекта.[file:13]

## Практические замечания

Стоит унифицировать формулировки уже сейчас: например, сейчас есть два похожих сообщения про минимум вершин — одно в facade (`Need at least 3 points to close polygon`), другое в core (`Polygon must have at least 3 vertices`).[file:14][file:11][file:12]
Для пользователя лучше иметь один согласованный вариант текста, а различия между сценариями хранить в коде ошибки и отладочном сообщении.[file:13]

Также полезно заранее решить судьбу debug-only проверок: `GEO_ASSERT_INVARIANT` и `assert(...)` не должны silently превращаться в пользовательские ошибки, потому что по смыслу это индикаторы дефекта алгоритма, а не плохого ввода.[file:12][file:13]
Оптимальная схема обычно такая: в debug-сборке — assert, в release-сборке — безопасный `Internal`-результат с логированием.[file:13]

## Минимальная цель для первой реализации

Для первого внедрения достаточно добиться трёх вещей:

- В каждом неуспешном результате есть `ErrorCode`.
- В UI показывается отдельное `user_message`, а не сырой текст из ядра.
- Внутренние инварианты отделены от обычных пользовательских ошибок хотя бы на уровне категории.[file:12][file:13][file:14]

Этого уже хватит, чтобы система ошибок перестала быть набором разрозненных строк и стала частью архитектуры проекта.[file:13]
