#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include "geometry/monotone_triangulation.h"
#include "geometry/types.h"
#include "test_triangulation_utils.h"

using namespace geo;

namespace {

// Абсолютная площадь полигона.
template<typename T>
T absArea(const Polygon2<T>& poly) {
    return std::abs(signedArea2D(poly));
}

// Суммарная площадь треугольников результата.
template<typename T>
T sumTrianglesArea(const std::vector<Triangle2<T>>& tris) {
    T sum = 0;
    for (const auto& t : tris) {
        Polygon2<T> triPoly = {t.v0, t.v1, t.v2};
        sum += absArea(triPoly);
    }
    return sum;
}

// Проверка, что все треугольники имеют ненулевую площадь.
template<typename T>
bool allTrianglesNonDegenerate(const std::vector<Triangle2<T>>& tris, T eps) {
    for (const auto& t : tris) {
        Polygon2<T> triPoly = {t.v0, t.v1, t.v2};
        if (absArea(triPoly) <= eps) {
            return false;
        }
    }
    return true;
}


template<typename T>
T signedTriangleArea2D(const Triangle2<T>& tri) {
    return (tri.v1 - tri.v0).cross(tri.v2 - tri.v0) * T(0.5);
}

template<typename T>
bool allTrianglesCCW(const std::vector<Triangle2<T>>& tris, T eps) {
    for (const auto& t : tris) {
        if (signedTriangleArea2D(t) <= eps) {
            return false;
        }
    }
    return true;
}

bool hasDiagonalStep(const std::vector<DebugStep<double>>& history) {
    for (const auto& step : history) {
        if (!step.added_diagonals.empty()) {
            return true;
        }
    }
    return false;
}

Polygon2<double> makeSquare() {
    return {
        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0}
    };
}

Polygon2<double> makeLShape() {
    return {
        {0.0, 0.0},
        {2.0, 0.0},
        {2.0, 1.0},
        {1.0, 1.0},
        {1.0, 2.0},
        {0.0, 2.0}
    };
}

Polygon2<double> makeBowTie() {
    return {
        {0.0, 0.0},
        {2.0, 2.0},
        {0.0, 2.0},
        {2.0, 0.0}
    };
}

Polygon2<double> makeSplitMergePolygon() {
    return {
        {0.0, 0.0},
        {4.0, 0.0},
        {4.0, 4.0},
        {2.5, 4.0},
        {2.5, 2.5},
        {1.5, 2.5},
        {1.5, 4.0},
        {0.0, 4.0}
    };
}

} // namespace

// Проверяем базовый контракт: полигон из менее чем 3 вершин
// не может быть триангулирован, алгоритм должен вернуть ошибку
// и не оставлять после себя debug-history.
TEST(MonotoneTriangulationTest, RejectsPolygonWithTooFewVertices) {
    Polygon2<double> poly = {{0.0, 0.0}, {1.0, 0.0}};
    MonotoneTriangulation<double> mt;

    auto result = mt.triangulate(poly);

    EXPECT_FALSE(result);
    EXPECT_EQ(result.error->code, geo::ErrorCode::NotEnoughVertices);
    EXPECT_TRUE(result.faces.empty());
    EXPECT_TRUE(mt.getDebugHistory().empty());
}

// Самопересекающийся полигон — некорректный вход.
// Ожидаем явный отказ до запуска основной логики разбиения.
TEST(MonotoneTriangulationTest, RejectsSelfIntersectingPolygon) {
    MonotoneTriangulation<double> mt;

    auto result = mt.triangulate(makeBowTie());

    EXPECT_FALSE(result);
    EXPECT_EQ(result.error->code, geo::ErrorCode::SelfIntersectingPolygon);
    EXPECT_TRUE(result.faces.empty());
    EXPECT_TRUE(mt.getDebugHistory().empty());
}

