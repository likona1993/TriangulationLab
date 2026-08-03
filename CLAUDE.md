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
- `geometry/ear_clipping.h(.hpp)` — `EarClipping<T>::triangulate()`. On failure to find an ear it retries once with a 10x-relaxed epsilon before giving up. Its debug output should flow through the shared `DebugStep<T>` model (polygon state before the step, added triangles/diagonals, highlighted or removed vertices, description), so the GUI layer is not tied to ear-clipping-only names.
- `geometry/monotone_triangulation.h(.hpp)` — `MonotoneTriangulation<T>`, the currently active GUI/debug path. `triangulate()` classifies every vertex via `classifyVertex()` (`START`/`END`/`SPLIT`/`MERGE`/`REGULAR`), runs `makeMonotone()` to add diagonals, materializes monotone pieces with `splitIntoMonotone()`, and triangulates each piece while recording shared `DebugStep<T>` history (descriptions, added diagonals, added triangles). In debug builds, `GEO_ASSERT_INVARIANT` (`utils/assert.h`) checks every piece with `isMonotonePiece()` to catch decomposition bugs early.
- `utils/geometry_predicates.h` — free predicate functions shared by both triangulation algorithms: `orientation`, `onSegment`, `segmentsIntersect`, `isSimplePolygon`, `isConvex`, `pointInTriangle`, `computeXIntersection`, `isAbove`.
- `utils/assert.h` — `GEO_ASSERT_INVARIANT(cond, msg)`: a debug-only check (no-op when `NDEBUG` is defined) for internal invariants whose failure means a bug in the algorithm, as opposed to bad input.

**`src/gui`** — the Qt application (target `TriangulationLab`, linked against `core`). Core algorithms are never called directly from widgets; everything goes through a facade layer that also does the `QPointF` ⇄ `geo::Point2<double>` conversion:

- `facade/TriangulationFacade` — owns the polygon (`geo::Polygon2<double>`), keeps both `geo::EarClipping<double>` and `geo::MonotoneTriangulation<double>`, stores the currently selected algorithm, and exposes point-add/remove/close/triangulate operations. Changing the algorithm should be treated as a facade concern: keep the input polygon, but reset derived triangulation/debug state so the GUI does not mix results from different algorithms. Continue to prefer algorithm-neutral debug names such as highlighted vertices and added diagonals over ear-clipping-specific terminology.
- `facade/DebugFacade` — wraps shared triangulation debug history and provides step navigation (`stepForward`/`stepBackward`/`goToStep`), with `currentStep == -1` meaning the initial state before the first recorded step. `TriangulationFacade` delegates its debugger API to this. Prefer neutral concepts such as accumulated triangles, highlighted vertices, and added diagonals.
- `rendering/IRenderer` — abstract drawing interface (`drawPolygon`, `drawTriangles`, `drawVertexLabels`, vertex highlight drawing, `drawGrid`, `drawText`) so `TriangulationWidget` isn't coupled to a specific paint backend. Keep renderer naming algorithm-neutral: the renderer should know about highlighted vertices, diagonals, and triangles, not about a specific triangulation method. `QPainterRenderer` is the only implementation currently.
- `widgets/TriangulationWidget` — the canvas: owns a `TriangulationFacade` and an `IRenderer`, handles mouse/keyboard input for building the polygon, and caches facade data for `paintEvent`.
- `widgets/DebugControlsWidget.h` — currently an empty stub (UI controls for the step debugger not yet implemented).
- `MainWindow` hosts the `TriangulationWidget` and top-level GUI controls such as the algorithm selector. It should coordinate user-facing controls, but algorithm execution and triangulation state must still flow through `TriangulationWidget` and `TriangulationFacade`.

## Current integration notes

- The project is in a transition state from ear-clipping-specific GUI/debug terminology to shared triangulation terminology.
- The facade layer already owns algorithm selection; GUI controls should switch algorithms through `TriangulationWidget`/`TriangulationFacade` rather than calling core algorithms directly.
- When switching algorithms, preserve the polygon input but clear derived triangulation/debug state so stale results are not shown for the newly selected algorithm.
- In UI/facade/renderer code, prefer names like `highlighted vertices`, `added diagonals`, `added triangles`, and `vertex highlight` over `ear`, `cut triangle`, or other algorithm-specific labels.
- `DebugStep<T>` is the common transport model between algorithms and GUI. When extending debug visualization, add generic per-step data there first instead of introducing algorithm-specific widget state.
- `MonotoneTriangulation` should currently be treated as the primary end-to-end GUI/debug path, even if algorithm selection is already exposed in the facade/UI.

## TODO

- Add focused unit tests for `MonotoneTriangulation`, especially for:
  - simple monotone polygons;
  - polygons that require split/merge handling;
  - validation of triangle count and basic coverage/invariants;
  - debug-history consistency (`added_diagonals`, `added_triangles`, step descriptions).
- Revisit the error-handling model across core and facade layers:
  - define clearer categories for input errors, algorithm failures, and internal invariant violations;
  - reduce reliance on ad-hoc string messages as the only error contract;
  - decide which failures should be reported to the user and which should remain developer/debug diagnostics.

## Conventions

- Core library code compiles with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`; keep new core code warning-clean under these flags.
- Comments and doc-blocks in the existing code are written in Russian; match the existing style within a file rather than mixing languages.