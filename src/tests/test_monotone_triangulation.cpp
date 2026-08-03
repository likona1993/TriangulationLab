#include <gtest/gtest.h>
#include "geometry/monotone_triangulation.h"
#include "geometry/types.h"

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
    EXPECT_TRUE(result.triangles.empty());
    EXPECT_TRUE(mt.getDebugHistory().empty());
}

// Самопересекающийся полигон — некорректный вход.
// Ожидаем явный отказ до запуска основной логики разбиения.
TEST(MonotoneTriangulationTest, RejectsSelfIntersectingPolygon) {
    MonotoneTriangulation<double> mt;

    auto result = mt.triangulate(makeBowTie());

    EXPECT_FALSE(result);
    EXPECT_EQ(result.error->code, geo::);
    EXPECT_TRUE(result.triangles.empty());
    EXPECT_TRUE(mt.getDebugHistory().empty());
}

// Простейший корректный случай: выпуклый квадрат.
// Должны получить ровно два треугольника и хотя бы несколько debug-шагов.
TEST(MonotoneTriangulationTest, TriangulatesSquareIntoTwoTriangles) {
    auto square = makeSquare();
    MonotoneTriangulation<double> mt;

    auto result = mt.triangulate(square);

    ASSERT_TRUE(result) << result.error->debugMessage;
    EXPECT_EQ(result.triangles.size(), 2u);

    EXPECT_NEAR(sumTrianglesArea(result.triangles), absArea(square), EPSILON<double>);
    EXPECT_TRUE(allTrianglesNonDegenerate(result.triangles, EPSILON<double>));
    EXPECT_TRUE(allTrianglesCCW(result.triangles, EPSILON<double>));

    const auto& history = mt.getDebugHistory();
    EXPECT_FALSE(history.empty());

    bool hasTriangleStep = false;
    for (const auto& step : history) {
        if (!step.added_triangles.empty()) {
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
    EXPECT_EQ(result.triangles.size(), poly.size() - 2);

    EXPECT_NEAR(sumTrianglesArea(result.triangles), absArea(poly), EPSILON<double>);
    EXPECT_TRUE(allTrianglesNonDegenerate(result.triangles, EPSILON<double>));
    EXPECT_TRUE(allTrianglesCCW(result.triangles, EPSILON<double>));

    EXPECT_FALSE(mt.getDebugHistory().empty());
}

// История отладки должна пересоздаваться на каждом запуске triangulate(),
// чтобы старые шаги не смешивались с новым результатом.
TEST(MonotoneTriangulationTest, ClearsDebugHistoryBeforeNextRun) {
    MonotoneTriangulation<double> mt;

    auto first = mt.triangulate(makeLShape());
    ASSERT_TRUE(first) << first.error;
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

    ASSERT_TRUE(result) << result.error_message;
    EXPECT_EQ(result.triangles.size(), poly.size() - 2);
    EXPECT_NEAR(sumTrianglesArea(result.triangles), absArea(poly), EPSILON<double>);
    EXPECT_TRUE(allTrianglesNonDegenerate(result.triangles, EPSILON<double>));
    EXPECT_TRUE(allTrianglesCCW(result.triangles, EPSILON<double>));

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
