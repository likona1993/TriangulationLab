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
- `geometry/triangulation_error.h` — the structured error model: `ErrorCategory` (`Input`/`Algorithm`/`Internal`/`Unknown`), `ErrorCode` (e.g. `NotEnoughVertices`, `SelfIntersectingPolygon`, `DuplicateVertices`, `EarNotFound`, `MonotoneDecompositionFailed`, `MonotonePieceTriangulationFailed`, `InvariantViolation`), and `TriangulationError` (`category`, `code`, `debugMessage`, `algorithm`). Includes `makeError(...)` and a few pre-built helpers like `makeNotEnoughVerticesError(algorithm)`.
- `geometry/triangulation_types.h` — `VertexIndex` (`uint32_t`) / `IndexedTriangle` (единый индексный формат всех алгоритмов), `Edge`/`EdgeLess`, `TriangulationResult<T>` (`vertices` + `faces` + `error` of type `TriangulationError`; координатного вывода в core нет — конвертация индексов в координаты только в точке экспорта), and `DebugStep<T>` (indexed: `polygon_before` as indices with empty = "all vertices" sentinel, `added_faces`, `added_diagonals` as index pairs, `highlighted_vertices`, `description`, plus `replaces_state` for full-state snapshot steps and `sweep_y` for sweep-line steps).
- `geometry/ear_clipping.h(.hpp)` — `EarClipping<T>::triangulate()`. On failure to find an ear it retries once with a 10x-relaxed epsilon before giving up. Its debug output should flow through the shared `DebugStep<T>` model (polygon state before the step, added triangles/diagonals, highlighted or removed vertices, description), so the GUI layer is not tied to ear-clipping-only names.
- `geometry/monotone_triangulation.h(.hpp)` — `MonotoneTriangulation<T>`, the currently active GUI/debug path. `triangulate()` classifies every vertex via `classifyVertex()` (`START`/`END`/`SPLIT`/`MERGE`/`REGULAR`), runs `makeMonotone()` to add diagonals, materializes monotone pieces with `splitIntoMonotone()`, and triangulates each piece while recording shared `DebugStep<T>` history (descriptions, added diagonals, added triangles). Sweep-phase steps also record `sweep_y` (the current sweep-line height) so the GUI can draw the line. In debug builds, `GEO_ASSERT_INVARIANT` (`utils/assert.h`) checks every piece with `isMonotonePiece()` to catch decomposition bugs early.
- `geometry/delaunay_flip.h(.hpp)` — `DelaunayFlip<T>`, flip-based 2D Delaunay triangulation. Builds an initial triangulation with `EarClipping`, then legalizes edges via the Lawson flip stack over a triangle-neighborhood structure (`m_adj[t][k]` — neighbor across the side opposite vertex k, `-1` = boundary); a flip rewrites 2 triangles and their adjacency links in O(1). It records a shared `DebugStep<T>` history. The history starts with the Ear Clipping build steps (accumulation semantics) followed by flip steps that carry a full triangulation snapshot in `added_faces` with `replaces_state = true`, an empty `polygon_before` (sentinel for "all run vertices"), and the four involved vertex indices in `highlighted_vertices`. Public API should stay neutral and minimal: `triangulate()`, `getDebugHistory()`, `clearDebugHistory()`.
- `utils/geometry_predicates.h` — free predicate functions shared by both triangulation algorithms: `orientation`, `onSegment`, `segmentsIntersect`, `isSimplePolygon`, `isConvex`, `pointInTriangle`, `computeXIntersection`, `isAbove`, `findDuplicateVertex` (input validation: O(n log n) duplicate/near-duplicate detection, threshold `EPSILON<T>`). `inCircumcircle` treats a determinant below `eps · Σ|terms|` as zero (cocircular ⇒ edge legal) — an interim error estimate until the exact adaptive predicates of Roadmap Phase 3.
- `utils/assert.h` — `GEO_ASSERT_INVARIANT(cond, msg)`: a debug-only check (no-op when `NDEBUG` is defined) for internal invariants whose failure means a bug in the algorithm, as opposed to bad input. Assert messages should stay short, technical, and in English, since they are developer diagnostics, not user-facing text.

**`src/gui`** — the Qt application (target `TriangulationLab`, linked against `core`). Core algorithms are never called directly from widgets; everything goes through a facade layer that also does the `QPointF` ⇄ `geo::Point2<double>` conversion:

