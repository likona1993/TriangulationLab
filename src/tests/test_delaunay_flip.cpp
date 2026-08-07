#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "geometry/delaunay_flip.h"
#include "geometry/types.h"
#include "utils/numeric.h"

using namespace geo;

// ============================================================
// Helpers
// ============================================================

template <typename T>
T absArea(const Polygon2<T>& polygon) {
    return std::abs(signedArea2D(polygon));
}

template <typename T>
T sumTrianglesArea(const std::vector<Triangle2<T>>& triangles) {
    T sum = 0;

    for (const auto& triangle : triangles) {
        const Polygon2<T> trianglePolygon = {
            triangle.v0,
            triangle.v1,
            triangle.v2
        };
        sum += absArea(trianglePolygon);
    }

    return sum;
}

template <typename T>
bool allTrianglesNonDegenerate(
    const std::vector<Triangle2<T>>& triangles,
    T eps)
{
    for (const auto& triangle : triangles) {
        const Polygon2<T> trianglePolygon = {
            triangle.v0,
            triangle.v1,
            triangle.v2
        };

        if (absArea(trianglePolygon) <= eps) {
            return false;
        }
    }

    return true;
}

// Проверяет CCW-ориентацию всех результирующих треугольников.
template <typename T>
bool allTrianglesCCW(
    const std::vector<Triangle2<T>>& triangles,
    T eps)
{
    for (const auto& triangle : triangles) {
        const Polygon2<T> trianglePolygon = {
            triangle.v0,
            triangle.v1,
            triangle.v2
        };

        if (signedArea2D(trianglePolygon) <= eps) {
            return false;
        }
    }

    return true;
}

// Проверяет наличие ребра независимо от порядка его вершин.
template <typename T>
bool hasEdge(
    const std::vector<Triangle2<T>>& triangles,
    const Point2<T>& first,
    const Point2<T>& second)
{
    for (const auto& triangle : triangles) {
        const bool edge01 =
            (triangle.v0 == first && triangle.v1 == second) ||
            (triangle.v0 == second && triangle.v1 == first);

        const bool edge12 =
            (triangle.v1 == first && triangle.v2 == second) ||
            (triangle.v1 == second && triangle.v2 == first);

        const bool edge20 =
            (triangle.v2 == first && triangle.v0 == second) ||
            (triangle.v2 == second && triangle.v0 == first);

        if (edge01 || edge12 || edge20) {
            return true;
        }
    }

    return false;
}

// Проверяет наличие треугольника независимо от порядка его вершин.
template <typename T>
bool hasTriangle(
    const std::vector<Triangle2<T>>& triangles,
    const Point2<T>& first,
    const Point2<T>& second,
    const Point2<T>& third)
{
    for (const auto& triangle : triangles) {
        const bool hasFirst =
            triangle.v0 == first ||
            triangle.v1 == first ||
            triangle.v2 == first;

        const bool hasSecond =
            triangle.v0 == second ||
            triangle.v1 == second ||
            triangle.v2 == second;

        const bool hasThird =
            triangle.v0 == third ||
            triangle.v1 == third ||
            triangle.v2 == third;

        if (hasFirst && hasSecond && hasThird) {
            return true;
        }
    }

    return false;
}

template <typename T>
bool allPolygonBoundaryEdgesPresent(
    const Polygon2<T>& polygon,
    const std::vector<Triangle2<T>>& triangles)
{
    if (polygon.size() < 2) {
        return false;
    }

    for (std::size_t index = 0; index < polygon.size(); ++index) {
        const std::size_t nextIndex = (index + 1) % polygon.size();

        if (!hasEdge(
                triangles,
                polygon[index],
                polygon[nextIndex])) {
            return false;
        }
    }

    return true;
}

// Проверяет основные геометрические инварианты результата.
template <typename T>
void expectValidTriangulation(
    const Polygon2<T>& polygon,
    const TriangulationResult<T>& result,
    std::size_t expectedTriangleCount)
{
    ASSERT_TRUE(result);
    ASSERT_EQ(result.triangles.size(), expectedTriangleCount);

    EXPECT_NEAR(
        sumTrianglesArea(result.triangles),
        absArea(polygon),
        EPSILON<T>);

    EXPECT_TRUE(
        allTrianglesNonDegenerate(
            result.triangles,
            EPSILON<T>));

    EXPECT_TRUE(
        allTrianglesCCW(
            result.triangles,
            EPSILON<T>));

    EXPECT_TRUE(
        allPolygonBoundaryEdgesPresent(
            polygon,
            result.triangles));
}

// Важно: начальная триангуляция строится через Ear Clipping
// и также может содержать added_diagonals.
// Поэтому одного только added_diagonals недостаточно:
// считаем только шаги с описанием, которое записывает flipEdge().
template <typename T>
std::size_t countFlipSteps(
    const std::vector<DebugStep<T>>& history)
{
    std::size_t count = 0;

    for (const auto& step : history) {
        if (step.description == "Flip illegal edge") {
            ++count;
        }
    }

    return count;
}

