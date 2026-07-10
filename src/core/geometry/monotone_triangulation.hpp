#include "monotone_triangulation.h"

#include "geometry_predicates.h"

namespace geo {

template <typename T>
bool EdgeCmp<T>::operator()(const Edge &a, const Edge &b) const {
  // сравнение по x-пересечению с текущей sweep line
  // нужен доступ к текущей координате y
  // TODO - необходимо обработать случай, когда ребро параллельно y(либо не
  // пересекает, либо совпадает)

  T xA = computeXIntersection(poly[a.from], poly[a.to], currentY);
  T xB = computeXIntersection(poly[b.from], poly[b.to], currentY);

  // Сравниваем x-координаты с учётом EPSILON
  if (std::abs(xA - xB) > EPSILON<T>()) {
    return xA < xB;
  }

  // Если x-координаты почти равны, сравниваем по углу наклона,
  // чтобы set не считал рёбра эквивалентными.
  // Для этого можно сравнить отношение dy/dx или просто использовать индексы.
  // Используем индексы вершин для детерминированного порядка.
  if (a.from != b.from)
    return a.from < b.from;
  return a.to < b.to;
}

template <typename T>
TriangulationResult<T>
MonotoneTriangulation<T>::triangulate(const Polygon &polygon) {
  // TODO  - дублирование кода (аналигичный блок в EarClipping)
  if (input.size() < 3) {
    result.error_message = "Polygon must have at least 3 vertices";
    return result;
  }

  if (!isSimplePolygon(input, eps)) {
    result.error_message = "Polygon is self-intersecting";
    return result;
  }
  // конец дублированного блока

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
              if (std::abs(a.point.y - b.point.y) > EPSILON<T>()) {
                return a.point.y > b.point.y; // убывание y
              }
              return a.point.x < b.point.x; // возрастание x
            });

  T currentY = vertices[0].point.y;
  EdgeCmp comp(&poly, &currentY);
  StatusSet status(comp);
  HelperMap helpers(
      comp); // тоже используем тот же компаратор для согласованности

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
  bool convex = isConvex(prev, curr, next);

  // Сравнение y-координат с учётом EPSILON
  bool prevBelow = prev.y < curr.y - EPSILON<T>();
  bool prevAbove = prev.y > curr.y + EPSILON<T>();
  bool nextBelow = next.y < curr.y - EPSILON<T>();
  bool nextAbove = next.y > curr.y + EPSILON<T>();

  // Если оба соседа ниже -> START (локальный минимум)
  if (prevBelow && nextBelow) {
    return VertexType::START;
  }

  // Если оба соседа выше -> END (локальный максимум)
  if (prevAbove && nextAbove) {
    return VertexType::END;
  }

  // Если один выше, один ниже -> REGULAR
  if ((prevAbove && nextBelow) || (prevBelow && nextAbove)) {
    return VertexType::REGULAR;
  }

  // Остаются случаи, когда один или оба соседа имеют такую же y
  // (коллинеарность) Здесь нужна более тонкая обработка, но мы предполагаем,
  // что предварительно удалили коллинеарные точки. Если они остались, то
  // классификация зависит от того, является ли вершина выпуклой или вогнутой, и
  // от расположения соседей. Обычно в литературе такие случаи обрабатывают с
  // помощью сравнения x при равной y. Для простоты можно считать, что если y
  // равны, то мы рассматриваем их как "выше" или "ниже" в зависимости от x (но
  // это сложно). Поэтому рекомендую перед запуском алгоритма гарантировать, что
  // никакие три последовательные точки не коллинеарны (у вас есть
  // removeCollinearPoints).

  // Если не удалось определить, используем выпуклость и дополнительный анализ.
  // Например, если вершина выпуклая и один сосед выше, другой на той же y, то
  // это может быть REGULAR. Но для надёжности лучше обработать как REGULAR.

  return VertexType::REGULAR;
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
void MonotoneTriangulation<T>::handleStart(
    VertexIndex vi, Polygon &poly, StatusSet &status, HelperMap &helpers,
    std::vector<std::pair<Point2<T>, Point2<T>>> &diagonals) {
  // Добавляем ребро (vi, vi+1) в статус, helper = vi
  Edge e = getNextEdge(poly, vi);
  status.insert(e);
  helpers[e] = vi;
}

} // namespace geo