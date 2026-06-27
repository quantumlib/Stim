#include "stim/util_bot/error_decomp.h"

#include <iostream>

#include "stim/perf.perf.h"
#include "stim/util_bot/str_util.h"

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
