#include "QPainterRenderer.h"
#include <QFont>
#include <QPen>
#include <QBrush>

QPainterRenderer::QPainterRenderer(QWidget* widget)
    : m_widget(widget)
{
}

void QPainterRenderer::beginFrame() {
    m_painter = std::make_unique<QPainter>(m_widget);
    m_painter->setRenderHint(QPainter::Antialiasing);
    m_isActive = true;
}

void QPainterRenderer::endFrame() {
    if (m_painter) {
        m_painter->end();
        m_painter.reset();
    }
    m_isActive = false;
}

void QPainterRenderer::setTransform(const QRectF& worldRect, const QRect& viewport) {
    if (!m_isActive) return;

    m_painter->resetTransform();

    // Масштабируем с сохранением пропорций
    qreal scaleX = viewport.width() / worldRect.width();
    qreal scaleY = viewport.height() / worldRect.height();
    qreal scale = std::min(scaleX, scaleY);

    // Центрируем
    qreal dx = viewport.center().x() - scale * worldRect.center().x();
    qreal dy = viewport.center().y() - scale * worldRect.center().y();

    m_painter->translate(dx, dy);
    m_painter->scale(scale, -scale); // инвертируем Y
}

void QPainterRenderer::drawPolygon(const std::vector<QPointF>& points,
                                   const QColor& color,
                                   float lineWidth,
                                   bool fill,
                                   const QColor& fillColor) {
    if (!m_isActive || points.size() < 2) return;

    QPolygonF poly;
    for (const auto& p : points) {
        poly << p;
    }

    m_painter->setPen(QPen(color, lineWidth));
    if (fill && points.size() >= 3) {
        m_painter->setBrush(QBrush(fillColor));
    } else {
        m_painter->setBrush(Qt::NoBrush);
    }
    m_painter->drawPolygon(poly);
}

void QPainterRenderer::drawTriangles(const std::vector<std::array<QPointF, 3>>& triangles,
                                     const QColor& color,
                                     float alpha,
                                     bool wireframe) {
    if (!m_isActive || triangles.empty()) return;

    QColor fillColor = color;
    fillColor.setAlphaF(alpha);
    m_painter->setBrush(QBrush(fillColor));
    m_painter->setPen(wireframe ? QPen(color, 1.0f) : Qt::NoPen);

    for (const auto& tri : triangles) {
        QPolygonF poly;
        poly << tri[0] << tri[1] << tri[2] << tri[0];
        m_painter->drawPolygon(poly);
    }
}

void QPainterRenderer::drawVertexLabels(const std::vector<QPointF>& points,
                                        const QColor& color) {
    if (!m_isActive || points.empty()) return;

    m_painter->setPen(color);
    QFont font = m_painter->font();
    font.setPointSize(10);
    m_painter->setFont(font);

    for (size_t i = 0; i < points.size(); ++i) {
        const QPointF& p = points[i];
        QString label = QString::number(i);
        m_painter->drawText(p + QPointF(5, -5), label);
    }
}

void QPainterRenderer::drawEarMarker(const QPointF& position,
                                     const QColor& color) {
    if (!m_isActive) return;

    m_painter->setPen(QPen(color, 3));
    m_painter->setBrush(Qt::NoBrush);
    qreal radius = 10.0;
    m_painter->drawEllipse(position, radius, radius);
}

void QPainterRenderer::drawGrid(const QRectF& worldRect) {
    if (!m_isActive) return;

    m_painter->setPen(QPen(Qt::darkGray, 1, Qt::DashLine));
    // Для простоты рисуем сетку с шагом 50 в мировых координатах,
    // но лучше привязать к масштабу. Здесь можно реализовать адаптивный шаг.
    // Пока пропустим, так как у нас нет функции определения шага.
    // Можно сделать заглушку.
}

void QPainterRenderer::drawText(const QString& text, const QPointF& position,
                                const QColor& color, int fontSize) {
    if (!m_isActive) return;

    m_painter->setPen(color);
    QFont font = m_painter->font();
    font.setPointSize(fontSize);
    m_painter->setFont(font);
    m_painter->drawText(position, text);
}