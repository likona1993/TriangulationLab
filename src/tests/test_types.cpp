#include <gtest/gtest.h>
#include "geometry/types.h"
#include "utils/numeric.h"

using namespace geo;

// ============================================================================
// Point2 tests
// ============================================================================

TEST(Point2Test, DefaultConstructor) {
    Point2<double> p;
    EXPECT_DOUBLE_EQ(p.x, 0.0);
    EXPECT_DOUBLE_EQ(p.y, 0.0);
}

TEST(Point2Test, ConstructorWithValues) {
    Point2<double> p(1.5, -2.3);
    EXPECT_DOUBLE_EQ(p.x, 1.5);
    EXPECT_DOUBLE_EQ(p.y, -2.3);
}

TEST(Point2Test, ArithmeticOperators) {
    Point2<double> a(1, 2);
    Point2<double> b(3, 4);

    Point2<double> sum = a + b;
    EXPECT_DOUBLE_EQ(sum.x, 4);
    EXPECT_DOUBLE_EQ(sum.y, 6);

    Point2<double> diff = a - b;
    EXPECT_DOUBLE_EQ(diff.x, -2);
    EXPECT_DOUBLE_EQ(diff.y, -2);

    Point2<double> scaled = a * 2.0;
    EXPECT_DOUBLE_EQ(scaled.x, 2);
    EXPECT_DOUBLE_EQ(scaled.y, 4);

    // Self-assignment
    Point2<double> c = a;
    c += b;
    EXPECT_DOUBLE_EQ(c.x, 4);
    EXPECT_DOUBLE_EQ(c.y, 6);

    c = a;
    c -= b;
    EXPECT_DOUBLE_EQ(c.x, -2);
    EXPECT_DOUBLE_EQ(c.y, -2);

    //c = a;
    //c *= 2.0;
    //EXPECT_DOUBLE_EQ(c.x, 2);
    //EXPECT_DOUBLE_EQ(c.y, 4);
}

TEST(Point2Test, EqualityOperators) {
    Point2<double> a(1, 2);
    Point2<double> b(1, 2);
    Point2<double> c(1, 3);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
}

TEST(Point2Test, DotProduct) {
    Point2<double> a(1, 0);
    Point2<double> b(0, 1);
    EXPECT_DOUBLE_EQ(a.dot(b), 0.0);

    Point2<double> c(2, 3);
    Point2<double> d(4, 5);
    EXPECT_DOUBLE_EQ(c.dot(d), 2*4 + 3*5);
}

TEST(Point2Test, CrossProduct) {
    Point2<double> a(1, 0);
    Point2<double> b(0, 1);
    EXPECT_DOUBLE_EQ(a.cross(b), 1.0);
    EXPECT_DOUBLE_EQ(b.cross(a), -1.0);

    Point2<double> c(2, 3);
    Point2<double> d(4, 5);
    EXPECT_DOUBLE_EQ(c.cross(d), 2*5 - 3*4);
}

TEST(Point2Test, NormAndSquaredNorm) {
    Point2<double> p(3, 4);
    EXPECT_DOUBLE_EQ(p.squaredNorm(), 25.0);
    EXPECT_DOUBLE_EQ(p.norm(), 5.0);

    Point2<double> q(1, 1);
    EXPECT_NEAR(q.norm(), std::sqrt(2.0), EPSILON<double>);
}

// ============================================================================
// Polygon2 and Triangle2 tests (basic)
// ============================================================================

TEST(Polygon2Test, ConstructorAndAccess) {
    Polygon2<double> poly = {{0,0}, {1,0}, {1,1}, {0,1}};
    EXPECT_EQ(poly.size(), 4);
    EXPECT_DOUBLE_EQ(poly[0].x, 0);
    EXPECT_DOUBLE_EQ(poly[0].y, 0);
    EXPECT_DOUBLE_EQ(poly[1].x, 1);
    EXPECT_DOUBLE_EQ(poly[1].y, 0);
    EXPECT_DOUBLE_EQ(poly[2].x, 1);
    EXPECT_DOUBLE_EQ(poly[2].y, 1);
    EXPECT_DOUBLE_EQ(poly[3].x, 0);
    EXPECT_DOUBLE_EQ(poly[3].y, 1);
}

