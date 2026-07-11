#pragma once

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

  // Создаём список вершин с индексами для сортировки
  struct VertexEntry {
    VertexIndex index;
    Point2<T> point;
  };

  std::vector<VertexEntry> vertices;
  vertices.reserve(poly.size());
  for (size_t i = 0; i < poly.size(); ++i) {
    vertices.push_back({i, poly[i]});
  }

  // Сортируем по убыванию y, при равенстве y – по возрастанию x
  std::sort(vertices.begin(), vertices.end(),
            [](const VertexEntry &a, const VertexEntry &b) {
              if (std::abs(a.point.y - b.point.y) > EPSILON<T>) {
                return a.point.y > b.point.y; // убывание y
              }
              return a.point.x < b.point.x; // возрастание x
            });

  T currentY = vertices[0].point.y;
  EdgeCmp<T> comp(&poly, &currentY);
  StatusSet status(comp);
  HelperMap helpers;

  // Вектор для хранения добавленных диагоналей (для истории)
  std::vector<std::pair<Point2<T>, Point2<T>>> diagonals;

  // Проходим по вершинам в порядке убывания y
  for (const auto &entry : vertices) {
    size_t vi = entry.index;
    const Point2<T> &v = poly[vi];
    currentY = v.y; // обновляем текущую y для компаратора

    // Определяем тип вершины
    VertexType type = classifyVertex(poly, vi);

    // Обрабатываем в зависимости от типа
    switch (type) {
    case VertexType::START:
      handleStart(vi, poly, status, helpers, diagonals);
      break;
    case VertexType::END:
      handleEnd(vi, poly, status, helpers, diagonals);
      break;
    case VertexType::SPLIT:
      handleSplit(vi, poly, status, helpers, diagonals);
      break;
    case VertexType::MERGE:
      handleMerge(vi, poly, status, helpers, diagonals);
      break;
    case VertexType::REGULAR:
      handleRegular(vi, poly, status, helpers, diagonals);
      break;
    }

    // Записываем шаг отладки (будем заполнять позже)
    // ...
  }

  return TriangulationResult<T>();
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
void MonotoneTriangulation<T>::handleStart(VertexIndex vi, Polygon &poly,
                                           StatusSet &status,
                                           HelperMap &helpers,
                                           DiagonalList &diagonals) {
  (void)diagonals;
  // Добавляем ребро (vi, vi+1) в статус, helper = vi
  Edge e = getNextEdge(poly, vi);
  status.insert(e);
  helpers[e] = vi;
}

} // namespace geo