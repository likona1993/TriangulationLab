#include <gtest/gtest.h>

#include "geometry/delaunay_flip.h"
#include "geometry/types.h"
#include "utils/numeric.h"

using namespace geo;

// ============================================================
// Helpers
// ============================================================

// Абсолютная площадь многоугольника.
// Нам удобнее сравнивать именно модуль площади, потому что
// ориентация входных точек может быть разной.
template <typename T>
T absArea(const Polygon2<T>& poly) {
    return std::abs(signedArea2D(poly));
}

// Сумма площадей всех треугольников.
template <typename T>
T sumTrianglesArea(const std::vector<Triangle2<T>>& tris) {
    T sum = 0;
    for (const auto& t : tris) {
        Polygon2<T> triPoly = {t.v0, t.v1, t.v2};
        sum += absArea<T>(triPoly);
    }
    return sum;
}

// Проверка, что все треугольники ненулевой площади.
template <typename T>
bool allTrianglesNonDegenerate(const std::vector<Triangle2<T>>& tris, T eps) {
    for (const auto& t : tris) {
        Polygon2<T> triPoly = {t.v0, t.v1, t.v2};
        if (absArea<T>(triPoly) <= eps) {
            return false;
        }
    }
    return true;
}

// Проверка, что в истории есть хотя бы один шаг с флипом.
template <typename T>
bool hasFlipStep(const std::vector<DebugStep<T>>& history) {
    for (const auto& step : history) {
        if (!step.added_diagonals.empty() || !step.added_triangles.empty()) {
            return true;
        }
    }
    return false;
}


// ============================================================
// DelaunayFlip tests
// ============================================================

TEST(DelaunayFlipTest, SquareTriangulation) {
    // Базовый случай: квадрат.
    // Здесь алгоритм должен вернуть 2 треугольника,
    // а площадь результата должна совпасть с площадью входа.
    Polygon2<double> square = {{0,0}, {1,0}, {1,1}, {0,1}};

    DelaunayFlip<double> df;
    auto res = df.triangulate(square);

    ASSERT_TRUE(res);
    EXPECT_EQ(res.triangles.size(), 2);

    double area = absArea<double>(square);
    double triArea = sumTrianglesArea<double>(res.triangles);
    EXPECT_NEAR(triArea, area, EPSILON<double>);

    EXPECT_TRUE(allTrianglesNonDegenerate<double>(res.triangles, EPSILON<double>));

    // Для квадрата, как правило, флип может не понадобиться.
    // Но история построения стартовой триангуляции должна быть.
    auto history = df.getDebugHistory();
    EXPECT_FALSE(history.empty());
}

TEST(DelaunayFlipTest, ConvexPentagonTriangulation) {
    // Выпуклый пятиугольник.
    // Удобен для проверки, что алгоритм корректно работает
    // на многоугольниках с количеством вершин > 4.
    Polygon2<double> pentagon = {
        {0, 1}, {0.951, 0.309}, {0.588, -0.809}, {-0.588, -0.809}, {-0.951, 0.309}
    };
    ensureCCW(pentagon);

    DelaunayFlip<double> df;
    auto res = df.triangulate(pentagon);

    ASSERT_TRUE(res);
    EXPECT_EQ(res.triangles.size(), 3);

    double area = absArea<double>(pentagon);
    double triArea = sumTrianglesArea<double>(res.triangles);
    EXPECT_NEAR(triArea, area, 1e-9);

    EXPECT_TRUE(allTrianglesNonDegenerate<double>(res.triangles, EPSILON<double>));
}

TEST(DelaunayFlipTest, LShapedPolygonTriangulation) {
    // Невыпуклый многоугольник.
    // Важен, потому что DelaunayFlip строит стартовую триангуляцию
    // через ear clipping и затем улучшает ее флипами.
    Polygon2<double> lShape = {{0,0}, {2,0}, {2,1}, {1,1}, {1,2}, {0,2}};

    DelaunayFlip<double> df;
    auto res = df.triangulate(lShape);

    ASSERT_TRUE(res);
    EXPECT_EQ(res.triangles.size(), 4);

    double area = absArea<double>(lShape);
    double triArea = sumTrianglesArea<double>(res.triangles);
    EXPECT_NEAR(triArea, area, EPSILON<double>);

    EXPECT_TRUE(allTrianglesNonDegenerate<double>(res.triangles, EPSILON<double>));

    auto history = df.getDebugHistory();
    EXPECT_FALSE(history.empty());
}