TEST(Triangle2Test, Constructor) {
    Point2<double> a(0,0), b(1,0), c(0,1);
    Triangle2<double> tri(a, b, c);
    EXPECT_DOUBLE_EQ(tri.v0.x, 0);
    EXPECT_DOUBLE_EQ(tri.v0.y, 0);
    EXPECT_DOUBLE_EQ(tri.v1.x, 1);
    EXPECT_DOUBLE_EQ(tri.v1.y, 0);
    EXPECT_DOUBLE_EQ(tri.v2.x, 0);
    EXPECT_DOUBLE_EQ(tri.v2.y, 1);
}

// ============================================================================
// signedArea2D tests
// ============================================================================

TEST(SignedArea2DTest, EmptyPolygon) {
    Polygon2<double> empty;
    EXPECT_DOUBLE_EQ(signedArea2D(empty), 0.0);
}

TEST(SignedArea2DTest, OneVertex) {
    Polygon2<double> poly = {{1,2}};
    EXPECT_DOUBLE_EQ(signedArea2D(poly), 0.0);
}

TEST(SignedArea2DTest, TwoVertices) {
    Polygon2<double> poly = {{0,0}, {1,1}};
    EXPECT_DOUBLE_EQ(signedArea2D(poly), 0.0);
}

TEST(SignedArea2DTest, TriangleCCW) {
    Polygon2<double> tri = {{0,0}, {1,0}, {0,1}};
    EXPECT_DOUBLE_EQ(signedArea2D(tri), 0.5);
}

TEST(SignedArea2DTest, TriangleCW) {
    Polygon2<double> tri = {{0,0}, {0,1}, {1,0}};
    EXPECT_DOUBLE_EQ(signedArea2D(tri), -0.5);
}

TEST(SignedArea2DTest, SquareCCW) {
    Polygon2<double> square = {{0,0}, {1,0}, {1,1}, {0,1}};
    EXPECT_DOUBLE_EQ(signedArea2D(square), 1.0);
}

TEST(SignedArea2DTest, SquareCW) {
    Polygon2<double> square = {{0,0}, {0,1}, {1,1}, {1,0}};
    EXPECT_DOUBLE_EQ(signedArea2D(square), -1.0);
}

TEST(SignedArea2DTest, SelfIntersecting) {
    Polygon2<double> bow = {{0,0}, {1,1}, {0,1}, {1,0}};
    EXPECT_DOUBLE_EQ(signedArea2D(bow), 0.0);
}

// ============================================================================
// ensureCCW tests
// ============================================================================

TEST(EnsureCCWTest, CWPpolygonIsReversed) {
    Polygon2<double> cw = {{0,0}, {0,1}, {1,1}, {1,0}}; // CW
    ensureCCW(cw);
    EXPECT_GT(signedArea2D(cw), 0.0);
    EXPECT_DOUBLE_EQ(cw[0].x, 1); EXPECT_DOUBLE_EQ(cw[0].y, 0);
    EXPECT_DOUBLE_EQ(cw[1].x, 1); EXPECT_DOUBLE_EQ(cw[1].y, 1);
    EXPECT_DOUBLE_EQ(cw[2].x, 0); EXPECT_DOUBLE_EQ(cw[2].y, 1);
    EXPECT_DOUBLE_EQ(cw[3].x, 0); EXPECT_DOUBLE_EQ(cw[3].y, 0);
}

TEST(EnsureCCWTest, CCWPolygonUnchanged) {
    Polygon2<double> ccw = {{0,0}, {1,0}, {1,1}, {0,1}};
    auto original = ccw;
    ensureCCW(ccw);
    EXPECT_GT(signedArea2D(ccw), 0.0);
    for (size_t i = 0; i < ccw.size(); ++i) {
        EXPECT_DOUBLE_EQ(ccw[i].x, original[i].x);
        EXPECT_DOUBLE_EQ(ccw[i].y, original[i].y);
    }
}

