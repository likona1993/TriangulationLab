# Error Mapping

This document maps the current string-based failures in the codebase to the structured error codes defined in `error-codes.md`.

It is a transition document: the current implementation still reports failures through strings in `TriangulationFacade`, `EarClipping`, and `MonotoneTriangulation`, but these messages should be treated as legacy diagnostics rather than the final contract.

## Mapping table

| Source | Current message | Suggested code | Category |
|---|---|---|---|
| `TriangulationFacade::closePolygon()` | `Need at least 3 points to close polygon` | `NotEnoughVertices` | `Input` |
| `TriangulationFacade::closePolygon()` | `Polygon is degenerate after cleaning` | `DegeneratePolygon` | `Input` |
| `TriangulationFacade::triangulateEarClipping()` | `Polygon must be closed and have at least 3 vertices` | `PolygonNotClosed` / `NotEnoughVertices` | `Input` |
| `TriangulationFacade::triangulateMonotone()` | `Polygon must be closed and have at least 3 vertices` | `PolygonNotClosed` / `NotEnoughVertices` | `Input` |
| `EarClipping::triangulate()` | `Polygon must have at least 3 vertices` | `NotEnoughVertices` | `Input` |
| `EarClipping::triangulate()` | `Polygon is self-intersecting` | `SelfIntersectingPolygon` | `Input` |
| `EarClipping::triangulate()` | `No ear found in polygon (possibly degenerate or self-intersecting)` | `EarNotFound` | `Algorithm` |
| `MonotoneTriangulation::triangulate()` | `Polygon must have at least 3 vertices` | `NotEnoughVertices` | `Input` |
| `MonotoneTriangulation::triangulate()` | `Polygon is self-intersecting` | `SelfIntersectingPolygon` | `Input` |
| `MonotoneTriangulation::triangulate()` | `Failed to decompose polygon into monotone pieces` | `MonotoneDecompositionFailed` | `Algorithm` |
| `MonotoneTriangulation::triangulate()` | `Monotone triangulation failed` | `MonotonePieceTriangulationFailed` | `Algorithm` |
| `GEO_ASSERT_INVARIANT(...)` / `assert(...)` in monotone code | invariant failure text | `InvariantViolation` | `Internal` |

## Notes

- If the same text appears in multiple places, the code should be chosen from the surrounding context, not from the string itself.
- `Polygon must be closed and have at least 3 vertices` is best split into two possible codes in the final model: `PolygonNotClosed` and `NotEnoughVertices`, depending on which precondition actually failed.
- `Internal` errors should stay separate from normal invalid-input cases.
- This file should be updated whenever a new error path is added to the core or facade layer.

## Suggested usage

During migration, the facade can translate legacy strings into structured errors like this:

- `result.error_message` -> `TriangulationError.code`
- `result.error_message` -> `TriangulationError.debugMessage`
- `TriangulationError.code` -> localized or user-friendly text for the GUI

The core goal is to stop using free-form strings as the only source of truth.