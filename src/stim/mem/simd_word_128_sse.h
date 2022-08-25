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

#ifndef _STIM_MEM_SIMD_WORD_128_SSE_H
#define _STIM_MEM_SIMD_WORD_128_SSE_H

/// Implements `simd_word` using SSE+SSE2 intrinsic instructions.
/// For example, `_mm_set1_epi8` is SSE2.

#include <algorithm>
#include <immintrin.h>

#include "stim/mem/simd_util.h"

namespace stim {

template <size_t bit_size>
struct bitword;

#ifdef __SSE2__

/// Implements a 128 bit bitword using SSE instructions.
template <>
struct bitword<128> {
    constexpr static size_t BIT_SIZE = 128;
    constexpr static size_t BIT_POW = 7;

    union {
        __m128i val;
        uint64_t u64[2];
        uint8_t u8[16];
    };

    static void *aligned_malloc(size_t bytes) {
        return _mm_malloc(bytes, sizeof(__m128i));
    }
    static void aligned_free(void *ptr) {
        _mm_free(ptr);
    }

    inline bitword<128>() : val(__m128i{}) {
    }
    inline bitword<128>(__m128i val) : val(val) {
    }

    inline static bitword<128> tile8(uint8_t pattern) {
        return {_mm_set1_epi8(pattern)};
    }

    inline static bitword<128> tile16(uint16_t pattern) {
        return {_mm_set1_epi16(pattern)};
    }

    inline static bitword<128> tile32(uint32_t pattern) {
        return {_mm_set1_epi32(pattern)};
    }

    inline static bitword<128> tile64(uint64_t pattern) {
        return {_mm_set1_epi64x(pattern)};
    }

    inline operator bool() const {  // NOLINT(hicpp-explicit-conversions)
        return u64[0] | u64[1];
    }

    inline bitword<128> &operator^=(const bitword<128> &other) {
        val = _mm_xor_si128(val, other.val);
        return *this;
    }

    inline bitword<128> &operator&=(const bitword<128> &other) {
        val = _mm_and_si128(val, other.val);
        return *this;
    }

    inline bitword<128> &operator|=(const bitword<128> &other) {
        val = _mm_or_si128(val, other.val);
        return *this;
    }

    inline bitword<128> operator^(const bitword<128> &other) const {
        return {_mm_xor_si128(val, other.val)};
    }

    inline bitword<128> operator&(const bitword<128> &other) const {
        return {_mm_and_si128(val, other.val)};
    }

    inline bitword<128> operator|(const bitword<128> &other) const {
        return {_mm_or_si128(val, other.val)};
    }

    inline bitword<128> andnot(const bitword<128> &other) const {
        return {_mm_andnot_si128(val, other.val)};
    }

    inline uint16_t popcount() const {
        return popcnt64(u64[0]) + popcnt64(u64[1]);
    }

    template <uint64_t shift>
    static void inplace_transpose_block_pass(bitword<128> *data, size_t stride, __m128i mask) {
        for (size_t k = 0; k < 128; k++) {
            if (k & shift) {
                continue;
            }
            bitword<128>& x = data[stride * k];
            bitword<128>& y = data[stride * (k + shift)];
            bitword<128> a = x & mask;
            bitword<128> b = x & ~mask;
            bitword<128> c = y & mask;
            bitword<128> d = y & ~mask;
            x = a | bitword<128>(_mm_slli_epi64(c.val, shift));
            y = bitword<128>(_mm_srli_epi64(b.val, shift)) | d;
        }
    }

    static void inplace_transpose_block_pass64(bitword<128> *data, size_t stride) {
        uint64_t *ptr = (uint64_t *)data;
        stride <<= 1;
        for (size_t k = 0; k < 64; k++) {
            std::swap(ptr[stride * k + 1], ptr[stride * (k + 64)]);
        }
    }

    static void inplace_transpose_square(bitword<128> *data, size_t stride) {
        inplace_transpose_block_pass<1>(data, stride, _mm_set1_epi8(0x55));
        inplace_transpose_block_pass<2>(data, stride, _mm_set1_epi8(0x33));
        inplace_transpose_block_pass<4>(data, stride, _mm_set1_epi8(0xF));
        inplace_transpose_block_pass<8>(data, stride, _mm_set1_epi16(0xFF));
        inplace_transpose_block_pass<16>(data, stride, _mm_set1_epi32(0xFFFF));
        inplace_transpose_block_pass<32>(data, stride, _mm_set1_epi64x(0xFFFFFFFF));
        inplace_transpose_block_pass64(data, stride);
    }
};
#endif

}  // namespace stim

#endif
