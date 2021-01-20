#ifndef SIMD_BITS_H
#define SIMD_BITS_H

#include <cstdint>
#include <immintrin.h>
#include "bit_ptr.h"
#include "simd_range.h"

/// Densely packed bits, allocated with an alignment that enables SIMD operations.
struct simd_bits {
    size_t num_bits;
    union {
        uint64_t *u64;
        __m256i *u256;
    };

    ~simd_bits();
    explicit simd_bits(size_t num_bits);
    simd_bits(size_t num_bits, const void *other);
    simd_bits(simd_bits&& other) noexcept;
    simd_bits(const simd_bits& other);
    simd_bits(const simd_range_ref& other);
    simd_bits& operator=(simd_bits&& other) noexcept;
    simd_bits& operator=(const simd_bits& other);

    static simd_bits random(size_t num_bits);

    simd_range_ref range_ref();
    simd_range_ref word_range_ref(size_t word_offset, size_t word_count);
    const simd_range_ref range_ref() const;
    const simd_range_ref word_range_ref(size_t word_offset, size_t word_count) const;
    bit_ref operator[](size_t k);
    const bit_ref operator[](size_t k) const;
    void clear();

    bool operator==(const simd_bits &other) const;
    bool operator!=(const simd_bits &other) const;
};

#endif
