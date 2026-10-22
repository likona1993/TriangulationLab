#include <gtest/gtest.h>
#include "facade/DebugFacade.h"

using facade::DebugFacade;
using geo::DebugStep;
using geo::Point2;
using geo::Triangle2;

namespace {

DebugStep<double> makeStep(std::vector<Triangle2<double>> triangles = {},
                           bool replacesState = false) {
    DebugStep<double> step;
    step.polygon_before = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}};
    step.added_triangles = std::move(triangles);
    step.replaces_state = replacesState;
    step.description = "step";
    return step;
}

Triangle2<double> makeTriangle(double x) {
    return Triangle2<double>{{x, 0.0}, {x + 1.0, 0.0}, {x + 1.0, 1.0}};
}

} // namespace

// Свежая история начинается с начального состояния (-1).
TEST(DebugFacadeTest, SetHistoryStartsAtInitialState) {
    DebugFacade facade;
    facade.setHistory({makeStep(), makeStep()});

    EXPECT_EQ(facade.getCurrentStep(), -1);
    EXPECT_EQ(facade.getTotalSteps(), 2);
    EXPECT_TRUE(facade.hasHistory());
}

// Начальное состояние показывает полигон из первого шага.
TEST(DebugFacadeTest, InitialStateShowsFirstStepPolygon) {
    DebugFacade facade;
    facade.setHistory({makeStep()});

    EXPECT_EQ(facade.getCurrentPolygon().size(), 3u);
}

// Навигация упирается в границы истории и не выходит за них.
TEST(DebugFacadeTest, NavigationStaysWithinBounds) {
    DebugFacade facade;
    facade.setHistory({makeStep(), makeStep()});

    EXPECT_FALSE(facade.stepBackward()); // уже в -1

    EXPECT_TRUE(facade.stepForward());
    EXPECT_TRUE(facade.stepForward());
    EXPECT_FALSE(facade.stepForward()); // уже на последнем шаге
    EXPECT_EQ(facade.getCurrentStep(), 1);

    facade.goToStep(-1);
    EXPECT_EQ(facade.getCurrentStep(), -1);

    facade.goToStep(5); // за границей — игнорируется
    EXPECT_EQ(facade.getCurrentStep(), -1);
}

// Обычные шаги накапливают треугольники по всем шагам до текущего.
TEST(DebugFacadeTest, AccumulatesTrianglesAcrossSteps) {
    DebugFacade facade;
    facade.setHistory({makeStep({makeTriangle(0.0)}),
                       makeStep({makeTriangle(1.0), makeTriangle(2.0)})});

    facade.goToStep(0);
    EXPECT_EQ(facade.getAccumulatedTriangles().size(), 1u);

    facade.goToStep(1);
    EXPECT_EQ(facade.getAccumulatedTriangles().size(), 3u);
}

// Шаг со снимком состояния (Delaunay flip) отбрасывает всё накопленное до него.
TEST(DebugFacadeTest, SnapshotStepReplacesAccumulation) {
    DebugFacade facade;
    facade.setHistory({makeStep({makeTriangle(0.0)}),
                       makeStep({makeTriangle(1.0)}),
                       makeStep({makeTriangle(5.0), makeTriangle(6.0)}, /*replacesState=*/true),
                       makeStep({makeTriangle(7.0)})});

    facade.goToStep(1);
    EXPECT_EQ(facade.getAccumulatedTriangles().size(), 2u);

    // После снимка видны только треугольники снимка.
    facade.goToStep(2);
    auto snapshot = facade.getAccumulatedTriangles();
    ASSERT_EQ(snapshot.size(), 2u);
    EXPECT_DOUBLE_EQ(snapshot[0].v0.x, 5.0);

    // Дельта после снимка добавляется уже к нему.
    facade.goToStep(3);
    EXPECT_EQ(facade.getAccumulatedTriangles().size(), 3u);
}

// sweep_y возвращается только для шагов, которые его записали.
TEST(DebugFacadeTest, SweepYOnlyWhenRecorded) {
    DebugFacade facade;

    auto withSweep = makeStep();
    withSweep.sweep_y = 4.5;

    facade.setHistory({makeStep(), withSweep});

    EXPECT_FALSE(facade.getSweepY().has_value()); // шаг -1

    facade.goToStep(0);
    EXPECT_FALSE(facade.getSweepY().has_value());

    facade.goToStep(1);
    ASSERT_TRUE(facade.getSweepY().has_value());
    EXPECT_DOUBLE_EQ(*facade.getSweepY(), 4.5);
}

// Данные текущего шага за границами истории возвращают пустые значения.
TEST(DebugFacadeTest, StepDataIsEmptyOutsideHistory) {
    DebugFacade facade;

    EXPECT_TRUE(facade.getAccumulatedTriangles().empty());
    EXPECT_TRUE(facade.getAddedDiagonals().empty());
    EXPECT_TRUE(facade.getHighlightedVertices().empty());
    EXPECT_FALSE(facade.getSweepY().has_value());
    EXPECT_EQ(facade.getStepDescription(), "Initial polygon");
}

// clear() сбрасывает историю и позицию.
TEST(DebugFacadeTest, ClearResetsState) {
    DebugFacade facade;
    facade.setHistory({makeStep()});
    facade.stepForward();

    facade.clear();

    EXPECT_EQ(facade.getCurrentStep(), -1);
    EXPECT_EQ(facade.getTotalSteps(), 0);
    EXPECT_FALSE(facade.hasHistory());
}
