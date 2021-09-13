/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STIM_MEM_SIMD_COMPAT_SSE2_H
#define _STIM_MEM_SIMD_COMPAT_SSE2_H

/// Implements `simd_word` using SSE+SSE2 intrinsic instructions.
/// For example, `_mm_set1_epi8` is SSE2.

#include <immintrin.h>

#include "stim/mem/simd_util.h"

namespace stim {

#define simd_word simd_word_sse2
struct simd_word_sse2 {
    union {
        __m128i val;
        __m128i u128[1];
        uint64_t u64[2];
        uint8_t u8[16];
    };

    static void *aligned_malloc(size_t bytes) {
        return _mm_malloc(bytes, sizeof(simd_word));
    }
    static void aligned_free(void *ptr) {
        _mm_free(ptr);
    }

    inline simd_word() : val(__m128i{}) {
    }
    inline simd_word(__m128i val) : val(val) {
    }

    inline static simd_word tile8(uint8_t pattern) {
        return {_mm_set1_epi8(pattern)};
    }

    inline static simd_word tile16(uint16_t pattern) {
        return {_mm_set1_epi16(pattern)};
    }

    inline static simd_word tile32(uint32_t pattern) {
        return {_mm_set1_epi32(pattern)};
    }

    inline static simd_word tile64(uint64_t pattern) {
        return {_mm_set1_epi64x(pattern)};
    }

    inline operator bool() const {  // NOLINT(hicpp-explicit-conversions)
        return u64[0] | u64[1];
    }

    inline simd_word &operator^=(const simd_word &other) {
        val = _mm_xor_si128(val, other.val);
        return *this;
    }

    inline simd_word &operator&=(const simd_word &other) {
        val = _mm_and_si128(val, other.val);
        return *this;
    }

    inline simd_word &operator|=(const simd_word &other) {
        val = _mm_or_si128(val, other.val);
        return *this;
    }

    inline simd_word operator^(const simd_word &other) const {
        return {_mm_xor_si128(val, other.val)};
    }

    inline simd_word operator&(const simd_word &other) const {
        return {_mm_and_si128(val, other.val)};
    }

    inline simd_word operator|(const simd_word &other) const {
        return {_mm_or_si128(val, other.val)};
    }

    inline simd_word andnot(const simd_word &other) const {
        return {_mm_andnot_si128(val, other.val)};
    }

    inline simd_word leftshift_tile64(uint8_t offset) const {
        return {_mm_slli_epi64(val, offset)};
    }

    inline simd_word rightshift_tile64(uint8_t offset) const {
        return {_mm_srli_epi64(val, offset)};
    }

    inline uint16_t popcount() const {
        return popcnt64(u64[0]) + popcnt64(u64[1]);
    }

    /// For each 128 bit word pair between the two registers, the byte order goes from this:
    /// [a0 a1 a2 a3 ... a14 a15] [b0 b1 b2 b3 ... b14 b15]
    /// to this:
    /// [a0 b0 a1 b1 ...  a7  b7] [a8 b8 a9 b9 ... a15 b15]
    inline void do_interleave8_tile128(simd_word &other) {
        auto t = _mm_unpackhi_epi8(val, other.val);
        val = _mm_unpacklo_epi8(val, other.val);
        other.val = t;
    }
};

}  // namespace stim

#endif
