#include <gtest/gtest.h>
#include "geometry/types.h"
#include "utils/geometry_predicates.h"
#include "utils/numeric.h"

using namespace geo;

// ============================================================================
// orientation
// ============================================================================

TEST(OrientationTest, Collinear) {
    Point2<double> p(0, 0), q(1, 0), r(2, 0);
    EXPECT_EQ(orientation(p, q, r, EPSILON<double>), 0);
}

TEST(OrientationTest, Clockwise) {
    Point2<double> p(0, 0), q(1, 0), r(1, -1);
    EXPECT_EQ(orientation(p, q, r, EPSILON<double>), 1);
}

TEST(OrientationTest, CounterClockwise) {
    Point2<double> p(0, 0), q(1, 0), r(1, 1);
    EXPECT_EQ(orientation(p, q, r, EPSILON<double>), 2);
}

TEST(OrientationTest, NearCollinearWithinEpsilon) {
    Point2<double> p(0, 0), q(1, 0), r(2, 1e-10);
    EXPECT_EQ(orientation(p, q, r, EPSILON<double>), 0);
}

// ============================================================================
// onSegment (assumes p, q, r already collinear)
// ============================================================================

TEST(OnSegmentTest, PointBetweenEndpoints) {
    Point2<double> p(0, 0), r(4, 0), q(2, 0);
    EXPECT_TRUE(onSegment(p, q, r, EPSILON<double>));
}

TEST(OnSegmentTest, PointOutsideEndpoints) {
    Point2<double> p(0, 0), r(4, 0), q(5, 0);
    EXPECT_FALSE(onSegment(p, q, r, EPSILON<double>));
}

TEST(OnSegmentTest, PointAtEndpoint) {
    Point2<double> p(0, 0), r(4, 0), q(0, 0);
    EXPECT_TRUE(onSegment(p, q, r, EPSILON<double>));
}

TEST(OnSegmentTest, DiagonalSegment) {
    Point2<double> p(0, 0), r(4, 4), q(2, 2);
    EXPECT_TRUE(onSegment(p, q, r, EPSILON<double>));
}

// ============================================================================
// segmentsIntersect
// ============================================================================

TEST(SegmentsIntersectTest, ProperCrossing) {
    Point2<double> p1(0, 0), q1(2, 2), p2(0, 2), q2(2, 0);
    EXPECT_TRUE(segmentsIntersect(p1, q1, p2, q2, EPSILON<double>));
}

TEST(SegmentsIntersectTest, DisjointSegments) {
    Point2<double> p1(0, 0), q1(1, 0), p2(0, 1), q2(1, 1);
    EXPECT_FALSE(segmentsIntersect(p1, q1, p2, q2, EPSILON<double>));
}

TEST(SegmentsIntersectTest, SharedEndpointCountsAsIntersection) {
    Point2<double> p1(0, 0), q1(1, 1), p2(1, 1), q2(2, 0);
    EXPECT_TRUE(segmentsIntersect(p1, q1, p2, q2, EPSILON<double>));
}

TEST(SegmentsIntersectTest, CollinearOverlapping) {
    Point2<double> p1(0, 0), q1(2, 0), p2(1, 0), q2(3, 0);
    EXPECT_TRUE(segmentsIntersect(p1, q1, p2, q2, EPSILON<double>));
}

TEST(SegmentsIntersectTest, CollinearNonOverlapping) {
    Point2<double> p1(0, 0), q1(1, 0), p2(2, 0), q2(3, 0);
    EXPECT_FALSE(segmentsIntersect(p1, q1, p2, q2, EPSILON<double>));
}

TEST(SegmentsIntersectTest, ParallelNonIntersecting) {
    Point2<double> p1(0, 0), q1(1, 0), p2(0, 1), q2(1, 2);
    EXPECT_FALSE(segmentsIntersect(p1, q1, p2, q2, EPSILON<double>));
}

// ============================================================================
// isSimplePolygon
// ============================================================================

TEST(IsSimplePolygonTest, SquareIsSimple) {
    Polygon2<double> square = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    EXPECT_TRUE(isSimplePolygon(square, EPSILON<double>));
}

TEST(IsSimplePolygonTest, BowTieIsNotSimple) {
    Polygon2<double> bow = {{0, 0}, {1, 1}, {1, 0}, {0, 1}};
    EXPECT_FALSE(isSimplePolygon(bow, EPSILON<double>));
}

