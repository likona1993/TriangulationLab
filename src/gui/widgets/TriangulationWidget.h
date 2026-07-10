#pragma once
#include <QWidget>
#include <memory>
#include <vector>
#include <QPointF>
#include <array>

#include "../facade/TriangulationFacade.h"
#include "../rendering/IRenderer.h"

/**
 * @brief Виджет для отображения полигона и триангуляции.
 *
 * Использует отдельный рендерер (QPainterRenderer или OpenGLRenderer)
 * для отрисовки. Всё управление данными делегируется фасаду.
 */
class TriangulationWidget : public QWidget {
    Q_OBJECT
public:
    explicit TriangulationWidget(QWidget* parent = nullptr);
    ~TriangulationWidget() override;

    // Доступ к фасаду (для MainWindow)
    facade::TriangulationFacade* facade() const { return m_facade.get(); }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onPolygonChanged(const std::vector<QPointF>& points);
    void onTriangulationCompleted(const std::vector<std::array<QPointF, 3>>& triangles);
    void onDebugStepChanged(int currentStep, int totalSteps);

private:
    // === Компоненты ===
    std::unique_ptr<facade::TriangulationFacade> m_facade;
    std::unique_ptr<IRenderer> m_renderer;

    // === Кэш данных для рендеринга ===
    std::vector<QPointF> m_polygonPoints;
    std::vector<std::array<QPointF, 3>> m_triangles;
    QRectF m_currentBBox;

    std::vector<QPointF> m_debugPolygon;
    std::vector<std::array<QPointF, 3>> m_debugTriangles;
    int m_currentStep = -1;
    int m_totalSteps = 0;
    bool m_showDebug = false;

    // === Вспомогательные методы ===
    void updateWorldBounds();
    QRectF getBoundingRect(const std::vector<QPointF>& points) const;
    void updateDebugData();
};