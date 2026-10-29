#pragma once
#include <QObject>
#include <QString>
#include <optional>
#include <vector>
#include "geometry/types.h"
#include "geometry/ear_clipping.h"

namespace facade {

/**
 * @brief Фасад для управления пошаговой отладкой триангуляции.
 *
 * Хранит историю шагов алгоритма и позволяет перемещаться по ним.
 * Текущий шаг -1 означает начальное состояние (полигон до отрезания).
 */
class DebugFacade : public QObject {
    Q_OBJECT

public:
    explicit DebugFacade(QObject* parent = nullptr);
    ~DebugFacade() override = default;

    // Управление историей
    // vertices — вершины запуска, на которые ссылаются индексы шагов
    // (Этап 1 Roadmap: DebugStep хранит индексы, конвертация в координаты
    // происходит здесь — в точке экспорта для визуализатора).
    void setHistory(const geo::Polygon2<double>& vertices,
                    const std::vector<geo::DebugStep<double>>& history);
    void clear();

    // Навигация
    bool stepForward();
    bool stepBackward();
    void goToStep(int step);

    // Получение состояния
    int getCurrentStep() const { return m_currentStep; }
    int getTotalSteps() const { return static_cast<int>(m_history.size()); }
    bool hasHistory() const { return !m_history.empty(); }

    // Данные текущего шага
    geo::Polygon2<double> getCurrentPolygon() const;
    std::vector<geo::Triangle2<double>> getAccumulatedTriangles() const;
    std::vector<std::pair<geo::Point2<double>, geo::Point2<double>>> getAddedDiagonals() const;
    std::vector<size_t> getHighlightedVertices() const;
    std::optional<double> getSweepY() const;
    QString getStepDescription() const;

signals:
    void stepChanged(int currentStep, int totalSteps);
    void errorOccurred(const QString& message);

private:
    geo::Polygon2<double> m_vertices; // вершины запуска для индексов шагов
    std::vector<geo::DebugStep<double>> m_history;
    int m_currentStep = -1; // -1 = начальное состояние (до первого отрезания)
};



} // namespace facade
