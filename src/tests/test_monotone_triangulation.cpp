#include <gtest/gtest.h>
#include "geometry/monotone_triangulation.h"
#include "geometry/types.h"

using namespace geo;

// Алгоритм ещё не реализован до конца (см. CLAUDE.md): этот тест сейчас
// нужен главным образом для того, чтобы заголовок реально компилировался
// как часть сборки и расхождения между .h и .hpp не оставались незамеченными.
TEST(MonotoneTriangulationTest, CompilesAndRunsOnSquare) {
    Polygon2<double> square = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

    MonotoneTriangulation<double> mt;
    TriangulationResult<double> result = mt.triangulate(square);
    (void)result;
}

// Невыпуклый (L-образный) полигон: makeMonotone обязан провести хотя бы одну
// диагональ, поэтому этот тест — в отличие от квадрата выше — реально
// прогоняет splitIntoMonotone(), а не только его early-return для
// уже монотонных полигонов.
TEST(MonotoneTriangulationTest, HandlesConcavePolygonWithDiagonals) {
    Polygon2<double> lshape = {{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}};

    MonotoneTriangulation<double> mt;
    TriangulationResult<double> result = mt.triangulate(lshape);
    EXPECT_TRUE(result.error_message.empty()) << result.error_message;
}
