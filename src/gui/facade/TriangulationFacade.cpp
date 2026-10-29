#include "TriangulationFacade.h"
#include "DebugFacade.h"
#include <QDebug>

#include "triangulation_error_text.h"
#include "algorithm_text.h"

namespace facade {

// ============================================================
//  Конструктор / Деструктор
// ============================================================

TriangulationFacade::TriangulationFacade(QObject *parent)
    : QObject(parent),
      m_earClipping(std::make_unique<geo::EarClipping<double>>()),
      m_monotone(std::make_unique<geo::MonotoneTriangulation<double>>()),
      m_delaunayFlip(std::make_unique<geo::DelaunayFlip<double>>()),
      m_debugFacade(std::make_unique<DebugFacade>(this)) {
  // Подключаем сигналы от DebugFacade, чтобы пробросить их наружу
  connect(m_debugFacade.get(), &DebugFacade::stepChanged, this,
          &TriangulationFacade::debugStepChanged);
  connect(m_debugFacade.get(), &DebugFacade::errorOccurred, this,
          &TriangulationFacade::errorOccurred);
}

TriangulationFacade::~TriangulationFacade() = default;

void TriangulationFacade::setAlgorithm(geo::TriangulationAlgorithm algo) {
    m_currentAlgorithm = algo;
    clearTriangulation();
    emit statusChanged(toString(algo));
}

// ============================================================
//  Управление полигоном
// ============================================================

void TriangulationFacade::addPoint(double x, double y) {
  addPoint(QPointF(x, y));
}

void TriangulationFacade::addPoint(const QPointF &point) {
  if (m_isClosed) {
    clearPolygon(); // Автоочистка при новом полигоне
  }

  m_polygon.push_back(toGeoPoint(point));
  emitPolygonUpdate();
  emit statusChanged(QString("Added point (%1, %2), total: %3")
                         .arg(point.x())
                         .arg(point.y())
                         .arg(m_polygon.size()));
}

void TriangulationFacade::removeLastPoint() {
  if (!m_polygon.empty() && !m_isClosed) {
    m_polygon.pop_back();
    emitPolygonUpdate();
    emit statusChanged(
        QString("Removed last point, remaining: %1").arg(m_polygon.size()));
  }
}

void TriangulationFacade::clearPolygon() {
  m_polygon.clear();
  m_triangles.clear();
  m_isClosed = false;
  m_debugFacade->clear();
  emitPolygonUpdate();
  emit triangulationCompleted({});
  emit statusChanged("Polygon cleared");
}

void TriangulationFacade::closePolygon() {
  if (m_polygon.size() < 3) {
    emit errorOccurred(gui::errorText(geo::ErrorCode::NotEnoughVertices));
    return;
  }

  //Приводим к CCW
  geo::ensureCCW(m_polygon);

  //Удаляем коллинеарные точки (критично!)
  geo::removeCollinearPoints(m_polygon);

  if (m_polygon.size() < 3) {
    emit errorOccurred(gui::errorText(geo::ErrorCode::DegeneratePolygon));
    return;
  }

  m_isClosed = true;
  emit statusChanged(
      QString("Polygon closed with %1 vertices").arg(m_polygon.size()));
  emitPolygonUpdate();

  triangulate();
}

// ============================================================
//  Триангуляция
// ============================================================

bool TriangulationFacade::triangulate() {
    if (!m_isClosed || m_polygon.size() < 3) {
        emit errorOccurred(gui::errorText(geo::ErrorCode::NotEnoughVertices));
        return false;
    }

    // Очищаем старые данные
    m_triangles.clear();

    // Запускаем алгоритм
    geo::TriangulationResult<double> result;
    switch (m_currentAlgorithm)
    {
    case geo::TriangulationAlgorithm::EarClipping:
        result = m_earClipping->triangulate(m_polygon);
        break;
    case geo::TriangulationAlgorithm::Monotone:
        result = m_monotone->triangulate(m_polygon);
        break;
    case geo::TriangulationAlgorithm::DelaunayFlip:
        result = m_delaunayFlip->triangulate(m_polygon);
        break;
    }

    if (bool(result)) {
        // Единственная точка конвертации «индексы → координаты» для результата
        // (Этап 1 Roadmap: визуализатор — легальный экспортный слой).
        m_triangles.reserve(result.faces.size());
        for (const auto& f : result.faces) {
            m_triangles.emplace_back(
                result.vertices[f[0]],
                result.vertices[f[1]],
                result.vertices[f[2]]);
        }

        // Передаём историю в DebugFacade вместе с вершинами запуска,
        // на которые ссылаются индексы шагов
        std::vector<geo::DebugStep<double>> history;
        switch (m_currentAlgorithm)
        {
        case geo::TriangulationAlgorithm::EarClipping:
            history = m_earClipping->getDebugHistory();
            break;
        case geo::TriangulationAlgorithm::Monotone:
            history = m_monotone->getDebugHistory();
            break;
        case geo::TriangulationAlgorithm::DelaunayFlip:
            history = m_delaunayFlip->getDebugHistory();
            break;
        }

        m_debugFacade->setHistory(result.vertices, history);


        emitTriangulationUpdate();
        emit statusChanged(QString("Triangulation complete: %1 triangles")
                               .arg(m_triangles.size()));
        emit debugStepChanged(getCurrentStep(), getTotalSteps());
        return true;
    } else {
        emit errorOccurred(gui::errorText(result.error->code));
        return false;
    }
}

void TriangulationFacade::clearTriangulation()
{
    m_triangles.clear();
    m_isClosed = false;

    if (m_debugFacade) {
        m_debugFacade->clear();
    }

    emitTriangulationUpdate();
    emit debugStepChanged(getCurrentStep(), getTotalSteps());
}

// ============================================================
//  Дебаггер (делегирование)
// ============================================================

bool TriangulationFacade::stepForward() {
  if (!m_debugFacade->stepForward()) {
    return false;
  }
  emit debugStepChanged(getCurrentStep(), getTotalSteps());
  return true;
}

bool TriangulationFacade::stepBackward() {
  if (!m_debugFacade->stepBackward()) {
    return false;
  }
  emit debugStepChanged(getCurrentStep(), getTotalSteps());
  return true;
}

void TriangulationFacade::goToStep(int step) {
  m_debugFacade->goToStep(step);
  emit debugStepChanged(getCurrentStep(), getTotalSteps());
}

int TriangulationFacade::getCurrentStep() const {
  return m_debugFacade->getCurrentStep();
}

int TriangulationFacade::getTotalSteps() const {
  return m_debugFacade->getTotalSteps();
}

std::vector<QPointF> TriangulationFacade::getDebugPolygon() const {
  auto poly = m_debugFacade->getCurrentPolygon();
  std::vector<QPointF> result;
  result.reserve(poly.size());
  for (const auto &p : poly) {
    result.push_back(toQPoint(p));
  }
  return result;
}

std::vector<std::array<QPointF, 3>>
TriangulationFacade::getDebugTriangles() const {
  auto tris = m_debugFacade->getAccumulatedTriangles();
  std::vector<std::array<QPointF, 3>> result;
  result.reserve(tris.size());
  for (const auto &t : tris) {
    result.push_back(toQTriangle(t));
  }
  return result;
}

QString TriangulationFacade::getDebugDescription() const {
  return m_debugFacade->getStepDescription();
}

std::vector<size_t> TriangulationFacade::getDebugHighlightedVertices() const {
    return m_debugFacade->getHighlightedVertices();
}

std::vector<QLineF> TriangulationFacade::getDebugDiagonals() const {
    auto diagonals = m_debugFacade->getAddedDiagonals();

    std::vector<QLineF> result;
    result.reserve(diagonals.size());

    for (const auto& d : diagonals) {
        result.emplace_back(toQPoint(d.first), toQPoint(d.second));
    }

    return result;
}

std::optional<double> TriangulationFacade::getDebugSweepY() const {
    return m_debugFacade->getSweepY();
}

// ============================================================
//  Получение данных
// ============================================================

std::vector<QPointF> TriangulationFacade::getPolygonPoints() const {
  std::vector<QPointF> result;
  result.reserve(m_polygon.size());
  for (const auto &p : m_polygon) {
    result.push_back(toQPoint(p));
  }
  return result;
}

std::vector<std::array<QPointF, 3>> TriangulationFacade::getTriangles() const {
  std::vector<std::array<QPointF, 3>> result;
  result.reserve(m_triangles.size());
  for (const auto &t : m_triangles) {
    result.push_back(toQTriangle(t));
  }
  return result;
}

// ============================================================
//  Вспомогательные приватные методы
// ============================================================

void TriangulationFacade::emitPolygonUpdate() {
  auto points = getPolygonPoints();
  emit polygonChanged(points);
}

void TriangulationFacade::emitTriangulationUpdate() {
  auto triangles = getTriangles();
  emit triangulationCompleted(triangles);
}

// ============================================================
//  Преобразования типов
// ============================================================

QPointF TriangulationFacade::toQPoint(const geo::Point2<double> &p) {
  return QPointF(static_cast<qreal>(p.x), static_cast<qreal>(p.y));
}

geo::Point2<double> TriangulationFacade::toGeoPoint(const QPointF &p) {
  return geo::Point2<double>(p.x(), p.y());
}

std::array<QPointF, 3>
TriangulationFacade::toQTriangle(const geo::Triangle2<double> &t) {
  return {toQPoint(t.v0), toQPoint(t.v1), toQPoint(t.v2)};
}

} // namespace facade
