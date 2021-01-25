#include "simd_bits.h"

#include <cstring>

#include "../benchmark_util.h"

BENCHMARK(simd_bits_randomize_10K) {
    size_t n = 10 * 1000;
    simd_bits data(n);
    std::mt19937_64 rng(0);
    benchmark_go([&]() { data.randomize(n, rng); }).goal_nanos(450).show_rate("Bits", n);
}

BENCHMARK(simd_bits_xor_10K) {
    size_t n = 10 * 1000;
    simd_bits d1(n);
    simd_bits d2(n);
    benchmark_go([&]() { d2 ^= d1; }).goal_nanos(20).show_rate("Bits", n);
}

BENCHMARK(simd_bits_not_zero_100K) {
    size_t n = 10 * 100;
    simd_bits d(n);
    benchmark_go([&]() { d.not_zero(); }).goal_nanos(4).show_rate("Bits", n);
}
