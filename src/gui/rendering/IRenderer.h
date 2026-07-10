#pragma once
#include <QPointF>
#include <QRectF>
#include <QColor>
#include <vector>
#include <array>

/**
 * @brief Интерфейс для рендеринга геометрических примитивов.
 *
 * Позволяет абстрагировать виджет от конкретной реализации рендеринга
 * (QPainter, OpenGL, и т.д.).
 */
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Управление контекстом рисования
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    // Настройка трансформации (мировые координаты -> экранные)
    virtual void resetView(const QRectF& worldRect, const QRect& viewport) = 0;

    virtual void applyTransform() = 0;

    //перевод экранных координат в мировую систему координат
    virtual QPointF screenToWorld(const QPointF& screenPosition) = 0;

    // Отрисовка примитивов
    virtual void drawPolygon(const std::vector<QPointF>& points,
                             const QColor& color,
                             float lineWidth = 1.0f,
                             bool fill = false,
                             const QColor& fillColor = QColor()) = 0;

    virtual void drawTriangles(const std::vector<std::array<QPointF, 3>>& triangles,
                               const QColor& color,
                               float alpha = 0.3f,
                               bool wireframe = true) = 0;

    virtual void drawVertexLabels(const std::vector<QPointF>& points,
                                  const QColor& color) = 0;

    virtual void drawEarMarker(const QPointF& position,
                               const QColor& color) = 0;

    // Дополнительно: сетка, оси и т.д.
    virtual void drawGrid(const QRectF& worldRect) = 0;

    // Информационные надписи
    virtual void drawText(const QString& text, const QPointF& position,
                          const QColor& color = Qt::white, int fontSize = 12) = 0;
    
    //Отрисовка текста в экранных координатах
    virtual void drawTextScreen(const QString& text, const QPointF& position,
                          const QColor& color = Qt::white, int fontSize = 12) = 0;

    //масштаб
    qreal m_scale = 1.0;
    //сдвиг
    QPointF m_offset = QPointF(0.0, 0.0); 
    
};