#ifndef SIMD_UTIL_H
#define SIMD_UTIL_H

#include <cstdint>

constexpr uint64_t interleave_mask(size_t step) {
    uint64_t result = (1ULL << step) - 1;
    while (step < 32) {
        step <<= 1;
        result |= result << step;
    }
    return result;
}

#endif
