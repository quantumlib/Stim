#ifndef SIMD_UTIL_H
#define SIMD_UTIL_H

#include <cstdint>
#include <cstddef>
#include <immintrin.h>

constexpr uint64_t interleave_mask(size_t step) {
    uint64_t result = (1ULL << step) - 1;
    while (step < 32) {
        step <<= 1;
        result |= result << step;
    }
    return result;
}

inline uint64_t spread_bytes_32_to_64(uint32_t v) {
    uint64_t r = (((uint64_t)v << 16) | v) & 0xFFFF0000FFFFULL;
    return ((r << 8) | r) & 0xFF00FF00FF00FFULL;
}

// HACK: use "Substitution failure is not an error" to use _mm_popcnt_u64 if available.
// Otherwise fallback to _mm_popcnt_u32.
template <typename T> static decltype(_mm_popcnt_u64(0)) popcount_fallback_select(uint64_t arg, uint64_t ignored) {
    return _mm_popcnt_u64(arg);
}
template <typename T> static decltype(_mm_popcnt_u32(0)) popcount_fallback_select(uint64_t arg, uint32_t ignored) {
    return _mm_popcnt_u32((uint32_t)arg) + _mm_popcnt_u32((uint32_t)(arg >> 32));
}
inline uint8_t popcnt64(uint64_t value) {
    return (uint8_t) popcount_fallback_select<void>(value, uint64_t {0});
}

#endif
