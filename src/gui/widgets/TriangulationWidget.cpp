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
    // Начинаем новый кадр QPainter.
    // После beginFrame() renderer готов принимать команды рисования.
    m_renderer->beginFrame();

    // Применяем мировую трансформацию:
    // масштаб, сдвиг и инверсию оси Y.
    m_renderer->applyTransform();

    // Рисуем сетку в мировых координатах.
    // Пока это скорее вспомогательный визуальный фон.
    m_renderer->drawGrid(m_currentBBox);

    // В обычном режиме показываем итоговую триангуляцию целиком.
    if (!m_showDebug && !m_triangles.empty()) {
        m_renderer->drawTriangles(m_triangles, QColor(0, 200, 100), 0.3f, true);
    }

    // В debug-режиме показываем треугольники, накопленные к текущему шагу.
    if (m_showDebug && !m_debugTriangles.empty()) {
        m_renderer->drawTriangles(m_debugTriangles, QColor(0, 200, 255), 0.2f, true);
    }

    // В debug-режиме дополнительно показываем диагонали,
    // которые были добавлены на текущем шаге.
    if (m_showDebug && !m_debugDiagonals.empty()) {
        m_renderer->drawDiagonals(
            m_debugDiagonals,
            QColor(255, 120, 0),
            2.0f,
            Qt::DashLine
            );
    }

    // Основной пользовательский полигон рисуем всегда,
    // чтобы не терять исходный контекст.
    if (!m_polygonPoints.empty()) {
        QColor polyColor = m_facade->isPolygonClosed() ? Qt::green : Qt::black;
        m_renderer->drawPolygon(m_polygonPoints, polyColor, 2.0f, false);
    }

    // В debug-режиме поверх основного полигона рисуем полигон текущего шага.
    // Для ear clipping это может быть "остаток",
    // для monotone triangulation — текущий monotone-piece.
    if (m_showDebug && !m_debugPolygon.empty()) {
        m_renderer->drawPolygon(m_debugPolygon, QColor(255, 200, 50), 2.0f, false);

        // Подсвечиваем вершины, которые текущий шаг считает важными.
        // Сейчас это универсальный механизм вместо ear-specific подсветки.
        const auto highlighted = m_facade->getDebugHighlightedVertices();
        for (size_t idx : highlighted) {
            if (idx < m_debugPolygon.size()) {
                m_renderer->drawVertexHighlight(m_debugPolygon[idx], Qt::red);
            }
        }

        // Нумерация вершин debug-полигона удобна для проверки индексов.
        m_renderer->drawVertexLabels(m_debugPolygon, Qt::yellow);
    } else {
        // Если debug-режим выключен, подписываем вершины исходного полигона.
        m_renderer->drawVertexLabels(m_polygonPoints, Qt::black);
    }

    // Информационную строку рисуем в экранных координатах,
    // чтобы текст не масштабировался вместе со сценой.
    if (m_showDebug && m_totalSteps > 0) {
        QString info = QString("Step %1 / %2: %3")
        .arg(m_currentStep + 1)
        .arg(m_totalSteps)
        .arg(m_facade->getDebugDescription());

        m_renderer->drawTextScreen(info, QPointF(10, 30));
    }

    // Завершаем кадр.
    m_renderer->endFrame();
}

// ============================================================
//  Обработка событий
// ============================================================

void TriangulationWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QPointF worldPoint = m_renderer->screenToWorld(event->pos());
        m_facade->addPoint(worldPoint);
    }
}

void TriangulationWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        updateWorldBounds();
        m_facade->closePolygon();
        break;

    case Qt::Key_Backspace:
        m_facade->removeLastPoint();
        break;

    case Qt::Key_C:
        m_facade->clearPolygon();
        m_showDebug = false;
        update();
        break;

    case Qt::Key_D:
        toggleDebugMode();
        break;

    case Qt::Key_Right:
        if (m_showDebug && m_facade->stepForward()) {
            refreshDebugView();
        }
        break;

    case Qt::Key_Left:
        if (m_showDebug && m_facade->stepBackward()) {
            refreshDebugView();
        }
        break;

    default:
        QWidget::keyPressEvent(event);
        break;
    }
}

