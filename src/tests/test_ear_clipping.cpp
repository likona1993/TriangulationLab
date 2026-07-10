#include <gtest/gtest.h>
#include "geometry/ear_clipping.h"
#include "geometry/types.h"
#include "utils/numeric.h"

using namespace geo;

// Helper: absolute area
template<typename T>
T absArea(const Polygon2<T>& poly) {
    return std::abs(signedArea2D(poly));
}

// Helper: sum of triangle areas
template<typename T>
T sumTrianglesArea(const std::vector<Triangle2<T>>& tris) {
    T sum = 0;
    for (const auto& t : tris) {
        Polygon2<T> triPoly = {t.v0, t.v1, t.v2};
        sum += absArea(triPoly);
    }
    return sum;
}

// Helper: check non-degenerate
template<typename T>
bool allTrianglesNonDegenerate(const std::vector<Triangle2<T>>& tris, T eps) {
    for (const auto& t : tris) {
        Polygon2<T> triPoly = {t.v0, t.v1, t.v2};
        if (absArea(triPoly) <= eps) return false;
    }
    return true;
}

// ============================================================================
// EarClipping tests
// ============================================================================

TEST(EarClippingTest, Square) {
    Polygon2<double> square = {{0,0}, {1,0}, {1,1}, {0,1}};
    EarClipping<double> ec;
    auto res = ec.triangulate(square);
    ASSERT_TRUE(res.success);
    EXPECT_EQ(res.triangles.size(), 2);

    double area = absArea(square);
    double triArea = sumTrianglesArea(res.triangles);
    EXPECT_NEAR(triArea, area, EPSILON<double>);

    EXPECT_TRUE(allTrianglesNonDegenerate(res.triangles, EPSILON<double>));

    auto history = ec.getDebugHistory();
    EXPECT_EQ(history.size(), 2);
    ASSERT_FALSE(history.empty());
    auto lastTri = history.back().added_triangles[0];
    Polygon2<double> lastPoly = {lastTri.v0, lastTri.v1, lastTri.v2};
    EXPECT_GT(absArea(lastPoly), EPSILON<double>);
}

TEST(EarClippingTest, ConvexPentagon) {
    Polygon2<double> pentagon = {
        {0, 1}, {0.951, 0.309}, {0.588, -0.809}, {-0.588, -0.809}, {-0.951, 0.309}
    };
    ensureCCW(pentagon);

    EarClipping<double> ec;
    auto res = ec.triangulate(pentagon);
    ASSERT_TRUE(res.success);
    EXPECT_EQ(res.triangles.size(), 3);

    double area = absArea(pentagon);
    double triArea = sumTrianglesArea(res.triangles);
    EXPECT_NEAR(triArea, area, 1e-9);

    EXPECT_TRUE(allTrianglesNonDegenerate(res.triangles, EPSILON<double>));
}

TEST(EarClippingTest, LShapedPolygon) {
    Polygon2<double> lShape = {{0,0}, {2,0}, {2,1}, {1,1}, {1,2}, {0,2}};
    EarClipping<double> ec;
    auto res = ec.triangulate(lShape);
    ASSERT_TRUE(res.success);
    EXPECT_EQ(res.triangles.size(), 4);

    double area = absArea(lShape);
    double triArea = sumTrianglesArea(res.triangles);
    EXPECT_NEAR(triArea, area, EPSILON<double>);

    EXPECT_TRUE(allTrianglesNonDegenerate(res.triangles, EPSILON<double>));

    auto history = ec.getDebugHistory();
    EXPECT_EQ(history.size(), 4);
}

TEST(EarClippingTest, CrossShapedPolygon) {
    Polygon2<double> cross = {
        {0, -2}, {1, -2}, {1, -1}, {2, -1}, {2, 0}, {1, 0}, {1, 1}, {0, 1},
        {-1, 1}, {-1, 0}, {-2, 0}, {-2, -1}, {-1, -1}, {-1, -2}
    };
    ensureCCW(cross);
    EarClipping<double> ec;
    auto res = ec.triangulate(cross);
    ASSERT_TRUE(res.success);
    EXPECT_EQ(res.triangles.size(), 12);

    double area = absArea(cross);
    double triArea = sumTrianglesArea(res.triangles);
    EXPECT_NEAR(triArea, area, 1e-9);

    EXPECT_TRUE(allTrianglesNonDegenerate(res.triangles, EPSILON<double>));
}

