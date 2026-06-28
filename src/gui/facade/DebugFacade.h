#pragma once
#include <QObject>
#include <QString>
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
    void setHistory(const std::vector<geo::DebugStep<double>>& history);
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
    std::vector<geo::Triangle2<double>> getCutTriangles() const;
    geo::Triangle2<double> getLastCutTriangle() const;
    size_t getEarIndex() const;
    QString getStepDescription() const;

signals:
    void stepChanged(int currentStep, int totalSteps);
    void errorOccurred(const QString& message);

private:
    std::vector<geo::DebugStep<double>> m_history;
    int m_currentStep = -1; // -1 = начальное состояние (до первого отрезания)
};

} // namespace facade