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

#ifndef _STIM_MEM_SIMD_WORD_256_AVX_H
#define _STIM_MEM_SIMD_WORD_256_AVX_H
#if __AVX2__

#include <array>
#include <bit>
#include <immintrin.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "stim/mem/bitword.h"
#include "stim/mem/simd_util.h"

namespace stim {

/// Implements a 256 bit bitword using AVX instructions.
template <>
struct bitword<256> {
    constexpr static size_t BIT_SIZE = 256;
    constexpr static size_t BIT_POW = 8;

    union {
        __m256i val;
        uint8_t u8[32];
    };

    static void *aligned_malloc(size_t bytes) {
        return _mm_malloc(bytes, sizeof(__m256i));
    }
    static void aligned_free(void *ptr) {
        _mm_free(ptr);
    }

    inline bitword() : val(__m256i{}) {
    }
    inline bitword(__m256i val) : val(val) {
    }
    inline bitword(std::array<uint64_t, 4> val) : val{_mm256_set_epi64x(val[3], val[2], val[1], val[0])} {
    }
    inline bitword(uint64_t val) : val{_mm256_set_epi64x(0, 0, 0, val)} {
    }
    inline bitword(int64_t val) : val{_mm256_set_epi64x(-(val < 0), -(val < 0), -(val < 0), val)} {
    }
    inline bitword(int val) : val{_mm256_set_epi64x(-(val < 0), -(val < 0), -(val < 0), val)} {
    }

    inline static bitword<256> tile8(uint8_t pattern) {
        return {_mm256_set1_epi8(pattern)};
    }

    inline static bitword<256> tile16(uint16_t pattern) {
        return {_mm256_set1_epi16(pattern)};
    }

    inline static bitword<256> tile32(uint32_t pattern) {
        return {_mm256_set1_epi32(pattern)};
    }

    inline static bitword<256> tile64(uint64_t pattern) {
        return {_mm256_set1_epi64x(pattern)};
    }

    inline std::array<uint64_t, 4> to_u64_array() const {
        return std::array<uint64_t, 4>{
            (uint64_t)_mm256_extract_epi64(val, 0),
            (uint64_t)_mm256_extract_epi64(val, 1),
            (uint64_t)_mm256_extract_epi64(val, 2),
            (uint64_t)_mm256_extract_epi64(val, 3),
        };
    }

    inline operator bool() const {  // NOLINT(hicpp-explicit-conversions)
        auto words = to_u64_array();
        return (bool)(words[0] | words[1] | words[2] | words[3]);
    }
    inline operator int() const {  // NOLINT(hicpp-explicit-conversions)
        return (int64_t)*this;
    }
    inline operator uint64_t() const {  // NOLINT(hicpp-explicit-conversions)
        auto words = to_u64_array();
        if (words[1] || words[2] || words[3]) {
            throw std::invalid_argument("Too large for uint64_t");
        }
        return words[0];
    }
    inline operator int64_t() const {  // NOLINT(hicpp-explicit-conversions)
        auto words = to_u64_array();
        int64_t result = (int64_t)words[0];
        uint64_t expected = result < 0 ? (uint64_t)-1 : (uint64_t)0;
        if (words[1] != expected || words[2] != expected || words[3] != expected) {
            throw std::invalid_argument("Out of bounds of int64_t");
        }
        return result;
    }

    inline bitword<256> &operator^=(const bitword<256> &other) {
        val = _mm256_xor_si256(val, other.val);
        return *this;
    }

    inline bitword<256> &operator&=(const bitword<256> &other) {
        val = _mm256_and_si256(val, other.val);
        return *this;
    }

    inline bitword<256> &operator|=(const bitword<256> &other) {
        val = _mm256_or_si256(val, other.val);
        return *this;
    }

    inline bitword<256> operator^(const bitword<256> &other) const {
        return {_mm256_xor_si256(val, other.val)};
    }

    inline bitword<256> operator&(const bitword<256> &other) const {
        return {_mm256_and_si256(val, other.val)};
    }

    inline bitword<256> operator|(const bitword<256> &other) const {
        return {_mm256_or_si256(val, other.val)};
    }

    inline bitword<256> andnot(const bitword<256> &other) const {
        return {_mm256_andnot_si256(val, other.val)};
    }

    inline uint16_t popcount() const {
        auto v = to_u64_array();
        return std::popcount(v[0]) + std::popcount(v[1]) + std::popcount(v[2]) + (uint16_t)std::popcount(v[3]);
    }

