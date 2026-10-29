#pragma once

// Общие хелперы для тестов триангуляции (Этап 1 Roadmap).
// Тесты — легальная точка конвертации «индексы → координаты»: инвариант-
// проверки (площадь, CCW, пересечения рёбер) исторически написаны над
// Triangle2 и остаются на координатном представлении.

#include <vector>

#include "geometry/triangulation_types.h"
#include "geometry/types.h"

namespace geo {

// Материализует faces + vertices в координатные треугольники.
template <typename T>
std::vector<Triangle2<T>> materializeTriangles(const TriangulationResult<T>& result) {
    std::vector<Triangle2<T>> triangles;
    triangles.reserve(result.faces.size());
    for (const auto& f : result.faces) {
        triangles.emplace_back(
            result.vertices[f[0]],
            result.vertices[f[1]],
            result.vertices[f[2]]);
    }
    return triangles;
}

} // namespace geo
