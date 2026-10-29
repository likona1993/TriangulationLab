#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <optional>

#include "types.h"
#include "triangulation_error.h"

namespace geo {

// ============================================================
//  Индексный формат триангуляции (единый для всех алгоритмов)
// ============================================================
using VertexIndex = std::uint32_t;

// Тройка индексов вершин, обход CCW (инвариант системы)
using IndexedTriangle = std::array<VertexIndex, 3>;

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
  // Индексный вывод (единый формат): faces ссылаются на vertices
  std::vector<Point2<T>> vertices;
  std::vector<IndexedTriangle> faces;

  std::optional<TriangulationError> error;
  explicit operator bool() const noexcept { return !error.has_value(); }
};

// ============================================================
//  Шаг отладки (для визуализации пошагового выполнения)
// ============================================================
// Все геометрические данные шага — индексы в массиве vertices запуска
// (vertices передаётся вместе с историей один раз, см. DebugFacade).
// Конвертация индексов в координаты происходит только в точке экспорта
// (визуализатор), не в алгоритмах (Этап 1 Roadmap).
template <typename T>
struct DebugStep {

  // Состояние полигона перед шагом как индексы в vertices запуска.
  // Пустой вектор = «все вершины» (sentinel для алгоритмов, работающих
  // сразу со всем набором точек: флипы Delaunay, sweep-шаги monotone) —
  // не копируем O(n) индексов на каждый шаг.
  // EarClipping хранит здесь индексы оставшегося working-полигона,
  // monotone piece-шаги — индексы вершин куска.
  std::vector<VertexIndex> polygon_before;

  // Изменения, внесённые на этом шаге:
  // - добавленные треугольники как индексы в vertices (обычно 1 для
  // ear clipping, несколько для монотонной)
  std::vector<IndexedTriangle> added_faces;
  // - добавленные диагонали (для алгоритмов, которые строят диагонали)
  std::vector<std::pair<VertexIndex, VertexIndex>> added_diagonals;
  // - подсвеченные вершины: позиции в polygon_before; при пустом
  // polygon_before — индексы в vertices запуска
  std::vector<size_t> highlighted_vertices;
  // Дополнительная информация для отладки/визуализации
  std::string description;

  // true, если added_faces — полный снимок триангуляции на этот момент
  // (Delaunay flip), а не дельта для накопления (ear clipping, monotone).
  bool replaces_state = false;

  // y-координата sweep line на этом шаге (monotone, фаза разбиения).
  std::optional<T> sweep_y;
};
} // namespace geo