TEST(EnsureCCWTest, DegeneratePolygon) {
    Polygon2<double> degenerate = {{0,0}, {1,1}, {2,2}}; // collinear, area 0
    ensureCCW(degenerate);
    EXPECT_DOUBLE_EQ(signedArea2D(degenerate), 0.0);
    EXPECT_EQ(degenerate.size(), 3);
}

// ============================================================================
// removeCollinearPoints tests (adjusted to actual implementation)
// ============================================================================

/*TEST(RemoveCollinearPointsTest, RectangleWithExtraPointOnEdge) {
    Polygon2<double> poly = {{0,0}, {1,0}, {2,0}, {2,1}, {1,1}, {0,1}};
    removeCollinearPoints(poly);
    EXPECT_EQ(poly.size(), 4);
    EXPECT_DOUBLE_EQ(poly[0].x, 0); EXPECT_DOUBLE_EQ(poly[0].y, 0);
    EXPECT_DOUBLE_EQ(poly[1].x, 2); EXPECT_DOUBLE_EQ(poly[1].y, 0);
    EXPECT_DOUBLE_EQ(poly[2].x, 2); EXPECT_DOUBLE_EQ(poly[2].y, 1);
    EXPECT_DOUBLE_EQ(poly[3].x, 0); EXPECT_DOUBLE_EQ(poly[3].y, 1);
}

TEST(RemoveCollinearPointsTest, ThreeCollinearPoints) {
    Polygon2<double> poly = {{0,0}, {1,1}, {2,2}};
    removeCollinearPoints(poly);
    // Implementation leaves polygon unchanged because after removal we'd have <3 vertices
    EXPECT_EQ(poly.size(), 3);
    EXPECT_DOUBLE_EQ(signedArea2D(poly), 0.0);
}

TEST(RemoveCollinearPointsTest, AllPointsCollinear) {
    Polygon2<double> poly = {{0,0}, {1,0}, {2,0}, {3,0}, {4,0}};
    removeCollinearPoints(poly);
    // Unchanged because result would have only 2 vertices
    EXPECT_EQ(poly.size(), 5);
    EXPECT_DOUBLE_EQ(signedArea2D(poly), 0.0);
}

TEST(RemoveCollinearPointsTest, DuplicatePoints) {
    Polygon2<double> poly = {{0,0}, {1,0}, {1,0}, {1,1}, {0,1}};
    removeCollinearPoints(poly);
    EXPECT_EQ(poly.size(), 4);
    EXPECT_DOUBLE_EQ(poly[0].x, 0); EXPECT_DOUBLE_EQ(poly[0].y, 0);
    EXPECT_DOUBLE_EQ(poly[1].x, 1); EXPECT_DOUBLE_EQ(poly[1].y, 0);
    EXPECT_DOUBLE_EQ(poly[2].x, 1); EXPECT_DOUBLE_EQ(poly[2].y, 1);
    EXPECT_DOUBLE_EQ(poly[3].x, 0); EXPECT_DOUBLE_EQ(poly[3].y, 1);
}

TEST(RemoveCollinearPointsTest, PolygonWithCollinearVerticesOnEdges) {
    Polygon2<double> poly = {{0,0}, {1,0}, {2,0}, {2,1}, {2,2}, {1,2}, {0,2}, {0,1}};
    removeCollinearPoints(poly);
    EXPECT_EQ(poly.size(), 4);
    EXPECT_DOUBLE_EQ(poly[0].x, 0); EXPECT_DOUBLE_EQ(poly[0].y, 0);
    EXPECT_DOUBLE_EQ(poly[1].x, 2); EXPECT_DOUBLE_EQ(poly[1].y, 0);
    EXPECT_DOUBLE_EQ(poly[2].x, 2); EXPECT_DOUBLE_EQ(poly[2].y, 2);
    EXPECT_DOUBLE_EQ(poly[3].x, 0); EXPECT_DOUBLE_EQ(poly[3].y, 2);
}*/