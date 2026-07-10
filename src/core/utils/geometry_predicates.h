#pragma once

#include "geometry/types.h"


namespace geo{

// Ориентация тройки точек: 0 - коллинеарны, 1 - по часовой, 2 - против часовой
template<typename T>
int orientation(const Point2<T>& p, const Point2<T>& q, const Point2<T>& r, T eps) {
    const T val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (isZero(val, eps)) return 0;
    return (val > T(0)) ? 1 : 2;
}

// Проверка, что точка q лежит внутри ограничивающего прямоугольника отрезка p-r
// (вызывается только когда p, q, r уже коллинеарны)
template<typename T>
bool onSegment(const Point2<T>& p, const Point2<T>& q, const Point2<T>& r, T eps) {
    return inRange(q.x, std::min(p.x, r.x), std::max(p.x, r.x), eps) &&
           inRange(q.y, std::min(p.y, r.y), std::max(p.y, r.y), eps);
}

// Пересекаются ли отрезки p1-q1 и p2-q2 (включая коллинеарное перекрытие)
template<typename T>
bool segmentsIntersect(const Point2<T>& p1, const Point2<T>& q1,
                       const Point2<T>& p2, const Point2<T>& q2, T eps) {
    const int o1 = orientation(p1, q1, p2, eps);
    const int o2 = orientation(p1, q1, q2, eps);
    const int o3 = orientation(p2, q2, p1, eps);
    const int o4 = orientation(p2, q2, q1, eps);

    if (o1 != o2 && o3 != o4) return true;

    if (o1 == 0 && onSegment(p1, p2, q1, eps)) return true;
    if (o2 == 0 && onSegment(p1, q2, q1, eps)) return true;
    if (o3 == 0 && onSegment(p2, p1, q2, eps)) return true;
    if (o4 == 0 && onSegment(p2, q1, q2, eps)) return true;

    return false;
}
}//namespace geo