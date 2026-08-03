#pragma once

#include <string>

#include "triangulation_algorithm.h"

namespace geo {

enum class ErrorCategory {
    Input,
    Algorithm,
    Internal,
    Unknown
};

enum class ErrorCode {
    NotEnoughVertices,
    PolygonNotClosed,
    DegeneratePolygon,
    SelfIntersectingPolygon,
    EarNotFound,
    MonotoneDecompositionFailed,
    MonotonePieceTriangulationFailed,
    InvariantViolation,
    Unknown
};

struct TriangulationError {
    ErrorCategory category = ErrorCategory::Unknown;
    ErrorCode code = ErrorCode::Unknown;
    std::string debugMessage;
    geo::TriangulationAlgorithm algorithm;
};

inline TriangulationError makeError(
    ErrorCategory category,
    ErrorCode code,
    std::string debugMessage = {},
    geo::TriangulationAlgorithm algorithm = {}
    ) {
    return TriangulationError{category, code, std::move(debugMessage), std::move(algorithm)};
}

inline TriangulationError makeNotEnoughVerticesError(geo::TriangulationAlgorithm algorithm) {
    return TriangulationError{
        ErrorCategory::Input,
        ErrorCode::NotEnoughVertices,
        "Input polygon size < 3",
        std::move(algorithm)
    };
}

} // namespace geo


