#pragma once
#include "types.h"
#include <cmath>
#include <algorithm>

namespace geo {

// ============================================================
//  Point2<T>
// ============================================================

template<typename T>
Point2<T>::Point2(T x_, T y_) : x(x_), y(y_) {}

template<typename T>
Point2<T> Point2<T>::operator+(const Point2& other) const {
    return Point2(x + other.x, y + other.y);
}

template<typename T>
Point2<T> Point2<T>::operator-(const Point2& other) const {
    return Point2(x - other.x, y - other.y);
}

template<typename T>
Point2<T> Point2<T>::operator*(T scalar) const {
    return Point2(x * scalar, y * scalar);
}

template<typename T>
Point2<T>& Point2<T>::operator+=(const Point2& other) {
    x += other.x;
    y += other.y;
    return *this;
}

template<typename T>
Point2<T>& Point2<T>::operator-=(const Point2& other) {
    x -= other.x;
    y -= other.y;
    return *this;
}

template<typename T>
bool Point2<T>::operator==(const Point2& other) const {
    return areEqual(x, other.x) && areEqual(y, other.y);
}

template<typename T>
bool Point2<T>::operator!=(const Point2& other) const {
    return !(*this == other);
}

template<typename T>
T Point2<T>::cross(const Point2& other) const {
    return x * other.y - y * other.x;
}

template<typename T>
T Point2<T>::dot(const Point2& other) const {
    return x * other.x + y * other.y;
}

template<typename T>
T Point2<T>::squaredNorm() const {
    return x * x + y * y;
}

template<typename T>
T Point2<T>::norm() const {
    return std::sqrt(squaredNorm());
}

// ============================================================
//  Triangle2<T>
// ============================================================

template<typename T>
Triangle2<T>::Triangle2(const Point2<T>& a, const Point2<T>& b, const Point2<T>& c)
    : v0(a), v1(b), v2(c) {}

template<typename T>
std::array<Point2<T>, 3> Triangle2<T>::asArray() const {
    return {v0, v1, v2};
}

// ============================================================
//  Функции работы с полигонами
// ============================================================

template<typename T>
T signedArea2D(const Polygon2<T>& poly) {
    if (poly.size() < 3) return T(0);
    T area = T(0);
    const size_t n = poly.size();
    for (size_t i = 0; i < n; ++i) {
        const size_t j = (i + 1) % n;
        area += poly[i].x * poly[j].y;
        area -= poly[j].x * poly[i].y;
    }
    return area * T(0.5);
}

template<typename T>
void ensureCCW(Polygon2<T>& poly) {
    if (signedArea2D(poly) < T(0)) {
        std::reverse(poly.begin(), poly.end());
    }
}

template<typename T>
void removeCollinearPoints(Polygon2<T>& poly, T eps) {
    if (poly.size() < 3) return;

    Polygon2<T> result;
    result.reserve(poly.size());
    const size_t n = poly.size();

    for (size_t i = 0; i < n; ++i) {
        const Point2<T>& prev = poly[(i - 1 + n) % n];
        const Point2<T>& curr = poly[i];
        const Point2<T>& next = poly[(i + 1) % n];

        // Если текущая точка совпадает с предыдущей — пропускаем (дубликат)
        if (areEqual(curr.x, prev.x, eps) && areEqual(curr.y, prev.y, eps)) {
            continue;
        }

        const Point2<T> v1 = prev - curr;
        const Point2<T> v2 = next - curr;
        const T cross = v1.cross(v2);

        // Если коллинеарны (cross ≈ 0) — пропускаем текущую вершину
        if (isZero(cross, eps)) {
            continue;
        }

        result.push_back(curr);
    }

    // Если после чистки осталось меньше 3 точек — оставляем исходный (или как есть)
    if (result.size() < 3) {
        // Можно просто вернуть исходный, но лучше не трогать
        return;
    }

    poly = std::move(result);
}

} // namespace geo