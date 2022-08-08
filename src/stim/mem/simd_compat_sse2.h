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

#include <algorithm>
#include <immintrin.h>

#include "stim/mem/simd_util.h"

namespace stim {

#define simd_word simd_word_sse2
struct simd_word_sse2 {
    constexpr static size_t BIT_SIZE = 128;
    constexpr static size_t BIT_POW = 7;

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

    inline uint16_t popcount() const {
        return popcnt64(u64[0]) + popcnt64(u64[1]);
    }

    template <uint64_t shift>
    static void inplace_transpose_block_pass(simd_word *data, size_t stride, __m128i mask) {
        for (size_t k = 0; k < 128; k++) {
            if (k & shift) {
                continue;
            }
            simd_word& x = data[stride * k];
            simd_word& y = data[stride * (k + shift)];
            simd_word a = x & mask;
            simd_word b = x & ~mask;
            simd_word c = y & mask;
            simd_word d = y & ~mask;
            x = a | simd_word(_mm_slli_epi64(c.val, shift));
            y = simd_word(_mm_srli_epi64(b.val, shift)) | d;
        }
    }

    static void inplace_transpose_block_pass64(simd_word *data, size_t stride) {
        uint64_t *ptr = (uint64_t *)data;
        stride <<= 1;
        for (size_t k = 0; k < 64; k++) {
            std::swap(ptr[stride * k + 1], ptr[stride * (k + 64)]);
        }
    }

    static void inplace_transpose_square(simd_word *data, size_t stride) {
        inplace_transpose_block_pass<1>(data, stride, _mm_set1_epi8(0x55));
        inplace_transpose_block_pass<2>(data, stride, _mm_set1_epi8(0x33));
        inplace_transpose_block_pass<4>(data, stride, _mm_set1_epi8(0xF));
        inplace_transpose_block_pass<8>(data, stride, _mm_set1_epi16(0xFF));
        inplace_transpose_block_pass<16>(data, stride, _mm_set1_epi32(0xFFFF));
        inplace_transpose_block_pass<32>(data, stride, _mm_set1_epi64x(0xFFFFFFFF));
        inplace_transpose_block_pass64(data, stride);
    }
};

}  // namespace stim

#endif