TEST(EarClippingTest, PolygonWithCollinearPointsCleaned) {
    Polygon2<double> raw = {{0,0}, {1,0}, {2,0}, {2,1}, {1,1}, {0,1}};
    removeCollinearPoints(raw);
    // Now raw is a valid square
    EarClipping<double> ec;
    auto res = ec.triangulate(raw);
    ASSERT_TRUE(res.success);
    EXPECT_EQ(res.triangles.size(), 2);

    double area = absArea(raw);
    double triArea = sumTrianglesArea(res.triangles);
    EXPECT_NEAR(triArea, area, EPSILON<double>);
}

TEST(EarClippingTest, SelfIntersectingPolygon) {
    Polygon2<double> bow = {{0,0}, {1,1}, {0,1}, {1,0}};
    EarClipping<double> ec;
    auto res = ec.triangulate(bow);
    EXPECT_FALSE(res.success);
    EXPECT_TRUE(res.triangles.empty());
}

TEST(EarClippingTest, PolygonWithLessThan3Vertices) {
    Polygon2<double> two = {{0,0}, {1,0}};
    EarClipping<double> ec;
    auto res = ec.triangulate(two);
    EXPECT_FALSE(res.success);
    EXPECT_TRUE(res.triangles.empty());

    Polygon2<double> one = {{0,0}};
    res = ec.triangulate(one);
    EXPECT_FALSE(res.success);
    EXPECT_TRUE(res.triangles.empty());

    Polygon2<double> empty;
    res = ec.triangulate(empty);
    EXPECT_FALSE(res.success);
    EXPECT_TRUE(res.triangles.empty());
}

TEST(EarClippingTest, DegeneratePolygonAllCollinear) {
    Polygon2<double> collinear = {{0,0}, {1,0}, {2,0}, {3,0}};
    EarClipping<double> ec;
    auto res = ec.triangulate(collinear);
    EXPECT_FALSE(res.success);
    EXPECT_TRUE(res.triangles.empty());
}

TEST(EarClippingTest, DebugHistoryCount) {
    Polygon2<double> square = {{0,0}, {1,0}, {1,1}, {0,1}};
    EarClipping<double> ec;
    auto res = ec.triangulate(square);
    ASSERT_TRUE(res.success);
    auto history = ec.getDebugHistory();
    EXPECT_EQ(history.size(), 2);

    Polygon2<double> pentagon = {
        {0, 1}, {0.951, 0.309}, {0.588, -0.809}, {-0.588, -0.809}, {-0.951, 0.309}
    };
    ensureCCW(pentagon);
    EarClipping<double> ec2;
    res = ec2.triangulate(pentagon);
    ASSERT_TRUE(res.success);
    history = ec2.getDebugHistory();
    EXPECT_EQ(history.size(), 3);

    Polygon2<double> lShape = {{0,0}, {2,0}, {2,1}, {1,1}, {1,2}, {0,2}};
    EarClipping<double> ec3;
    res = ec3.triangulate(lShape);
    ASSERT_TRUE(res.success);
    history = ec3.getDebugHistory();
    EXPECT_EQ(history.size(), 4);
}

TEST(EarClippingTest, FinalTriangleInDebugHistory) {
    Polygon2<double> square = {{0,0}, {1,0}, {1,1}, {0,1}};
    EarClipping<double> ec;
    auto res = ec.triangulate(square);
    ASSERT_TRUE(res.success);
    auto history = ec.getDebugHistory();
    ASSERT_GE(history.size(), 1);
    auto last = history.back();
    bool found = false;
    for (const auto& t : res.triangles) {
        auto cut_triangle = last.added_triangles[0];
        if ( (t.v0 == cut_triangle.v0 && t.v1 == cut_triangle.v1 && t.v2 == cut_triangle.v2) ||
            (t.v0 == cut_triangle.v0 && t.v1 == cut_triangle.v2 && t.v2 == cut_triangle.v1) ) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST(EarClippingTest, ConvexPolygonWithManyVertices) {
    Polygon2<double> hex = {
        {1,0}, {0.5, 0.866}, {-0.5, 0.866}, {-1,0}, {-0.5, -0.866}, {0.5, -0.866}
    };
    ensureCCW(hex);
    EarClipping<double> ec;
    auto res = ec.triangulate(hex);
    ASSERT_TRUE(res.success);
    EXPECT_EQ(res.triangles.size(), 4);

    double area = absArea(hex);
    double triArea = sumTrianglesArea(res.triangles);
    EXPECT_NEAR(triArea, area, 1e-9);
    EXPECT_TRUE(allTrianglesNonDegenerate(res.triangles, EPSILON<double>));
}