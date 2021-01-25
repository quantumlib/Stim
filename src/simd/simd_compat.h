#ifndef SIMD_COMPAT_H
#define SIMD_COMPAT_H

#include <cstdint>
#include <immintrin.h>
#include <iostream>

#define SIMD_WORD_TYPE simd_word_256

// HACK: use "Substitution failure is not an error" to use _mm_popcnt_u64 if available.
// Otherwise fallback to _mm_popcnt_u32.
template <typename T> static decltype(_mm_popcnt_u64(0)) popcount_fallback_select(uint64_t arg) {
    return _mm_popcnt_u64(arg);
}
template <typename T> static decltype(_mm_popcnt_u32(0)) popcount_fallback_select(uint64_t arg) {
    return _mm_popcnt_u32((uint32_t)arg) + _mm_popcnt_u32((uint32_t)(arg >> 32));
}
inline uint8_t popcnt(uint64_t value) {
    return (uint8_t) popcount_fallback_select<void>(value);
}

struct simd_word_256 {
    __m256i val;

    inline static simd_word_256 tile8(uint8_t pattern) {
        return {_mm256_set1_epi8(pattern)};
    }

    inline static simd_word_256 tile16(uint16_t pattern) {
        return {_mm256_set1_epi16(pattern)};
    }

    inline static simd_word_256 tile32(uint32_t pattern) {
        return {_mm256_set1_epi32(pattern)};
    }

    inline static simd_word_256 tile64(uint64_t pattern) {
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

    inline simd_word_256 leftshift_tile64(uint8_t offset) {
        return {_mm256_slli_epi64(val, offset)};
    }

    inline simd_word_256 rightshift_tile64(uint8_t offset) {
        return {_mm256_srli_epi64(val, offset)};
    }

    inline uint16_t popcount() const {
        auto p = (uint64_t *)&val;
        return popcnt(p[0]) + popcnt(p[1]) + popcnt(p[2]) + (uint16_t)popcnt(p[3]);
    }

    /// For each 128 bit word pair between the two registers, the byte order goes from this:
    /// [a0 a1 a2 a3 ... a14 a15] [b0 b1 b2 b3 ... b14 b15]
    /// to this:
    /// [a0 b0 a1 b1 ...  a7  b7] [a8 b8 a9 b9 ... a15 b15]
    inline void do_interleave8_tile128(simd_word_256 &other) {
        auto t = _mm256_unpackhi_epi8(val, other.val);
        val = _mm256_unpacklo_epi8(val, other.val);
        other.val = t;
    }
};

struct simd_word_128 {
    __m128i val;

    inline static simd_word_128 tile8(uint8_t pattern) {
        return {_mm_set1_epi8(pattern)};
    }

    inline static simd_word_128 tile16(uint16_t pattern) {
        return {_mm_set1_epi16(pattern)};
    }

    inline static simd_word_128 tile32(uint32_t pattern) {
        return {_mm_set1_epi32(pattern)};
    }

    inline static simd_word_128 tile64(uint64_t pattern) {
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
        return popcnt(p[0]) + popcnt(p[1]);
    }

    /// For each 128 bit word pair between the two registers, the byte order goes from this:
    /// [a0 a1 a2 a3 ... a14 a15] [b0 b1 b2 b3 ... b14 b15]
    /// to this:
    /// [a0 b0 a1 b1 ...  a7  b7] [a8 b8 a9 b9 ... a15 b15]
    inline void do_interleave8_tile128(simd_word_128 &other) {
        auto t = _mm_unpackhi_epi8(val, other.val);
        val = _mm_unpacklo_epi8(val, other.val);
        other.val = t;
    }
};

struct simd_word_64 {
    uint64_t val;

    constexpr inline static simd_word_64 tile8(uint8_t pattern) {
        uint64_t result = pattern;
        result |= result << 8;
        result |= result << 16;
        result |= result << 32;
        return {result};
    }

    constexpr inline static simd_word_64 tile16(uint16_t pattern) {
        uint64_t result = pattern;
        result |= result << 16;
        result |= result << 32;
        return {result};
    }

    constexpr inline static simd_word_64 tile32(uint32_t pattern) {
        uint64_t result = pattern;
        result |= result << 32;
        return {result};
    }

    constexpr inline static simd_word_64 tile64(uint64_t pattern) {
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
        return popcnt(val);
    }
};

#endif