- `facade/TriangulationFacade` — owns the polygon (`geo::Polygon2<double>`), keeps both `geo::EarClipping<double>` and `geo::MonotoneTriangulation<double>`, stores the currently selected algorithm (via `geo::TriangulationAlgorithm`, not a local enum), and exposes point-add/remove/close/triangulate operations. Changing the algorithm should be treated as a facade concern: keep the input polygon, but reset derived triangulation/debug state so the GUI does not mix results from different algorithms. Continue to prefer algorithm-neutral debug names such as highlighted vertices and added diagonals over ear-clipping-specific terminology.
- `facade/DebugFacade` — wraps shared triangulation debug history and provides step navigation (`stepForward`/`stepBackward`/`goToStep`), with `currentStep == -1` meaning the initial state before the first recorded step. `TriangulationFacade` delegates its debugger API to this. Prefer neutral concepts such as accumulated triangles, highlighted vertices, and added diagonals. History steps are index-based (`added_faces`, index-pair diagonals, index-based `polygon_before`); `setHistory(vertices, steps)` takes the run's vertices once, and DebugFacade resolves indices to coordinates in its getters — the single conversion point for debug data. Accumulation is snapshot-aware: a step with `replaces_state = true` discards everything accumulated before it, which is what makes Delaunay flip histories (Ear Clipping prefix + full-state flip snapshots) render correctly.
- error text mapping — user-facing error strings are not stored in `TriangulationError::userMessage` by the algorithms themselves. Instead, core algorithms fill in `category`, `code`, `debugMessage`, and `algorithm` only; a single GUI-side helper (e.g. `errorText(geo::ErrorCode)`) maps each `ErrorCode` to one canonical, user-facing string. This keeps the same error meaning worded identically regardless of which algorithm raised it, and keeps `TriangulationFacade` as the place that turns a `TriangulationError` into the text emitted via `errorOccurred`.
- `rendering/IRenderer` — abstract drawing interface (`drawPolygon`, `drawTriangles`, `drawVertexLabels`, vertex highlight drawing, `drawDiagonals`, `drawSweepLine`, `drawGrid`, `drawText`) so `TriangulationWidget` isn't coupled to a specific paint backend. Keep renderer naming algorithm-neutral: the renderer should know about highlighted vertices, diagonals, sweep lines, and triangles, not about a specific triangulation method. `QPainterRenderer` is the only implementation currently.
- `widgets/TriangulationWidget` — the canvas: owns a `TriangulationFacade` and an `IRenderer`, handles mouse/keyboard input for building the polygon, and caches facade data for `paintEvent`.
- `widgets/DebugControlsWidget` — step-debugger controls (prev/next buttons and a step indicator) hosted in the `MainWindow` toolbar. Talks to `TriangulationFacade` only; emits `userNavigated()` so the canvas can switch itself into debug mode.
- `MainWindow` hosts the `TriangulationWidget` and top-level GUI controls such as the algorithm selector. It should coordinate user-facing controls, but algorithm execution and triangulation state must still flow through `TriangulationWidget` and `TriangulationFacade`.

## Error handling model

