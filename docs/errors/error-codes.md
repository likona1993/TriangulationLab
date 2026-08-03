# Error Codes

This document defines the canonical error codes used by TriangulationLab.

The project is transitioning from ad-hoc string-based error reporting to a structured model with explicit categories and stable codes. That makes error handling easier to test, document, and display in the GUI.

## Principles

- Core algorithms should report structured errors, not just free-form text.
- The facade layer should translate core errors into UI-friendly messages.
- Internal invariants are not user errors; they indicate a bug or an unexpected implementation state.
- Error text shown to the user should stay short and neutral.

## Categories

| Category | Meaning |
|---|---|
| `Input` | The input polygon or user action is invalid. |
| `Algorithm` | The input is valid, but the algorithm failed to complete triangulation. |
| `Internal` | A violated invariant or implementation defect. |
| `Unknown` | A temporary fallback for unmapped errors. |

## Error codes

| Code | Category | Meaning |
|---|---|---|
| `NotEnoughVertices` | `Input` | The polygon has fewer than 3 vertices. |
| `PolygonNotClosed` | `Input` | Triangulation was requested before the polygon was closed. |
| `DegeneratePolygon` | `Input` | The polygon became invalid after preprocessing, for example after removing collinear points. |
| `SelfIntersectingPolygon` | `Input` | The polygon contains self-intersections and cannot be triangulated safely. |
| `EarNotFound` | `Algorithm` | Ear clipping could not find a valid ear, even after retrying with a relaxed epsilon. |
| `MonotoneDecompositionFailed` | `Algorithm` | The sweep-line phase failed to decompose the polygon into monotone pieces. |
| `MonotonePieceTriangulationFailed` | `Algorithm` | A monotone piece could not be triangulated. |
| `InvariantViolation` | `Internal` | A debug/runtime invariant failed and should be treated as a bug. |
| `Unknown` | `Unknown` | A fallback for unexpected or unmapped errors. |

## Suggested fields

A structured error object should normally contain:

- `code` — stable machine-readable identifier.
- `category` — input, algorithm, internal, or unknown.
- `userMessage` — short text suitable for the GUI.
- `debugMessage` — technical description for logs and diagnostics.
- `algorithm` — optional algorithm name, for example `EarClipping` or `MonotoneTriangulation`.
- `context` — optional extra data such as vertex count, epsilon, or step name.

## Usage rules

- Do not branch on message text.
- Use `code` for tests, logs, and UI decisions.
- Use `userMessage` only for presentation.
- Treat `Internal` errors as diagnostics, not as normal user feedback.
- Keep the set of codes stable once they are adopted by tests and GUI code.

## Notes

This file is intentionally algorithm-neutral. It should remain valid even if more triangulation methods are added later.