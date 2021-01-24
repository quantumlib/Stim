#include "simd_compat.h"

#include "../benchmark_util.h"
#include "simd_compat.h"
#include "simd_bits.h"

BENCHMARK(simd_compat_popcnt256) {
    constexpr size_t n = 1024;
    simd_bits d(n * 256);
    std::mt19937_64 rng(0);
    d.randomize(n, rng);

    auto u256 = (__m256i *)d.ptr_simd;
    uint16_t optimization_blocker = 0;
    benchmark_go([&]() {
        for (size_t k = 0; k < n; k++) {
            optimization_blocker ^= simd_word_256{u256[k]}.popcount();
        }
    }).goal_micros(1.5).show_rate("Bits", d.num_bits_padded());
    if (optimization_blocker == 257) {
        std::cout << '!';
    }
}

BENCHMARK(simd_compat_popcnt128) {
    constexpr size_t n = 1024 * 2;
    simd_bits d(n * 128);
    std::mt19937_64 rng(0);
    d.randomize(n, rng);

    auto u128 = (__m128i *)d.ptr_simd;
    uint16_t optimization_blocker = 0;
    benchmark_go([&]() {
        for (size_t k = 0; k < n; k++) {
            optimization_blocker ^= simd_word_128{u128[k]}.popcount();
        }
    }).goal_micros(1.5).show_rate("Bits", d.num_bits_padded());
    if (optimization_blocker == 129) {
        std::cout << '!';
    }
}

BENCHMARK(simd_compat_popcnt64) {
    constexpr size_t n = 1024 * 4;
    simd_bits d(n * 64);
    std::mt19937_64 rng(0);
    d.randomize(n, rng);

    auto u64 = d.u64;
    uint16_t optimization_blocker = 0;
    benchmark_go([&]() {
        for (size_t k = 0; k < n; k++) {
            optimization_blocker ^= simd_word_64{u64[k]}.popcount();
        }
    }).goal_micros(1.5).show_rate("Bits", d.num_bits_padded());
    if (optimization_blocker == 65) {
        std::cout << '!';
    }
}
