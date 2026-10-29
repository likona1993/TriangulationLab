#include "DebugFacade.h"

namespace facade {

DebugFacade::DebugFacade(QObject* parent)
    : QObject(parent)
{
}

void DebugFacade::setHistory(const geo::Polygon2<double>& vertices,
                             const std::vector<geo::DebugStep<double>>& history) {
    m_vertices = vertices;
    m_history = history;
    m_currentStep = -1; // Начинаем с начального состояния
    emit stepChanged(m_currentStep, getTotalSteps());
}

void DebugFacade::clear() {
    m_vertices.clear();
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
    const geo::DebugStep<double>* step = nullptr;
    if (m_currentStep == -1) {
        // Начальное состояние: полигон из первого шага (до отрезания)
        if (!m_history.empty()) {
            step = &m_history[0];
        }
    } else if (m_currentStep < getTotalSteps()) {
        step = &m_history[m_currentStep];
    }

    if (!step) {
        return {};
    }

    // Пустой polygon_before = «все вершины запуска» (см. geo::DebugStep)
    if (step->polygon_before.empty()) {
        return m_vertices;
    }

    geo::Polygon2<double> polygon;
    polygon.reserve(step->polygon_before.size());
    for (geo::VertexIndex idx : step->polygon_before) {
        polygon.push_back(m_vertices[idx]);
    }
    return polygon;
}

std::vector<geo::Triangle2<double>> DebugFacade::getAccumulatedTriangles() const {
    std::vector<geo::Triangle2<double>> result;
    for (int i = 0; i <= m_currentStep && i < getTotalSteps(); ++i) {
        const auto& step = m_history[i];
        if (step.replaces_state) {
            // Шаг со снимком полного состояния: всё накопленное до него
            // больше не актуально (Delaunay flip после ear-clipping префикса).
            result.clear();
        }
        result.reserve(result.size() + step.added_faces.size());
        for (const auto& f : step.added_faces) {
            result.emplace_back(m_vertices[f[0]], m_vertices[f[1]], m_vertices[f[2]]);
        }
    }
    return result;
}

std::vector<std::pair<geo::Point2<double>, geo::Point2<double> > > DebugFacade::getAddedDiagonals() const
{
    if (m_currentStep >= 0 && m_currentStep < getTotalSteps()) {
        std::vector<std::pair<geo::Point2<double>, geo::Point2<double>>> result;
        const auto& diagonals = m_history[m_currentStep].added_diagonals;
        result.reserve(diagonals.size());
        for (const auto& d : diagonals) {
            result.emplace_back(m_vertices[d.first], m_vertices[d.second]);
        }
        return result;
    }
    return {};
}

std::vector<size_t> DebugFacade::getHighlightedVertices() const
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