    inline bitword<256> shifted(int offset) const {
        auto w = to_u64_array();
        while (offset <= -64) {
            w[0] = w[1];
            w[1] = w[2];
            w[2] = w[3];
            w[3] = 0;
            offset += 64;
        }
        while (offset >= 64) {
            w[3] = w[2];
            w[2] = w[1];
            w[1] = w[0];
            w[0] = 0;
            offset -= 64;
        }
        __m256i low2high;
        __m256i high2low;
        if (offset < 0) {
            low2high = _mm256_set_epi64x(0, w[3], w[2], w[1]);
            high2low = _mm256_set_epi64x(w[3], w[2], w[1], w[0]);
            offset += 64;
        } else {
            low2high = _mm256_set_epi64x(w[3], w[2], w[1], w[0]);
            high2low = _mm256_set_epi64x(w[2], w[1], w[0], 0);
        }
        uint64_t m = (uint64_t{1} << offset) - uint64_t{1};
        low2high = _mm256_slli_epi64(low2high, offset);
        high2low = _mm256_srli_epi64(high2low, 64 - offset);
        low2high = _mm256_and_si256(low2high, _mm256_set1_epi64x(~m));
        high2low = _mm256_and_si256(high2low, _mm256_set1_epi64x(m));
        return _mm256_or_si256(low2high, high2low);
    }

    inline std::string str() const {
        std::stringstream out;
        out << *this;
        return out.str();
    }

    inline bool operator==(const bitword<256> &other) const {
        return to_u64_array() == other.to_u64_array();
    }
    inline bool operator!=(const bitword<256> &other) const {
        return !(*this == other);
    }
    inline bool operator==(int other) const {
        return *this == (bitword<256>)other;
    }
    inline bool operator!=(int other) const {
        return *this != (bitword<256>)other;
    }
    inline bool operator==(uint64_t other) const {
        return *this == (bitword<256>)other;
    }
    inline bool operator!=(uint64_t other) const {
        return *this != (bitword<256>)other;
    }
    inline bool operator==(int64_t other) const {
        return *this == (bitword<256>)other;
    }
    inline bool operator!=(int64_t other) const {
        return *this != (bitword<256>)other;
    }

    template <uint64_t shift>
    static void inplace_transpose_block_pass(bitword<256> *data, size_t stride, __m256i mask) {
        for (size_t k = 0; k < 256; k++) {
            if (k & shift) {
                continue;
            }
            bitword<256> &x = data[stride * k];
            bitword<256> &y = data[stride * (k + shift)];
            bitword<256> a = x & mask;
            bitword<256> b = x & ~mask;
            bitword<256> c = y & mask;
            bitword<256> d = y & ~mask;
            x = a | bitword<256>(_mm256_slli_epi64(c.val, shift));
            y = bitword<256>(_mm256_srli_epi64(b.val, shift)) | d;
        }
    }

    static void inplace_transpose_block_pass_64_and_128(bitword<256> *data, size_t stride) {
        uint64_t *ptr = (uint64_t *)data;
        stride <<= 2;

        for (size_t k = 0; k < 64; k++) {
            std::swap(ptr[stride * (k + 64 * 0) + 1], ptr[stride * (k + 64 * 1) + 0]);
            std::swap(ptr[stride * (k + 64 * 0) + 2], ptr[stride * (k + 64 * 2) + 0]);
            std::swap(ptr[stride * (k + 64 * 0) + 3], ptr[stride * (k + 64 * 3) + 0]);
            std::swap(ptr[stride * (k + 64 * 1) + 2], ptr[stride * (k + 64 * 2) + 1]);
            std::swap(ptr[stride * (k + 64 * 1) + 3], ptr[stride * (k + 64 * 3) + 1]);
            std::swap(ptr[stride * (k + 64 * 2) + 3], ptr[stride * (k + 64 * 3) + 2]);
        }
    }

    static void inplace_transpose_square(bitword<256> *data, size_t stride) {
        inplace_transpose_block_pass<1>(data, stride, _mm256_set1_epi8(0x55));
        inplace_transpose_block_pass<2>(data, stride, _mm256_set1_epi8(0x33));
        inplace_transpose_block_pass<4>(data, stride, _mm256_set1_epi8(0xF));
        inplace_transpose_block_pass<8>(data, stride, _mm256_set1_epi16(0xFF));
        inplace_transpose_block_pass<16>(data, stride, _mm256_set1_epi32(0xFFFF));
        inplace_transpose_block_pass<32>(data, stride, _mm256_set1_epi64x(0xFFFFFFFF));
        inplace_transpose_block_pass_64_and_128(data, stride);
    }
};

}  // namespace stim

#endif
#endif
