#pragma once
#include "../utils/numeric.h"
#include <array>
#include <cstddef>
#include <vector>

namespace geo {

// ============================================================
//  Point2<T>
// ============================================================
template <typename T> struct Point2 {
  T x = 0, y = 0;

  Point2() = default;
  Point2(T x_, T y_);

  // Арифметические операции
  Point2 operator+(const Point2 &other) const;
  Point2 operator-(const Point2 &other) const;
  Point2 operator*(T scalar) const;
  Point2 &operator+=(const Point2 &other);
  Point2 &operator-=(const Point2 &other);

  // Сравнение
  bool operator==(const Point2 &other) const;
  bool operator!=(const Point2 &other) const;

  // Геометрические операции
  T cross(const Point2 &other) const;
  T dot(const Point2 &other) const;
  T squaredNorm() const;
  T norm() const;
};

// ============================================================
//  Типы-алиасы
// ============================================================
template <typename T> using Polygon2 = std::vector<Point2<T>>;

template <typename T> struct Triangle2 {
  Point2<T> v0, v1, v2;

  Triangle2() = default;
  Triangle2(const Point2<T> &a, const Point2<T> &b, const Point2<T> &c);

  std::array<Point2<T>, 3> asArray() const;
};

// ============================================================
//  Функции работы с полигонами
// ============================================================

// Ориентированная площадь (формула Гаусса)
template <typename T> T signedArea2D(const Polygon2<T> &poly);

// Приведение к обходу против часовой стрелки (CCW)
template <typename T> void ensureCCW(Polygon2<T> &poly);

// Удаление коллинеарных и дублирующихся точек
template <typename T>
void removeCollinearPoints(Polygon2<T> &poly, T eps = EPSILON<T>);

} // namespace geo

// ============================================================
//  Подключение реализаций (шаблонных)
// ============================================================
#include "types.hpp"