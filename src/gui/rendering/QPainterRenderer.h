#pragma once
#include "IRenderer.h"
#include <QPainter>
#include <QWidget>

/**
 * @brief Реализация рендерера через QPainter.
 */
class QPainterRenderer : public IRenderer {
public:
    explicit QPainterRenderer(QWidget* widget);
    ~QPainterRenderer() override = default;

    void beginFrame() override;
    void endFrame() override;
    void resetView(const QRectF& worldRect, const QRect& viewport) override;

    void applyTransform() override;

    QPointF screenToWorld(const QPointF& screenPos) override;

    void drawPolygon(const std::vector<QPointF>& points,
                     const QColor& color,
                     float lineWidth,
                     bool fill,
                     const QColor& fillColor) override;

    void drawTriangles(const std::vector<std::array<QPointF, 3>>& triangles,
                       const QColor& color,
                       float alpha,
                       bool wireframe) override;

    void drawVertexLabels(const std::vector<QPointF>& points,
                          const QColor& color) override;

    void drawEarMarker(const QPointF& position,
                       const QColor& color) override;

    void drawGrid(const QRectF& worldRect) override;

    void drawText(const QString& text, const QPointF& position,
                  const QColor& color, int fontSize) override;

    void drawTextScreen(const QString& text, const QPointF& position,
        const QColor& color = Qt::black, int fontSize = 12) override;

private:
    QWidget* m_widget = nullptr;
    std::unique_ptr<QPainter> m_painter;
    bool m_isActive = false;
};