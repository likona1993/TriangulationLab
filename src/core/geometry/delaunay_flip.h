#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "triangulation_types.h"
#include "types.h"

namespace geo {

// ============================================================
// Триангуляция Делоне методом флипов
// ============================================================
// Структура соседства «треугольник + 3 соседа» (Этап 2 Roadmap):
// m_adj[t][k] — индекс соседнего треугольника через сторону напротив
// вершины k треугольника t, -1 = граница триангуляции.
// Флип перезаписывает 2 треугольника и обновляет ссылки соседства за O(1);
// поиск незаконных рёбер — стек кандидатов (алгоритм Lawson),
// без полной перестройки карты рёбер после каждого флипа.
template <typename T>
class DelaunayFlip {
public:
    using Point = Point2<T>;
    using Points = std::vector<Point>;
    using VertexIndex = geo::VertexIndex;

    TriangulationResult<T> triangulate(const Points& input);

    const std::vector<DebugStep<T>>& getDebugHistory() const { return m_history; }
    void clearDebugHistory() { m_history.clear(); }

private:
    // Ребро внутри триангуляции: треугольник + локальный индекс
    // противолежащей вершины (сторона напротив неё — само ребро).
    struct EdgeRef {
        size_t tri;
        size_t k;
    };

    static constexpr std::int32_t kNoNeighbor = -1;

    std::vector<Point2<T>> m_points;
    std::vector<IndexedTriangle> m_triangles;
    std::vector<std::array<std::int32_t, 3>> m_adj;

    std::vector<DebugStep<T>> m_history;

    bool buildInitialTriangulation();
    void buildAdjacency();
    void legalizeEdges();
    bool isIllegalEdge(size_t tri, size_t k) const;
    void flipEdge(size_t tri, size_t k);
    void normalizeTriangle(IndexedTriangle& tri) const;
};

} // namespace geo

// ============================================================
// Подключение реализаций
// =============================
#include "delaunay_flip.hpp"
