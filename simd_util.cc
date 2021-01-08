#include "simd_util.h"
#include <sstream>
#include <cassert>
#include <thread>
#include <algorithm>
#include <cstring>
#include <bitset>

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

void transpose_bit_matrix_256x256blocks(uint64_t *matrix, size_t bit_area) noexcept {
    if (bit_area < 1 << 24) {
        transpose_bit_matrix_thread_body(matrix, bit_area);
    } else {
        transpose_bit_matrix_threaded(matrix, bit_area, 4);
    }
}

size_t ceil256(size_t n) {
    return (n + 0xFF) & ~0xFF;
}

bool any_non_zero(const __m256i *data, size_t words256, size_t stride256) {
    union {__m256i m256; uint64_t u64[4]; } acc {};
    for (size_t i = 0; i < words256; i++) {
        acc.m256 |= data[i * stride256];
    }
    for (size_t k = 0; k < 4; k++) {
        if (acc.u64[k]) {
            return true;
        }
    }
    return false;
}