TEST(IsSimplePolygonTest, ConcaveLShapeIsSimple) {
    Polygon2<double> lshape = {{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}};
    EXPECT_TRUE(isSimplePolygon(lshape, EPSILON<double>));
}

// ============================================================================
// isConvex
// ============================================================================

TEST(IsConvexTest, ConvexVertexOfCCWSquare) {
    Point2<double> prev(0, 0), curr(1, 0), next(1, 1);
    EXPECT_TRUE(isConvex(prev, curr, next, EPSILON<double>));
}

TEST(IsConvexTest, ReflexVertexOfLShape) {
    // Reflex (inner) vertex of an L-shaped CCW polygon.
    Point2<double> prev(2, 1), curr(1, 1), next(1, 2);
    EXPECT_FALSE(isConvex(prev, curr, next, EPSILON<double>));
}

// ============================================================================
// pointInTriangle
// ============================================================================

TEST(PointInTriangleTest, PointStrictlyInside) {
    Point2<double> a(0, 0), b(4, 0), c(0, 4);
    Point2<double> p(1, 1);
    EXPECT_TRUE(pointInTriangle(p, a, b, c, EPSILON<double>));
}

TEST(PointInTriangleTest, PointStrictlyOutside) {
    Point2<double> a(0, 0), b(4, 0), c(0, 4);
    Point2<double> p(3, 3);
    EXPECT_FALSE(pointInTriangle(p, a, b, c, EPSILON<double>));
}

TEST(PointInTriangleTest, PointOnEdge) {
    Point2<double> a(0, 0), b(4, 0), c(0, 4);
    Point2<double> p(2, 0);
    EXPECT_TRUE(pointInTriangle(p, a, b, c, EPSILON<double>));
}

TEST(PointInTriangleTest, PointOnVertex) {
    Point2<double> a(0, 0), b(4, 0), c(0, 4);
    EXPECT_TRUE(pointInTriangle(a, a, b, c, EPSILON<double>));
}

TEST(PointInTriangleTest, WindingOrderDoesNotMatter) {
    // Same triangle, opposite (CW) winding.
    Point2<double> a(0, 0), b(0, 4), c(4, 0);
    Point2<double> p(1, 1);
    EXPECT_TRUE(pointInTriangle(p, a, b, c, EPSILON<double>));
}

// ============================================================================
// computeXIntersection
// ============================================================================

TEST(ComputeXIntersectionTest, MidpointOfSlantedEdge) {
    Point2<double> p1(0, 0), p2(2, 2);
    EXPECT_DOUBLE_EQ(computeXIntersection(p1, p2, 1.0), 1.0);
}

TEST(ComputeXIntersectionTest, AtLowerEndpoint) {
    Point2<double> p1(0, 0), p2(2, 4);
    EXPECT_DOUBLE_EQ(computeXIntersection(p1, p2, 0.0), 0.0);
}

TEST(ComputeXIntersectionTest, AtUpperEndpoint) {
    Point2<double> p1(0, 0), p2(2, 4);
    EXPECT_DOUBLE_EQ(computeXIntersection(p1, p2, 4.0), 2.0);
}

TEST(ComputeXIntersectionTest, OutOfRangeYIsClamped) {
    Point2<double> p1(0, 0), p2(2, 4);
    EXPECT_DOUBLE_EQ(computeXIntersection(p1, p2, 10.0), 2.0);
    EXPECT_DOUBLE_EQ(computeXIntersection(p1, p2, -10.0), 0.0);
}

TEST(ComputeXIntersectionTest, HorizontalEdgeReturnsLeftEnd) {
    Point2<double> p1(3, 5), p2(1, 5);
    EXPECT_DOUBLE_EQ(computeXIntersection(p1, p2, 5.0), 1.0);
}

// ============================================================================
// isAbove
// ============================================================================

TEST(IsAboveTest, HigherYIsAbove) {
    Point2<double> a(0, 5), b(0, 1);
    EXPECT_TRUE(isAbove(a, b));
    EXPECT_FALSE(isAbove(b, a));
}

TEST(IsAboveTest, SameYTieBreaksOnSmallerX) {
    Point2<double> a(1, 3), b(2, 3);
    EXPECT_TRUE(isAbove(a, b));
    EXPECT_FALSE(isAbove(b, a));
}
