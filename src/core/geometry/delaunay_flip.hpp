#include "delanuay_flip.h"

#include "ear_clipping.h"

namespace geo{
template <typename T>
TriangulationResult<T> DelaunayFlip<T>::triangulate(const Points& input)
{
    TriangulationResult<T> result;

    m_points = input;
    m_triangles.clear();
    m_history.clear();

    if (m_points.size() < 3) {
        result.error = makeNotEnoughVerticesError(TriangulationAlgorithm::DelaunayFlip);
        return result;
    }

    if (!buildInitialTriangulation()) {
        result.error = makeError(
            ErrorCategory::Algorithm,
            ErrorCode::InitialTriangulationFailed,
            "Failed to build initial triangulation",
            TriangulationAlgorithm::DelaunayFlip
            );
        return result;
    }

    rebuildEdgeMap();

    while (true) {
        auto illegalEdge = findIllegalEdge();
        if (!illegalEdge.has_value()) {
            break;
        }

        flipEdge(illegalEdge->tri1, illegalEdge->tri2, illegalEdge->edge);
        rebuildEdgeMap();
        recordFlipStep(*illegalEdge);
    }

    result.triangles = exportTriangles();
    return result;
}

template <typename T>
std::optional<size_t> DelaunayFlip<T>::indexOfPoint(const Point& p) const
{
    for (size_t i = 0; i < m_points.size(); ++i) {
        if (m_points[i] == p) {
            return i;
        }
    }

    return std::nullopt;
}

template <typename T>
bool DelaunayFlip<T>::buildInitialTriangulation()
{
    geo::EarClipping<T> earClipping;
    auto result = earClipping.triangulate(m_points);

    if (!result) {
        return false;
    }

    m_triangles.clear();
    m_triangles.reserve(result.triangles.size());

    for (auto tri : result.triangles) {
        ensureCCW(tri);

        auto ia = indexOfPoint(tri.v0);
        auto ib = indexOfPoint(tri.v1);
        auto ic = indexOfPoint(tri.v2);

        if (!ia.has_value() || !ib.has_value() || !ic.has_value()) {
            return false;
        }

        TriangleRef ref;
        ref.a = *ia;
        ref.b = *ib;
        ref.c = *ic;

        m_triangles.push_back(ref);
    }

    return true;
}

template <typename T>
void DelaunayFlip<T>::rebuildEdgeMap()
{
    m_edgeMap.clear();

    auto addEdge = [this](VertexIndex u, VertexIndex v, size_t triIndex) {
        if (u > v) {
            std::swap(u, v);
        }

        m_edgeMap[{u, v}].push_back(triIndex);
    };

    for (size_t i = 0; i < m_triangles.size(); ++i) {
        const auto& tri = m_triangles[i];

        addEdge(tri.a, tri.b, i);
        addEdge(tri.b, tri.c, i);
        addEdge(tri.c, tri.a, i);
    }
}

template <typename T>
std::optional<typename DelaunayFlip<T>::IllegalEdgeInfo>
DelaunayFlip<T>::findIllegalEdge() const
{
    for (const auto& [edge, tris] : m_edgeMap) {
        if (tris.size() != 2) {
            continue; // граничное ребро
        }

        const size_t t1 = tris[0];
        const size_t t2 = tris[1];

        if (isIllegalEdge(t1, t2, edge)) {
            return IllegalEdgeInfo{t1, t2, edge};
        }
    }

    return std::nullopt;
}
//TODO - нужно ли перенести в geometry_predicates
template <typename T>
bool inCircumcircle(const geo::Point2<T>& a,
                    const geo::Point2<T>& b,
                    const geo::Point2<T>& c,
                    const geo::Point2<T>& d)
{
    T ax = a.x - d.x;
    T ay = a.y - d.y;
    T bx = b.x - d.x;
    T by = b.y - d.y;
    T cx = c.x - d.x;
    T cy = c.y - d.y;

    T det =
        (ax * ax + ay * ay) * (bx * cy - by * cx) -
        (bx * bx + by * by) * (ax * cy - ay * cx) +
        (cx * cx + cy * cy) * (ax * by - ay * bx);

    return det > T(0);
}

template <typename T>
std::optional<size_t> DelaunayFlip<T>::oppositeVertex(const TriangleRef& tri, const Edge& edge) const
{
    auto matches = [&](size_t x, size_t y) {
        return (x == edge.from && y == edge.to) ||
               (x == edge.to && y == edge.from);
    };

    if (matches(tri.a, tri.b)) {
        return tri.c;
    }
    if (matches(tri.b, tri.c)) {
        return tri.a;
    }
    if (matches(tri.c, tri.a)) {
        return tri.b;
    }

    return std::nullopt;
}


template <typename T>
bool DelaunayFlip<T>::isIllegalEdge(size_t tri1, size_t tri2, const Edge& edge) const
{
    const auto& t1 = m_triangles[tri1];
    const auto& t2 = m_triangles[tri2];

    auto opposite1 = oppositeVertex(t1, edge);
    auto opposite2 = oppositeVertex(t2, edge);

    if (!opposite1.has_value() || !opposite2.has_value()) {
        return false;
    }

    const auto a = m_points[edge.from];
    const auto b = m_points[edge.to];
    const auto c = m_points[*opposite1];
    const auto d = m_points[*opposite2];

    if (orientation(a, b, c) * orientation(a, b, d) > T(0)) {
        return false;
    }

    return inCircumcircle(a, b, c, d);
}


}// namespace geo
