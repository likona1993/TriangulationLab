# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working in this repository.

## Project overview

TriangulationLab is a Qt6/Qt5 desktop app for visualizing 2D polygon triangulation algorithms. The project is split into a template-based, Qt-independent core geometry library and a Qt Widgets GUI that drives it through a facade layer.

Both ear clipping and monotone triangulation are present in the facade layer, and the GUI is being updated to switch between them through a unified workflow. At the moment, `MonotoneTriangulation` should still be treated as the primary end-to-end GUI/debug path: it emits the shared debug history used by the current visualization flow, while the rest of the UI/facade path is being generalized around algorithm switching.

## Build

Out-of-source CMake build (C++20, requires Qt6 or Qt5 with Widgets+OpenGL, and GoogleTest for tests; Eigen3 is optional/auto-detected):

```sh
cmake -S . -B build
cmake --build build -j
```

Build type defaults to `Debug`, which compiles with `-fsanitize=address,undefined`. Use `-DCMAKE_BUILD_TYPE=Release` for an optimized build (`-O3 -march=native`, no sanitizers).

Run the app:

```sh
./build/TriangulationLab
```

## Tests

Tests use GoogleTest and live in `src/tests/`, built as the `test_core` binary (covers `src/core` only — there's no GUI/facade test coverage yet).

The current `MonotoneTriangulation` tests cover basic invalid-input handling, convex/concave polygons, result invariants (triangle count, area preservation, non-degenerate triangles, CCW orientation), debug-history reset, and at least one split/merge-style polygon that records added diagonals.

```sh
cmake --build build --target test_core -j
ctest --test-dir build --output-on-failure
# or run the binary directly, e.g. for a single test:
./build/src/tests/test_core --gtest_filter=EarClippingTest.Square
```

`BUILD_TESTS` is a CMake option (default `ON`). A `test_gui` target is defined only if Qt Test is found, but its source file (`test_triangulation_widget.cpp`) does not exist yet.

## Architecture

**`src/core`** — a header-only-style, Qt-independent static library (`core`) of template geometry code under `geo::`. Templates split declaration/implementation across `.h` (declarations) and `.hpp` (definitions, included at the bottom of the `.h`), e.g. `types.h`/`types.hpp`. All core code is templated on the scalar type `T` (used as `double` throughout the GUI); `EPSILON<T>` (see `utils/numeric.h`) is the default tolerance for geometric comparisons, with a coarser specialization for `float`.

- `geometry/types.h(.hpp)` — `Point2<T>`, `Polygon2<T>` (alias for `vector<Point2<T>>`), `Triangle2<T>`, and polygon utilities (`signedArea2D`, `ensureCCW`, `removeCollinearPoints`).
- `geometry/triangulation_algorithm.h` — `TriangulationAlgorithm` enum (`EarClipping`, `Monotone`). This is the single canonical algorithm identifier for the whole codebase; both `TriangulationError::algorithm` and any GUI-level algorithm selection should reference this type instead of declaring their own local enum.
- `geometry/triangulation_error.h` — the structured error model: `ErrorCategory` (`Input`/`Algorithm`/`Internal`/`Unknown`), `ErrorCode` (e.g. `NotEnoughVertices`, `SelfIntersectingPolygon`, `EarNotFound`, `MonotoneDecompositionFailed`, `MonotonePieceTriangulationFailed`, `InvariantViolation`), and `TriangulationError` (`category`, `code`, `userMessage`, `debugMessage`, `algorithm`). Includes `makeError(...)` and a few pre-built helpers like `makeNotEnoughVerticesError(algorithm)`.
- `geometry/triangulation_types.h` — `Edge`/`EdgeLess`, `TriangulationResult<T>` (`triangles`, `success`, `error_message` — legacy, being phased out — and the new `error` field of type `TriangulationError`), and `DebugStep<T>`.
- `geometry/ear_clipping.h(.hpp)` — `EarClipping<T>::triangulate()`. On failure to find an ear it retries once with a 10x-relaxed epsilon before giving up. Its debug output should flow through the shared `DebugStep<T>` model (polygon state before the step, added triangles/diagonals, highlighted or removed vertices, description), so the GUI layer is not tied to ear-clipping-only names.
- `geometry/monotone_triangulation.h(.hpp)` — `MonotoneTriangulation<T>`, the currently active GUI/debug path. `triangulate()` classifies every vertex via `classifyVertex()` (`START`/`END`/`SPLIT`/`MERGE`/`REGULAR`), runs `makeMonotone()` to add diagonals, materializes monotone pieces with `splitIntoMonotone()`, and triangulates each piece while recording shared `DebugStep<T>` history (descriptions, added diagonals, added triangles). In debug builds, `GEO_ASSERT_INVARIANT` (`utils/assert.h`) checks every piece with `isMonotonePiece()` to catch decomposition bugs early.
- `utils/geometry_predicates.h` — free predicate functions shared by both triangulation algorithms: `orientation`, `onSegment`, `segmentsIntersect`, `isSimplePolygon`, `isConvex`, `pointInTriangle`, `computeXIntersection`, `isAbove`.
- `utils/assert.h` — `GEO_ASSERT_INVARIANT(cond, msg)`: a debug-only check (no-op when `NDEBUG` is defined) for internal invariants whose failure means a bug in the algorithm, as opposed to bad input. Assert messages should stay short, technical, and in English, since they are developer diagnostics, not user-facing text.

**`src/gui`** — the Qt application (target `TriangulationLab`, linked against `core`). Core algorithms are never called directly from widgets; everything goes through a facade layer that also does the `QPointF` ⇄ `geo::Point2<double>` conversion:

- `facade/TriangulationFacade` — owns the polygon (`geo::Polygon2<double>`), keeps both `geo::EarClipping<double>` and `geo::MonotoneTriangulation<double>`, stores the currently selected algorithm (via `geo::TriangulationAlgorithm`, not a local enum), and exposes point-add/remove/close/triangulate operations. Changing the algorithm should be treated as a facade concern: keep the input polygon, but reset derived triangulation/debug state so the GUI does not mix results from different algorithms. Continue to prefer algorithm-neutral debug names such as highlighted vertices and added diagonals over ear-clipping-specific terminology.
- `facade/DebugFacade` — wraps shared triangulation debug history and provides step navigation (`stepForward`/`stepBackward`/`goToStep`), with `currentStep == -1` meaning the initial state before the first recorded step. `TriangulationFacade` delegates its debugger API to this. Prefer neutral concepts such as accumulated triangles, highlighted vertices, and added diagonals.
- error text mapping — user-facing error strings are not stored in `TriangulationError::userMessage` by the algorithms themselves. Instead, core algorithms fill in `category`, `code`, `debugMessage`, and `algorithm` only; a single GUI-side helper (e.g. `errorText(geo::ErrorCode)`) maps each `ErrorCode` to one canonical, user-facing string. This keeps the same error meaning worded identically regardless of which algorithm raised it, and keeps `TriangulationFacade` as the place that turns a `TriangulationError` into the text emitted via `errorOccurred`.
- `rendering/IRenderer` — abstract drawing interface (`drawPolygon`, `drawTriangles`, `drawVertexLabels`, vertex highlight drawing, `drawGrid`, `drawText`) so `TriangulationWidget` isn't coupled to a specific paint backend. Keep renderer naming algorithm-neutral: the renderer should know about highlighted vertices, diagonals, and triangles, not about a specific triangulation method. `QPainterRenderer` is the only implementation currently.
- `widgets/TriangulationWidget` — the canvas: owns a `TriangulationFacade` and an `IRenderer`, handles mouse/keyboard input for building the polygon, and caches facade data for `paintEvent`.
- `widgets/DebugControlsWidget.h` — currently an empty stub (UI controls for the step debugger not yet implemented).
- `MainWindow` hosts the `TriangulationWidget` and top-level GUI controls such as the algorithm selector. It should coordinate user-facing controls, but algorithm execution and triangulation state must still flow through `TriangulationWidget` and `TriangulationFacade`.

## Error handling model

- `geo::ErrorCategory` classifies *why* something failed: `Input` (bad polygon/user action), `Algorithm` (valid input, algorithm couldn't finish), `Internal` (invariant violation — a bug), `Unknown` (unmapped fallback).
- `geo::ErrorCode` is the stable, machine-readable identifier for a specific failure (e.g. `SelfIntersectingPolygon`, `EarNotFound`, `MonotoneDecompositionFailed`). Code should branch on `code`, never on message text.
- `geo::TriangulationAlgorithm` records which algorithm raised the error, so a shared `ErrorCode` (like `SelfIntersectingPolygon`) can be reported for both `EarClipping` and `MonotoneTriangulation` without duplicating logic.
- Algorithms construct errors via `makeError(category, code, /*userMessage=*/{}, debugMessage, algorithm)` (or a dedicated helper like `makeNotEnoughVerticesError`) and leave `userMessage` empty — the canonical user-facing wording lives in exactly one place on the GUI side, not inside each algorithm.
- `debugMessage` is for developer diagnostics (may stay in Russian or English, matching surrounding code) and is never shown to the user as-is.
- `TriangulationResult<T>::error_message` is a **legacy field** kept only for the ongoing migration; new code should populate and read `error` (`TriangulationError`) instead. Do not add new call sites that only set `error_message`.
- `GEO_ASSERT_INVARIANT` failures are not part of this error model — they represent bugs, not reportable `TriangulationError`s, and should never be caught and converted into a user-facing error.

## Current integration notes

- The project is in a transition state from ear-clipping-specific GUI/debug terminology to shared triangulation terminology.
- The facade layer already owns algorithm selection; GUI controls should switch algorithms through `TriangulationWidget`/`TriangulationFacade` rather than calling core algorithms directly.
- When switching algorithms, preserve the polygon input but clear derived triangulation/debug state so stale results are not shown for the newly selected algorithm.
- In UI/facade/renderer code, prefer names like `highlighted vertices`, `added diagonals`, `added triangles`, and `vertex highlight` over `ear`, `cut triangle`, or other algorithm-specific labels.
- `DebugStep<T>` is the common transport model between algorithms and GUI. When extending debug visualization, add generic per-step data there first instead of introducing algorithm-specific widget state.
- `MonotoneTriangulation` should currently be treated as the primary end-to-end GUI/debug path, even if algorithm selection is already exposed in the facade/UI.
- `geo::TriangulationAlgorithm` is the single source of truth for algorithm identity. `TriangulationFacade` and `TriangulationWidget` should not each keep their own local `Algorithm` enum — migrate any remaining local enum to `geo::TriangulationAlgorithm`.
- `TriangulationFacade::triangulateEarClipping()` / `triangulateMonotone()` still read `result.error_message` and `result.success` in places; as `EarClipping` and `MonotoneTriangulation` are migrated to populate `result.error`, update these call sites to branch on `result.error` and format text via the GUI-side `errorText(ErrorCode)` helper instead of forwarding a raw string.

## TODO

- Expand `MonotoneTriangulation` test coverage beyond the current baseline:
  - add more targeted split/merge polygons and edge cases;
  - strengthen debug-history assertions where useful;
  - consider more pathological inputs (near-collinear vertices, repeated y-levels, narrow monotone pieces);
  - keep result-level invariants covered: triangle count, area preservation, non-degenerate triangles, CCW output orientation.


## Conventions

- Core library code compiles with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`; keep new core code warning-clean under these flags.
- Comments and doc-blocks in the existing code are written in Russian; match the existing style within a file rather than mixing languages.
- Commit messages follow the Conventional Commits style.
