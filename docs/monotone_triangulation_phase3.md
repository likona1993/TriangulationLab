# Фаза 3: триангуляция монотонного куска (`triangulateMonotonePiece`)

Разбор алгоритма стековой триангуляции y-монотонного многоугольника
(алгоритм де Берга), написан по мере разбора в чате. Фазы 1 (`makeMonotone`)
и 2 (`splitIntoMonotone`) описаны в `monotone_context_phase3.md` в корне
репозитория — этот файл продолжает оттуда.

Вход фазы 3: один y-монотонный кусок `MonotonePiece` (индексы вершин
исходного полигона, хранится в CCW-порядке — это гарантирует `splitIntoMonotone`
отбором граней с `signedArea2D > 0`). Выход: `m - 2` треугольника.

---

## 1. Левая и правая цепь

Принадлежность вершины к цепи — это **позиция в массиве куска относительно
top/bottom**, а не геометрическое свойство отдельной точки (сравнение x).

Правило: найти `top` (самая верхняя по sweep-порядку, `isAbove`) и `bottom`
(самая нижняя) вершины куска. Кусок хранится в CCW — значит проход **вперёд**
по массиву от `top` к `bottom` — это всегда **левая цепь**, а продолжение от
`bottom` обратно к `top` — **правая цепь**. Это следствие ориентации обхода:
у CCW-контура, если стоять в верхней вершине и идти вперёд, внутренность
всегда остаётся справа — то есть путь идёт по левой границе.

```cpp
enum class Chain { LEFT, RIGHT };

// Правило: top/bottom — по sweep-порядку (isAbove); проход ВПЕРЁД по piece
// от top до bottom (кусок хранится в CCW) — левая цепь, продолжение от
// bottom обратно до top — правая.
template <typename T>
std::vector<Chain> MonotoneTriangulation<T>::classifyChains(
    const Polygon &poly, const MonotonePiece &piece) const {
  const size_t m = piece.size();

  size_t topPos = 0, bottomPos = 0;
  for (size_t k = 1; k < m; ++k) {
    if (isAbove(poly[piece[k]], poly[piece[topPos]]))
      topPos = k;
    if (isAbove(poly[piece[bottomPos]], poly[piece[k]]))
      bottomPos = k;
  }

  std::vector<Chain> chain(m);
  size_t k = topPos;
  bool onLeft = true;
  do {
    chain[k] = onLeft ? Chain::LEFT : Chain::RIGHT;
    if (k == bottomPos)
      onLeft = false;
    k = (k + 1) % m;
  } while (k != topPos);

  return chain;
}
```

`top` и `bottom` в результате помечены LEFT — это только тег для прохода
слияния (шаг 2), в стековом алгоритме их роль — "первая/последняя вершина",
а не принадлежность к конкретной цепи.

### Проверка на куске A (прямоугольник с вырезом, из фазы 1/2)

Кусок `A = {0, 1, 2, 7, 8, 9}`, `v9=(0,10)`, `v0=(0,0)`, `v1=(3,0)`,
`v2=(5,4)`, `v7=(5,6)`, `v8=(3,10)`.

`top = v9` (позиция 5), `bottom = v1` (позиция 1). Проход вперёд от позиции 5:
`5→0→1` → `{v9, v0, v1}` = LEFT (левый + нижний край, x≈0). Продолжение
`1→2→3→4→5` → `{v1, v2, v7, v8, v9}` = RIGHT (через вырез и верх).

```
   9 ●───────● 8         y=10   (правая цепь: 1→2→7→8→9)
     │        \
     │         \
     │          ● 7                y=6
     │          ‖
     │          ● 2                y=4
     │         /
     │        /
   0 ●───────● 1         y=0   (левая цепь: 9→0→1)
```

---

## 2. Стек: два случая

Стек хранит "воронку" (funnel) — пройденные, но ещё не разрезанные на
треугольники вершины, от старой к новой. При обработке `uj`:

