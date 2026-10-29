#include <gtest/gtest.h>
#include "facade/DebugFacade.h"

using facade::DebugFacade;
using geo::DebugStep;
using geo::IndexedTriangle;
using geo::Point2;
using geo::Triangle2;
using geo::VertexIndex;

namespace {

// Вершины запуска, на которые ссылаются индексы шагов.
// x-координата первой вершины каждой тройки совпадает с номером тройки,
// чтобы проверки могли различать «треугольники» по координате.
geo::Polygon2<double> makeVertices() {
    return {
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0},   // тройка 0
        {3.0, 0.0}, {4.0, 0.0}, {4.0, 1.0},   // тройка 1
        {6.0, 0.0}, {7.0, 0.0}, {7.0, 1.0},   // тройка 2
        {9.0, 0.0}, {10.0, 0.0}, {10.0, 1.0}, // тройка 3
        {12.0, 0.0}, {13.0, 0.0}, {13.0, 1.0} // тройка 4
    };
}

// face(k) — тройка индексов {3k, 3k+1, 3k+2} в makeVertices()
IndexedTriangle makeFace(VertexIndex k) {
    return {3 * k, 3 * k + 1, 3 * k + 2};
}

DebugStep<double> makeStep(std::vector<IndexedTriangle> faces = {},
                           bool replacesState = false) {
    DebugStep<double> step;
    step.polygon_before = {0, 1, 2};
    step.added_faces = std::move(faces);
    step.replaces_state = replacesState;
    step.description = "step";
    return step;
}

} // namespace

// Свежая история начинается с начального состояния (-1).
TEST(DebugFacadeTest, SetHistoryStartsAtInitialState) {
    DebugFacade facade;
    facade.setHistory(makeVertices(), {makeStep(), makeStep()});

    EXPECT_EQ(facade.getCurrentStep(), -1);
    EXPECT_EQ(facade.getTotalSteps(), 2);
    EXPECT_TRUE(facade.hasHistory());
}

// Начальное состояние показывает полигон из первого шага.
TEST(DebugFacadeTest, InitialStateShowsFirstStepPolygon) {
    DebugFacade facade;
    facade.setHistory(makeVertices(), {makeStep()});

    EXPECT_EQ(facade.getCurrentPolygon().size(), 3u);
}

// Пустой polygon_before — sentinel «все вершины запуска».
TEST(DebugFacadeTest, EmptyPolygonBeforeMeansAllVertices) {
    DebugFacade facade;
    auto step = makeStep();
    step.polygon_before.clear();
    facade.setHistory(makeVertices(), {step});

    EXPECT_EQ(facade.getCurrentPolygon().size(), makeVertices().size());
}

// Навигация упирается в границы истории и не выходит за них.
TEST(DebugFacadeTest, NavigationStaysWithinBounds) {
    DebugFacade facade;
    facade.setHistory(makeVertices(), {makeStep(), makeStep()});

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
    facade.setHistory(makeVertices(),
                      {makeStep({makeFace(0)}),
                       makeStep({makeFace(1), makeFace(2)})});

    facade.goToStep(0);
    EXPECT_EQ(facade.getAccumulatedTriangles().size(), 1u);

    facade.goToStep(1);
    EXPECT_EQ(facade.getAccumulatedTriangles().size(), 3u);
}

// Шаг со снимком состояния (Delaunay flip) отбрасывает всё накопленное до него.
TEST(DebugFacadeTest, SnapshotStepReplacesAccumulation) {
    DebugFacade facade;
    facade.setHistory(makeVertices(),
                      {makeStep({makeFace(0)}),
                       makeStep({makeFace(1)}),
                       makeStep({makeFace(3), makeFace(4)}, /*replacesState=*/true),
                       makeStep({makeFace(2)})});

    facade.goToStep(1);
    EXPECT_EQ(facade.getAccumulatedTriangles().size(), 2u);

    // После снимка видны только треугольники снимка.
    facade.goToStep(2);
    auto snapshot = facade.getAccumulatedTriangles();
    ASSERT_EQ(snapshot.size(), 2u);
    EXPECT_DOUBLE_EQ(snapshot[0].v0.x, 9.0);

    // Дельта после снимка добавляется уже к нему.
    facade.goToStep(3);
    EXPECT_EQ(facade.getAccumulatedTriangles().size(), 3u);
}

// sweep_y возвращается только для шагов, которые его записали.
TEST(DebugFacadeTest, SweepYOnlyWhenRecorded) {
    DebugFacade facade;

    auto withSweep = makeStep();
    withSweep.sweep_y = 4.5;

    facade.setHistory(makeVertices(), {makeStep(), withSweep});

    EXPECT_FALSE(facade.getSweepY().has_value()); // шаг -1

    facade.goToStep(0);
    EXPECT_FALSE(facade.getSweepY().has_value());

    facade.goToStep(1);
    ASSERT_TRUE(facade.getSweepY().has_value());
    EXPECT_DOUBLE_EQ(*facade.getSweepY(), 4.5);
}

// Диагонали шага резолвятся из индексов в координаты вершин запуска.
TEST(DebugFacadeTest, DiagonalsResolveThroughVertices) {
    DebugFacade facade;

    auto step = makeStep();
    step.added_diagonals.emplace_back(VertexIndex{0}, VertexIndex{4});

    facade.setHistory(makeVertices(), {step});
    facade.goToStep(0);

    const auto diagonals = facade.getAddedDiagonals();
    ASSERT_EQ(diagonals.size(), 1u);
    EXPECT_DOUBLE_EQ(diagonals[0].first.x, 0.0);
    EXPECT_DOUBLE_EQ(diagonals[0].second.x, 4.0); // m_vertices[4] = {4.0, 0.0}
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
    facade.setHistory(makeVertices(), {makeStep()});
    facade.stepForward();

    facade.clear();

    EXPECT_EQ(facade.getCurrentStep(), -1);
    EXPECT_EQ(facade.getTotalSteps(), 0);
    EXPECT_FALSE(facade.hasHistory());
}
