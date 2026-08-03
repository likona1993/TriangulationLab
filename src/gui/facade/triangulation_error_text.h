#pragma once

#include <QString>
#include "geometry/triangulation_error.h"

namespace gui {

inline QString errorText(geo::ErrorCode code) {
    using geo::ErrorCode;
    switch (code) {
    case ErrorCode::NotEnoughVertices:
        return QStringLiteral("Polygon must have at least 3 vertices");
    case ErrorCode::PolygonNotClosed:
        return QStringLiteral("Polygon must be closed");
    case ErrorCode::DegeneratePolygon:
        return QStringLiteral("Polygon is degenerate");
    case ErrorCode::SelfIntersectingPolygon:
        return QStringLiteral("Polygon is self-intersecting");
    case ErrorCode::EarNotFound:
        return QStringLiteral("No ear found in polygon");
    case ErrorCode::MonotoneDecompositionFailed:
        return QStringLiteral("Failed to decompose polygon into monotone pieces");
    case ErrorCode::MonotonePieceTriangulationFailed:
        return QStringLiteral("Monotone triangulation failed");
    case ErrorCode::InvariantViolation:
        return QStringLiteral("Internal invariant violation");
    case ErrorCode::Unknown:
    default:
        return QStringLiteral("Unknown error");
    }
}

} // namespace gui
