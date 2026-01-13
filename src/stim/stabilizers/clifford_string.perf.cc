#include "stim/stabilizers/clifford_string.h"

#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(CliffordString_multiplication_10K) {
    size_t n = 10 * 1000;
    CliffordString<MAX_BITWORD_WIDTH> p1(n);
    CliffordString<MAX_BITWORD_WIDTH> p2(n);
    benchmark_go([&]() {
        p1 *= p2;
    })
        .goal_nanos(430)
        .show_rate("Rots", n);
}
