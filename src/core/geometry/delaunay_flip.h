#pragma once

#include <vector>
#include <map>
#include <utility>

#include "triangulation_types.h"
#include "types.h"

namespace geo {

// ============================================================
// Триангуляция Делоне методом флипов
// ============================================================
template <typename T>
class DelaunayFlip {
public:
    using Point = Point2<T>;
    using Points = std::vector<Point>;
    using VertexIndex = geo::VertexIndex;
    using EdgeKey = geo::Edge;
    using EdgeToTriangles = std::map<EdgeKey, std::vector<size_t>, geo::EdgeLess>;

    TriangulationResult<T> triangulate(const Points& input);

    const std::vector<DebugStep<T>>& getDebugHistory() const { return m_history; }
    void clearDebugHistory() { m_history.clear(); }

private:
    struct TriangleRef {
        VertexIndex a = 0;
        VertexIndex b = 0;
        VertexIndex c = 0;
    };

    struct IllegalEdgeInfo {
        size_t tri1;
        size_t tri2;
        Edge edge;
    };

    std::vector<Point2<T>> m_points;
    std::vector<TriangleRef> m_triangles;
    EdgeToTriangles m_edgeMap;

    std::vector<DebugStep<T>> m_history;

    bool buildInitialTriangulation();
    void rebuildEdgeMap();
    std::optional<IllegalEdgeInfo> findIllegalEdge() const;
    std::optional<VertexIndex> oppositeVertex(const TriangleRef& tri, const Edge& edge) const;
    bool isIllegalEdge(size_t tri1, size_t tri2, const Edge& edge) const;
    void flipEdge(size_t tri1, size_t tri2, const Edge& edge);
    void normalizeTriangle(TriangleRef& tri) const;
};

} // namespace geo

// ============================================================
// Подключение реализаций
// =============================
#include "delaunay_flip.hpp"
