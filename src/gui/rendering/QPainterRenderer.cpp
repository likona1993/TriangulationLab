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

void QPainterRenderer::resetView(const QRectF &worldRect, const QRect &viewport)
{
    // Масштабируем с сохранением пропорций
    qreal scaleX = viewport.width() / worldRect.width();
    qreal scaleY = viewport.height() / worldRect.height();
    m_scale = std::min(scaleX, scaleY);

    // Центрируем
    qreal dx = viewport.center().x() - m_scale * worldRect.center().x();
    qreal dy = viewport.center().y() + m_scale * worldRect.center().y();
    m_offset = {dx, dy};
}

void QPainterRenderer::applyTransform()
{
    if (!m_isActive || !m_painter) return;

    m_painter->resetTransform();

    // Применяем сохраненный сдвиг и масштаб с инверсией Y
    m_painter->translate(m_offset.x(), m_offset.y());
    m_painter->scale(m_scale, -m_scale);
}

QPointF QPainterRenderer::screenToWorld(const QPointF &screenPos)
{
    // 1. Создаем пустую матрицу трансформации
    QTransform matrix;

    // 2. Повторяем ровно те же шаги, что и в applyTransform()
    matrix.translate(m_offset.x(), m_offset.y());
    matrix.scale(m_scale, -m_scale); // Инверсия Y

    // 3. Получаем обратную матрицу
    QTransform invertedMatrix = matrix.inverted();

    // 4. Безопасно переводим координаты без обращения к m_painter
    return invertedMatrix.map(screenPos);
}

void QPainterRenderer::drawPolygon(const std::vector<QPointF> &points,
                                   const QColor &color,
                                   float lineWidth,
                                   bool fill,
                                   const QColor &fillColor)
{
    if (!m_isActive || points.size() < 1) return;

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

    // 1. Сохраняем состояние глобальной (мировой) матрицы
    m_painter->save();

    // Настраиваем шрифт один раз (размер 10pt на экране)
    QFont font = m_painter->font();
    font.setPointSize(10);
    m_painter->setFont(font);

    // Настраиваем перо и кисть для маркеров
    m_painter->setPen(Qt::NoPen); // маркеры без обводки
    m_painter->setBrush(QBrush(color)); // заливка цвета маркера

    for (size_t i = 0; i < points.size(); ++i) {
        const QPointF& p = points[i];
        
        m_painter->save(); // Сохраняем матрицу для текущей точки

        // 2. Сдвигаем начало координат в мировую точку 'p'
        m_painter->translate(p);

        // 3. Компенсируем инверсию Y и убираем мировой масштаб для этого шага.
        // Теперь (0,0) — это центр точки, ось X вправо, ось Y — вверх экрана.
        m_painter->scale(1.0, -1.0);

        // 4. Рисуем маркер (круг радиусом 2 пикселя)
        // Так как масштаб сброшен в 1.0, размеры задаются строго в пикселях экрана
        m_painter->drawEllipse(QPointF(0, 0), 2.0, 2.0);

        // 5. Рисуем текст (черным или цветом 'color')
        m_painter->setPen(color); 
        QString label = QString::number(i);
        
        // Смещение (5, -5) теперь работает в нормальных экранных координатах:
        // +5 вправо, -5 вверх относительно маркера
        m_painter->drawText(QPointF(5, -5), label);

        m_painter->restore(); // Возвращаем мировую матрицу для следующей точки
    }

    // 6. Восстанавливаем исходное состояние сцены
    m_painter->restore();
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

void QPainterRenderer::drawTextScreen(const QString &text, const QPointF &position, const QColor &color, int fontSize)
{
    if (!m_isActive) return;

    m_painter->save();
    m_painter->resetTransform(); // Сбрасываем мировую матрицу в экранные
    drawTextScreen(text, position, color, fontSize);

    m_painter->restore(); // Возвращаем мировую матрицу обратно

}