**Другая цепь, чем верхушка стека** — воронка переключилась на другую
сторону. `uj` видит **весь** стек целиком (веер треугольников), стек
сбрасывается до `{u_{j-1}, uj}`.

**Та же цепь** — неизвестно заранее, сколько вершин стека видно от `uj`
(часть могла спрятаться за рефлексным изгибом). Идём по одной от верхушки,
пока треугольник `(t0, t1=last, uj)` выпуклый в нужную сторону — как только
перестал быть выпуклым, останавливаемся.

### Знак `cross` зависит от цепи

Цепи — зеркальные отражения друг друга относительно sweep line, поэтому
"повернуть внутрь полигона" — это повороты в разные стороны:

```
валидно (можно резать), если:
  uj на RIGHT-цепи:  cross(t0, t1, uj) < 0
  uj на LEFT-цепи:   cross(t0, t1, uj) > 0
```

(`cross(o,a,b) = (a-o) × (b-o)`, `t0` — элемент под верхушкой стека, `t1`/`last`
— верхушка/последняя вытолкнутая вершина, `uj` — текущая).

---

## 3. Гарантия CCW-ориентации выходных треугольников

```cpp
template <typename T>
void MonotoneTriangulation<T>::emitTriangle(const Polygon &poly,
                                             VertexIndex a, VertexIndex b,
                                             VertexIndex c,
                                             Triangles &out) const {
  if (cross(poly[a], poly[b], poly[c]) < T(0))
    out.push_back({poly[a], poly[c], poly[b]});
  else
    out.push_back({poly[a], poly[b], poly[c]});
}
```

---

## 4. Полный код `triangulateMonotonePiece`

```cpp
template <typename T>
bool MonotoneTriangulation<T>::triangulateMonotonePiece(
    const Polygon &poly, const MonotonePiece &piece, Triangles &out) const {
  const size_t m = piece.size();
  if (m < 3)
    return false;

  const std::vector<Chain> chain = classifyChains(poly, piece);

  // Слить обе цепи в одну последовательность ПОЗИЦИЙ внутри piece,
  // отсортированную по sweep-порядку (тем же isAbove, что и в фазе 1).
  std::vector<size_t> order(m);
  std::iota(order.begin(), order.end(), size_t{0});
  std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
    return isAbove(poly[piece[a]], poly[piece[b]]);
  });

  // Стек хранит ПОЗИЦИИ внутри piece (не индексы вершин в poly).
  std::vector<size_t> stack{order[0], order[1]};

  for (size_t j = 2; j + 1 < m; ++j) {
    const size_t uj = order[j];

    if (chain[uj] != chain[stack.back()]) {
      // Другая цепь: воронка переключилась — видно весь стек целиком.
      for (size_t k = stack.size() - 1; k > 0; --k)
        emitTriangle(poly, piece[uj], piece[stack[k]], piece[stack[k - 1]],
                     out);

      stack.clear();
      stack.push_back(order[j - 1]); // = старая верхушка стека
      stack.push_back(uj);
    } else {
      // Та же цепь: расширяем клин, пока треугольник смотрит внутрь.
      size_t last = stack.back();
      stack.pop_back();

      while (!stack.empty()) {
        const size_t t0 = stack.back();
        const T cr = cross(poly[piece[t0]], poly[piece[last]], poly[piece[uj]]);
        const bool inside =
            (chain[uj] == Chain::RIGHT) ? (cr < T(0)) : (cr > T(0));
        if (!inside)
          break;

        emitTriangle(poly, piece[uj], piece[last], piece[t0], out);
        last = t0;
        stack.pop_back();
      }
      stack.push_back(last);
      stack.push_back(uj);
    }
  }

  // Финал: последняя вершина забирает весь оставшийся стек — это всегда
  // монотонный "веер", (stack.size() - 1) треугольников. В отличие от
  // текстбучной формулировки ("вставить диагональ кроме концов" — та про
  // НОВЫЕ диагонали), нам нужны ВСЕ треугольники, включая те, что опираются
  // на уже существующие граничные рёбра.
  const size_t un = order[m - 1];
  for (size_t k = 0; k + 1 < stack.size(); ++k)
    emitTriangle(poly, piece[un], piece[stack[k]], piece[stack[k + 1]], out);

  return true;
}
```