// Простейший корректный случай: выпуклый квадрат.
// Должны получить ровно два треугольника и хотя бы несколько debug-шагов.
TEST(MonotoneTriangulationTest, TriangulatesSquareIntoTwoTriangles) {
    auto square = makeSquare();
    MonotoneTriangulation<double> mt;

    auto result = mt.triangulate(square);

    ASSERT_TRUE(result) << result.error->debugMessage;
    const auto triangles = materializeTriangles(result);
    EXPECT_EQ(triangles.size(), 2u);

    EXPECT_NEAR(sumTrianglesArea(triangles), absArea(square), EPSILON<double>);
    EXPECT_TRUE(allTrianglesNonDegenerate(triangles, EPSILON<double>));
    EXPECT_TRUE(allTrianglesCCW(triangles, EPSILON<double>));

    const auto& history = mt.getDebugHistory();
    EXPECT_FALSE(history.empty());

    bool hasTriangleStep = false;
    for (const auto& step : history) {
        if (!step.added_faces.empty()) {
            hasTriangleStep = true;
            EXPECT_FALSE(step.description.empty());
        }
    }
    EXPECT_TRUE(hasTriangleStep);
}

// Вогнутый, но простой полигон: проверяем, что алгоритм справляется
// не только с выпуклым случаем и выдает стандартное число треугольников n - 2.
TEST(MonotoneTriangulationTest, TriangulatesConcavePolygonIntoNMinusTwoTriangles) {
    MonotoneTriangulation<double> mt;
    auto poly = makeLShape();

    auto result = mt.triangulate(poly);

    ASSERT_TRUE(result) << result.error->debugMessage;
    const auto triangles = materializeTriangles(result);
    EXPECT_EQ(triangles.size(), poly.size() - 2);

    EXPECT_NEAR(sumTrianglesArea(triangles), absArea(poly), EPSILON<double>);
    EXPECT_TRUE(allTrianglesNonDegenerate(triangles, EPSILON<double>));
    EXPECT_TRUE(allTrianglesCCW(triangles, EPSILON<double>));

    EXPECT_FALSE(mt.getDebugHistory().empty());
}

// История отладки должна пересоздаваться на каждом запуске triangulate(),
// чтобы старые шаги не смешивались с новым результатом.
TEST(MonotoneTriangulationTest, ClearsDebugHistoryBeforeNextRun) {
    MonotoneTriangulation<double> mt;

    auto first = mt.triangulate(makeLShape());
    ASSERT_TRUE(first) << first.error->debugMessage;
    ASSERT_FALSE(mt.getDebugHistory().empty());

    auto second = mt.triangulate(Polygon2<double>{{0.0, 0.0}, {1.0, 0.0}});
    EXPECT_FALSE(second);
    EXPECT_EQ(second.error->code, geo::ErrorCode::NotEnoughVertices);
    EXPECT_TRUE(mt.getDebugHistory().empty());
}

// Полигон с "врезкой" требует разбиения на монотонные куски.
// Проверяем, что алгоритм действительно добавляет хотя бы одну диагональ
// и корректно завершает триангуляцию.
TEST(MonotoneTriangulationTest, RecordsDiagonalsForSplitMergePolygon) {
    MonotoneTriangulation<double> mt;
    auto poly = makeSplitMergePolygon();

    auto result = mt.triangulate(poly);

    ASSERT_TRUE(result) << result.error->debugMessage;
    const auto triangles = materializeTriangles(result);
    EXPECT_EQ(triangles.size(), poly.size() - 2);
    EXPECT_NEAR(sumTrianglesArea(triangles), absArea(poly), EPSILON<double>);
    EXPECT_TRUE(allTrianglesNonDegenerate(triangles, EPSILON<double>));
    EXPECT_TRUE(allTrianglesCCW(triangles, EPSILON<double>));

    const auto& history = mt.getDebugHistory();
    ASSERT_FALSE(history.empty());
    EXPECT_TRUE(hasDiagonalStep(history));

    bool diagonalStepIsWellFormed = false;
    for (const auto& step : history) {
        if (!step.added_diagonals.empty()) {
            diagonalStepIsWellFormed = true;
            EXPECT_FALSE(step.description.empty());
            EXPECT_FALSE(step.highlighted_vertices.empty());
        }
    }
    EXPECT_TRUE(diagonalStepIsWellFormed);
}

