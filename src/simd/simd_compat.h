#ifndef SIMD_COMPAT_H
#define SIMD_COMPAT_H

#include <bit>
#include <cstdint>
#include <immintrin.h>

#define SIMD_WORD_TYPE __m256i

inline uint16_t popcount(const __m256i &val) {
    auto p = (uint64_t *)&val;
    return std::popcount(p[0]) + std::popcount(p[1]) + std::popcount(p[2]) + std::popcount(p[3]);
}

inline uint8_t popcount(const __m128i &val) {
    auto p = (uint64_t *)&val;
    return std::popcount(p[0]) + std::popcount(p[1]);
}

inline uint8_t popcount(const uint64_t &val) {
    return std::popcount(val);
}

inline __m256i andnot(const __m256i &complemented, const __m256i &kept) {
    return _mm256_andnot_si256(complemented, kept);
}

inline __m128i andnot(const __m128i &complemented, const __m128i &kept) {
    return _mm_andnot_si128(complemented, kept);
}

inline uint64_t andnot(const uint64_t &complemented, const uint64_t &kept) {
    return ~complemented & kept;
}

#endif