- `geo::ErrorCategory` classifies *why* something failed: `Input` (bad polygon/user action), `Algorithm` (valid input, algorithm couldn't finish), `Internal` (invariant violation — a bug), `Unknown` (unmapped fallback).
- `geo::ErrorCode` is the stable, machine-readable identifier for a specific failure (e.g. `SelfIntersectingPolygon`, `EarNotFound`, `MonotoneDecompositionFailed`). Code should branch on `code`, never on message text.
- `geo::TriangulationAlgorithm` records which algorithm raised the error, so a shared `ErrorCode` (like `SelfIntersectingPolygon`) can be reported for both `EarClipping` and `MonotoneTriangulation` without duplicating logic.
- Algorithms construct errors via `makeError(category, code, debugMessage, algorithm)` (or a dedicated helper like `makeNotEnoughVerticesError`) — the canonical user-facing wording lives in exactly one place on the GUI side, not inside each algorithm.
- `debugMessage` is for developer diagnostics (may stay in Russian or English, matching surrounding code) and is never shown to the user as-is.
- `GEO_ASSERT_INVARIANT` failures are not part of this error model — they represent bugs, not reportable `TriangulationError`s, and should never be caught and converted into a user-facing error.

## Current integration notes

- The project is in a transition state from ear-clipping-specific GUI/debug terminology to shared triangulation terminology.
- The facade layer already owns algorithm selection; GUI controls should switch algorithms through `TriangulationWidget`/`TriangulationFacade` rather than calling core algorithms directly.
- When switching algorithms, preserve the polygon input but clear derived triangulation/debug state so stale results are not shown for the newly selected algorithm.
- In UI/facade/renderer code, prefer names like `highlighted vertices`, `added diagonals`, `added triangles`, and `vertex highlight` over `ear`, `cut triangle`, or other algorithm-specific labels.
- `DebugStep<T>` is the common transport model between algorithms and GUI. When extending debug visualization, add generic per-step data there first instead of introducing algorithm-specific widget state.
- `MonotoneTriangulation` should currently be treated as the primary end-to-end GUI/debug path, even if algorithm selection is already exposed in the facade/UI.
- `geo::TriangulationAlgorithm` is the single source of truth for algorithm identity. `TriangulationFacade` and `TriangulationWidget` should not each keep their own local `Algorithm` enum — migrate any remaining local enum to `geo::TriangulationAlgorithm`.
- `TriangulationFacade::triangulate()` dispatches to the selected algorithm, branches on `result.error`, and formats user-facing text via the GUI-side `errorText(ErrorCode)` helper. It also materializes `result.faces` + `result.vertices` into coordinate triangles — the single conversion point for triangulation output (core results are index-only).
- `DelaunayFlip` is now part of the unified GUI/facade workflow: the algorithm selector, facade dispatch, and shared debug history should treat it as a first-class option alongside the existing triangulation algorithms.
- Keep `DelaunayFlip` API-neutral: core code should expose triangulation and debug history only, while all GUI wording stays algorithm-neutral where possible.


### TODO

- **Expand test coverage for `MonotoneTriangulation`** — this is still the main end-to-end path, so it is the first area to strengthen.  
  - add more targeted polygons with split/merge vertices;
  - cover edge cases such as near-collinear points, repeated `y` levels, and narrow monotone pieces;
  - strengthen debug-history assertions where they help catch regressions;
  - keep the core result invariants covered: triangle count, area preservation, non-degenerate triangles, and CCW orientation.

- **Keep the 3D point-cloud triangulation roadmap in focus** — this is the long-term goal, and it naturally builds in stages.  
  - next step: 2D Delaunay via edge flips;
  - then incremental 2D Delaunay (Bowyer–Watson);
  - after that, constrained Delaunay and finally 3D tetrahedralization.

## Roadmap: point cloud triangulation (2D → 3D)

Long-term goal: triangulation of a 3D point cloud. Planned algorithm sequence, each stage builds on the previous one's data structures/intuition:

1. **Delaunay triangulation 2D, flip-based** — start from an existing triangulation (e.g. the current `EarClipping` output) and legalize it via edge flips using the in-circle test. Introduces the core Delaunay concept (locally optimal edge) without new spatial data structures.
2. **Delaunay triangulation 2D, incremental (Bowyer–Watson)** — insert points one at a time, find conflicting triangles, retriangulate the cavity. This is the algorithm that generalizes almost directly to 3D in stage 5.
3. **Delaunay triangulation 2D, divide & conquer** — recursive split of points, merge two sub-triangulations along a separating line/chain. Non-randomized O(n log n) alternative.
4. **Constrained Delaunay Triangulation (CDT) 2D** — Delaunay triangulation that must respect a given set of mandatory edges (polygon boundary/sharp features). Bridges "triangulate a polygon" (ear clipping / monotone) and "triangulate a free point set" (Delaunay).
5. **Delaunay tetrahedralization 3D (Bowyer–Watson in 3D)** — same incremental insertion algorithm as stage 2, generalized: circumcircle → circumsphere, triangles → tetrahedra, star-shaped cavity retriangulation. This is the direct target for the point-cloud goal.
6. **Surface extraction from 3D Delaunay (alpha shapes / surface reconstruction)** — filtering step to recover the actual object surface from the tetrahedralization, since raw 3D Delaunay of a point cloud is a solid (often near-convex-hull) tetrahedral mesh, not yet a surface mesh.

Notes:
- `geo::TriangulationAlgorithm` will need new enum values as each stage lands (e.g. `DelaunayFlip`, `DelaunayIncremental`, `DelaunayDivideConquer`, `ConstrainedDelaunay`, `DelaunayTetrahedralization3D`).
- Stages 1–4 stay within the existing 2D `geo::Point2<T>`/`geo::Polygon2<T>` core; stage 5–6 will likely need new 3D primitives (`geo::Point3<T>`, `Tetrahedron<T>`) and probably a new core sub-namespace, kept Qt-independent like the rest of `src/core`.
- Currently in progress: **stage 1, flip-based Delaunay 2D**.


## Conventions

- Core library code compiles with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`; keep new core code warning-clean under these flags.
- Comments and doc-blocks in the existing code are written in Russian; match the existing style within a file rather than mixing languages.
- Commit messages follow the Conventional Commits style.
