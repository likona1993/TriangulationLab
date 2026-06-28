#pragma once
#include <QMainWindow>

class TriangulationWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    TriangulationWidget* m_widget = nullptr;
};