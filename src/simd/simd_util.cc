#include "simd_util.h"
#include <sstream>
#include <cassert>
#include <thread>
#include <algorithm>
#include <cstring>
#include <bitset>

void transpose_bit_block_256x256(uint64_t *matrix256x256) noexcept {
    mat256_permute_address_swap_ck_rk<1>(matrix256x256, _mm256_set1_epi8(0x55));
    mat256_permute_address_swap_ck_rk<2>(matrix256x256, _mm256_set1_epi8(0x33));
    mat256_permute_address_swap_ck_rk<4>(matrix256x256, _mm256_set1_epi8(0xF));
    mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<64>(matrix256x256);
    mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<32>(matrix256x256);
    mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<16>(matrix256x256);
    mat256_permute_address_rotate_c3_c4_c5_c6_swap_c6_rk<8>(matrix256x256);
    mat256_permute_address_swap_c7_r7(matrix256x256);
}

void mat256_permute_address_swap_c7_r7(uint64_t *matrix256x256) noexcept {
    auto u128 = (__m128i *)matrix256x256;
    for (size_t m = 0; m < 256; m += 2) {
        std::swap(u128[m | 0x100], u128[m | 1]);
    }
}

void transpose_bit_matrix_thread_body(uint64_t *matrix, size_t area) noexcept {
    for (size_t k = 0; k < area; k += 1 << 16) {
        transpose_bit_block_256x256(matrix + (k >> 6));
    }
}

void transpose_bit_matrix_threaded(uint64_t *matrix, size_t area, size_t thread_count) noexcept {
    std::vector<std::thread> threads;
    size_t chunk_area = (((area >> 16) + thread_count - 1) / thread_count) << 16;
    for (size_t k = chunk_area; k < area; k += chunk_area) {
        size_t n = std::min(chunk_area, area - k);
        uint64_t *start = matrix + (k >> 6);
        threads.emplace_back([=]() { transpose_bit_matrix_thread_body(start, n); });
    }
    transpose_bit_matrix_thread_body(matrix, chunk_area);
    for (auto &t : threads) {
        t.join();
    }
}

void blockwise_transpose_256x256(uint64_t *matrix, size_t num_bits, bool allow_threading) noexcept {
    if (allow_threading && num_bits < 1 << 24) {
        transpose_bit_matrix_thread_body(matrix, num_bits);
    } else {
        transpose_bit_matrix_threaded(matrix, num_bits, 4);
    }
}

/// Transposes within the 64x64 bit blocks of a 256x256 block subset of a boolean matrix.
///
/// For example, if we were transposing 2x2 blocks inside a 4x4 matrix, the order would go from:
///
///     aA bB cC dD
///     eE fF gG hH
///
///     iI jJ kK lL
///     mM nN oO pP
///
/// To:
///
///     ae bf cg dh
///     AE BF CG DH
///
///     im jn ko lp
///     IM JN KO LP
///
/// Args:
///     matrix: Pointer to the matrix data to transpose.
///     row_stride_256: Distance, in 256 bit words, between matrix rows.
void avx_transpose_64x64s_within_256x256(uint64_t *matrix, size_t row_stride_256) noexcept {
    mat_permute_address_swap_ck_rs<1>(matrix, row_stride_256, _mm256_set1_epi8(0x55));
    mat_permute_address_swap_ck_rs<2>(matrix, row_stride_256, _mm256_set1_epi8(0x33));
    mat_permute_address_swap_ck_rs<4>(matrix, row_stride_256, _mm256_set1_epi8(0xF));
    mat_permute_address_swap_ck_rs<8>(matrix, row_stride_256, _mm256_set1_epi16(0xFF));
    mat_permute_address_swap_ck_rs<16>(matrix, row_stride_256, _mm256_set1_epi32(0xFFFF));
    mat_permute_address_swap_ck_rs<32>(matrix, row_stride_256, _mm256_set1_epi64x(0xFFFFFFFF));
}

void transpose_bit_matrix(uint64_t *matrix, size_t bit_width) noexcept {
    assert((bit_width & 255) == 0);

    // Transpose bits inside each 64x64 bit block.
    size_t stride = bit_width >> 8;
    for (size_t col = 0; col < bit_width; col += 256) {
        for (size_t row = 0; row < bit_width; row += 256) {
            avx_transpose_64x64s_within_256x256(
                    matrix + ((col + row * bit_width) >> 6),
                    stride);
        }
    }

    // Transpose between 64x64 bit blocks.
    size_t u64_width = bit_width >> 6;
    size_t u64_block = u64_width << 6;
    for (size_t block_row = 0; block_row < bit_width; block_row += 64) {
        for (size_t block_col = block_row + 64; block_col < bit_width; block_col += 64) {
            size_t w0 = (block_row * bit_width + block_col) >> 6;
            size_t w1 = (block_col * bit_width + block_row) >> 6;
            for (size_t k = 0; k < u64_block; k += u64_width) {
                std::swap(matrix[w0 + k], matrix[w1 + k]);
            }
        }
    }
}

uint16_t pop_count(const __m256i &val) {
    auto p = (uint64_t *)&val;
    uint16_t result = 0;
    result += std::popcount(p[0]);
    result += std::popcount(p[1]);
    result += std::popcount(p[2]);
    result += std::popcount(p[3]);
    return result;
}
