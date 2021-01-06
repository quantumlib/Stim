#include "simd_util.h"
#include <sstream>
#include <cassert>
#include <thread>
#include <algorithm>
#include <cstring>
#include <bitset>

__m256i popcnt2(__m256i r) {
    auto m = _mm256_set1_epi16(0x5555);
    auto low = _mm256_and_si256(m, r);
    auto high = _mm256_srli_epi16(_mm256_andnot_si256(m, r), 1);
    return _mm256_add_epi16(low, high);
}

__m256i popcnt4(__m256i r) {
    r = popcnt2(r);
    auto m = _mm256_set1_epi16(0x3333);
    auto low = _mm256_and_si256(m, r);
    auto high = _mm256_srli_epi16(_mm256_andnot_si256(m, r), 2);
    return _mm256_add_epi16(low, high);
}

__m256i popcnt8(__m256i r) {
    r = popcnt4(r);
    auto m = _mm256_set1_epi16(0x0F0F);
    auto low = _mm256_and_si256(m, r);
    auto high = _mm256_srli_epi16(_mm256_andnot_si256(m, r), 4);
    return _mm256_add_epi16(low, high);
}

__m256i acc_plus_minus_epi2(__m256i acc, __m256i plus, __m256i minus) {
    // This method is based on the 2 bit reversible inplace addition circuit:
    //
    // off1 --@--@---- off1
    // off2 --|--|-@-- off2
    // acc1 --@--X-|-- acc1'
    // acc2 --X----X-- acc2'
    //
    // Take that circuit, reverse it to get the minus circuit, put the minus
    // part before the addition part, and fuse the Toffoli gates together
    // (i.e. combine the carry handling).
    acc = _mm256_xor_si256(acc, minus);
    auto carries = _mm256_slli_epi16(
            _mm256_and_si256(
                _mm256_set1_epi8(0b01010101),
                _mm256_and_si256(
                    _mm256_xor_si256(plus, minus),
                    acc)),
            1);
    acc = _mm256_xor_si256(acc, carries);
    return _mm256_xor_si256(acc, plus);
}

__m256i popcnt16(__m256i r) {
    r = popcnt8(r);
    auto m = _mm256_set1_epi16(0x00FF);
    auto low = _mm256_and_si256(m, r);
    auto high = _mm256_srli_epi16(_mm256_andnot_si256(m, r), 8);
    return _mm256_add_epi16(low, high);
}

std::vector<bool> m256i_to_bits(__m256i r) {
    std::vector<bool> result;
    auto u64 = (uint64_t *)&r;
    for (size_t k = 0; k < 4; k++) {
        auto e = u64[k];
        for (size_t i = 0; i < 64; i++) {
            result.push_back((e >> i) & 1);
        }
    }
    return result;
}

__m256i bits_to_m256i(std::vector<bool> data) {
    union {__m256i m256; uint64_t u64[4];} result {};
    for (size_t i = 0; i < data.size(); i++) {
        result.u64[i >> 6] |= (uint64_t)data[i] << (i & 63);
    }
    return result.m256;
}

std::string hex(__m256i data) {
    std::stringstream out;
    auto chars = ".123456789ABCDEF";
    auto u64 = (uint64_t *)&data;
    for (size_t w = 0; w < 4; w++) {
        if (w) {
            out << " ";
        }
        for (size_t i = 64; i > 0; i -= 4) {
            out << chars[(u64[w] >> (i - 4)) & 0xF];
        }
    }
    return out.str();
}

std::string bin(__m256i data) {
    std::stringstream out;
    auto chars = "01";
    auto u64 = (uint64_t *)&data;
    for (size_t w = 0; w < 4; w++) {
        if (w) {
            out << " ";
        }
        for (size_t i = 64; i > 0; i--) {
            out << chars[(u64[w] >> (i - 1)) & 0x1];
        }
    }
    return out.str();
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
void avx_transpose_pass(uint64_t *matrix, size_t row_stride_256, __m256i mask) noexcept {
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
    avx_transpose_pass<1>(matrix, row_stride_256, _mm256_set1_epi8(0x55));
    avx_transpose_pass<2>(matrix, row_stride_256, _mm256_set1_epi8(0x33));
    avx_transpose_pass<4>(matrix, row_stride_256, _mm256_set1_epi8(0xF));
    avx_transpose_pass<8>(matrix, row_stride_256, _mm256_set1_epi16(0xFF));
    avx_transpose_pass<16>(matrix, row_stride_256, _mm256_set1_epi32(0xFFFF));
    avx_transpose_pass<32>(matrix, row_stride_256, _mm256_set1_epi64x(0xFFFFFFFF));
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

void mike_transpose_bit_matrix(uint64_t* matrix, uint64_t* out, size_t bit_width) noexcept {
    union { uint8_t arr[32]; __m256i reg; } simd_vec;

    uint8_t* byte_matrix = (uint8_t*)matrix;
    uint8_t* byte_out = (uint8_t*)out;
    for (int i = 0; i < bit_width; i += 32){
        for (int j = 0; j < bit_width; j += 8) {
            for (int k = 0; k < 32; k++){
                simd_vec.arr[k] = byte_matrix[((i + k) * bit_width  + j) >> 3];
            }
            for (int k = 7; k>=0; k--){
                *reinterpret_cast<uint32_t*>(&byte_out[((j + k) * bit_width + i) >> 3]) = _mm256_movemask_epi8(simd_vec.reg);
                simd_vec.reg = _mm256_slli_epi64(simd_vec.reg, 1);
            }
        }
    }
}

void transpose_bit_matrix_256x256(uint64_t *matrix256x256) noexcept {
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
        transpose_bit_matrix_256x256(matrix + (k >> 6));
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

void transpose_bit_matrix_256x256blocks(uint64_t *matrix, size_t bit_width) noexcept {
    auto area = bit_width * bit_width;
    if (bit_width < 256 * 16) {
        transpose_bit_matrix_thread_body(matrix, area);
    } else {
        transpose_bit_matrix_threaded(matrix, area, 4);
    }
}
