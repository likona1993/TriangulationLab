#pragma once

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

#include "geometry/types.h"

namespace geo {

// ============================================================
//  Проверка: Проверка, что точка q лежит внутри ограничивающего прямоугольника
//  отрезка p-r
// (вызывается только когда p, q, r уже коллинеарны)
// ============================================================
template <typename T>
bool onSegment(const Point2<T> &p, const Point2<T> &q, const Point2<T> &r,
               T eps) {
  return inRange(q.x, std::min(p.x, r.x), std::max(p.x, r.x), eps) &&
         inRange(q.y, std::min(p.y, r.y), std::max(p.y, r.y), eps);
}

// ============================================================
//  Проверка: Пересекаются ли отрезки p1-q1 и p2-q2 (включая коллинеарное
//  перекрытие)
// ============================================================
template <typename T>
bool segmentsIntersect(const Point2<T> &p1, const Point2<T> &q1,
                       const Point2<T> &p2, const Point2<T> &q2, T eps) {
  const int o1 = orientation(p1, q1, p2, eps);
  const int o2 = orientation(p1, q1, q2, eps);
  const int o3 = orientation(p2, q2, p1, eps);
  const int o4 = orientation(p2, q2, q1, eps);

  if (o1 != o2 && o3 != o4)
    return true;

  if (o1 == 0 && onSegment(p1, p2, q1, eps))
    return true;
  if (o2 == 0 && onSegment(p1, q2, q1, eps))
    return true;
  if (o3 == 0 && onSegment(p2, p1, q2, eps))
    return true;
  if (o4 == 0 && onSegment(p2, q1, q2, eps))
    return true;

  return false;
}

// ============================================================
//  Проверка: является ли полигон простым (без самопересечений рёбер)
// ============================================================
template <typename T> bool isSimplePolygon(const Polygon2<T> &poly, T eps) {
  const size_t n = poly.size();
  for (size_t i = 0; i < n; ++i) {
    const size_t i_next = (i + 1) % n;
    for (size_t j = i + 1; j < n; ++j) {
      const size_t j_next = (j + 1) % n;
      // Смежные рёбра имеют общую вершину - это не пересечение
      if (j == i_next || j_next == i)
        continue;
      if (segmentsIntersect(poly[i], poly[i_next], poly[j], poly[j_next],
                            eps)) {
        return false;
      }
    }
  }
  return true;
}

// ============================================================
//  Проверка: выпуклая ли вершина (для CCW полигона)
// ============================================================
template <typename T>
bool isConvex(const Point2<T> &prev, const Point2<T> &curr,
              const Point2<T> &next, T eps = EPSILON<T>) {
  const Point2<T> v1 = curr - prev;
  const Point2<T> v2 = next - curr;
  const T cross = v1.cross(v2);
  return cross > eps;
}

// ============================================================
//  Проверка: находится ли точка P внутри треугольника (A, B, C)
// ============================================================
template <typename T>
bool pointInTriangle(const Point2<T> &p, const Point2<T> &a, const Point2<T> &b,
                     const Point2<T> &c, T eps = EPSILON<T>) {
  const T d1 = (p - a).cross(b - a);
  const T d2 = (p - b).cross(c - b);
  const T d3 = (p - c).cross(a - c);

  const bool has_neg = (d1 < -eps) || (d2 < -eps) || (d3 < -eps);
  const bool has_pos = (d1 > eps) || (d2 > eps) || (d3 > eps);

  return !(has_neg && has_pos);
}

// ============================================================
// Ориентация тройки точек: 0 - коллинеарны, 1 - по часовой, 2 - против часовой
// ============================================================
template <typename T>
int orientation(const Point2<T> &p, const Point2<T> &q, const Point2<T> &r,
                T eps = EPSILON<T>) {
  const T val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
  if (isZero(val, eps))
    return 0;
  return (val > T(0)) ? 1 : 2;
}

// ============================================================
//  Координата X пересечения ребра с прямой Y
// ============================================================
template <typename T>
T computeXIntersection(const Point2<T> &p1, const Point2<T> &p2, T currentY) {

  T dy = p2.y - p1.y;

  // Ребро горизонтально: при лексикографическом sweep-порядке оно направлено
  // слева направо и «проходит» через текущую вершину. Возвращаем левый конец —
  // так ребро гарантированно попадает в кандидаты findLeftEdge
  // (условие xEdge <= v.x + EPSILON).
  // TODO
  // Полностью корректная альтернатива — тай-брейк в EdgeCmp по наклону (для
  // двух нисходящих рёбер левее то, у которого cross(dirA, dirB) > 0), тогда
  // горизонталь всегда окажется крайне левой относительно наклонных рёбер с
  // общей вершиной.
  if (std::abs(dy) <= EPSILON<T>) {
    return std::min(p1.x, p2.x);
  }

  // Вычисляем параметр t ∈ [0,1] для интерполяции
  T t = (currentY - p1.y) / dy;

  // Защита от выхода за пределы отрезка из-за погрешностей
  if (t < 0)
    t = 0;
  if (t > 1)
    t = 1;

  return p1.x + t * (p2.x - p1.x);
};

// ============================================================
// a выше b в смысле sweep line
// ============================================================
template <typename T>
static bool isAbove(const Point2<T> &a, const Point2<T> &b) {
  if (std::abs(a.y - b.y) > EPSILON<T>)
    return a.y > b.y;
  return a.x < b.x;
}

// ============================================================
// проверяет, лежит ли точка d в окружности, описанной около abc
// ============================================================
template <typename T>
bool inCircumcircle(const geo::Point2<T>& a,
                    const geo::Point2<T>& b,
                    const geo::Point2<T>& c,
                    const geo::Point2<T>& d)
{
    T ax = a.x - d.x;
    T ay = a.y - d.y;
    T bx = b.x - d.x;
    T by = b.y - d.y;
    T cx = c.x - d.x;
    T cy = c.y - d.y;

    T det =
        (ax * ax + ay * ay) * (bx * cy - by * cx) -
        (bx * bx + by * by) * (ax * cy - ay * cx) +
        (cx * cx + cy * cy) * (ax * by - ay * bx);

    return det > T(0);
}

// ============================================================
//  Поиск дубликатов/близких вершин на входе (Этап 1 Roadmap:
//  явная ошибка вместо молчаливого отказа EarClipping).
//  Возвращает индекс первой вершины, для которой нашлась другая
//  на расстоянии < eps, либо nullopt.
//  O(n log n): сортировка по (x, y) + sweep с окном по x.
// ============================================================
template <typename T>
std::optional<size_t> findDuplicateVertex(const Polygon2<T> &poly,
                                          T eps = EPSILON<T>) {
  const size_t n = poly.size();
  std::vector<size_t> order(n);
  for (size_t i = 0; i < n; ++i)
    order[i] = i;

  std::sort(order.begin(), order.end(), [&poly](size_t a, size_t b) {
    if (poly[a].x != poly[b].x)
      return poly[a].x < poly[b].x;
    return poly[a].y < poly[b].y;
  });

  for (size_t i = 0; i < n; ++i) {
    for (size_t j = i + 1; j < n; ++j) {
      const T dx = poly[order[j]].x - poly[order[i]].x;
      if (dx >= eps)
        break; // окно по x закончилось
      const T dy = poly[order[j]].y - poly[order[i]].y;
      if (dx * dx + dy * dy < eps * eps)
        return order[i];
    }
  }
  return std::nullopt;
}

} // namespace geo
