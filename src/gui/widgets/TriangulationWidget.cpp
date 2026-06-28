#include "TriangulationWidget.h"
#include "../rendering/QPainterRenderer.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPainter>
#include <algorithm>

TriangulationWidget::TriangulationWidget(QWidget* parent)
    : QWidget(parent)
    , m_facade(std::make_unique<facade::TriangulationFacade>(this))
    , m_renderer(std::make_unique<QPainterRenderer>(this))
{
    setMinimumSize(600, 600);
    setFocusPolicy(Qt::StrongFocus);

    // Подключаем сигналы фасада
    connect(m_facade.get(), &facade::TriangulationFacade::polygonChanged,
            this, &TriangulationWidget::onPolygonChanged);
    connect(m_facade.get(), &facade::TriangulationFacade::triangulationCompleted,
            this, &TriangulationWidget::onTriangulationCompleted);
    connect(m_facade.get(), &facade::TriangulationFacade::debugStepChanged,
            this, &TriangulationWidget::onDebugStepChanged);
    connect(m_facade.get(), &facade::TriangulationFacade::errorOccurred,
            [this](const QString& msg) {
                QMessageBox::warning(this, "Triangulation Error", msg);
            });
}

TriangulationWidget::~TriangulationWidget() = default;

// ============================================================
//  Рендеринг
// ============================================================

void TriangulationWidget::paintEvent(QPaintEvent* /*event*/) {
    // Начинаем кадр
    m_renderer->beginFrame();

    // Собираем все точки для вычисления bounding box
    std::vector<QPointF> allPoints = m_polygonPoints;
    if (m_showDebug && !m_debugPolygon.empty()) {
        allPoints.insert(allPoints.end(), m_debugPolygon.begin(), m_debugPolygon.end());
    }
    if (!m_triangles.empty()) {
        for (const auto& tri : m_triangles) {
            allPoints.push_back(tri[0]);
            allPoints.push_back(tri[1]);
            allPoints.push_back(tri[2]);
        }
    }
    if (!m_debugTriangles.empty()) {
        for (const auto& tri : m_debugTriangles) {
            allPoints.push_back(tri[0]);
            allPoints.push_back(tri[1]);
            allPoints.push_back(tri[2]);
        }
    }

    QRectF bbox = getBoundingRect(allPoints);
    if (bbox.isEmpty()) {
        bbox = QRectF(0, 0, width(), height());
    }

    // Устанавливаем трансформацию
    QRect viewport(0, 0, width(), height());
    m_renderer->setTransform(bbox, viewport);

    // Рисуем сетку (пока пусто)
    m_renderer->drawGrid(bbox);

    // 1. Отладочные треугольники (если режим отладки)
    if (m_showDebug && !m_debugTriangles.empty()) {
        m_renderer->drawTriangles(m_debugTriangles, QColor(0, 200, 255), 0.2f, true);
    }

    // 2. Финальные треугольники (если не режим отладки)
    if (!m_showDebug && !m_triangles.empty()) {
        m_renderer->drawTriangles(m_triangles, QColor(0, 200, 100), 0.3f, true);
    }

    // 3. Основной полигон
    if (!m_polygonPoints.empty()) {
        QColor polyColor = m_facade->isPolygonClosed() ? Qt::green : Qt::white;
        m_renderer->drawPolygon(m_polygonPoints, polyColor, 2.0f, false);
    }

    // 4. Отладочный полигон и подсветка уха
    if (m_showDebug && !m_debugPolygon.empty()) {
        m_renderer->drawPolygon(m_debugPolygon, QColor(255, 200, 50), 2.0f, false);
        size_t earIdx = m_facade->getDebugEarIndex();
        if (earIdx < m_debugPolygon.size()) {
            m_renderer->drawEarMarker(m_debugPolygon[earIdx], Qt::red);
        }
        m_renderer->drawVertexLabels(m_debugPolygon, Qt::yellow);
    } else {
        m_renderer->drawVertexLabels(m_polygonPoints, Qt::white);
    }

    // 5. Информационная строка (в экранных координатах)
    if (m_showDebug && m_totalSteps > 0) {
        QString info = QString("Step %1 / %2: %3")
        .arg(m_currentStep + 1)
            .arg(m_totalSteps)
            .arg(m_facade->getDebugDescription());
        // Рисуем текст поверх трансформации — сбрасываем в экранные координаты
        // Просто вызываем drawText с позицией в экранных координатах, но наш drawText
        // работает в мировых координатах. Чтобы нарисовать текст в экранных,
        // можно временно сбросить трансформацию. Для этого в QPainterRenderer
        // можно добавить отдельный метод, но пока мы просто нарисуем текст
        // через обычный QPainter напрямую. Но чтобы не нарушать абстракцию,
        // мы можем добавить метод drawTextScreen в IRenderer.
        // Для простоты пока нарисуем через QPainter отдельно.
        // Временно: используем QPainter напрямую (нарушаем абстракцию, но для прототипа сойдёт).
        QPainter painter(this);
        painter.setPen(Qt::white);
        QFont font = painter.font();
        font.setPointSize(12);
        painter.setFont(font);
        painter.drawText(10, 30, info);
        painter.end();
        // В будущем можно добавить метод в IRenderer: drawTextInScreenCoords
    }

    m_renderer->endFrame();
}

