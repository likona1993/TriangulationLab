#pragma once

#include <QString>

#include "geometry/triangulation_algorithm.h"

inline QString toString(geo::TriangulationAlgorithm algo) {
    switch (algo) {
    case geo::TriangulationAlgorithm::EarClipping:
        return QStringLiteral("Ear Clipping");
    case geo::TriangulationAlgorithm::Monotone:
        return QStringLiteral("Monotone");
    }
    return QStringLiteral("Unknown");
}
