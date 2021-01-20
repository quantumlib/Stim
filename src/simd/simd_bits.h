#ifndef simd_bits_H
#define simd_bits_H

#include <cstdint>
#include <immintrin.h>
#include "bit_ptr.h"

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
    simd_bits& operator=(simd_bits&& other) noexcept;
    simd_bits& operator=(const simd_bits& other);

    static simd_bits random(size_t num_bits);

    BitRef operator[](size_t k);
    bool operator[](size_t k) const;
    void clear();

    bool operator==(const simd_bits &other) const;
    bool operator!=(const simd_bits &other) const;
};

#endif
