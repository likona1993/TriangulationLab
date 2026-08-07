#pragma once
#include "delaunay_flip.h"

#include "ear_clipping.h"
#include "monotone_triangulation.h"
#include "utils/geometry_predicates.h"

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
    geo::EarClipping<T> initial_algo;
    auto result = initial_algo.triangulate(m_points);
    m_history = initial_algo.getDebugHistory();

    if (!result) {
        return false;
    }

    m_triangles.clear();
    m_triangles.reserve(result.triangles.size());

    for (auto tri : result.triangles) {

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

        normalizeTriangle(ref);

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

    const int o1 = orientation(a, b, c);
    const int o2 = orientation(a, b, d);

    if (o1 == 0 || o2 == 0 || o1 == o2) {
        return false;
    }

    // Важно: inCircumcircle зависит от ориентации (a,b,c).
    // Чтобы знак был корректным, приводим треугольник abc к CCW.
    if (o1 == 1) {
        return inCircumcircle(a, c, b, d);
    }

    return inCircumcircle(a, b, c, d);
}

template<typename T>
void DelaunayFlip<T>::normalizeTriangle(TriangleRef& tri) const
{
    Triangle2<T> tmp{ m_points[tri.a], m_points[tri.b], m_points[tri.c] };
    ensureCCW(tmp);

    auto i0 = indexOfPoint(tmp.v0);
    auto i1 = indexOfPoint(tmp.v1);
    auto i2 = indexOfPoint(tmp.v2);

    if (!i0 || !i1 || !i2) {
        return;
    }

    tri.a = *i0;
    tri.b = *i1;
    tri.c = *i2;
}

template<typename T>
void DelaunayFlip<T>::flipEdge(size_t tri1, size_t tri2, const Edge& edge)
{
    const auto& t1 = m_triangles[tri1];
    const auto& t2 = m_triangles[tri2];

    auto o1 = oppositeVertex(t1, edge);
    auto o2 = oppositeVertex(t2, edge);

    if (!o1.has_value() || !o2.has_value()) {
        return;
    }

    TriangleRef new1{ *o1, *o2, edge.from };
    TriangleRef new2{ *o2, *o1, edge.to };

    normalizeTriangle(new1);
    normalizeTriangle(new2);

    m_triangles[tri1] = new1;
    m_triangles[tri2] = new2;

    DebugStep<T> step;
    step.description = "Flip illegal edge";
    step.added_diagonals.emplace_back(m_points[edge.from], m_points[edge.to]);
    step.added_triangles = exportTriangles();
    m_history.push_back(step);
}



template<typename T>
typename DelaunayFlip<T>::Triangles DelaunayFlip<T>::exportTriangles() const
{
    Triangles result;
    result.reserve(m_triangles.size());

    for (const auto& tri : m_triangles) {
        result.emplace_back(
            m_points[tri.a],
            m_points[tri.b],
            m_points[tri.c]
            );
    }

    return result;
}


}// namespace geo
