#include "DebugFacade.h"

namespace facade {

DebugFacade::DebugFacade(QObject* parent)
    : QObject(parent)
{
}

void DebugFacade::setHistory(const std::vector<geo::DebugStep<double>>& history) {
    m_history = history;
    m_currentStep = -1; // Начинаем с начального состояния
    emit stepChanged(m_currentStep, getTotalSteps());
}

void DebugFacade::clear() {
    m_history.clear();
    m_currentStep = -1;
    emit stepChanged(m_currentStep, 0);
}

bool DebugFacade::stepForward() {
    if (m_currentStep + 1 < getTotalSteps()) {
        ++m_currentStep;
        emit stepChanged(m_currentStep, getTotalSteps());
        return true;
    }
    return false;
}

bool DebugFacade::stepBackward() {
    if (m_currentStep > -1) {
        --m_currentStep;
        emit stepChanged(m_currentStep, getTotalSteps());
        return true;
    }
    return false;
}

void DebugFacade::goToStep(int step) {
    if (step >= -1 && step < getTotalSteps()) {
        m_currentStep = step;
        emit stepChanged(m_currentStep, getTotalSteps());
    }
}

geo::Polygon2<double> DebugFacade::getCurrentPolygon() const {
    if (m_currentStep == -1) {
        // Начальное состояние: возвращаем полигон из первого шага (до отрезания)
        if (!m_history.empty()) {
            return m_history[0].remaining_vertices;
        }
        return {};
    }

    if (m_currentStep < getTotalSteps()) {
        return m_history[m_currentStep].remaining_vertices;
    }
    return {};
}

std::vector<geo::Triangle2<double>> DebugFacade::getCutTriangles() const {
    std::vector<geo::Triangle2<double>> result;
    for (int i = 0; i <= m_currentStep && i < getTotalSteps(); ++i) {
        result.push_back(m_history[i].cut_triangle);
    }
    return result;
}

geo::Triangle2<double> DebugFacade::getLastCutTriangle() const {
    if (m_currentStep >= 0 && m_currentStep < getTotalSteps()) {
        return m_history[m_currentStep].cut_triangle;
    }
    return {};
}

size_t DebugFacade::getEarIndex() const {
    if (m_currentStep >= 0 && m_currentStep < getTotalSteps()) {
        return m_history[m_currentStep].ear_index;
    }
    return 0;
}

QString DebugFacade::getStepDescription() const {
    if (m_currentStep == -1) {
        return "Initial polygon";
    }
    if (m_currentStep < getTotalSteps()) {
        return QString("Step %1: Cut ear at vertex %2")
               .arg(m_currentStep + 1)
               .arg(m_history[m_currentStep].ear_index);
    }
    return "Done";
}

} // namespace facade