// Выпуклые полигоны произвольной ориентации: sweep не должен зависеть
// от того, что полигон выровнен по осям.
TEST(MonotoneTriangulationTest, TriangulatesConvexPentagon) {
    Polygon2<double> pentagon = {
        {0.0, 1.0}, {0.951, 0.309}, {0.588, -0.809}, {-0.588, -0.809}, {-0.951, 0.309}
    };
    ensureCCW(pentagon);
    MonotoneTriangulation<double> mt;

    auto result = mt.triangulate(pentagon);

    ASSERT_TRUE(result) << result.error->debugMessage;
    const auto triangles = materializeTriangles(result);
    EXPECT_EQ(triangles.size(), pentagon.size() - 2);
    EXPECT_NEAR(sumTrianglesArea(triangles), absArea(pentagon), 1e-9);
    EXPECT_TRUE(allTrianglesNonDegenerate(triangles, EPSILON<double>));
    EXPECT_TRUE(allTrianglesCCW(triangles, EPSILON<double>));
}

TEST(MonotoneTriangulationTest, TriangulatesConvexHexagon) {
    Polygon2<double> hexagon = {
        {1.0, 0.0}, {0.5, 0.866}, {-0.5, 0.866}, {-1.0, 0.0}, {-0.5, -0.866}, {0.5, -0.866}
    };
    ensureCCW(hexagon);
    MonotoneTriangulation<double> mt;

    auto result = mt.triangulate(hexagon);

    ASSERT_TRUE(result) << result.error->debugMessage;
    const auto triangles = materializeTriangles(result);
    EXPECT_EQ(triangles.size(), hexagon.size() - 2);
    EXPECT_NEAR(sumTrianglesArea(triangles), absArea(hexagon), 1e-9);
    EXPECT_TRUE(allTrianglesNonDegenerate(triangles, EPSILON<double>));
    EXPECT_TRUE(allTrianglesCCW(triangles, EPSILON<double>));
}

// Прямоугольник 10x10 с двумя треугольными вырезами
// (пример из docs/monotone_triangulation_phase3.md): два split/merge-случая
// на одном полигоне.
TEST(MonotoneTriangulationTest, TriangulatesRectangleWithTriangularCutouts) {
    Polygon2<double> poly = {
        {0.0, 0.0},  {3.0, 0.0},  {5.0, 4.0},  {7.0, 0.0},  {10.0, 0.0},
        {10.0, 10.0}, {7.0, 10.0}, {5.0, 6.0},  {3.0, 10.0}, {0.0, 10.0}
    };
    MonotoneTriangulation<double> mt;

    auto result = mt.triangulate(poly);

    ASSERT_TRUE(result) << result.error->debugMessage;
    const auto triangles = materializeTriangles(result);
    EXPECT_EQ(triangles.size(), poly.size() - 2);
    EXPECT_NEAR(sumTrianglesArea(triangles), absArea(poly), 1e-9);
    EXPECT_TRUE(allTrianglesNonDegenerate(triangles, EPSILON<double>));
    EXPECT_TRUE(allTrianglesCCW(triangles, EPSILON<double>));
}

// Вход, заданный по часовой стрелке: после нормализации ensureCCW
// (так делает фасад перед запуском) алгоритм обязан выдать корректный результат.
TEST(MonotoneTriangulationTest, TriangulatesClockwiseInputAfterNormalization) {
    Polygon2<double> square = {{0.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}, {1.0, 0.0}}; // CW
    ensureCCW(square);
    MonotoneTriangulation<double> mt;

    auto result = mt.triangulate(square);

    ASSERT_TRUE(result) << result.error->debugMessage;
    const auto triangles = materializeTriangles(result);
    EXPECT_EQ(triangles.size(), 2u);
    EXPECT_NEAR(sumTrianglesArea(triangles), 1.0, EPSILON<double>);
    EXPECT_TRUE(allTrianglesCCW(triangles, EPSILON<double>));
}

