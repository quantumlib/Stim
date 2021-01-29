#include "pauli_string.h"

#include "../benchmark_util.h"

BENCHMARK(PauliString_multiplication_1M) {
    size_t n = 1000 * 1000;
    PauliString p1(n);
    PauliString p2(n);
    benchmark_go([&]() { p1.ref().inplace_right_mul_returning_log_i_scalar(p2); })
        .goal_micros(8.5)
        .show_rate("Paulis", n);
}

BENCHMARK(PauliString_multiplication_100K) {
    size_t n = 100 * 1000;
    PauliString p1(n);
    PauliString p2(n);
    benchmark_go([&]() { p1.ref().inplace_right_mul_returning_log_i_scalar(p2); })
        .goal_nanos(700)
        .show_rate("Paulis", n);
}

BENCHMARK(PauliString_multiplication_10K) {
    size_t n = 10 * 1000;
    PauliString p1(n);
    PauliString p2(n);
    benchmark_go([&]() { p1.ref().inplace_right_mul_returning_log_i_scalar(p2); })
        .goal_nanos(90)
        .show_rate("Paulis", n);
}