// ============================================================
//  Обработка событий
// ============================================================

void TriangulationWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        m_facade->addPoint(scenePos);
    }
}

void TriangulationWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        m_facade->closePolygon();
        break;
    case Qt::Key_Backspace:
        m_facade->removeLastPoint();
        break;
    case Qt::Key_C:
        m_facade->clearPolygon();
        m_showDebug = false;
        break;
    case Qt::Key_D:
        m_showDebug = !m_showDebug;
        if (m_showDebug && m_totalSteps > 0) {
            m_facade->goToStep(-1);
        }
        updateDebugData();
        update();
        break;
    case Qt::Key_Right:
        if (m_showDebug) {
            m_facade->stepForward();
            updateDebugData();
            update();
        }
        break;
    case Qt::Key_Left:
        if (m_showDebug) {
            m_facade->stepBackward();
            updateDebugData();
            update();
        }
        break;
    default:
        QWidget::keyPressEvent(event);
        break;
    }
}

// ============================================================
//  Слоты обновления из фасада
// ============================================================

void TriangulationWidget::onPolygonChanged(const std::vector<QPointF>& points) {
    m_polygonPoints = points;
    m_showDebug = false;
    updateDebugData();
    update();
}

void TriangulationWidget::onTriangulationCompleted(const std::vector<std::array<QPointF, 3>>& triangles) {
    m_triangles = triangles;
    if (m_showDebug && m_totalSteps > 0) {
        m_facade->goToStep(-1);
    }
    updateDebugData();
    update();
}

void TriangulationWidget::onDebugStepChanged(int currentStep, int totalSteps) {
    m_currentStep = currentStep;
    m_totalSteps = totalSteps;
    if (m_showDebug) {
        updateDebugData();
        update();
    }
}

// ============================================================
//  Вспомогательные методы
// ============================================================

QRectF TriangulationWidget::getBoundingRect(const std::vector<QPointF>& points) const {
    if (points.empty()) return QRectF(0, 0, width(), height());
    qreal minX = points[0].x(), maxX = points[0].x();
    qreal minY = points[0].y(), maxY = points[0].y();
    for (const auto& p : points) {
        minX = std::min(minX, p.x());
        maxX = std::max(maxX, p.x());
        minY = std::min(minY, p.y());
        maxY = std::max(maxY, p.y());
    }
    qreal margin = 50;
    return QRectF(minX - margin, minY - margin,
                  maxX - minX + 2*margin, maxY - minY + 2*margin);
}

QPointF TriangulationWidget::mapToScene(const QPoint& screenPos) const {
    // Пока просто инвертируем Y (так как в трансформации мы инвертируем Y)
    // В будущем нужно будет учитывать масштаб и смещение.
    return QPointF(screenPos.x(), height() - screenPos.y());
}

void TriangulationWidget::updateDebugData() {
    m_debugPolygon = m_facade->getDebugPolygon();
    m_debugTriangles = m_facade->getDebugTriangles();
    m_currentStep = m_facade->getCurrentStep();
    m_totalSteps = m_facade->getTotalSteps();
}