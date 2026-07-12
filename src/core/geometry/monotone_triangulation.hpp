#pragma once

#include <algorithm>
#include <cassert>
#include <numeric>

#include "geometry/triangulation_types.h"
#include "monotone_triangulation.h"
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

  bool ok = makeMonotone(poly, types, diagonals);

  return TriangulationResult<T>();
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

} // namespace geo