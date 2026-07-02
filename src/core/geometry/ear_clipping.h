#pragma once
#include "types.h"
#include <string>
#include <vector>

namespace geo {

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
    Polygon2<T> remaining_vertices;   // вершины, которые остались после отрезания
    Triangle2<T> cut_triangle;        // треугольник, который отрезали на этом шаге
    size_t ear_index;                 // индекс вершины-уха в оставшемся полигоне (до удаления)
};

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
    bool isConvex(const Point2<T>& prev, const Point2<T>& curr,
                  const Point2<T>& next, T eps);
    bool pointInTriangle(const Point2<T>& p, const Point2<T>& a,
                         const Point2<T>& b, const Point2<T>& c, T eps);
    bool isSimplePolygon(const Polygon2<T>& poly, T eps);
};

} // namespace geo

// ============================================================
//  Подключение реализаций
// ============================================================
#include "ear_clipping.hpp"