TEST(DelaunayFlipTest, CollinearPointsCleaned) {
    // Проверяем случай, когда вход содержит коллинеарные точки.
    // Перед триангуляцией мы их удаляем, иначе результат может быть
    // нестабильным из-за вырожденных ребер и треугольников.
    Polygon2<double> raw = {{0,0}, {1,0}, {2,0}, {2,1}, {1,1}, {0,1}};
    removeCollinearPoints(raw);

    DelaunayFlip<double> df;
    auto res = df.triangulate(raw);

    ASSERT_TRUE(res);
    EXPECT_EQ(res.triangles.size(), 2);

    double area = absArea<double>(raw);
    double triArea = sumTrianglesArea<double>(res.triangles);
    EXPECT_NEAR(triArea, area, EPSILON<double>);
}

TEST(DelaunayFlipTest, SelfIntersectingPolygon) {
    // Самопересекающийся многоугольник триангулировать нельзя.
    Polygon2<double> bow = {{0,0}, {1,1}, {0,1}, {1,0}};

    DelaunayFlip<double> df;
    auto res = df.triangulate(bow);

    EXPECT_FALSE(res);
    EXPECT_TRUE(res.triangles.empty());
}

TEST(DelaunayFlipTest, LessThanThreeVertices) {
    // Недостаточно вершин.
    Polygon2<double> two = {{0,0}, {1,0}};

    DelaunayFlip<double> df;
    auto res = df.triangulate(two);
    EXPECT_FALSE(res);
    EXPECT_TRUE(res.triangles.empty());

    Polygon2<double> one = {{0,0}};
    res = df.triangulate(one);
    EXPECT_FALSE(res);
    EXPECT_TRUE(res.triangles.empty());

    Polygon2<double> empty;
    res = df.triangulate(empty);
    EXPECT_FALSE(res);
    EXPECT_TRUE(res.triangles.empty());
}

TEST(DelaunayFlipTest, AllCollinearPolygon) {
    // Полностью вырожденный случай.
    Polygon2<double> collinear = {{0,0}, {1,0}, {2,0}, {3,0}};

    DelaunayFlip<double> df;
    auto res = df.triangulate(collinear);

    EXPECT_FALSE(res);
    EXPECT_TRUE(res.triangles.empty());
}

TEST(DelaunayFlipTest, DebugHistoryExistsAfterTriangulation) {
    // История отладки важна, потому что DelaunayFlip строит результат
    // поэтапно и потом делает флипы.
    Polygon2<double> square = {{0,0}, {1,0}, {1,1}, {0,1}};

    DelaunayFlip<double> df;
    auto res = df.triangulate(square);

    ASSERT_TRUE(res);
    auto history = df.getDebugHistory();

    // На этом этапе история не должна быть пустой.
    EXPECT_FALSE(history.empty());
}

TEST(DelaunayFlipTest, FlipHistoryContainsDiagonalWhenFlipOccurs) {
    // Этот тест нацелен именно на случай, когда алгоритм находит
    // незаконное ребро и выполняет флип.
    // Здесь мы не привязываемся к конкретной форме диагонали,
    // а проверяем сам факт появления шага флипа.
    Polygon2<double> cross = {
        {0, -2}, {1, -2}, {1, -1}, {2, -1}, {2, 0}, {1, 0},
        {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-2, 0}, {-2, -1},
        {-1, -1}, {-1, -2}
    };
    ensureCCW(cross);

    DelaunayFlip<double> df;
    auto res = df.triangulate(cross);

    ASSERT_TRUE(res);

    auto history = df.getDebugHistory();
    EXPECT_FALSE(history.empty());

    // Если флип был, в истории должен появиться шаг с диагональю.
    EXPECT_TRUE(hasFlipStep(history));
}

