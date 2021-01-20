#ifndef SIMD_RANGE_H
#define SIMD_RANGE_H

#include <immintrin.h>
#include <vector>
#include <iostream>
#include "bit_ptr.h"

struct SimdRange {
    __m256i *start;
    size_t count;
    SimdRange &operator^=(const SimdRange &other);
    SimdRange &operator^=(const __m256i *other);
    void overwrite_with(const SimdRange &other);
    void overwrite_with(const __m256i *other);
    void clear();
    void swap_with(SimdRange other);
    void swap_with(__m256i *other);

    BitRef operator[](size_t k);
    bool operator[](size_t k) const;
};

#endif
