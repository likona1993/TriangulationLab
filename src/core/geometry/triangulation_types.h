#pragma once

#include <string>
#include <optional>

#include "types.h"
#include "triangulation_error.h"

namespace geo {

// ============================================================
//  Ребро
// ============================================================
struct Edge {
  size_t from, to;
};
// Статический порядок на рёбрах — НЕ зависит от sweep line.
struct EdgeLess {
  bool operator()(const Edge &a, const Edge &b) const {
    if (a.from != b.from)
      return a.from < b.from;
    return a.to < b.to;
  }
};
// ============================================================
//  Результат триангуляции
// ============================================================
template <typename T>
struct TriangulationResult {
  std::vector<Triangle2<T>> triangles;
  std::optional<TriangulationError> error;
  explicit operator bool() const noexcept { return !error.has_value(); }
};

// ============================================================
//  Шаг отладки (для визуализации пошагового выполнения)
// ============================================================
template <typename T>
struct DebugStep {

  // Состояние полигона перед шагом (для отрисовки)
  Polygon2<T> polygon_before;

  // Изменения, внесённые на этом шаге:
  // - добавленные треугольники (обычно 1 для ear clipping, несколько для
  // монотонной)
  std::vector<Triangle2<T>> added_triangles;
  // - добавленные диагонали (для алгоритмов, которые строят диагонали)
  std::vector<std::pair<Point2<T>, Point2<T>>> added_diagonals;
  // - удалённые вершины (индексы в polygon_before) – полезно для ear clipping
  std::vector<size_t> highlighted_vertices;
  // Дополнительная информация для отладки/визуализации
  std::string description;

  // true, если added_triangles — полный снимок триангуляции на этот момент
  // (Delaunay flip), а не дельта для накопления (ear clipping, monotone).
  bool replaces_state = false;

  // y-координата sweep line на этом шаге (monotone, фаза разбиения).
  std::optional<T> sweep_y;
};
} // namespace geo
