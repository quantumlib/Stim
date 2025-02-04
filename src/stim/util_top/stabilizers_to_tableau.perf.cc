#include "stim/util_top/stabilizers_to_tableau.h"

#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(stabilizers_to_tableau_144) {
    std::vector<std::complex<float>> offsets{
        {1, 0},
        {-1, 0},
        {0, 1},
        {0, -1},
        {3, 6},
        {-6, 3},
    };
    size_t w = 24;
    size_t h = 12;

    auto normalize = [&](std::complex<float> c) -> std::complex<float> {
        return {fmodf(c.real() + w * 10, w), fmodf(c.imag() + h * 10, h)};
    };
    auto q2i = [&](std::complex<float> c) -> size_t {
        c = normalize(c);
        return (int)c.real() / 2 + c.imag() * (w / 2);
    };

    std::vector<stim::PauliString<64>> stabilizers;
    for (size_t x = 0; x < w; x++) {
        for (size_t y = x % 2; y < h; y += 2) {
            std::complex<float> s{x % 2 ? -1.0f : +1.0f, 0.0f};
            std::complex<float> c{(float)x, (float)y};
            stim::PauliString<64> ps(w * h / 2);
            for (const auto &offset : offsets) {
                size_t i = q2i(c + offset * s);
                if (x % 2 == 0) {
                    ps.xs[i] = 1;
                } else {
                    ps.zs[i] = 1;
                }
            }
            stabilizers.push_back(ps);
        }
    }

    size_t dep = 0;
    benchmark_go([&]() {
        Tableau<64> t = stabilizers_to_tableau(stabilizers, true, true, false);
        dep += t.xs[0].zs[0];
    }).goal_micros(500);
    if (dep == 99999999) {
        std::cout << "data dependence";
    }
}

BENCHMARK(stabilizers_to_tableau_576) {
    std::vector<std::complex<float>> offsets{
        {1, 0},
        {-1, 0},
        {0, 1},
        {0, -1},
        {3, 6},
        {-6, 3},
    };
    size_t w = 24 * 4;
    size_t h = 12 * 4;

    auto normalize = [&](std::complex<float> c) -> std::complex<float> {
        return {fmodf(c.real() + w * 10, w), fmodf(c.imag() + h * 10, h)};
    };
    auto q2i = [&](std::complex<float> c) -> size_t {
        c = normalize(c);
        return (int)c.real() / 2 + c.imag() * (w / 2);
    };

    std::vector<stim::PauliString<64>> stabilizers;
    for (size_t x = 0; x < w; x++) {
        for (size_t y = x % 2; y < h; y += 2) {
            std::complex<float> s{x % 2 ? -1.0f : +1.0f, 0.0f};
            std::complex<float> c{(float)x, (float)y};
            stim::PauliString<64> ps(w * h / 2);
            for (const auto &offset : offsets) {
                size_t i = q2i(c + offset * s);
                if (x % 2 == 0) {
                    ps.xs[i] = 1;
                } else {
                    ps.zs[i] = 1;
                }
            }
            stabilizers.push_back(ps);
        }
    }

    size_t dep = 0;
    benchmark_go([&]() {
        Tableau<64> t = stabilizers_to_tableau(stabilizers, true, true, false);
        dep += t.xs[0].zs[0];
    }).goal_millis(200);
    if (dep == 99999999) {
        std::cout << "data dependence";
    }
}
