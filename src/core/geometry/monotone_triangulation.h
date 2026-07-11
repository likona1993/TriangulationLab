#pragma once

#include <map>
#include <set>
#include <vector>

#include "triangulation_types.h"
#include "types.h"

namespace geo {

// Статус активных рёбер
template <typename T> struct EdgeCmp {
  const Polygon2<T> *poly; // указатель на полигон
  T *currentY;             // текущая y-координата sweep line

  EdgeCmp(const Polygon2<T> *p, T *y) : poly(p), currentY(y) {}

  bool operator()(const Edge &a, const Edge &b) const;
};

template <typename T> class MonotoneTriangulation {
public:
  using Polygon = Polygon2<T>;
  using Triangles = std::vector<Triangle2<T>>;
  // Тип для индекса вершины
  using VertexIndex = size_t;
  // Для хранения статуса используем set с компаратором, зависящим от currentY
  using StatusSet = std::set<Edge, EdgeCmp<T>>;

  // Словарь helper: для каждого ребра храним индекс вершины-помощника
  using HelperMap = std::map<Edge, VertexIndex, EdgeLess>;

  TriangulationResult<T> triangulate(const Polygon &polygon);

  const std::vector<DebugStep<T>> &getHistory() const { return history; }

private:
  // Вспомогательные функции
  enum VertexType { START, END, SPLIT, MERGE, REGULAR };

  // Вспомогательные методы
  VertexType classifyVertex(const Polygon &poly, size_t i) const;

  // Найти ребро в статусе, расположенное непосредственно слева от вершины с
  // индексом vi
  typename StatusSet::iterator findLeftEdge(const Polygon &poly,
                                            StatusSet &status, VertexIndex vi,
                                            T currentY);

  // Вспомогательная функция для получения ребра, выходящего из вершины vi (vi
  // -> vi+1)
  Edge getNextEdge(const Polygon &poly, VertexIndex vi);

  // Вспомогательная функция для получения ребра, входящего в вершину vi (vi-1
  // -> vi)
  Edge getPrevEdge(const Polygon &poly, VertexIndex vi);

  using DiagonalList = std::vector<std::pair<Point2<T>, Point2<T>>>;

  void handleStart(VertexIndex vi, Polygon &poly, StatusSet &status,
                   HelperMap &helpers, DiagonalList &diagonals);

  void handleEnd(VertexIndex vi, Polygon &poly, StatusSet &status,
                 HelperMap &helpers, DiagonalList &diagonals);

  void handleSplit(VertexIndex vi, Polygon &poly, StatusSet &status,
                   HelperMap &helpers, DiagonalList &diagonals);

  void handleMerge(VertexIndex vi, Polygon &poly, StatusSet &status,
                   HelperMap &helpers, DiagonalList &diagonals);

  void handleRegular(VertexIndex vi, Polygon &poly, StatusSet &status,
                     HelperMap &helpers, DiagonalList &diagonals);

  std::vector<DebugStep<T>> history;
};
} // namespace geo

// ============================================================
//  Подключение реализаций (шаблонных)
// ============================================================
#include "monotone_triangulation.hpp"