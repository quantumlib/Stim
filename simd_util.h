#ifndef SIMD_UTIL_H
#define SIMD_UTIL_H

#include <immintrin.h>
#include <vector>
#include <iostream>

union BitsPtr {
    __m256i *u256;
    __m128i *u128;
    uint64_t *u64;
    uint32_t *u32;
    uint16_t *u16;
    uint8_t *u8;
};

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

const uint64_t *m256i_u64(const __m256i &result);

uint64_t *m256i_u64(__m256i &result);

uint16_t *m256i_u16(__m256i &result);

/// Transposes a bit packed square boolean matrix from row major order to column major order.
///
/// Args:
///     bit_width: Must be a multiple of 256. The number of bits in a row of the matrix.
///         Also the number of bits in a column, since the matrix must be square.
///     matrix: Must be aligned on a 32 byte boundary. Pointer to the matrix data.
///         The storage order is such that the bit at column c row r is toggled by
///         performing `matrix[(r*bit_width + c) / 64] ^= 1 << (c & 63)`.
void transpose_bit_matrix(uint64_t *matrix, size_t bit_width) noexcept;

#endif