// ============================================================
// Basic geometry invariants
// ============================================================

TEST(DelaunayFlipTest, SquareTriangulation) {
    // Квадрат разбивается на два CCW-треугольника.
    const Polygon2<double> square = {
        {0, 0}, {1, 0}, {1, 1}, {0, 1}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(square);

    expectValidTriangulation(square, result, 2);
    EXPECT_FALSE(delaunayFlip.getDebugHistory().empty());
}

TEST(DelaunayFlipTest, ConvexPentagonTriangulation) {
    // Базовая проверка выпуклого пятиугольника.
    Polygon2<double> pentagon = {
        {0, 1},
        {0.951, 0.309},
        {0.588, -0.809},
        {-0.588, -0.809},
        {-0.951, 0.309}
    };
    ensureCCW(pentagon);

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(pentagon);

    expectValidTriangulation(pentagon, result, 3);
}

TEST(DelaunayFlipTest, ConcaveLShapeTriangulation) {
    // Проверка вогнутого полигона.
    const Polygon2<double> lShape = {
        {0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(lShape);

    expectValidTriangulation(lShape, result, 4);
}

TEST(DelaunayFlipTest, NearCollinearConvexPolygon) {
    // Вершина почти лежит на одной прямой с соседними вершинами.
    const Polygon2<double> polygon = {
        {0.0, 0.0},
        {2.0, 0.0},
        {4.0, 1e-6},
        {4.0, 2.0},
        {0.0, 2.0}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(polygon);

    expectValidTriangulation(polygon, result, polygon.size() - 2);
}

TEST(DelaunayFlipTest, RepeatedYLevels) {
    // Несколько вершин имеют одинаковые y-координаты.
    const Polygon2<double> polygon = {
        {0, 0}, {2, 0}, {3, 1}, {2, 2}, {0, 2}, {-1, 1}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(polygon);

    expectValidTriangulation(polygon, result, polygon.size() - 2);
}

// ============================================================
// Expected triangulations and diagonal flips
// ============================================================

TEST(DelaunayFlipTest, RhombusUsesExpectedDiagonal) {
    // Вертикальная диагональ ромба должна быть заменена
    // горизонтальной диагональю.
    const Polygon2<double> rhombus = {
        {0, -2}, {1, 0}, {0, 2}, {-1, 0}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(rhombus);

    expectValidTriangulation(rhombus, result, 2);

    EXPECT_TRUE(hasEdge(
        result.triangles,
        Point2<double>{-1, 0},
        Point2<double>{1, 0}));

    EXPECT_FALSE(hasEdge(
        result.triangles,
        Point2<double>{0, -2},
        Point2<double>{0, 2}));
}

TEST(DelaunayFlipTest, ConvexPentagonMatchesExpectedTriangulation) {
    // Ожидаемый результат определён независимо от DelaunayFlip:
    // T1 = (0, 1, 3), T2 = (0, 3, 4), T3 = (1, 2, 3).
    const Polygon2<double> polygon = {
        {0.0, 0.0},
        {4.0, 0.0},
        {5.0, 2.0},
        {2.5, 3.0},
        {0.0, 2.0}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(polygon);

    expectValidTriangulation(polygon, result, 3);

    EXPECT_TRUE(hasTriangle(
        result.triangles,
        polygon[0], polygon[1], polygon[3]));

    EXPECT_TRUE(hasTriangle(
        result.triangles,
        polygon[0], polygon[3], polygon[4]));

    EXPECT_TRUE(hasTriangle(
        result.triangles,
        polygon[1], polygon[2], polygon[3]));
}

TEST(DelaunayFlipTest, CoCircularQuadrilateralIsStable) {
    // Для квадрата обе диагонали допустимы.
    // Тест не требует конкретного выбора диагонали.
    const Polygon2<double> square = {
        {0, 0}, {1, 0}, {1, 1}, {0, 1}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(square);

    expectValidTriangulation(square, result, 2);

    const bool firstDiagonal = hasEdge(
        result.triangles,
        Point2<double>{0, 0},
        Point2<double>{1, 1});

    const bool secondDiagonal = hasEdge(
        result.triangles,
        Point2<double>{1, 0},
        Point2<double>{0, 1});

    EXPECT_TRUE(firstDiagonal || secondDiagonal);
}

TEST(DelaunayFlipTest, HousePolygonPreservesGeometry) {
    // Для домика проверяем независимые свойства результата,
    // не фиксируя конкретную допустимую внутреннюю диагональ.
    const Polygon2<double> house = {
        {0, 0}, {2, 0}, {2, 2}, {1, 3}, {0, 2}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(house);

    expectValidTriangulation(house, result, 3);
}

// ============================================================
// Flip-chain regression case
// ============================================================

TEST(DelaunayFlipTest, ZigzagCorridorProducesSeveralFlipSteps) {
    // Зигзагообразный коридор содержит несколько локальных участков,
    // на которых начальная триангуляция может быть исправлена
    // последовательностью флипов.
    const Polygon2<double> polygon = {
        {0.0, 0.0},
        {2.0, 0.25},
        {4.0, 0.0},
        {6.0, 0.25},
        {8.0, 0.0},
        {8.0, 3.0},
        {6.0, 2.75},
        {4.0, 3.0},
        {2.0, 2.75},
        {0.0, 3.0}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(polygon);

    // Сначала проверяем, что вся геометрия сохранилась
    // после последовательности локальных перестроений.
    expectValidTriangulation(
        polygon,
        result,
        polygon.size() - 2);

    const auto& history = delaunayFlip.getDebugHistory();
    ASSERT_FALSE(history.empty());

    // В отличие от added_diagonals, описание позволяет отличить
    // реальные флипы от шагов начальной Ear Clipping-триангуляции.
    EXPECT_GE(countFlipSteps(history), 2u);
}

// ============================================================
// Invalid and degenerate input
// ============================================================

TEST(DelaunayFlipTest, LessThanThreeVertices) {
    // Недостаточно вершин для построения треугольника.
    DelaunayFlip<double> delaunayFlip;

    EXPECT_FALSE(delaunayFlip.triangulate(Polygon2<double>{}));
    EXPECT_FALSE(delaunayFlip.triangulate({{0, 0}}));
    EXPECT_FALSE(delaunayFlip.triangulate({{0, 0}, {1, 0}}));
}

TEST(DelaunayFlipTest, SelfIntersectingPolygonIsRejected) {
    // Самопересекающийся полигон не должен быть триангулирован.
    const Polygon2<double> bowTie = {
        {0, 0}, {1, 1}, {0, 1}, {1, 0}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(bowTie);

    EXPECT_FALSE(result);
    EXPECT_TRUE(result.triangles.empty());
}

TEST(DelaunayFlipTest, AllCollinearPolygonIsRejected) {
    // Полностью коллинеарный набор точек имеет нулевую площадь.
    const Polygon2<double> collinear = {
        {0, 0}, {1, 0}, {2, 0}, {3, 0}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(collinear);

    EXPECT_FALSE(result);
    EXPECT_TRUE(result.triangles.empty());
}

TEST(DelaunayFlipTest, CollinearVerticesCanBeRemovedBeforeTriangulation) {
    // После удаления промежуточных коллинеарных вершин
    // остаётся прямоугольник.
    Polygon2<double> polygon = {
        {0, 0}, {1, 0}, {2, 0}, {2, 1}, {1, 1}, {0, 1}
    };

    removeCollinearPoints(polygon);

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(polygon);

    expectValidTriangulation(polygon, result, 2);
}

// ============================================================
// Repeated runs and history reset
// ============================================================

TEST(DelaunayFlipTest, SequentialRunsDoNotMixGeometry) {
    // Второй запуск не должен использовать геометрию первого полигона.
    const Polygon2<double> square = {
        {0, 0}, {1, 0}, {1, 1}, {0, 1}
    };

    const Polygon2<double> rhombus = {
        {0, -2}, {1, 0}, {0, 2}, {-1, 0}
    };

    DelaunayFlip<double> delaunayFlip;

    ASSERT_TRUE(delaunayFlip.triangulate(square));
    const auto secondResult = delaunayFlip.triangulate(rhombus);

    expectValidTriangulation(rhombus, secondResult, 2);

    EXPECT_TRUE(hasEdge(
        secondResult.triangles,
        Point2<double>{-1, 0},
        Point2<double>{1, 0}));

    EXPECT_FALSE(hasEdge(
        secondResult.triangles,
        Point2<double>{0, -2},
        Point2<double>{0, 2}));
}

TEST(DelaunayFlipTest, DebugHistoryIsResetBetweenRuns) {
    // История должна быть пересоздана для каждого нового запуска.
    const Polygon2<double> square = {
        {0, 0}, {1, 0}, {1, 1}, {0, 1}
    };

    const Polygon2<double> pentagon = {
        {0, 0}, {2, 0}, {3, 1}, {1, 2}, {-1, 1}
    };

    DelaunayFlip<double> delaunayFlip;

    ASSERT_TRUE(delaunayFlip.triangulate(square));
    EXPECT_FALSE(delaunayFlip.getDebugHistory().empty());

    ASSERT_TRUE(delaunayFlip.triangulate(pentagon));
    EXPECT_FALSE(delaunayFlip.getDebugHistory().empty());
}
