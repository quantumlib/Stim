#ifndef SIMD_UTIL_H
#define SIMD_UTIL_H

#include <immintrin.h>
#include <vector>
#include <iostream>

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
void mem_xor256(__m256i *dst, __m256i *src, size_t words256);

void transpose_bit_matrix(uint64_t *matrix, size_t bit_width) noexcept;
void avx_transpose_64x64s_within_256x256(uint64_t *matrix, size_t row_stride_256) noexcept;

#endif
