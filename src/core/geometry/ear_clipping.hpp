#pragma once
#include "ear_clipping.h"
#include <algorithm>
#include <numeric>
#include <string>

#include "utils/geometry_predicates.h"
#include "triangulation_error.h"
#include "triangulation_algorithm.h"

namespace geo {

// ============================================================
//  Проверка: является ли вершина idx ухом
// ============================================================
template<typename T>
bool EarClipping<T>::isEar(const Polygon2<T>& poly, size_t idx, T eps) {
    const size_t n = poly.size();
    if (n < 3) return false;

    const Point2<T>& prev = poly[(idx - 1 + n) % n];
    const Point2<T>& curr = poly[idx];
    const Point2<T>& next = poly[(idx + 1) % n];

    if (!isConvex(prev, curr, next, eps)) {
        return false;
    }

    for (size_t i = 0; i < n; ++i) {
        if (i == idx || i == (idx - 1 + n) % n || i == (idx + 1) % n) {
            continue;
        }
        if (pointInTriangle(poly[i], prev, curr, next, eps)) {
            return false;
        }
    }
    return true;
}

// ============================================================
//  Основной алгоритм триангуляции (рефакторинг с лямбдой)
// ============================================================
template<typename T>
TriangulationResult<T> EarClipping<T>::triangulate(const Polygon2<T>& input, T eps) {
    TriangulationResult<T> result;
    m_history.clear();

    if (input.size() < 3) {
        result.error = makeNotEnoughVerticesError(TriangulationAlgorithm::EarClipping);
        return result;
    }

    if (const auto dup = findDuplicateVertex(input, eps)) {
        result.error = makeDuplicateVerticesError(TriangulationAlgorithm::EarClipping, *dup);
        return result;
    }

    if (!isSimplePolygon(input, eps)) {
        result.error =  makeSelfIntersectingPolygonError(TriangulationAlgorithm::EarClipping);
        return result;
    }

    Polygon2<T> working = input;
    result.vertices = input;

    // Исходные индексы вершин, упорядоченные как working:
    // при отрезании уха вершина удаляется из обоих векторов одновременно
    std::vector<VertexIndex> indices(input.size());
    std::iota(indices.begin(), indices.end(), VertexIndex{0});

    // Лямбда, пытающаяся отрезать одно ухо с заданным эпсилон
    auto tryCutEar = [&](T current_eps) -> bool {
        const size_t n = working.size();
        for (size_t i = 0; i < n; ++i) {
            if (isEar(working, i, current_eps)) {
                // Сохраняем состояние ДО удаления
                DebugStep<T> step;
                step.polygon_before = indices;
                step.highlighted_vertices.push_back(i);
                step.description = "Cut ear at vertex " + std::to_string(i);

                const size_t prev_idx = (i - 1 + n) % n;
                const size_t next_idx = (i + 1) % n;
                step.added_faces.push_back({indices[prev_idx], indices[i], indices[next_idx]});

                m_history.push_back(step);
                result.faces.push_back({indices[prev_idx], indices[i], indices[next_idx]});

                // Удаляем вершину-ухо
                working.erase(working.begin() + i);
                indices.erase(indices.begin() + i);
                return true;
            }
        }
        return false;
    };

    // Основной цикл
    while (working.size() > 3) {
        // Сначала пробуем с заданным эпсилон
        if (tryCutEar(eps)) {
            continue;
        }

        // Если не вышло — пробуем с ослабленным эпсилон (×10)
        const T relaxed_eps = eps * T(10);
        if (tryCutEar(relaxed_eps)) {
            continue;
        }

        // Если и это не помогло — ошибка
        result.error = makeError(
            ErrorCategory::Algorithm,
            ErrorCode::EarNotFound,
            "Could not find an ear even after relaxed epsilon",
            TriangulationAlgorithm::EarClipping
            );;
        return result;
    }

    // Остался последний треугольник
    if (working.size() == 3) {
        DebugStep<T> step;
        step.polygon_before = indices;
        step.highlighted_vertices.push_back(0);
        step.description = "Final triangle";
        step.added_faces.push_back({indices[0], indices[1], indices[2]});
        m_history.push_back(step);
        result.faces.push_back({indices[0], indices[1], indices[2]});
    }

    return result;
}

} // namespace geo
