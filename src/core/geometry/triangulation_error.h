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
    DuplicateVertices,
    EarNotFound,
    MonotoneDecompositionFailed,
    MonotonePieceTriangulationFailed,
    InvariantViolation,
    InitialTriangulationFailed,
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

inline TriangulationError makeSelfIntersectingPolygonError(geo::TriangulationAlgorithm algorithm) {
    return TriangulationError{
        ErrorCategory::Input,
        ErrorCode::SelfIntersectingPolygon,
        "Input polygon size < 3",
        std::move(algorithm)
    };
}

inline TriangulationError makeDuplicateVerticesError(geo::TriangulationAlgorithm algorithm, size_t vertexIndex) {
    return TriangulationError{
        ErrorCategory::Input,
        ErrorCode::DuplicateVertices,
        "Duplicate or near-duplicate vertex at index " + std::to_string(vertexIndex),
        std::move(algorithm)
    };
}

} // namespace geo


