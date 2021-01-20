#ifndef SIMD_RANGE_H
#define SIMD_RANGE_H

#include <immintrin.h>
#include <vector>
#include <iostream>
#include "bit_ptr.h"

struct simd_range_ref {
    __m256i *start;
    size_t count;
    simd_range_ref &operator^=(const simd_range_ref &other);
    simd_range_ref &operator^=(const __m256i *other);
    simd_range_ref &operator=(const simd_range_ref &other);
    simd_range_ref &operator=(const __m256i *other);
    bool operator==(const simd_range_ref &other) const;
    bool operator!=(const simd_range_ref &other) const;
    void swap_with(simd_range_ref other);
    void swap_with(__m256i *other);
    void clear();
    bit_ref operator[](size_t k);
    bool operator[](size_t k) const;
    bool not_zero() const;
    const simd_range_ref word_range_ref(size_t word_offset, size_t word_count) const;
    simd_range_ref word_range_ref(size_t word_offset, size_t word_count);
};

struct SimdRange {
    __m256i *start;
    size_t count;
    void clear();
    void swap_with(SimdRange other);
    void swap_with(__m256i *other);

    bit_ref operator[](size_t k);
    bool operator[](size_t k) const;
    simd_range_ref operator*();
    const simd_range_ref operator*() const;
};

#endif
