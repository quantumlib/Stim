#ifndef SIMD_UTIL_H
#define SIMD_UTIL_H

#include <immintrin.h>
#include <vector>
#include <iostream>

__m256i bits_to_m256i(std::vector<bool> data);

std::vector<bool> m256i_to_bits(__m256i data);

std::string hex(__m256i data);
std::string bin(__m256i data);

/// Transposes the 256x256 blocks of a block bit packed square boolean matrix.
///
/// Data Format:
///     The matrix data storage order is such that the bit at column c row r is toggled by
///     performing:
///
///        ```
///        auto block_col = col >> 8;
///        auto inner_col = col & 0xFF;
///        auto block_row = row >> 8;
///        auto inner_row = row & 0xFF;
///        auto bit = inner_col + (inner_row << 8) + (block_col << 16) + block_row * (bit_width << 8)
///        matrix[bit / 64] ^= 1 << (bit & 63)`.
///        ```
///
///     The block transpose operation swaps the roles inner_col and inner_row while leaving block_col
///     and block_row unchanged.
///
/// Args:
///     bit_width: Must be a multiple of 256. The number of bits in a row of the matrix.
///         Also the number of bits in a column, since the matrix must be square.
///     matrix: Must be aligned on a 32 byte boundary. Pointer to the matrix data.
void transpose_bit_matrix_256x256blocks(uint64_t *matrix, size_t bit_width) noexcept;

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

size_t ceil256(size_t n);

bool any_non_zero(const __m256i *data, size_t words256, size_t stride256);

#endif
