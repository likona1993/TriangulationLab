#pragma once
#include "delaunay_flip.h"

#include <unordered_map>

#include "ear_clipping.h"
#include "utils/assert.h"
#include "utils/geometry_predicates.h"

namespace geo{
template <typename T>
TriangulationResult<T> DelaunayFlip<T>::triangulate(const Points& input)
{
    TriangulationResult<T> result;

    m_points = input;
    m_triangles.clear();
    m_adj.clear();
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

    buildAdjacency();
    legalizeEdges();

    result.vertices = input;
    result.faces = m_triangles;

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
        IndexedTriangle tri = f;
        normalizeTriangle(tri);
        m_triangles.push_back(tri);
    }

    return true;
}

template <typename T>
void DelaunayFlip<T>::buildAdjacency()
{
    m_adj.assign(m_triangles.size(), {kNoNeighbor, kNoNeighbor, kNoNeighbor});

    // Направленное полуребро (from -> to) -> (треугольник, локальный индекс).
    // В CCW-триангуляции внутреннее ребро встречается ровно дважды —
    // в противоположных направлениях у двух соседних треугольников.
    std::unordered_map<std::uint64_t, EdgeRef> halfEdges;
    halfEdges.reserve(m_triangles.size() * 3);

    auto key = [](VertexIndex from, VertexIndex to) {
        return (std::uint64_t(from) << 32) | std::uint64_t(to);
    };

    for (size_t t = 0; t < m_triangles.size(); ++t) {
        const auto& tri = m_triangles[t];
        for (size_t k = 0; k < 3; ++k) {
            const VertexIndex from = tri[(k + 1) % 3];
            const VertexIndex to = tri[(k + 2) % 3];

            const auto it = halfEdges.find(key(to, from));
            if (it != halfEdges.end()) {
                m_adj[t][k] = static_cast<std::int32_t>(it->second.tri);
                m_adj[it->second.tri][it->second.k] = static_cast<std::int32_t>(t);
                halfEdges.erase(it);
            } else {
                halfEdges.emplace(key(from, to), EdgeRef{t, k});
            }
        }
    }
}

template <typename T>
void DelaunayFlip<T>::legalizeEdges()
{
    // Алгоритм Lawson: стек рёбер-кандидатов вместо полного перебора.
    // После флипа незаконными могут стать только внешние рёбра
    // затронутого четырёхугольника — проверяем только их.
    std::vector<EdgeRef> stack;
    stack.reserve(m_triangles.size() * 3);
    for (size_t t = 0; t < m_triangles.size(); ++t) {
        for (size_t k = 0; k < 3; ++k) {
            if (m_adj[t][k] != kNoNeighbor) {
                stack.push_back({t, k});
            }
        }
    }

    while (!stack.empty()) {
        const EdgeRef edge = stack.back();
        stack.pop_back();

        const std::int32_t other = m_adj[edge.tri][edge.k];
        if (other == kNoNeighbor) {
            continue; // граничное ребро — флип невозможен
        }

        if (!isIllegalEdge(edge.tri, edge.k)) {
            continue;
        }

        flipEdge(edge.tri, edge.k);

        // Новая диагональ законна по построению; в стек — 4 внешних
        // ребра четырёхугольника (стороны обоих новых треугольников,
        // кроме общей).
        const size_t t1 = edge.tri;
        const size_t t2 = static_cast<size_t>(other);
        for (const size_t t : {t1, t2}) {
            for (size_t k = 0; k < 3; ++k) {
                const std::int32_t nb = m_adj[t][k];
                if (nb != kNoNeighbor &&
                    nb != static_cast<std::int32_t>(t1) &&
                    nb != static_cast<std::int32_t>(t2)) {
                    stack.push_back({t, k});
                }
            }
        }
    }
}

template <typename T>
bool DelaunayFlip<T>::isIllegalEdge(size_t tri, size_t k) const
{
    const std::int32_t other = m_adj[tri][k];
    if (other == kNoNeighbor) {
        return false;
    }

    const auto& t1 = m_triangles[tri];
    const auto& t2 = m_triangles[static_cast<size_t>(other)];

    const VertexIndex o1 = t1[k];
    const VertexIndex p1 = t1[(k + 1) % 3];
    const VertexIndex p2 = t1[(k + 2) % 3];

    // Противолежащая вершина соседа — та, что не лежит на общем ребре
    VertexIndex o2 = o1;
    for (const VertexIndex v : t2) {
        if (v != p1 && v != p2) {
            o2 = v;
            break;
        }
    }
    if (o2 == o1) {
        return false;
    }

    // t1 = (o1, p1, p2) — CCW, поэтому o1 всегда слева от ребра p1 -> p2.
    // Флип возможен, только если o2 строго справа (четырёхугольник выпуклый);
    // 0 = коллинеарна, 2 = та же сторона (orientation: 1 = CW, 2 = CCW).
    if (orientation(m_points[p1], m_points[p2], m_points[o2]) != 1) {
        return false;
    }

    // inCircumcircle требует CCW-треугольник — (o1, p1, p2) уже CCW
    return inCircumcircle(m_points[o1], m_points[p1], m_points[p2], m_points[o2]);
}

template<typename T>
void DelaunayFlip<T>::normalizeTriangle(IndexedTriangle& tri) const
{
    // Приводим к CCW перестановкой двух индексов (без конвертации в координаты)
    const auto& a = m_points[tri[0]];
    if ((m_points[tri[1]] - a).cross(m_points[tri[2]] - a) < T(0)) {
        std::swap(tri[1], tri[2]);
    }
}

