#include "MainWindow.h"
#include "widgets/TriangulationWidget.h"
#include "widgets/DebugControlsWidget.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QAction>
#include <QComboBox>
#include <QToolBar>

#include "../core/geometry/triangulation_algorithm.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("Triangulation Lab");
    resize(800, 600);

    m_widget = new TriangulationWidget(this);
    setCentralWidget(m_widget);

    // Меню
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* clearAction = fileMenu->addAction("&Clear");
    connect(clearAction, &QAction::triggered, this, [this]() {
        m_widget->facade()->clearPolygon();
    });

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    //комбо-бокс с выбором фасада
    auto* toolBar = addToolBar("Algorithms");
    toolBar->setMovable(false);

    auto* combo = new QComboBox(this);
    combo->addItem("Ear clipping", static_cast<int>(geo::TriangulationAlgorithm::EarClipping));
    combo->addItem("Monotone", static_cast<int>(geo::TriangulationAlgorithm::Monotone));
    combo->addItem("Delaunay Flip", static_cast<int>(geo::TriangulationAlgorithm::DelaunayFlip));

    toolBar->addWidget(combo);

    connect(combo, &QComboBox::currentIndexChanged, this, [this, combo](int index) {
        Q_UNUSED(index);

        const auto algorithm =
            static_cast<geo::TriangulationAlgorithm>(combo->currentData().toInt());

        m_widget->setAlgorithm(algorithm);
        m_widget->setFocus();
    });

    // Панель пошагового просмотра: навигация по истории текущего алгоритма
    auto* debugControls = new DebugControlsWidget(this);
    debugControls->setFacade(m_widget->facade());
    toolBar->addWidget(debugControls);

    // Нажатие кнопок навигации включает debug-режим на холсте
    connect(debugControls, &DebugControlsWidget::userNavigated,
            m_widget, &TriangulationWidget::ensureDebugVisible);

    // Статус-бар (пока пустой, но можно использовать для сообщений)
    statusBar()->showMessage("Draw polygon with left click, press Enter to triangulate");
}