---

## 5. Разобранный пример (с рефлексными вершинами на обеих... нет, на правой цепи)

Компактный пример, отдельный от куска A (тот выпуклый, случай "остановиться
раньше" на нём не проверить):

```
T  = (0,10)   top,  LEFT
R1 = (3,8)    RIGHT
R2 = (1,6)    RIGHT  ← рефлексная, "вдавлена" внутрь
R3 = (3,4)    RIGHT
B  = (0,0)    bottom

левая цепь: T—B (одно ребро, без внутренних вершин)
правая цепь: T—R1—R2—R3—B
```

```
T●               y=10
 │╲
 │ ╲___●R1        y=8
 │     │
 │   ●R2  (вдавлена)   y=6
 │     │
 │ ●___/R3          y=4
 │ /
B●                y=0
```

sweep-порядок: `T, R1, R2, R3, B` (m=5, ожидается **3** треугольника).

- **j=3, uj=R2 (RIGHT)**: та же цепь, что верхушка R1. Вытолкнуть `R1`.
  `cross(T,R1,R2) = -10 < 0` → валидно → вытолкнуть `T`, диагональ `(R2,T)`
  → треугольник `T-R1-R2`. Стек пуст → положить `T`, `R2`. Стек = `[T,R2]`.
- **j=4, uj=R3 (RIGHT)**: та же цепь. Вытолкнуть `R2`.
  `cross(T,R2,R3) = 6`, не `< 0` → невалидно → остановка сразу.
  Положить `R2` обратно, `R3`. Стек = `[T,R2,R3]`.
- **финал** (`un=B`): все соседние пары стека `[T,R2,R3]` →
  `(B,T,R2)` и `(B,R2,R3)` — 2 треугольника.

Итог: `(T,R1,R2)`, `(B,T,R2)`, `(B,R2,R3)` — 3 треугольника = `m-2`. ✓

---

## 6. Тренировочный пример (разобрать самостоятельно)

Кусок уже y-монотонен (это готовый `MonotonePiece`, фазы 1/2 не нужны).
Решение сюда намеренно не записано — см. чат для хода собственной проверки.

```
T  = (2, 12)   top
L1 = (0, 9)
L2 = (1, 6)    ← рефлексная (вогнутая), но REGULAR: один сосед выше, один ниже
B  = (2, 0)    bottom
R3 = (5, 2)
R2 = (3, 5)    ← рефлексная (вогнутая), тоже REGULAR
R1 = (5, 8)

Граница (CCW): T → L1 → L2 → B → R3 → R2 → R1 → (T)
```

Задание:
1. Определить `Chain` (LEFT/RIGHT) для всех 7 вершин.
2. Построить sweep-порядок (слияние цепей по `isAbove`).
3. Пройти стековый алгоритм вручную, шаг за шагом фиксируя: та же цепь или
   другая, что вытолкнулось, какие треугольники выданы.
4. Самопроверка: должно получиться ровно **5** треугольников (`m-2`, `m=7`),
   и сумма их площадей должна равняться площади полигона (по шоулейсу — `31.5`).

## Открытые TODO

- Добавить `Chain`, `classifyChains`, `emitTriangle`, `triangulateMonotonePiece`
  в объявления `monotone_triangulation.h` (сейчас код есть только здесь и в
  переписке — в реальные `.h`/`.hpp` ещё не перенесён).
- Вызвать `triangulateMonotonePiece` для каждого куска из `triangulate()`,
  собрать все треугольники в `TriangulationResult<T>::triangles`.
- Заполнить `history` (`DebugStep<T>`) по аналогии с `EarClipping`, чтобы
  фаза 3 тоже была видна в пошаговом отладчике GUI.
- Тесты в `src/tests/test_monotone_triangulation.cpp`: куски A/C из
  сквозного примера и рефлексный пример выше.
