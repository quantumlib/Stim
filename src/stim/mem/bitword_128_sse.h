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
#ifdef __SSE2__

#include <algorithm>
#include <array>
#include <bit>
#include <immintrin.h>
#include <sstream>
#include <stdexcept>

#include "stim/mem/bitword.h"
#include "stim/mem/simd_util.h"

namespace stim {

/// Implements a 128 bit bitword using SSE instructions.
template <>
struct bitword<128> {
    constexpr static size_t BIT_SIZE = 128;
    constexpr static size_t BIT_POW = 7;

    union {
        __m128i val;
        uint8_t u8[16];
        uint64_t u64[2];
    };

    static void *aligned_malloc(size_t bytes) {
        return _mm_malloc(bytes, sizeof(__m128i));
    }
    static void aligned_free(void *ptr) {
        _mm_free(ptr);
    }

    inline bitword() : val(__m128i{}) {
    }
    inline bitword(__m128i val) : val(val) {
    }
    inline bitword(std::array<uint64_t, 2> val) : val{_mm_set_epi64x(val[1], val[0])} {
    }
    inline bitword(uint64_t val) : val{_mm_set_epi64x(0, val)} {
    }
    inline bitword(int64_t val) : val{_mm_set_epi64x(-(val < 0), val)} {
    }
    inline bitword(int val) : val{_mm_set_epi64x(-(val < 0), val)} {
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

    inline std::array<uint64_t, 2> to_u64_array() const {
        // I would use std::bit_cast here, but it failed to build in CI.

        // I would use '_mm_extract_epi64' here, but it failed to build in CI when using `-O3`.
        // Failures were on linux systems with gcc 12.2.0

        uint64_t w0 = u64[0];
        uint64_t w1 = u64[1];
        return std::array<uint64_t, 2>{(uint64_t)w0, (uint64_t)w1};
    }
    inline operator bool() const {  // NOLINT(hicpp-explicit-conversions)
        auto words = to_u64_array();
        return (bool)(words[0] | words[1]);
    }
    inline operator int() const {  // NOLINT(hicpp-explicit-conversions)
        return (int64_t)*this;
    }
    inline operator uint64_t() const {  // NOLINT(hicpp-explicit-conversions)
        auto words = to_u64_array();
        if (words[1]) {
            throw std::invalid_argument("Too large for uint64_t");
        }
        return words[0];
    }
    inline operator int64_t() const {  // NOLINT(hicpp-explicit-conversions)
        auto words = to_u64_array();
        int64_t result = (int64_t)words[0];
        uint64_t expected = result < 0 ? (uint64_t)-1 : (uint64_t)0;
        if (words[1] != expected) {
            throw std::invalid_argument("Out of bounds of int64_t");
        }
        return result;
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
        auto words = to_u64_array();
        return std::popcount(words[0]) + std::popcount(words[1]);
    }

    inline bitword<128> shifted(int offset) const {
        auto w = to_u64_array();
        while (offset <= -64) {
            w[0] = w[1];
            w[1] = 0;
            offset += 64;
        }
        while (offset >= 64) {
            w[1] = w[0];
            w[0] = 0;
            offset -= 64;
        }
        __m128i low2high;
        __m128i high2low;
        if (offset < 0) {
            low2high = _mm_set_epi64x(0, w[1]);
            high2low = _mm_set_epi64x(w[1], w[0]);
            offset += 64;
        } else {
            low2high = _mm_set_epi64x(w[1], w[0]);
            high2low = _mm_set_epi64x(w[0], 0);
        }
        uint64_t m = (uint64_t{1} << offset) - uint64_t{1};
        low2high = _mm_slli_epi64(low2high, offset);
        high2low = _mm_srli_epi64(high2low, 64 - offset);
        low2high = _mm_and_si128(low2high, _mm_set1_epi64x(~m));
        high2low = _mm_and_si128(high2low, _mm_set1_epi64x(m));
        return _mm_or_si128(low2high, high2low);
    }

    inline std::string str() const {
        std::stringstream out;
        out << *this;
        return out.str();
    }

    template <uint64_t shift>
    static void inplace_transpose_block_pass(bitword<128> *data, size_t stride, __m128i mask) {
        for (size_t k = 0; k < 128; k++) {
            if (k & shift) {
                continue;
            }
            bitword<128> &x = data[stride * k];
            bitword<128> &y = data[stride * (k + shift)];
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

}  // namespace stim

#endif
#endif