void TriangulationWidget::resizeEvent(QResizeEvent * event)
{
    // 1. Всегда вызываем базовый класс, чтобы Qt корректно обработал событие
    QWidget::resizeEvent(event);

    // 2. Если у нас еще нет точек (полигон пустой), центрировать нечего
    if (m_polygonPoints.empty()) return;

    // 3. Пересчитываем масштаб и сдвиг под новые размеры виджета.
    // Внутри updateWorldBounds() вызовется m_renderer->resetView(bbox, rect())
    updateWorldBounds();

    // 4. Запрашиваем принудительную перерисовку виджета
    update();
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

void TriangulationWidget::updateWorldBounds()
{
    // Собираем все точки, которые сейчас могут участвовать в отображении.
    // По ним будет вычислен bounding box для камеры.
    std::vector<QPointF> allPoints = m_polygonPoints;

    // В debug-режиме добавляем точки debug-полигона,
    // чтобы он тоже гарантированно попадал в видимую область.
    if (m_showDebug && !m_debugPolygon.empty()) {
        allPoints.insert(allPoints.end(), m_debugPolygon.begin(), m_debugPolygon.end());
    }

    // Добавляем вершины финальных треугольников.
    // Это важно, если триангуляция выходит за текущий bbox исходного полигона
    // или если мы хотим надёжно учитывать всё, что рисуется на сцене.
    if (!m_triangles.empty()) {
        for (const auto& tri : m_triangles) {
            allPoints.push_back(tri[0]);
            allPoints.push_back(tri[1]);
            allPoints.push_back(tri[2]);
        }
    }

    // В debug-режиме добавляем и вершины debug-треугольников,
    // чтобы камера учитывала текущее промежуточное состояние.
    if (!m_debugTriangles.empty()) {
        for (const auto& tri : m_debugTriangles) {
            allPoints.push_back(tri[0]);
            allPoints.push_back(tri[1]);
            allPoints.push_back(tri[2]);
        }
    }

    // Вычисляем ограничивающий прямоугольник всех собранных точек.
    m_currentBBox = getBoundingRect(allPoints);

    // Защитный случай: если bbox по какой-то причине пуст,
    // используем прямоугольник размеров виджета.
    if (m_currentBBox.isEmpty()) {
        m_currentBBox = QRectF(0, 0, width(), height());
    }

    // Передаём renderer новый worldRect и текущий viewport,
    // чтобы он пересчитал масштаб и смещение камеры.
    QRect viewport(0, 0, width(), height());
    m_renderer->resetView(m_currentBBox, viewport);
}

QRectF TriangulationWidget::getBoundingRect(const std::vector<QPointF> &points) const
{
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

void TriangulationWidget::updateDebugData() {
    // Забираем из фасада полигон текущего debug-шага.
    m_debugPolygon = m_facade->getDebugPolygon();

    // Забираем все треугольники, накопленные к текущему шагу.
    m_debugTriangles = m_facade->getDebugTriangles();

    // Забираем диагонали, добавленные на текущем debug-шаге.
    m_debugDiagonals = m_facade->getDebugDiagonals();

    // Синхронизируем локальные счётчики шага с фасадом.
    m_currentStep = m_facade->getCurrentStep();
    m_totalSteps = m_facade->getTotalSteps();
}

void TriangulationWidget::refreshDebugView() {
    updateDebugData();
    updateWorldBounds();
    update();
}

void TriangulationWidget::enterDebugModeFromStart() {
    m_showDebug = true;
    if (m_totalSteps > 0) {
        m_facade->goToStep(-1);
    }
    refreshDebugView();
}

void TriangulationWidget::toggleDebugMode() {
    m_showDebug = !m_showDebug;
    if (m_showDebug) {
        enterDebugModeFromStart();
    } else {
        update();
    }
}
