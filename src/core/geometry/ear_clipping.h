#pragma once
#include "types.h"
#include <string>
#include <vector>

#include "geometry/triangulation_types.h"

namespace geo {



// ============================================================
//  Класс триангуляции методом отрезания ушей
// ============================================================
template<typename T>
class EarClipping {
public:
    // Основной метод
    TriangulationResult<T> triangulate(const Polygon2<T>& input, T eps = EPSILON<T>);

    // Для визуального дебаггера
    const std::vector<DebugStep<T>>& getDebugHistory() const { return m_history; }
    void clearDebugHistory() { m_history.clear(); }

private:
    std::vector<DebugStep<T>> m_history;

    // Вспомогательные методы
    bool isEar(const Polygon2<T>& poly, size_t idx, T eps);
};

} // namespace geo

// ============================================================
//  Подключение реализаций
// ============================================================
#include "ear_clipping.hpp"