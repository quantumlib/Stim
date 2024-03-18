#include "stim/stabilizers/conversions.h"

#include "stim/benchmark_util.perf.h"

using namespace stim;

BENCHMARK(disjoint_to_independent_xyz_errors_approx_exact) {
    double w = 0;
    benchmark_go([&]() {
        double a;
        double b;
        double c;
        try_disjoint_to_independent_xyz_errors_approx(0.1, 0.2, 0.15, &a, &b, &c);
        w += a;
        w += b;
        w += c;
    }).goal_nanos(11);
    if (w == 0) {
        std::cout << "data dependence";
    }
}

BENCHMARK(disjoint_to_independent_xyz_errors_approx_p10) {
    double w = 0;
    benchmark_go([&]() {
        double a;
        double b;
        double c;
        try_disjoint_to_independent_xyz_errors_approx(0.1, 0.2, 0.0, &a, &b, &c);
        w += a;
        w += b;
        w += c;
    }).goal_nanos(550);
    if (w == 0) {
        std::cout << "data dependence";
    }
}

BENCHMARK(disjoint_to_independent_xyz_errors_approx_p100) {
    double w = 0;
    benchmark_go([&]() {
        double a;
        double b;
        double c;
        try_disjoint_to_independent_xyz_errors_approx(0.01, 0.02, 0.0, &a, &b, &c);
        w += a;
        w += b;
        w += c;
    }).goal_nanos(550);
    if (w == 0) {
        std::cout << "data dependence";
    }
}

BENCHMARK(independent_to_disjoint_xyz_errors) {
    double w = 0;
    benchmark_go([&]() {
        double a;
        double b;
        double c;
        independent_to_disjoint_xyz_errors(0.1, 0.2, 0.3, &a, &b, &c);
        w += a;
        w += b;
        w += c;
    }).goal_nanos(4);
    if (w == 0) {
        std::cout << "data dependence";
    }
}

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
