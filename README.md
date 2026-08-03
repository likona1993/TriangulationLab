# Triangulation Lab

A Qt desktop app for interactively visualizing 2D polygon triangulation algorithms. Draw a polygon by hand and triangulate it with different algorithms through a unified GUI.

## Features

- Draw arbitrary polygons with the mouse
- Switch between triangulation algorithms in the GUI
- Triangulate polygons with ear clipping and monotone triangulation
- Step-by-step debugger built around a shared triangulation debug model
- Rendering abstracted behind an `IRenderer` interface (currently backed by `QPainter`)

At the moment, the monotone triangulation path is the primary end-to-end GUI/debug scenario, while the algorithm-switching flow is being generalized across the UI and facade layers.

## Controls

| Action | Input |
|---|---|
| Add vertex | Left click |
| Close polygon & triangulate | Enter / Return |
| Remove last vertex | Backspace |
| Clear polygon | C |
| Toggle step-by-step debugger | D |
| Step forward / backward (in debugger) | Right / Left arrow |
| Select triangulation algorithm | GUI combo box |

## Requirements

- CMake 3.16+
- A C++20 compiler
- Qt 6 (or Qt 5) with the Widgets and OpenGL components
- GoogleTest (for building tests)
- Eigen3 (optional, auto-detected)

## Building

```sh
cmake -S . -B build
cmake --build build -j
./build/TriangulationLab
```

The default build type is `Debug`, which compiles with AddressSanitizer/UBSan enabled. Use `-DCMAKE_BUILD_TYPE=Release` for an optimized build without sanitizers.

## Testing

```sh
cmake --build build --target test_core -j
ctest --test-dir build --output-on-failure
```

To run a single test:

```sh
./build/src/tests/test_core --gtest_filter=EarClippingTest.Square
```

## Project layout

```text
src/core/    Qt-independent, template-based geometry library (points, polygons, triangulation algorithms)
src/gui/     Qt Widgets application (facades, rendering, widgets)
src/tests/   GoogleTest unit tests for src/core
```

See `CLAUDE.md` for a more detailed architecture overview.подтверждаю
