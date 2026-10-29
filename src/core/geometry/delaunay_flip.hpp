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

    if (const auto dup = findDuplicateVertex(m_points)) {
        result.error = makeDuplicateVerticesError(TriangulationAlgorithm::DelaunayFlip, *dup);
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

    result.vertices = input;
    result.faces.reserve(m_triangles.size());
    for (const auto& tri : m_triangles) {
        result.faces.push_back({tri.a, tri.b, tri.c});
    }

    return result;
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

    // EarClipping уже отдаёт faces как индексы в исходный полигон — берём как есть
    m_triangles.clear();
    m_triangles.reserve(result.faces.size());

    for (const auto& f : result.faces) {
        TriangleRef ref{f[0], f[1], f[2]};
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
std::optional<typename DelaunayFlip<T>::VertexIndex>
DelaunayFlip<T>::oppositeVertex(const TriangleRef& tri, const Edge& edge) const
{
    auto matches = [&](VertexIndex x, VertexIndex y) {
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
    // Приводим к CCW перестановкой двух индексов (без конвертации в координаты)
    const auto& a = m_points[tri.a];
    if ((m_points[tri.b] - a).cross(m_points[tri.c] - a) < T(0)) {
        std::swap(tri.b, tri.c);
    }
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

    TriangleRef new1{ *o1, *o2, static_cast<VertexIndex>(edge.from) };
    TriangleRef new2{ *o2, *o1, static_cast<VertexIndex>(edge.to) };

    normalizeTriangle(new1);
    normalizeTriangle(new2);

    m_triangles[tri1] = new1;
    m_triangles[tri2] = new2;

    DebugStep<T> step;
    step.description = "Flip illegal edge";
    // polygon_before пустой = «все вершины запуска» (см. DebugStep):
    // highlighted_vertices — индексы в m_points.
    step.highlighted_vertices = {edge.from, edge.to, *o1, *o2};
    step.added_diagonals.emplace_back(*o1, *o2);
    step.added_faces.reserve(m_triangles.size());
    for (const auto& tri : m_triangles) {
        step.added_faces.push_back({tri.a, tri.b, tri.c});
    }
    step.replaces_state = true;
    m_history.push_back(step);
}

}// namespace geo
