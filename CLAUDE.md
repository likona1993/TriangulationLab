# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

TriangulationLab is a Qt6/Qt5 desktop app for visualizing 2D polygon triangulation algorithms (currently ear clipping; monotone-polygon triangulation is in progress and not yet functional). It's split into a template-based, Qt-independent core geometry library and a Qt Widgets GUI that drives it through a facade layer.

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
- `geometry/ear_clipping.h(.hpp)` — `EarClipping<T>::triangulate()`. On failure to find an ear it retries once with a 10x-relaxed epsilon before giving up. Every ear cut is recorded as a `DebugStep<T>` (polygon state before the cut, the cut triangle, the ear index) in `m_history`, which is what powers the GUI's step-through debugger.
- `geometry/monotone_triangulation.h(.hpp)` — `MonotoneTriangulation<T>`, a second (sweep-line) triangulation algorithm, in progress. `triangulate()` sorts vertices and dispatches each to a `handle*` method by `classifyVertex()`'s result (`START`/`END`/`SPLIT`/`MERGE`/`REGULAR`); only `handleStart` is implemented, `handleEnd`/`handleSplit`/`handleMerge`/`handleRegular` and `findLeftEdge` are still unimplemented stubs, and `triangulate()` doesn't yet populate `TriangulationResult<T>` or debug history. Note for this `.h`/`.hpp` pair specifically: the `.h` must `#include` the `.hpp` at its end like the other core headers do — this was missing for a while, so the definitions in `.hpp` were never compiled anywhere and silently diverged from their declarations. The `.hpp` also needs its own `#pragma once` (`ear_clipping.hpp` has one too) — without it, an editor opening the `.hpp` directly chokes on the `.hpp` → `.h` → `.hpp` include cycle when building its preamble, even though the cycle is harmless in a real build thanks to the `.h`'s guard.

**`src/gui`** — the Qt application (target `TriangulationLab`, linked against `core`). Core algorithms are never called directly from widgets; everything goes through a facade layer that also does the `QPointF` ⇄ `geo::Point2<double>` conversion:

- `facade/TriangulationFacade` — owns the polygon (`geo::Polygon2<double>`) and the `geo::EarClipping<double>` instance, exposes point-add/remove/close/triangulate operations, and emits Qt signals (`polygonChanged`, `triangulationCompleted`, `debugStepChanged`, etc.) for the widget to react to.
- `facade/DebugFacade` — wraps a `EarClipping`'s debug history and provides step navigation (`stepForward`/`stepBackward`/`goToStep`), with `currentStep == -1` meaning "before the first cut". `TriangulationFacade` delegates its debugger API to this.
- `rendering/IRenderer` — abstract drawing interface (`drawPolygon`, `drawTriangles`, `drawVertexLabels`, `drawEarMarker`, `drawGrid`, `drawText`) so `TriangulationWidget` isn't coupled to a specific paint backend. `QPainterRenderer` is the only implementation currently.
- `widgets/TriangulationWidget` — the canvas: owns a `TriangulationFacade` and an `IRenderer`, handles mouse/keyboard input for building the polygon, and caches facade data for `paintEvent`.
- `widgets/DebugControlsWidget.h` — currently an empty stub (UI controls for the step debugger not yet implemented).
- `MainWindow` hosts a single `TriangulationWidget`.

## Conventions

- Core library code compiles with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`; keep new core code warning-clean under these flags.
- Comments and doc-blocks in the existing code are written in Russian; match the existing style within a file rather than mixing languages.
