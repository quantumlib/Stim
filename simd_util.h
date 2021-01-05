#ifndef SIMD_UTIL_H
#define SIMD_UTIL_H

#include <immintrin.h>
#include <vector>
#include <iostream>

/// For each 2 bit subgroup, performs x -> popcnt(x).
__m256i popcnt2(__m256i r);

/// For each 4 bit subgroup, performs x -> popcnt(x).
__m256i popcnt4(__m256i r);

/// For each 8 bit subgroup, performs x -> popcnt(x).
__m256i popcnt8(__m256i r);

/// For each 16 bit subgroup, performs x -> popcnt(x).
__m256i popcnt16(__m256i r);

__m256i bits_to_m256i(std::vector<bool> data);

std::vector<bool> m256i_to_bits(__m256i data);

std::string hex(__m256i data);
std::string bin(__m256i data);

/// Transposes a bit packed square boolean matrix from row major order to column major order.
///
/// Args:
///     bit_width: Must be a multiple of 256. The number of bits in a row of the matrix.
///         Also the number of bits in a column, since the matrix must be square.
///     matrix: Must be aligned on a 32 byte boundary. Pointer to the matrix data.
///         The storage order is such that the bit at column c row r is toggled by
///         performing `matrix[(r*bit_width + c) / 64] ^= 1 << (c & 63)`.
void transpose_bit_matrix(uint64_t *matrix, size_t bit_width) noexcept;

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

/// Performs `acc + plus - minus` in parallel across every 2 bit word.
__m256i acc_plus_minus_epi2(__m256i acc, __m256i plus, __m256i minus);

#endif
