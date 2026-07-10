#pragma once
#include <QObject>
#include <QPointF>
#include <memory>
#include <vector>
#include <array>

#include "geometry/types.h"
#include "geometry/ear_clipping.h"

// Forward declaration для DebugFacade (чтобы не тянуть заголовок везде)
namespace facade {
class DebugFacade;
}

namespace facade {

/**
 * @brief Главный фасад для управления триангуляцией.
 *
 * Отвечает за:
 * - Хранение и модификацию полигона
 * - Вызов алгоритмов триангуляции
 * - Управление дебаггером (пошаговый просмотр)
 * - Эмит сигналов для обновления UI
 */
class TriangulationFacade : public QObject {
    Q_OBJECT

public:
    explicit TriangulationFacade(QObject* parent = nullptr);
    ~TriangulationFacade() override;

    // ==================== Управление полигоном ====================
    void setPolygon(const geo::Polygon2<double>& polygon);
    void addPoint(double x, double y);
    void addPoint(const QPointF& point);
    void removeLastPoint();
    void clearPolygon();
    void closePolygon(); // Замыкает и запускает триангуляцию

    // ==================== Триангуляция ====================
    bool triangulateEarClipping(); // Явный вызов (если нужен)
    // bool triangulateMonotone(); // на будущее

    // ==================== Получение данных ====================
    std::vector<QPointF> getPolygonPoints() const;
    std::vector<std::array<QPointF, 3>> getTriangles() const;

    bool isPolygonClosed() const { return m_isClosed; }
    bool hasTriangulation() const { return !m_triangles.empty(); }
    size_t getVertexCount() const { return m_polygon.size(); }
    size_t getTriangleCount() const { return m_triangles.size(); }

    // ==================== Дебаггер (делегируется DebugFacade) ====================
    bool stepForward();
    bool stepBackward();
    void goToStep(int step);
    int getCurrentStep() const;
    int getTotalSteps() const;

    // Данные для отрисовки дебаг-состояния
    std::vector<QPointF> getDebugPolygon() const;
    std::vector<std::array<QPointF, 3>> getDebugTriangles() const;
    QString getDebugDescription() const;
    size_t getDebugEarIndex() const;

signals:
    // Сигналы для UI
    void polygonChanged(const std::vector<QPointF>& points);
    void triangulationCompleted(const std::vector<std::array<QPointF, 3>>& triangles);
    void debugStepChanged(int currentStep, int totalSteps);
    void errorOccurred(const QString& message);
    void statusChanged(const QString& message);

private:
    // Данные
    geo::Polygon2<double> m_polygon;
    std::vector<geo::Triangle2<double>> m_triangles;
    bool m_isClosed = false;

    // Алгоритмы
    std::unique_ptr<geo::EarClipping<double>> m_earClipping;

    // Дебаггер (отдельный фасад)
    std::unique_ptr<DebugFacade> m_debugFacade;

    // Вспомогательные методы
    void emitPolygonUpdate();
    void emitTriangulationUpdate();

    // Преобразования типов
    static QPointF toQPoint(const geo::Point2<double>& p);
    static geo::Point2<double> toGeoPoint(const QPointF& p);
    static std::array<QPointF, 3> toQTriangle(const geo::Triangle2<double>& t);
};

} // namespace facade