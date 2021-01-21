#ifndef SIMD_COMPAT_H
#define SIMD_COMPAT_H

#include <bit>
#include <cstdint>
#include <immintrin.h>

#define SIMD_WORD_TYPE simd_word_256

struct simd_word_256 {
    __m256i val;

    inline static simd_word_256 tile(uint8_t pattern) {
        return {_mm256_set1_epi8(pattern)};
    }

    inline static simd_word_256 tile(uint16_t pattern) {
        return {_mm256_set1_epi16(pattern)};
    }

    inline static simd_word_256 tile(uint32_t pattern) {
        return {_mm256_set1_epi32(pattern)};
    }

    inline static simd_word_256 tile(uint64_t pattern) {
        return {_mm256_set1_epi64x(pattern)};
    }

    inline operator bool() const { // NOLINT(hicpp-explicit-conversions)
        auto p = (uint64_t *)&val;
        return p[0] | p[1] | p[2] | p[3];
    }

    inline simd_word_256 &operator^=(const simd_word_256 &other) {
        val = _mm256_xor_si256(val, other.val);
        return *this;
    }

    inline simd_word_256 &operator&=(const simd_word_256 &other) {
        val = _mm256_and_si256(val, other.val);
        return *this;
    }

    inline simd_word_256 &operator|=(const simd_word_256 &other) {
        val = _mm256_or_si256(val, other.val);
        return *this;
    }

    inline simd_word_256 operator^(const simd_word_256 &other) const {
        return {_mm256_xor_si256(val, other.val)};
    }

    inline simd_word_256 operator&(const simd_word_256 &other) const {
        return {_mm256_and_si256(val, other.val)};
    }

    inline simd_word_256 operator|(const simd_word_256 &other) const {
        return {_mm256_or_si256(val, other.val)};
    }

    inline simd_word_256 andnot(const simd_word_256 &other) const {
        return {_mm256_andnot_si256(val, other.val)};
    }

    inline uint16_t popcount() const {
        auto p = (uint64_t *)&val;
        return std::popcount(p[0]) + std::popcount(p[1]) + std::popcount(p[2]) + std::popcount(p[3]);
    }
};

struct simd_word_128 {
    __m128i val;

    inline static simd_word_128 tile(uint8_t pattern) {
        return {_mm_set1_epi8(pattern)};
    }

    inline static simd_word_128 tile(uint16_t pattern) {
        return {_mm_set1_epi16(pattern)};
    }

    inline static simd_word_128 tile(uint32_t pattern) {
        return {_mm_set1_epi32(pattern)};
    }

    inline static simd_word_128 tile(uint64_t pattern) {
        return {_mm_set1_epi64x(pattern)};
    }

    inline operator bool() const { // NOLINT(hicpp-explicit-conversions)
        auto p = (uint64_t *)&val;
        return p[0] | p[1] | p[2] | p[3];
    }

    inline simd_word_128 &operator^=(const simd_word_128 &other) {
        val = _mm_xor_si128(val, other.val);
        return *this;
    }

    inline simd_word_128 &operator&=(const simd_word_128 &other) {
        val = _mm_and_si128(val, other.val);
        return *this;
    }

    inline simd_word_128 &operator|=(const simd_word_128 &other) {
        val = _mm_or_si128(val, other.val);
        return *this;
    }

    inline simd_word_128 operator^(const simd_word_128 &other) const {
        return {_mm_xor_si128(val, other.val)};
    }

    inline simd_word_128 operator&(const simd_word_128 &other) const {
        return {_mm_and_si128(val, other.val)};
    }

    inline simd_word_128 operator|(const simd_word_128 &other) const {
        return {_mm_or_si128(val, other.val)};
    }

    inline simd_word_128 andnot(const simd_word_128 &other) const {
        return {_mm_andnot_si128(val, other.val)};
    }

    inline uint16_t popcount() const {
        auto p = (uint64_t *)&val;
        return std::popcount(p[0]) + std::popcount(p[1]);
    }
};

struct simd_word_64 {
    uint64_t val;

    constexpr inline static simd_word_64 tile(uint8_t pattern) {
        uint64_t result = pattern;
        result |= result << 8;
        result |= result << 16;
        result |= result << 32;
        return {result};
    }

    constexpr inline static simd_word_64 tile(uint16_t pattern) {
        uint64_t result = pattern;
        result |= result << 16;
        result |= result << 32;
        return {result};
    }

    constexpr inline static simd_word_64 tile(uint32_t pattern) {
        uint64_t result = pattern;
        result |= result << 32;
        return {result};
    }

    constexpr inline static simd_word_64 tile(uint64_t pattern) {
        return {pattern};
    }

    inline operator bool() const { // NOLINT(hicpp-explicit-conversions)
        auto p = (uint64_t *)&val;
        return p[0] | p[1] | p[2] | p[3];
    }

    inline simd_word_64 &operator^=(const simd_word_64 &other) {
        val ^= other.val;
        return *this;
    }

    inline simd_word_64 &operator&=(const simd_word_64 &other) {
        val &= other.val;
        return *this;
    }

    inline simd_word_64 &operator|=(const simd_word_64 &other) {
        val |= other.val;
        return *this;
    }

    inline simd_word_64 operator^(const simd_word_64 &other) const {
        return {val ^ other.val};
    }

    inline simd_word_64 operator&(const simd_word_64 &other) const {
        return {val & other.val};
    }

    inline simd_word_64 operator|(const simd_word_64 &other) const {
        return {val | other.val};
    }

    inline simd_word_64 andnot(const simd_word_64 &other) const {
        return {~val & other.val};
    }

    inline uint16_t popcount() const {
        return std::popcount(val);
    }
};

#endif