// Пустой полигон и полигон из одной вершины — тот же отказ, что и для двух вершин.
TEST(MonotoneTriangulationTest, RejectsEmptyAndSingleVertexPolygons) {
    MonotoneTriangulation<double> mt;

    auto empty = mt.triangulate(Polygon2<double>{});
    EXPECT_FALSE(empty);
    EXPECT_EQ(empty.error->code, geo::ErrorCode::NotEnoughVertices);
    EXPECT_TRUE(empty.faces.empty());

    auto single = mt.triangulate(Polygon2<double>{{0.0, 0.0}});
    EXPECT_FALSE(single);
    EXPECT_EQ(single.error->code, geo::ErrorCode::NotEnoughVertices);
    EXPECT_TRUE(single.faces.empty());
}

// Полностью коллинеарный полигон не имеет площади и не может быть
// триангулирован — алгоритм должен отказать, а не вернуть пустой "успех".
TEST(MonotoneTriangulationTest, RejectsDegenerateCollinearPolygon) {
    Polygon2<double> collinear = {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}};
    MonotoneTriangulation<double> mt;

    auto result = mt.triangulate(collinear);

    EXPECT_FALSE(result);
    EXPECT_TRUE(result.faces.empty());
}

// Шаги sweep-фазы должны записывать y-координату линии заметания,
// чтобы GUI мог её отрисовать. Заметание идёт сверху вниз,
// поэтому значения sweep_y не возрастают.
TEST(MonotoneTriangulationTest, SweepStepsRecordSweepLineY) {
    auto square = makeSquare();
    MonotoneTriangulation<double> mt;

    auto result = mt.triangulate(square);
    ASSERT_TRUE(result) << result.error->debugMessage;

    std::vector<double> sweepYs;
    for (const auto& step : mt.getDebugHistory()) {
        if (step.sweep_y.has_value()) {
            sweepYs.push_back(*step.sweep_y);
        }
    }

    // На каждую из 4 вершин квадрата — один sweep-шаг.
    ASSERT_EQ(sweepYs.size(), square.size());
    EXPECT_DOUBLE_EQ(sweepYs.front(), 1.0);
    EXPECT_DOUBLE_EQ(sweepYs.back(), 0.0);
    for (size_t i = 1; i < sweepYs.size(); ++i) {
        EXPECT_LE(sweepYs[i], sweepYs[i - 1]);
    }
}

// ============================================================================
// Индексный вывод (faces + vertices) — Этап 1 Roadmap
// ============================================================================

namespace {

void expectMonotoneIndexedResultValid(const Polygon2<double>& input) {
    MonotoneTriangulation<double> mt;
    auto res = mt.triangulate(input);
    ASSERT_TRUE(res) << res.error->debugMessage;

    const size_t n = input.size();

    // vertices — это исходный полигон без изменений
    ASSERT_EQ(res.vertices.size(), n);
    for (size_t i = 0; i < n; ++i) {
        EXPECT_EQ(res.vertices[i], input[i]);
    }

    // количество граней = n - 2
    ASSERT_EQ(res.faces.size(), n - 2);

    for (size_t i = 0; i < res.faces.size(); ++i) {
        const auto& f = res.faces[i];
        for (auto idx : f) {
            EXPECT_LT(idx, res.vertices.size());
        }

        // ориентация грани — CCW
        Polygon2<double> triPoly = {
            res.vertices[f[0]], res.vertices[f[1]], res.vertices[f[2]]};
        EXPECT_GT(signedArea2D(triPoly), 0.0);
    }
}

} // namespace

TEST(MonotoneIndexedTest, SquareFaces) {
    expectMonotoneIndexedResultValid(makeSquare());
}

TEST(MonotoneIndexedTest, LShapeFaces) {
    expectMonotoneIndexedResultValid(makeLShape());
}

TEST(MonotoneIndexedTest, SplitMergeFaces) {
    expectMonotoneIndexedResultValid(makeSplitMergePolygon());
}

TEST(MonotoneIndexedTest, ErrorPathsLeaveFacesEmpty) {
    MonotoneTriangulation<double> mt;

    auto res = mt.triangulate(makeBowTie()); // самопересечение
    EXPECT_FALSE(res);
    EXPECT_TRUE(res.faces.empty());

    res = mt.triangulate(Polygon2<double>{{0.0, 0.0}, {1.0, 0.0}}); // < 3 вершин
    EXPECT_FALSE(res);
    EXPECT_TRUE(res.faces.empty());
}
