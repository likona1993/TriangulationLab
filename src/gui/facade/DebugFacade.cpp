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
            return m_history[0].polygon_before;
        }
        return {};
    }

    if (m_currentStep < getTotalSteps()) {
        return m_history[m_currentStep].polygon_before;
    }
    return {};
}

std::vector<geo::Triangle2<double>> DebugFacade::getAccumulatedTriangles() const {
    std::vector<geo::Triangle2<double>> result;
    for (int i = 0; i <= m_currentStep && i < getTotalSteps(); ++i) {
        const auto& step = m_history[i];
        if (step.replaces_state) {
            // Шаг со снимком полного состояния: всё накопленное до него
            // больше не актуально (Delaunay flip после ear-clipping префикса).
            result = step.added_triangles;
        } else {
            result.insert(result.end(), step.added_triangles.begin(), step.added_triangles.end());
        }
    }
    return result;
}

std::vector<geo::Triangle2<double>> DebugFacade::getLastCutTriangles() const {
    if (m_currentStep >= 0 && m_currentStep < getTotalSteps()) {
        return m_history[m_currentStep].added_triangles;
    }
    return {};
}

size_t DebugFacade::getHighlightedVertexIndex() const {
    auto removed = getRemovedVertices();
    return removed.empty() ? 0 : removed.front();
}

std::vector<std::pair<geo::Point2<double>, geo::Point2<double> > > DebugFacade::getAddedDiagonals() const
{
    if (m_currentStep >= 0 && m_currentStep < getTotalSteps()) {
        return m_history[m_currentStep].added_diagonals;
    }
    return {};
}

std::vector<size_t> DebugFacade::getRemovedVertices() const
{
    if (m_currentStep >= 0 && m_currentStep < getTotalSteps()) {
        return m_history[m_currentStep].highlighted_vertices;
    }
    return {};
}

std::optional<double> DebugFacade::getSweepY() const
{
    if (m_currentStep >= 0 && m_currentStep < getTotalSteps()) {
        return m_history[m_currentStep].sweep_y;
    }
    return std::nullopt;
}

QString DebugFacade::getStepDescription() const {
    if (m_currentStep == -1) {
        return "Initial polygon";
    }
    if (m_currentStep < getTotalSteps()) {
        const auto& desc = m_history[m_currentStep].description;
        if (!desc.empty()) {
            return QString("Step %1: %2")
            .arg(m_currentStep + 1)
                .arg(QString::fromStdString(desc));
        }
        return QString("Step %1").arg(m_currentStep + 1);
    }
    return "Done";
}

} // namespace facade
