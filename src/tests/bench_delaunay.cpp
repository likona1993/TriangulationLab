// Базовый бенчмарк DelaunayFlip (ROADMAP.md, Этап 0).
// Не часть ctest — запускается вручную, цифры фиксируются в docs/benchmarks.md.
//
// Использование: bench_delaunay [n1 n2 ...]   (по умолчанию 1000 10000 100000)

#include "geometry/delaunay_flip.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace geo;

// Тот же генератор, что и в DelaunayFlipTest.ThousandContourPointsSatisfyAllInvariants:
// точки на контуре с детерминированным дрожанием радиуса.
static Polygon2<double> makeContourPolygon(std::size_t n) {
    const double pi = std::acos(-1.0);
    Polygon2<double> polygon;
    polygon.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double angle = 2.0 * pi * static_cast<double>(i) / static_cast<double>(n);
        const double radius = 1.0 + 0.05 * std::sin(12.9898 * static_cast<double>(i));
        polygon.push_back({radius * std::cos(angle), radius * std::sin(angle)});
    }
    return polygon;
}

int main(int argc, char** argv) {
    std::size_t defaults[] = {1000, 10000, 100000};

    std::vector<std::size_t> sizes;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            sizes.push_back(static_cast<std::size_t>(std::atoll(argv[i])));
        }
    } else {
        sizes.assign(std::begin(defaults), std::end(defaults));
    }

    std::printf("%8s %4s %10s %12s\n", "n", "ok", "triangles", "time, ms");
    for (std::size_t n : sizes) {
        const auto polygon = makeContourPolygon(n);

        DelaunayFlip<double> delaunayFlip;
        const auto t0 = std::chrono::steady_clock::now();
        const auto result = delaunayFlip.triangulate(polygon);
        const auto t1 = std::chrono::steady_clock::now();

        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::printf("%8zu %4d %10zu %12.1f\n",
                    n, static_cast<int>(static_cast<bool>(result)),
                    result.faces.size(), ms);
        std::fflush(stdout);
    }
    return 0;
}
