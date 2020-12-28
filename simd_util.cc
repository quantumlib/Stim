#include "simd_util.h"

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
