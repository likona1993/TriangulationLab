#pragma once
#include "ear_clipping.h"
#include <algorithm>
#include <string>

#include "utils/geometry_predicates.h"

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
        result.error_message = "Polygon must have at least 3 vertices";
        return result;
    }

    if (!isSimplePolygon(input, eps)) {
        result.error_message = "Polygon is self-intersecting";
        return result;
    }

    Polygon2<T> working = input;

    // Лямбда, пытающаяся отрезать одно ухо с заданным эпсилон
    auto tryCutEar = [&](T current_eps) -> bool {
        const size_t n = working.size();
        for (size_t i = 0; i < n; ++i) {
            if (isEar(working, i, current_eps)) {
                // Сохраняем состояние ДО удаления
                DebugStep<T> step;
                step.polygon_before = working;
                step.ear_index = i;

                const size_t prev_idx = (i - 1 + n) % n;
                const size_t next_idx = (i + 1) % n;
                step.added_triangles.push_back(Triangle2<T>(working[prev_idx], working[i], working[next_idx]));

                m_history.push_back(step);
                result.triangles.push_back(step.added_triangles[0]);

                // Удаляем вершину-ухо
                working.erase(working.begin() + i);
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
        result.success = false;
        result.error_message = "No ear found in polygon (possibly degenerate or self-intersecting)";
        return result;
    }

    // Остался последний треугольник
    if (working.size() == 3) {
        DebugStep<T> step;
        step.polygon_before = working;
        step.ear_index = 0;
        step.added_triangles.push_back(Triangle2<T>(working[0], working[1], working[2]));
        m_history.push_back(step);
        result.triangles.push_back(step.added_triangles[0]);
    }

    result.success = true;
    return result;
}

} // namespace geo