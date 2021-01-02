#include "simd_util.h"
#include <sstream>

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

__m256i popcnt16(__m256i r) {
    r = popcnt8(r);
    auto m = _mm256_set1_epi16(0x00FF);
    auto low = _mm256_and_si256(m, r);
    auto high = _mm256_srli_epi16(_mm256_andnot_si256(m, r), 8);
    return _mm256_add_epi16(low, high);
}

std::vector<bool> m256i_to_bits(__m256i r) {
    std::vector<bool> result;
    for (size_t k = 0; k < 4; k++) {
        auto e = m256i_u64(r)[k];
        for (size_t i = 0; i < 64; i++) {
            result.push_back((e >> i) & 1);
        }
    }
    return result;
}

const uint64_t *m256i_u64(const __m256i &result) {
    return (uint64_t *)&result;
}

uint64_t *m256i_u64(__m256i &result) {
    return (uint64_t *)&result;
}

uint16_t *m256i_u16(__m256i &result) {
    return (uint16_t *)&result;
}

__m256i bits_to_m256i(std::vector<bool> data) {
    __m256i result {};
    for (size_t i = 0; i < data.size(); i++) {
        m256i_u64(result)[i >> 6] |= (uint64_t)data[i] << (i & 63);
    }
    return result;
}

std::string hex(__m256i data) {
    std::stringstream out;
    auto chars = ".123456789ABCDEF";
    for (size_t w = 0; w < 4; w++) {
        if (w) {
            out << " ";
        }
        for (size_t i = 64; i > 0; i -= 4) {
            out << chars[(m256i_u64(data)[w] >> (i - 4)) & 0xF];
        }
    }
    return out.str();
}

std::string bin(__m256i data) {
    std::stringstream out;
    auto chars = "01";
    for (size_t w = 0; w < 4; w++) {
        if (w) {
            out << " ";
        }
        for (size_t i = 64; i > 0; i--) {
            out << chars[(m256i_u64(data)[w] >> (i - 1)) & 0x1];
        }
    }
    return out.str();
}

template <uint8_t h>
void transpose256_helper(BitsPtr matrix, __m256i mask) {
    for (size_t i = 0; i < 256; i += h << 1) {
        for (size_t j = i; j < i + h; j++) {
            auto &a = matrix.u256[j];
            auto &b = matrix.u256[j + h];
            auto a1 = _mm256_andnot_si256(mask, a);
            auto b0 = _mm256_and_si256(mask, b);
            a = _mm256_and_si256(mask, a);
            b = _mm256_andnot_si256(mask, b);
            a = _mm256_or_si256(a, _mm256_slli_epi64(b0, h));
            b = _mm256_or_si256(_mm256_srli_epi64(a1, h), b);
        }
    }
}

void transpose256(BitsPtr matrix) {
    transpose256_helper<1>(matrix, _mm256_set1_epi8(0x55));
    transpose256_helper<2>(matrix, _mm256_set1_epi8(0x33));
    transpose256_helper<4>(matrix, _mm256_set1_epi8(0xF));
    transpose256_helper<8>(matrix, _mm256_set1_epi16(0xFF));
    transpose256_helper<16>(matrix, _mm256_set1_epi32(0xFFFF));
    transpose256_helper<32>(matrix, _mm256_set1_epi64x(0xFFFFFFFF));
    for (size_t s0 = 0; s0 < 4; s0++) {
        for (size_t s1 = s0 + 1; s1 < 4; s1++) {
            size_t i0 = s0 | (s1 << 8);
            size_t i1 = s1 | (s0 << 8);
            for (size_t m = 0; m < 256; m += 4) {
                size_t j0 = i0 | m;
                size_t j1 = i1 | m;
                std::swap(matrix.u64[j0], matrix.u64[j1]);
            }
        }
    }
}
