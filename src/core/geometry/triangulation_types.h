#pragma once
#include "types.h"

namespace geo{

// ============================================================
//  Ребро
// ============================================================
    struct Edge { size_t from, to; };
// ============================================================
//  Результат триангуляции
// ============================================================
template<typename T>
struct TriangulationResult {
    std::vector<Triangle2<T>> triangles;
    bool success = false;
    std::string error_message;
};

// ============================================================
//  Шаг отладки (для визуализации пошагового выполнения)
// ============================================================
template<typename T>
struct DebugStep {
    /*Polygon2<T> remaining_vertices;   // вершины, которые остались после отрезания
    Triangle2<T> cut_triangle;        // треугольник, который отрезали на этом шаге
    size_t ear_index;                 // индекс вершины-уха в оставшемся полигоне (до удаления)*/

    // Состояние полигона перед шагом (для отрисовки)
    Polygon2<T> polygon_before;

    // Изменения, внесённые на этом шаге:
    // - добавленные треугольники (обычно 1 для ear clipping, несколько для монотонной)
    std::vector<Triangle2<T>> added_triangles;
    // - добавленные диагонали (для алгоритмов, которые строят диагонали)
    std::vector<std::pair<Point2<T>, Point2<T>>> added_diagonals;
    // - удалённые вершины (индексы в polygon_before) – полезно для ear clipping
    std::vector<size_t> removed_vertices;

    // Дополнительная информация для отладки/визуализации
    std::string description;
    //Пока оставим для EarClipping
    size_t ear_index;
};
}// namespace geo