template<typename T>
void DelaunayFlip<T>::flipEdge(size_t tri1, size_t k1)
{
    const std::int32_t tri2raw = m_adj[tri1][k1];
    if (tri2raw == kNoNeighbor) {
        return;
    }
    const size_t tri2 = static_cast<size_t>(tri2raw);

    const IndexedTriangle t1 = m_triangles[tri1];
    const IndexedTriangle t2 = m_triangles[tri2];

    const VertexIndex o1 = t1[k1];
    const VertexIndex p1 = t1[(k1 + 1) % 3];
    const VertexIndex p2 = t1[(k1 + 2) % 3];

    size_t k2 = 3;
    for (size_t j = 0; j < 3; ++j) {
        if (t2[j] != p1 && t2[j] != p2) {
            k2 = j;
            break;
        }
    }
    GEO_ASSERT_INVARIANT(k2 < 3, "flipEdge: neighbor does not share the edge");
    if (k2 == 3) {
        return;
    }
    const VertexIndex o2 = t2[k2];

    // Внешние соседи четырёхугольника (по сторонам (o1,p1), (p2,o1),
    // (p1,o2), (o2,p2)) — снимаем до перезаписи m_adj.
    const std::int32_t nA = m_adj[tri1][(k1 + 2) % 3]; // через (o1, p1)
    const std::int32_t nB = m_adj[tri1][(k1 + 1) % 3]; // через (p2, o1)
    const std::int32_t nC = m_adj[tri2][(k2 + 1) % 3]; // через (p1, o2)
    const std::int32_t nD = m_adj[tri2][(k2 + 2) % 3]; // через (o2, p2)

    IndexedTriangle newT1{o1, o2, p1};
    IndexedTriangle newT2{o2, o1, p2};
    normalizeTriangle(newT1);
    normalizeTriangle(newT2);

    // Сосед через сторону определяется её содержимым, а не позицией
    // после normalizeTriangle: диагональ (o1,o2) — другой новый
    // треугольник, остальные стороны — внешние соседи.
    auto externalNeighbor = [&](VertexIndex u, VertexIndex v) -> std::int32_t {
        const bool hasP1 = (u == p1 || v == p1);
        const bool hasO1 = (u == o1 || v == o1);
        if (hasO1 && hasP1) {
            return nA;
        }
        if (hasO1) {
            return nB; // (o1, p2)
        }
        if (hasP1) {
            return nC; // (o2, p1)
        }
        return nD;     // (o2, p2)
    };

    const std::int32_t i1 = static_cast<std::int32_t>(tri1);
    const std::int32_t i2 = static_cast<std::int32_t>(tri2);
    for (size_t k = 0; k < 3; ++k) {
        const VertexIndex u1 = newT1[(k + 1) % 3];
        const VertexIndex v1 = newT1[(k + 2) % 3];
        const bool diag1 = (u1 == o1 || u1 == o2) && (v1 == o1 || v1 == o2);
        m_adj[tri1][k] = diag1 ? i2 : externalNeighbor(u1, v1);

        const VertexIndex u2 = newT2[(k + 1) % 3];
        const VertexIndex v2 = newT2[(k + 2) % 3];
        const bool diag2 = (u2 == o1 || u2 == o2) && (v2 == o1 || v2 == o2);
        m_adj[tri2][k] = diag2 ? i1 : externalNeighbor(u2, v2);
    }

    // Обратные ссылки внешних соседей: стороны (o1,p1) и (o2,p2) остаются
    // у тех же слотов tri1/tri2, а стороны (o1,p2) и (o2,p1) переезжают
    // к другому треугольнику — перелинковываем только их.
    auto relink = [this](std::int32_t neighbor, std::int32_t from, std::int32_t to) {
        if (neighbor == kNoNeighbor) {
            return;
        }
        for (auto& entry : m_adj[static_cast<size_t>(neighbor)]) {
            if (entry == from) {
                entry = to;
                return;
            }
        }
        GEO_ASSERT_INVARIANT(false, "flipEdge: neighbor back-reference not found");
    };
    relink(nB, i1, i2);
    relink(nC, i2, i1);

    m_triangles[tri1] = newT1;
    m_triangles[tri2] = newT2;

    // Соседство остаётся симметричным: ссылка t -> u влечёт u -> t
    for (const size_t t : {tri1, tri2}) {
        for (size_t k = 0; k < 3; ++k) {
            const std::int32_t nb = m_adj[t][k];
            if (nb == kNoNeighbor) {
                continue;
            }
            const auto& back = m_adj[static_cast<size_t>(nb)];
            const auto it = static_cast<std::int32_t>(t);
            GEO_ASSERT_INVARIANT(
                back[0] == it || back[1] == it || back[2] == it,
                "flipEdge: adjacency is not symmetric");
        }
    }

    DebugStep<T> step;
    step.description = "Flip illegal edge";
    // polygon_before пустой = «все вершины запуска» (см. DebugStep):
    // highlighted_vertices — индексы в m_points.
    step.highlighted_vertices = {p1, p2, o1, o2};
    step.added_diagonals.emplace_back(o1, o2);
    step.added_faces.reserve(m_triangles.size());
    for (const auto& tri : m_triangles) {
        step.added_faces.push_back(tri);
    }
    step.replaces_state = true;
    m_history.push_back(step);
}

}// namespace geo
