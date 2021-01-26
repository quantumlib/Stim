#include "simd_compat.h"

#include "../benchmark_util.h"
#include "simd_compat.h"
#include "simd_bits.h"

BENCHMARK(simd_compat_popcnt) {
    simd_bits d(1024 * 256);
    std::mt19937_64 rng(0);
    d.randomize(d.num_bits_padded(), rng);

    uint64_t optimization_blocker = 0;
    benchmark_go([&]() {
        d[300] ^= true;
        for (size_t k = 0; k < d.num_simd_words; k++) {
            optimization_blocker += d.ptr_simd[k].popcount();
        }
    }).goal_micros(1.5).show_rate("Bits", d.num_bits_padded());
    if (optimization_blocker == 0) {
        std::cout << '!';
    }
}