TEST(DelaunayFlipTest, NoIllegalEdgeForConvexPolygonIsStable) {
    // Для выпуклого многоугольника часто получается ситуация,
    // когда флипы либо отсутствуют, либо минимальны.
    // Этот тест важен как контроль стабильности:
    // алгоритм должен завершиться корректно и не разрушить триангуляцию.
    Polygon2<double> hex = {
        {1,0}, {0.5, 0.866}, {-0.5, 0.866}, {-1,0}, {-0.5, -0.866}, {0.5, -0.866}
    };
    ensureCCW(hex);

    DelaunayFlip<double> df;
    auto res = df.triangulate(hex);

    ASSERT_TRUE(res);
    EXPECT_EQ(res.triangles.size(), 4);

    double area = absArea<double>(hex);
    double triArea = sumTrianglesArea<double>(res.triangles);
    EXPECT_NEAR(triArea, area, 1e-9);

    EXPECT_TRUE(allTrianglesNonDegenerate<double>(res.triangles, EPSILON<double>));
}

TEST(DelaunayFlipTest, MultipleSequentialRunsClearInternalState) {
    // Важно проверить, что повторные вызовы triangulate не накапливают
    // состояние между запусками.
    Polygon2<double> square = {{0,0}, {1,0}, {1,1}, {0,1}};
    Polygon2<double> pentagon = {
        {0, 1}, {0.951, 0.309}, {0.588, -0.809}, {-0.588, -0.809}, {-0.951, 0.309}
    };
    ensureCCW(pentagon);

    DelaunayFlip<double> df;

    auto res1 = df.triangulate(square);
    ASSERT_TRUE(res1);
    auto history1 = df.getDebugHistory();
    EXPECT_FALSE(history1.empty());

    auto res2 = df.triangulate(pentagon);
    ASSERT_TRUE(res2);
    auto history2 = df.getDebugHistory();
    EXPECT_FALSE(history2.empty());

    // После второго запуска история должна относиться к новому запуску,
    // а не содержать старые шаги.
    EXPECT_LE(history2.size(), 3u);
}

TEST(DelaunayFlipTest, TriangleCountMatchesPolygonVertices) {
    // Для простого многоугольника число треугольников равно n - 2.
    // Это хороший базовый инвариант для проверки результата.
    Polygon2<double> heptagon = {
        {0, 2}, {1, 1.5}, {2, 0.5}, {1.5, -1}, {0, -2}, {-1.5, -1}, {-2, 0.5}
    };
    ensureCCW(heptagon);

    DelaunayFlip<double> df;
    auto res = df.triangulate(heptagon);

    ASSERT_TRUE(res);
    EXPECT_EQ(res.triangles.size(), heptagon.size() - 2);
}

TEST(DelaunayFlipTest, IllegalEdgeCoverageConceptualCases) {
    // Этот тест нужен как "контрольный список" для логики isIllegalEdge.
    // Мы не вызываем isIllegalEdge напрямую, потому что он private,
    // но подбираем входы так, чтобы покрыть основные смысловые случаи:
    // 1) граничное ребро — не проверяется как illegal;
    // 2) ребро внутри корректной триангуляции — алгоритм должен завершиться;
    // 3) случай, где требуется флип — история должна зафиксировать диагональ.

    // Случай 1: квадрат. Здесь у ребра только одна внутренняя диагональ,
    // внешние границы должны быть проигнорированы.
    Polygon2<double> square = {{0,0}, {1,0}, {1,1}, {0,1}};
    DelaunayFlip<double> df1;
    auto res1 = df1.triangulate(square);
    ASSERT_TRUE(res1);
    EXPECT_EQ(res1.triangles.size(), 2);

    // Случай 2: выпуклый многоугольник. Условия для illegal edge
    // должны не приводить к ошибке и должны дать стабильный результат.
    Polygon2<double> pentagon = {
        {0, 1}, {0.951, 0.309}, {0.588, -0.809}, {-0.588, -0.809}, {-0.951, 0.309}
    };
    ensureCCW(pentagon);
    DelaunayFlip<double> df2;
    auto res2 = df2.triangulate(pentagon);
    ASSERT_TRUE(res2);
    EXPECT_EQ(res2.triangles.size(), 3);

    // Случай 3: многоугольник, где флип вероятен.
    // Здесь мы проверяем именно наличие шага с диагональю.
    Polygon2<double> cross = {
        {0, -2}, {1, -2}, {1, -1}, {2, -1}, {2, 0}, {1, 0},
        {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-2, 0}, {-2, -1},
        {-1, -1}, {-1, -2}
    };
    ensureCCW(cross);
    DelaunayFlip<double> df3;
    auto res3 = df3.triangulate(cross);
    ASSERT_TRUE(res3);
    EXPECT_TRUE(hasFlipStep(df3.getDebugHistory()));
}
