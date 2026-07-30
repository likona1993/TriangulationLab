#pragma once

#include <algorithm>
#include <cassert>
#include <numeric>

#include "geometry/triangulation_types.h"
#include "monotone_triangulation.h"
#include "utils/assert.h"
#include "utils/geometry_predicates.h"

namespace geo {

template <typename T>
bool EdgeCmp<T>::operator()(const Edge &a, const Edge &b) const {
  // сравнение по x-пересечению с текущей sweep line
  // нужен доступ к текущей координате y
  // TODO - необходимо обработать случай, когда ребро параллельно y(либо не
  // пересекает, либо совпадает)

  T xA = computeXIntersection((*poly)[a.from], (*poly)[a.to], *currentY);
  T xB = computeXIntersection((*poly)[b.from], (*poly)[b.to], *currentY);

  // Сравниваем x-координаты с учётом EPSILON
  if (std::abs(xA - xB) > EPSILON<T>) {
    return xA < xB;
  }

  // Если x-координаты почти равны, сравниваем по углу наклона,
  // чтобы set не считал рёбра эквивалентными.
  // Для этого можно сравнить отношение dy/dx или просто использовать индексы.
  // Используем индексы вершин для детерминированного порядка.
  return EdgeLess{}(a, b);
}

template <typename T>
TriangulationResult<T>
MonotoneTriangulation<T>::triangulate(const Polygon &polygon) {
  // TODO  - дублирование кода (аналигичный блок в EarClipping)
  TriangulationResult<T> result;

  if (polygon.size() < 3) {
    result.error_message = "Polygon must have at least 3 vertices";
    return result;
  }

  const T eps = EPSILON<T>;
  if (!isSimplePolygon(polygon, eps)) {
    result.error_message = "Polygon is self-intersecting";
    return result;
  }
  // конец дублированного блока

  Polygon poly = polygon;

  // Вектор для хранения добавленных диагоналей (для истории)
  DiagonalList diagonals;

  // классификация вершин
  VertexTypes types(poly.size());
  for (size_t i = 0; i < poly.size(); ++i)
    types[i] = classifyVertex(poly, i);

  if (!makeMonotone(poly, types, diagonals)) {
    result.error_message = "Failed to decompose polygon into monotone pieces";
    return result;
  }

  auto pieces = splitIntoMonotone(poly, diagonals);
  for (const auto &piece : pieces) {
    GEO_ASSERT_INVARIANT(isMonotonePiece(poly, piece),
                         "фаза 1 оставила SPLIT/MERGE — баг в makeMonotone");
    if (!triangulateMonotonePiece(poly, piece, result.triangles)) {
      result.success = false;
      result.error_message = "Monotone triangulation failed";
      return result;
    }
  }

  return result;
}

template <typename T>
bool MonotoneTriangulation<T>::makeMonotone(
    const Polygon &poly, const std::vector<VertexType> &types,
    std::vector<Diagonal> &diagonals) {

  const size_t n = poly.size();
  if (n < 3)
    return false;

  // ---------------------------------------------------------------
  // 1. Порядок обработки вершин: y убывает, при равной y — x возрастает.
  //    ВАЖНО: тот же предикат isAbove, что и в classifyVertex.
  //    Иначе классификация и порядок обработки противоречат друг другу.
  // ---------------------------------------------------------------
  std::vector<VertexIndex> order(n);
  std::iota(order.begin(), order.end(), VertexIndex{0});

  std::sort(order.begin(), order.end(), [&poly](VertexIndex a, VertexIndex b) {
    return isAbove(poly[a], poly[b]);
  });

  // ---------------------------------------------------------------
  // 2. Структуры sweep line.
  //    currentY — единственный экземпляр: на него смотрит и EdgeCmp
  //    (через указатель), и findLeftEdge (через ctx).
  // ---------------------------------------------------------------
  T currentY = poly[order.front()].y;

  EdgeCmp<T> comp(&poly, &currentY); // явный <T>: без CTAD
  StatusSet status(comp);
  HelperMap helpers; // компаратор EdgeLess — НЕ зависит от sweep line

  SweepCtx ctx{&poly, &types, &status, &helpers, &diagonals, &currentY};

  // ---------------------------------------------------------------
  // 3. Заметание сверху вниз
  // ---------------------------------------------------------------
  for (VertexIndex vi : order) {

    // Обновляем sweep line ДО любых операций со статусом:
    // status.erase() ищет ребро бинарным поиском через EdgeCmp,
    // а тот сравнивает рёбра по x пересечения с currentY.
    *ctx.currentY = poly[vi].y;

    bool ok = true;
    switch (types[vi]) {
    case START:
      ok = handleStart(vi, ctx);
      break;
    case END:
      ok = handleEnd(vi, ctx);
      break;
    case SPLIT:
      ok = handleSplit(vi, ctx);
      break;
    case MERGE:
      ok = handleMerge(vi, ctx);
      break;
    case REGULAR:
      ok = handleRegular(vi, ctx);
      break;
    }

    if (!ok)
      return false; // findLeftEdge не нашёл ребро слева — вырожденные данные

    // TODO: history.push_back(...) — снимок status/helpers/diagonals
  }

  // ---------------------------------------------------------------
  // 4. Инварианты: каждое нисходящее ребро вставлено и удалено ровно раз.
  // ---------------------------------------------------------------
  assert(status.empty() && "статус не опустел: ошибка в классификации вершин");
  assert(helpers.empty() && "helpers не опустел: забыт helpers.erase()");

  return true;
}

template <typename T>
MonotoneTriangulation<T>::VertexType
MonotoneTriangulation<T>::classifyVertex(const Polygon &poly, size_t i) const {
  const size_t n = poly.size();
  if (n < 3)
    return VertexType::REGULAR; // не должно случиться

  const Point2<T> &prev = poly[(i - 1 + n) % n];
  const Point2<T> &curr = poly[i];
  const Point2<T> &next = poly[(i + 1) % n];

  // Определяем, является ли вершина выпуклой (угол < 180 градусов)
  bool convex = isConvex(prev, curr, next, EPSILON<T>);

  // Сравнение y-координат с учётом EPSILON
  bool prevBelow = isAbove(curr, prev);
  bool nextBelow = isAbove(curr, next);

  if (prevBelow && nextBelow) // локальный максимум
    return convex ? VertexType::START : VertexType::SPLIT;

  if (!prevBelow && !nextBelow) // локальный минимум
    return convex ? VertexType::END : VertexType::MERGE;

  return VertexType::REGULAR; // один сосед выше, другой ниже
}

template <typename T>
typename MonotoneTriangulation<T>::StatusSet::iterator
MonotoneTriangulation<T>::findLeftEdge(const Polygon &poly, StatusSet &status,
                                       VertexIndex vi, T currentY) {
  if (status.empty())
    return status.end();

  const Point2<T> &v = poly[vi];

  // Простой и надёжный способ – линейный поиск, но статус может содержать
  // O(n) рёбер, и это даст O(n^2) в худшем случае. Для начала можно так, но
  // потом оптимизировать. Предлагаю пока использовать линейный поиск, а позже
  // заменить на сбалансированное дерево с возможностью поиска по x. TODO

  auto best = status.end();
  for (auto it = status.begin(); it != status.end(); ++it) {
    const T xEdge =
        computeXIntersection(poly[it->from], poly[it->to], currentY);
    if (xEdge > v.x + EPSILON<T>)
      break;   // дальше все рёбра ещё правее — статус отсортирован
    best = it; // это ребро левее v, пока оно и есть «ближайшее слева»
  }
  return best; // end(), если слева ничего нет
}

template <typename T>
Edge MonotoneTriangulation<T>::getNextEdge(const Polygon &poly,
                                           VertexIndex vi) {
  return {vi, (vi + 1) % poly.size()};
}

template <typename T>
Edge MonotoneTriangulation<T>::getPrevEdge(const Polygon &poly,
                                           VertexIndex vi) {
  return {(vi - 1 + poly.size()) % poly.size(), vi};
}

template <typename T>
void MonotoneTriangulation<T>::tryCloseMerge(VertexIndex vi, const Edge &e,
                                             SweepCtx &ctx) {
  auto it = ctx.helpers->find(e);
  if (it == ctx.helpers->end())
    return; // не должно случаться
  if ((*ctx.types)[it->second] == MERGE)
    ctx.diagonals->push_back({vi, it->second});
}

template <typename T>
void MonotoneTriangulation<T>::closeEdge(VertexIndex vi, SweepCtx &ctx) {
  Edge e = getPrevEdge(*ctx.poly, vi);
  tryCloseMerge(vi, e, ctx);
  ctx.status->erase(e);
  ctx.helpers->erase(e); // ОБЯЗАТЕЛЬНО, иначе мусор
}

template <typename T>
void MonotoneTriangulation<T>::openEdge(VertexIndex vi, SweepCtx &ctx) {
  Edge e = getNextEdge(*ctx.poly, vi);
  ctx.status->insert(e);
  (*ctx.helpers)[e] = vi;
}

template <typename T>
bool MonotoneTriangulation<T>::updateLeftHelper(VertexIndex vi, SweepCtx &ctx) {
  auto ej = findLeftEdge(*ctx.poly, *ctx.status, vi, *ctx.currentY);
  if (ej == ctx.status->end())
    return false;
  tryCloseMerge(vi, *ej, ctx);
  (*ctx.helpers)[*ej] = vi;
  return true;
}

// Оба соседа ниже, выпуклая. Открывается новый интервал.
template <typename T>
bool MonotoneTriangulation<T>::handleStart(VertexIndex vi, SweepCtx &ctx) {
  openEdge(vi, ctx);
  return true;
}

// Оба соседа выше, выпуклая. Интервал закрывается.
template <typename T>
bool MonotoneTriangulation<T>::handleEnd(VertexIndex vi, SweepCtx &ctx) {
  closeEdge(vi, ctx);
  return true;
}

// Оба соседа ниже, рефлексная. Интервал раздваивается.
// Единственный тип, проводящий диагональ БЕЗУСЛОВНО.
template <typename T>
bool MonotoneTriangulation<T>::handleSplit(VertexIndex vi, SweepCtx &ctx) {
  auto ej = findLeftEdge(*ctx.poly, *ctx.status, vi, *ctx.currentY);
  if (ej == ctx.status->end())
    return false; // слева обязано быть ребро

  ctx.diagonals->push_back({vi, ctx.helpers->at(*ej)}); // ← безусловно
  (*ctx.helpers)[*ej] = vi;

  openEdge(vi, ctx); // своя новая граница
  return true;
}

// Оба соседа выше, рефлексная. Интервалы сливаются.
// Зеркало SPLIT, но диагональ вниз провести НЕЛЬЗЯ — партнёр ещё не известен.
template <typename T>
bool MonotoneTriangulation<T>::handleMerge(VertexIndex vi, SweepCtx &ctx) {
  closeEdge(vi, ctx);               // правая часть закрылась
  return updateLeftHelper(vi, ctx); // vi повисает как ДОЛГ
}

// Один сосед выше, другой ниже. Интервал продолжается.
template <typename T>
bool MonotoneTriangulation<T>::handleRegular(VertexIndex vi, SweepCtx &ctx) {
  const Polygon &poly = *ctx.poly;
  const size_t n = poly.size();
  const Point2<T> &prev = poly[(vi + n - 1) % n];

  // prev выше => цепь спускается => внутренность справа => ЛЕВАЯ цепь
  if (isAbove(prev, poly[vi])) {
    closeEdge(vi, ctx); // эстафета: старое ребро ушло,
    openEdge(vi, ctx);  //           новое пришло
    return true;
  }

  // ПРАВАЯ цепь: своих нисходящих рёбер нет, только чужой helper слева
  return updateLeftHelper(vi, ctx);
}

template <typename T>
std::vector<typename MonotoneTriangulation<T>::MonotonePiece>
MonotoneTriangulation<T>::splitIntoMonotone(
    const Polygon &poly, const std::vector<Diagonal> &diagonals) const {

  const size_t n = poly.size();

  // Диагоналей нет => полигон уже y-монотонен, разрезать нечего.
  // (Общий код тоже отработает верно, это просто короткий путь.)
  if (diagonals.empty()) {
    MonotonePiece whole(n);
    std::iota(whole.begin(), whole.end(), VertexIndex{0});
    return {std::move(whole)};
  }

  // ---------------------------------------------------------------
  // 1. Планарный граф: рёбра границы + диагонали, каждое — в обе стороны.
  //    Куски = внутренние грани этого графа.
  // ---------------------------------------------------------------
  std::vector<std::vector<VertexIndex>> adj(n);
  for (VertexIndex i = 0; i < n; ++i) {
    adj[i].push_back((i + 1) % n);
    adj[i].push_back((i + n - 1) % n);
  }
  for (const auto &d : diagonals) {
    adj[d.first].push_back(d.second);
    adj[d.second].push_back(d.first);
  }

  // ---------------------------------------------------------------
  // 2. Соседи каждой вершины — по возрастанию угла (CCW).
  // ---------------------------------------------------------------
  for (VertexIndex i = 0; i < n; ++i) {
    const Point2<T> &p = poly[i];

    auto half = [&p](const Point2<T> &q) -> int {
      const T dx = q.x - p.x;
      const T dy = q.y - p.y;
      if (dy < -EPSILON<T>)
        return 1; // строго вниз
      if (std::abs(dy) <= EPSILON<T> && dx < T(0))
        return 1; // строго влево (ось −x)
      return 0;
    };

    std::sort(adj[i].begin(), adj[i].end(), [&](VertexIndex a, VertexIndex b) {
      const int ha = half(poly[a]);
      const int hb = half(poly[b]);
      if (ha != hb)
        return ha < hb; // верхняя половина всегда раньше

      const T cr = (poly[a] - p).cross(poly[b] - p);
      if (std::abs(cr) > EPSILON<T>)
        return cr > T(0); // a раньше b, если поворот к b — CCW

      // Сонаправленные векторы: в простом полигоне не должно быть
      // (два ребра/диагонали из одной вершины в одном направлении).
      // Тай-брейк по индексу — только ради детерминизма sort.
      return a < b;
    });
  }

  // ---------------------------------------------------------------
  // 3. Обход граней: пришли в b по ребру a->b, уходим к соседу b,
  //    следующему по ЧАСОВОЙ стрелке от a.
  // ---------------------------------------------------------------
  // TODO(perf): linear search внутри цикла обхода => O(deg) на каждый шаг.
  // Заменить на предпосчитанную таблицу позиций:
  //   std::vector<std::unordered_map<VertexIndex, size_t>> pos(n);
  // (или, при полном переходе на нумерацию направленных рёбер, — на twin[])
  auto indexOf = [&](VertexIndex v, VertexIndex u) -> size_t {
    return std::find(adj[v].begin(), adj[v].end(), u) - adj[v].begin();
  };

  // TODO(perf): std::set<pair> => O(log E) на каждое посещение ребра
  // и куча аллокаций. Правильное решение — занумеровать направленные рёбра
  // (halfedge id = смещение в плоском массиве смежности) и держать
  // std::vector<bool> visited(2 * E). Это заодно снимет и TODO про indexOf:
  // переход к следующему полуребру станет чистой арифметикой над индексами.
  std::set<std::pair<VertexIndex, VertexIndex>> visited;

  std::vector<MonotonePiece> pieces;

  for (VertexIndex u = 0; u < n; ++u) {
    for (VertexIndex v : adj[u]) {
      if (visited.count({u, v}))
        continue;

      MonotonePiece face;
      VertexIndex a = u, b = v;
      do {
        visited.insert({a, b});
        face.push_back(a);

        const size_t k = indexOf(b, a);
        const VertexIndex c = adj[b][(k + adj[b].size() - 1) % adj[b].size()];

        a = b;
        b = c;
      } while (!(a == u && b == v));

      // Внутренние грани обходятся CCW (площадь > 0),
      // внешняя — CW (площадь < 0). Её и отбрасываем. TODO
      Polygon facePoly;
      facePoly.reserve(face.size());
      for (VertexIndex idx : face)
        facePoly.push_back(poly[idx]);

      if (signedArea2D(facePoly) > EPSILON<T>)
        pieces.push_back(std::move(face));
    }
  }

  return pieces;
}

// Монотонность — инвариант, гарантированный фазой 1.
// Проверка только в debug: провал означает баг в makeMonotone,
// а не некорректные входные данные.
template <typename T>
bool MonotoneTriangulation<T>::isMonotonePiece(
    const Polygon &poly, const MonotonePiece &piece) const {
  const size_t m = piece.size();
  if (m < 3)
    return false;

  // Кусок монотонен <=> у него нет SPLIT и MERGE.
  // Здесь достаточно считать локальные экстремумы: их должно быть ровно
  // по одному (один максимум = вершина, один минимум = низ).
  size_t maxima = 0, minima = 0;
  for (size_t k = 0; k < m; ++k) {
    const Point2<T> &prev = poly[piece[(k + m - 1) % m]];
    const Point2<T> &curr = poly[piece[k]];
    const Point2<T> &next = poly[piece[(k + 1) % m]];

    const bool prevBelow = isAbove(curr, prev);
    const bool nextBelow = isAbove(curr, next);

    if (prevBelow && nextBelow)
      ++maxima;
    if (!prevBelow && !nextBelow)
      ++minima;
  }
  return maxima == 1 && minima == 1;
}

// Правило: top/bottom — по sweep-порядку (isAbove); проход ВПЕРЁД по piece
// от top до bottom (кусок хранится в CCW) — левая цепь, продолжение от
// bottom обратно до top — правая.
template <typename T>
std::vector<typename MonotoneTriangulation<T>::Chain>
MonotoneTriangulation<T>::classifyChains(const Polygon &poly,
                                         const MonotonePiece &piece) const {
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

template <typename T>
void MonotoneTriangulation<T>::emitTriangle(const Polygon &poly, VertexIndex a,
                                            VertexIndex b, VertexIndex c,
                                            Triangles &out) const {
  if ((poly[b] - poly[a]).cross(poly[c] - poly[a]) < T(0))
    out.push_back({poly[a], poly[c], poly[b]});
  else
    out.push_back({poly[a], poly[b], poly[c]});
}

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
        const T cr = (poly[piece[last]] - poly[piece[t0]])
                         .cross(poly[piece[uj]] - poly[piece[t0]]);
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

} // namespace geo