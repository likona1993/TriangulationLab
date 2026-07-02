# Triangulation Lab

A Qt desktop app for interactively visualizing 2D polygon triangulation algorithms. Draw a polygon by hand and watch the ear-clipping algorithm cut it into triangles step by step.

## Features

- Draw arbitrary polygons with the mouse
- Triangulate via the ear-clipping algorithm
- Step-by-step debugger showing each ear cut, with the current ear vertex and remaining polygon highlighted
- Rendering abstracted behind an `IRenderer` interface (currently backed by `QPainter`)

Monotone-polygon triangulation is planned but not yet implemented.

## Controls

| Action | Input |
|---|---|
| Add vertex | Left click |
| Close polygon & triangulate | Enter / Return |
| Remove last vertex | Backspace |
| Clear polygon | C |
| Toggle step-by-step debugger | D |
| Step forward / backward (in debugger) | Right / Left arrow |

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

```
src/core/    Qt-independent, template-based geometry library (points, polygons, triangulation algorithms)
src/gui/     Qt Widgets application (facades, rendering, widgets)
src/tests/   GoogleTest unit tests for src/core
```

See `CLAUDE.md` for a more detailed architecture overview.
