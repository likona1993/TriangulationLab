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

  // Диагональ хранится как пара индексов вершин исходного полигона.
  using Diagonal = std::pair<VertexIndex, VertexIndex>;
  using DiagonalList = std::vector<Diagonal>;
  using VertexTypes = std::vector<VertexType>;

  // Общий контекст sweep-обхода, чтобы не таскать по пять указателей в
  // каждый handle*.
  struct SweepCtx {
    const Polygon *poly;      // геометрия
    const VertexTypes *types; // типы вершин (для проверки "helper — merge?")
    StatusSet *status;        // активные нисходящие рёбра
    HelperMap *helpers;       // helper для каждого активного ребра
    DiagonalList *diagonals;  // накопитель результата
    T *currentY;              // sweep line; на неё же смотрит EdgeCmp
  };

  // Погасить долг: если helper ребра e — merge-вершина, провести к ней
  // диагональ.
  // Вызывается ВЕЗДЕ, где мы «трогаем» helper.
  void tryCloseMerge(VertexIndex vi, const Edge &e, SweepCtx &ctx);

  // Своё нисходящее ребро закончилось в vi: погасить долг и убрать из статуса.
  void closeEdge(VertexIndex vi, SweepCtx &ctx);

  // Своё нисходящее ребро начинается в vi: вставить, helper = vi.
  void openEdge(VertexIndex vi, SweepCtx &ctx);

  // Обновить helper ЧУЖОГО ребра слева: погасить его долг и стать новым
  // helper'ом. Возвращает false, если слева ничего нет (вырожденные данные).
  bool updateLeftHelper(VertexIndex vi, SweepCtx &ctx);

  bool handleStart(VertexIndex vi, SweepCtx &ctx);
  bool handleEnd(VertexIndex vi, SweepCtx &ctx);
  bool handleSplit(VertexIndex vi, SweepCtx &ctx);
  bool handleMerge(VertexIndex vi, SweepCtx &ctx);
  bool handleRegular(VertexIndex vi, SweepCtx &ctx);

  bool makeMonotone(const Polygon &poly, const std::vector<VertexType> &types,
                    std::vector<Diagonal> &diagonals);

  std::vector<DebugStep<T>> history;
};
} // namespace geo

// ============================================================
//  Подключение реализаций (шаблонных)
// ============================================================
#include "monotone_triangulation.hpp"