#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "geometry/delaunay_flip.h"
#include "geometry/types.h"
#include "utils/geometry_predicates.h"
#include "utils/numeric.h"
#include "test_triangulation_utils.h"

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

// ---------------------------------------------------------------
//  Инварианты Этапа 0 (ROADMAP.md): независимые проверки результата
// ---------------------------------------------------------------

// Ключ ребра по координатам вершин, порядок вершин нормализован.
struct CoordEdge {
    Point2<double> a;
    Point2<double> b;
};

bool pointLess(const Point2<double>& l, const Point2<double>& r) {
    if (l.x != r.x) return l.x < r.x;
    return l.y < r.y;
}

bool operator<(const CoordEdge& l, const CoordEdge& r) {
    if (l.a != r.a) return pointLess(l.a, r.a);
    return pointLess(l.b, r.b);
}

CoordEdge makeCoordEdge(const Point2<double>& p, const Point2<double>& q) {
    return pointLess(p, q) ? CoordEdge{p, q} : CoordEdge{q, p};
}

// Каждая вершина входного полигона должна встретиться хотя бы в одном
// треугольнике результата.
bool allVerticesCovered(
    const Polygon2<double>& polygon,
    const std::vector<Triangle2<double>>& triangles)
{
    for (const auto& v : polygon) {
        bool found = false;
        for (const auto& t : triangles) {
            if (t.v0 == v || t.v1 == v || t.v2 == v) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

// Рёбра триангуляции не должны пересекаться иначе как в общих вершинах.
// Квадратичная проверка: для больших входов (>= ~1k треугольников) её
// стоит пропускать ради времени прогона.
bool noEdgeIntersections(const std::vector<Triangle2<double>>& triangles) {
    struct Segment {
        Point2<double> p;
        Point2<double> q;
    };

    std::vector<Segment> segments;
    segments.reserve(triangles.size() * 3);
    for (const auto& t : triangles) {
        segments.push_back({t.v0, t.v1});
        segments.push_back({t.v1, t.v2});
        segments.push_back({t.v2, t.v0});
    }

    auto shareEndpoint = [](const Segment& s, const Segment& r) {
        return s.p == r.p || s.p == r.q || s.q == r.p || s.q == r.q;
    };

    for (std::size_t i = 0; i < segments.size(); ++i) {
        for (std::size_t j = i + 1; j < segments.size(); ++j) {
            if (shareEndpoint(segments[i], segments[j])) continue;
            if (segmentsIntersect(
                    segments[i].p, segments[i].q,
                    segments[j].p, segments[j].q,
                    EPSILON<double>)) {
                return false;
            }
        }
    }
    return true;
}

// Локальный критерий Делоне для каждого внутреннего ребра, по определению:
// противоположная вершина соседнего треугольника не лежит СТРОГО внутри
// описанной окружности (кокруговой случай считается законным).
// Проверка независима от внутренних структур DelaunayFlip.
bool allInteriorEdgesLocallyDelaunay(
    const std::vector<Triangle2<double>>& triangles)
{
    std::map<CoordEdge, std::vector<Point2<double>>> edgeToOpposites;
    for (const auto& t : triangles) {
        edgeToOpposites[makeCoordEdge(t.v0, t.v1)].push_back(t.v2);
        edgeToOpposites[makeCoordEdge(t.v1, t.v2)].push_back(t.v0);
        edgeToOpposites[makeCoordEdge(t.v2, t.v0)].push_back(t.v1);
    }

    for (const auto& [edge, opposites] : edgeToOpposites) {
        if (opposites.size() == 1) continue;    // граничное ребро
        if (opposites.size() != 2) return false; // ребро у 3+ треугольников — брак

        const auto& a = edge.a;
        const auto& b = edge.b;
        const auto& c = opposites[0];
        const auto& d = opposites[1];

        // Критерий применим только к выпуклому четырёхугольнику:
        // c и d по разные стороны от ребра.
        const int o1 = orientation(a, b, c);
        const int o2 = orientation(a, b, d);
        if (o1 == 0 || o2 == 0 || o1 == o2) continue;

        // inCircumcircle требует CCW-ориентацию треугольника.
        const bool illegal = (o1 == 2)
            ? inCircumcircle(a, b, c, d)
            : inCircumcircle(a, c, b, d);
        if (illegal) return false;
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
    // Инвариант-проверки ниже исторически написаны над координатными
    // треугольниками; materializeTriangles — легальная точка конвертации
    // в тестовом слое (Этап 1 Roadmap).
    const auto triangles = materializeTriangles(result);
    ASSERT_EQ(triangles.size(), expectedTriangleCount);

    EXPECT_NEAR(
        sumTrianglesArea(triangles),
        absArea(polygon),
        EPSILON<T>);

    EXPECT_TRUE(
        allTrianglesNonDegenerate(
            triangles,
            EPSILON<T>));

    EXPECT_TRUE(
        allTrianglesCCW(
            triangles,
            EPSILON<T>));

    EXPECT_TRUE(
        allPolygonBoundaryEdgesPresent(
            polygon,
            triangles));

    EXPECT_TRUE(
        allVerticesCovered(
            polygon,
            triangles));

    // Квадратично по числу рёбер — для больших входов пропускаем.
    if (triangles.size() <= 5000) {
        EXPECT_TRUE(noEdgeIntersections(triangles));
    }

    EXPECT_TRUE(allInteriorEdgesLocallyDelaunay(triangles));

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

    const auto triangles = materializeTriangles(result);
    EXPECT_TRUE(hasEdge(
        triangles,
        Point2<double>{-1, 0},
        Point2<double>{1, 0}));

    EXPECT_FALSE(hasEdge(
        triangles,
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

    const auto triangles = materializeTriangles(result);
    EXPECT_TRUE(hasTriangle(
        triangles,
        polygon[0], polygon[1], polygon[3]));

    EXPECT_TRUE(hasTriangle(
        triangles,
        polygon[0], polygon[3], polygon[4]));

    EXPECT_TRUE(hasTriangle(
        triangles,
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

    const auto triangles = materializeTriangles(result);
    const bool firstDiagonal = hasEdge(
        triangles,
        Point2<double>{0, 0},
        Point2<double>{1, 1});

    const bool secondDiagonal = hasEdge(
        triangles,
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

TEST(DelaunayFlipTest, FlipStepsCarryFullStateSnapshot) {
    // Шаг флипа должен содержать полный снимок текущей триангуляции,
    // а не пару треугольников-дельту: иначе накопление шагов в GUI
    // рисует новые треугольники поверх ещё не перевёрнутых.
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
    ASSERT_TRUE(result);

    const auto& history = delaunayFlip.getDebugHistory();
    ASSERT_GE(countFlipSteps(history), 1u);

    for (const auto& step : history) {
        if (step.description != "Flip illegal edge") {
            continue; // шаги начальной Ear Clipping-триангуляции
        }

        EXPECT_TRUE(step.replaces_state);
        EXPECT_EQ(step.added_faces.size(), polygon.size() - 2);
        // polygon_before пустой = sentinel «все вершины запуска»
        EXPECT_TRUE(step.polygon_before.empty());
        EXPECT_EQ(step.highlighted_vertices.size(), 4u);
    }
}

// ============================================================
// Этап 0 (ROADMAP.md): большой вход и зафиксированное поведение
// на дубликатах / близко лежащих точках
// ============================================================

TEST(DelaunayFlipTest, ThousandContourPointsSatisfyAllInvariants) {
    // 1024 точки на контуре с детерминированным дрожанием радиуса:
    // полигон остаётся звёздным (и потому простым), а точки не кокруговы.
    const std::size_t n = 1024;
    const double pi = std::acos(-1.0);

    Polygon2<double> polygon;
    polygon.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double angle = 2.0 * pi * static_cast<double>(i) / static_cast<double>(n);
        const double radius = 1.0 + 0.05 * std::sin(12.9898 * static_cast<double>(i));
        polygon.push_back({radius * std::cos(angle), radius * std::sin(angle)});
    }

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(polygon);

    expectValidTriangulation(polygon, result, polygon.size() - 2);
}

TEST(DelaunayFlipTest, DuplicatePointsAreRejected) {
    // Поведение с Этапа 1 ROADMAP.md: точный дубликат вершины (соседний
    // или нет) отклоняется явной ошибкой DuplicateVertices на входе,
    // а не побочным отказом начальной Ear Clipping-фазы.
    const Polygon2<double> dupConsecutive = {
        {0, 0}, {2, 0}, {2, 0}, {2, 2}, {0, 2}
    };
    const Polygon2<double> dupNonConsecutive = {
        {0, 0}, {2, 0}, {2, 2}, {0, 2}, {0, 0}
    };

    for (const auto& polygon : {dupConsecutive, dupNonConsecutive}) {
        DelaunayFlip<double> delaunayFlip;
        const auto result = delaunayFlip.triangulate(polygon);

        EXPECT_FALSE(result);
        ASSERT_TRUE(result.error.has_value());
        EXPECT_EQ(result.error->code, ErrorCode::DuplicateVertices);
        EXPECT_TRUE(result.faces.empty());
    }
}

TEST(DelaunayFlipTest, NearDuplicatePointsBehavior) {
    // Поведение зависит от EPSILON: точки ближе ~1e-10 считаются совпадающими
    // и дают DuplicateVertices; точки дальше ~1e-9 триангулируются как обычные.
    const Polygon2<double> tooClose = {
        {0, 0}, {2, 0}, {2 + 1e-12, 1e-12}, {2, 2}, {0, 2}
    };
    const Polygon2<double> farEnough = {
        {0, 0}, {2, 0}, {2 + 1e-9, 1e-9}, {2, 2}, {0, 2}
    };

    DelaunayFlip<double> rejected;
    const auto rejectedResult = rejected.triangulate(tooClose);
    EXPECT_FALSE(rejectedResult);
    ASSERT_TRUE(rejectedResult.error.has_value());
    EXPECT_EQ(rejectedResult.error->code, ErrorCode::DuplicateVertices);

    DelaunayFlip<double> accepted;
    const auto acceptedResult = accepted.triangulate(farEnough);
    expectValidTriangulation(farEnough, acceptedResult, 3);
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
    EXPECT_TRUE(result.faces.empty());
}

TEST(DelaunayFlipTest, AllCollinearPolygonIsRejected) {
    // Полностью коллинеарный набор точек имеет нулевую площадь.
    const Polygon2<double> collinear = {
        {0, 0}, {1, 0}, {2, 0}, {3, 0}
    };

    DelaunayFlip<double> delaunayFlip;
    const auto result = delaunayFlip.triangulate(collinear);

    EXPECT_FALSE(result);
    EXPECT_TRUE(result.faces.empty());
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

    const auto triangles = materializeTriangles(secondResult);
    EXPECT_TRUE(hasEdge(
        triangles,
        Point2<double>{-1, 0},
        Point2<double>{1, 0}));

    EXPECT_FALSE(hasEdge(
        triangles,
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

// ============================================================
// Индексный вывод (faces + vertices) — Этап 1 Roadmap
// ============================================================

namespace {

void expectIndexedResultValid(const Polygon2<double>& input) {
    DelaunayFlip<double> delaunayFlip;
    auto res = delaunayFlip.triangulate(input);
    ASSERT_TRUE(res);

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

        // ориентация грани — CCW (инвариант сохраняется и после флипов)
        Polygon2<double> triPoly = {
            res.vertices[f[0]], res.vertices[f[1]], res.vertices[f[2]]};
        EXPECT_GT(signedArea2D(triPoly), 0.0);
    }
}

} // namespace

TEST(DelaunayFlipIndexedTest, SquareFaces) {
    expectIndexedResultValid({{0, 0}, {1, 0}, {1, 1}, {0, 1}});
}

TEST(DelaunayFlipIndexedTest, LShapeFaces) {
    expectIndexedResultValid({{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}});
}

TEST(DelaunayFlipIndexedTest, ZigzagCorridorFacesAfterFlips) {
    // Вход с несколькими флипами: faces должны оставаться валидными
    // и после локальных перестроений триангуляции.
    expectIndexedResultValid({
        {0.0, 0.0}, {2.0, 0.25}, {4.0, 0.0}, {6.0, 0.25}, {8.0, 0.0},
        {8.0, 3.0}, {6.0, 2.75}, {4.0, 3.0}, {2.0, 2.75}, {0.0, 3.0}});
}

TEST(DelaunayFlipIndexedTest, ErrorPathsLeaveFacesEmpty) {
    DelaunayFlip<double> delaunayFlip;

    auto res = delaunayFlip.triangulate({{0, 0}, {1, 1}, {0, 1}, {1, 0}}); // self-intersecting
    EXPECT_FALSE(res);
    EXPECT_TRUE(res.faces.empty());

    res = delaunayFlip.triangulate({{0, 0}, {1, 0}}); // < 3 вершин
    EXPECT_FALSE(res);
    EXPECT_TRUE(res.faces.empty());
}
