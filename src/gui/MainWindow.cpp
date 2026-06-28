#include "MainWindow.h"
#include "widgets/TriangulationWidget.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QAction>

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

    // Статус-бар (пока пустой, но можно использовать для сообщений)
    statusBar()->showMessage("Draw polygon with left click, press Enter to triangulate");
}