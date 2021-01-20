#ifndef SIMD_UTIL_H
#define SIMD_UTIL_H

#include <immintrin.h>
#include <vector>
#include <iostream>
#include "bit_ptr.h"

__m256i bits_to_m256i(std::vector<bool> data);

std::vector<bool> m256i_to_bits(__m256i data);

std::string hex(__m256i data);
std::string bin(__m256i data);

/// Transposes 256x256 blocks of bits.
///
/// Args:
///     num_bits: Must be a multiple of 256*256. The number of blocks to transpose within, times 256*256.
///     blocks: Pointer to the block data. Must be aligned on a 32 byte boundary.
void blockwise_transpose_256x256(uint64_t *blocks, size_t num_bits, bool allow_threading = true) noexcept;

void mat256_permute_address_swap_c7_r7(uint64_t *matrix256x256) noexcept;

template <uint8_t bit_val>
void mat256_permute_address_swap_ck_rk(uint64_t *matrix256x256, __m256i mask) noexcept {
    auto m = (__m256i *)matrix256x256;
    for (size_t i = 0; i < 256; i += bit_val << 1) {
        for (size_t j = i; j < i + bit_val; j++) {
            auto &a = m[j];
            auto &b = m[j + bit_val];
            auto a1 = _mm256_andnot_si256(mask, a);
            auto b0 = b & mask;
            a &= mask;
            b = _mm256_andnot_si256(mask, b);
            a |= _mm256_slli_epi64(b0, bit_val);
            b |= _mm256_srli_epi64(a1, bit_val);
        }
    }
}

/// Transposes a single 256x256 bit block.
void transpose_bit_block_256x256(uint64_t *matrix256x256) noexcept;

template <uint8_t row_bit_val>
void mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk(uint64_t *matrix256x256) noexcept {
    auto m = (__m256i *)matrix256x256;
    for (size_t i = 0; i < 256; i += row_bit_val << 1) {
        for (size_t j = i; j < i + row_bit_val; j++) {
            auto &a = m[j];
            auto &b = m[j + row_bit_val];
            auto t = _mm256_unpackhi_epi8(a, b);
            a = _mm256_unpacklo_epi8(a, b);
            b = t;
        }
    }
}

/// Permutes a bit-packed matrix block to swap a column address bit for a row address bit.
///
/// Args:
///    h: Must be a power of two. The weight of the address bit to swap.
///    matrix: Pointer into the matrix bit data.
///    row_stride_256: The distance (in 256 bit words) between rows of the matrix.
///    mask: Precomputed bit pattern for h. The pattern should be
///        0b...11110000111100001111 where you alternate between 1 and 0 every
///        h'th bit.
template <uint8_t h>
void mat_permute_address_swap_ck_rs(uint64_t *matrix, size_t row_stride_256, __m256i mask) noexcept {
    auto u256 = (__m256i *) matrix;
    for (size_t col = 0; col < 256; col += h << 1) {
        for (size_t row = col; row < col + h; row++) {
            auto &a = u256[row * row_stride_256];
            auto &b = u256[(row + h) * row_stride_256];
            auto a1 = _mm256_andnot_si256(mask, a);
            auto b0 = _mm256_and_si256(mask, b);
            a = _mm256_and_si256(mask, a);
            b = _mm256_andnot_si256(mask, b);
            a = _mm256_or_si256(a, _mm256_slli_epi64(b0, h));
            b = _mm256_or_si256(_mm256_srli_epi64(a1, h), b);
        }
    }
}

size_t ceil256(size_t n);

bool any_non_zero(const __m256i *data, size_t words256);
void mem_xor256(__m256i *dst, const __m256i *src, size_t words256);
void mem_swap256(__m256i *v0, __m256i *v1, size_t words256);

void transpose_bit_matrix(uint64_t *matrix, size_t bit_width) noexcept;
void avx_transpose_64x64s_within_256x256(uint64_t *matrix, size_t row_stride_256) noexcept;

template <typename BODY>
inline void simd_for_each(__m256i *v0, size_t num_words256, BODY body) {
    auto v0_end = v0 + num_words256;
    while (v0 != v0_end) {
        body(v0);
        v0++;
    }
}

template <typename BODY>
inline void simd_for_each(const __m256i *v0, size_t num_words256, BODY body) {
    auto v0_end = v0 + num_words256;
    while (v0 != v0_end) {
        body(v0);
        v0++;
    }
}

bool not_zero(const __m256i &v);

template <typename BODY>
inline void simd_for_each_2(__m256i *v0, __m256i *v1, size_t num_words256, BODY body) {
    auto v0_end = v0 + num_words256;
    while (v0 != v0_end) {
        body(v0, v1);
        v0++;
        v1++;
    }
}

template <typename BODY>
inline void simd_for_each_3(__m256i *v0, __m256i *v1, __m256i *v2, size_t num_words256, BODY body) {
    auto v0_end = v0 + num_words256;
    while (v0 != v0_end) {
        body(v0, v1, v2);
        v0++;
        v1++;
        v2++;
    }
}

template <typename BODY>
inline void simd_for_each_4(__m256i *v0, __m256i *v1, __m256i *v2, __m256i *v3, size_t num_words256, BODY body) {
    auto v0_end = v0 + num_words256;
    while (v0 != v0_end) {
        body(v0, v1, v2, v3);
        v0++;
        v1++;
        v2++;
        v3++;
    }
}

template <typename BODY>
inline void simd_for_each_5(__m256i *v0, __m256i *v1, __m256i *v2, __m256i *v3, __m256i *v4, size_t num_words256, BODY body) {
    auto v0_end = v0 + num_words256;
    while (v0 != v0_end) {
        body(v0, v1, v2, v3, v4);
        v0++;
        v1++;
        v2++;
        v3++;
        v4++;
    }
}

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

    BitPtr bit_ptr(size_t k);
    bool get_bit(size_t k) const;
    void set_bit(size_t k, bool value);
    void toggle_bit(size_t k);
    void toggle_bit_if(size_t k, bool value);
};

uint16_t pop_count(const __m256i &val);

#endif
