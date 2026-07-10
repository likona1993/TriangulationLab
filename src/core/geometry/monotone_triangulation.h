#pragma once

#include <map>
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
  using HelperMap = std::map<Edge, VertexIndex, EdgeCmp<T>>;

  TriangulationResult<T> triangulate(const Polygon &polygon);

  const std::vector<DebugStep> &getHistory() const { return history; }

private:
  // Вспомогательные функции
  enum VertexType { START, END, SPLIT, MERGE, REGULAR };

  // Вспомогательные методы
  VertexType classifyVertex(const Polygon &poly, size_t i) const;

  // Найти ребро в статусе, расположенное непосредственно слева от вершины с
  // индексом vi
  typename StatusSet::iterator findLeftEdge(StatusSet &status, VertexIndex vi,
                                            T currentY) {

    if (status.empty())
      return status.end();

    const Point2<T> &v = (*status.key_comp().poly)[vi];
    T xV = v.x; // x-координата вершины

    // Простой и надёжный способ – линейный поиск, но статус может содержать
    // O(n) рёбер, и это даст O(n^2) в худшем случае. Для начала можно так, но
    // потом оптимизировать. Предлагаю пока использовать линейный поиск, а позже
    // заменить на сбалансированное дерево с возможностью поиска по x. TODO

    auto it = status.begin();
    auto best = status.end();
    T bestX = -std::numeric_limits<T>::infinity();

    /*while (it != status.end()) {
      // Вычисляем x пересечения ребра с текущей горизонталью
      T xEdge =
          computeXIntersection((*poly)[it->from], (*poly)[it->to], currentY);
      if (xEdge < xV - EPSILON<T>() && xEdge > bestX) {
        bestX = xEdge;
        best = it;
      }
      ++it;
    }*/
    return best;
  }

  // Вспомогательная функция для получения ребра, выходящего из вершины vi (vi
  // -> vi+1)
  Edge getNextEdge(const Polygon &poly, VertexIndex vi);

  // Вспомогательная функция для получения ребра, входящего в вершину vi (vi-1
  // -> vi)
  Edge getPrevEdge(const Polygon &poly, VertexIndex vi);

  void handleStart(VertexIndex vi, Polygon &poly, StatusSet &status,
                   HelperMap &helpers);

  void handleEnd(VertexIndex vi, Polygon &poly, StatusSet &status,
                 HelperMap &helpers);

  void handleSplit(VertexIndex vi, Polygon &poly, StatusSet &status,
                   HelperMap &helpers);

  void handleMerge(VertexIndex vi, Polygon &poly, StatusSet &status,
                   HelperMap &helpers);

  void handleRegular(VertexIndex vi, Polygon &poly, StatusSet &status,
                     HelperMap &helpers);

  std::vector<DebugStep> history;
};
} // namespace geo