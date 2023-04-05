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

#ifndef _STIM_MEM_SIMD_WORD_512_AVX512_H
#define _STIM_MEM_SIMD_WORD_512_AVX512_H

/// Implements `simd_word` using AVX512 intrinsic instructions.
/// For example, `_mm256_xor_si256` is AVX2.

#include <immintrin.h>
#include <iostream>

#include "stim/mem/simd_util.h"

namespace stim {

template <size_t bit_size>
struct bitword;

#if __AVX512__

/// Implements a 512 bit bitword using AVX instructions.
template <>
struct bitword<512> {
    constexpr static size_t BIT_SIZE = 512;
    constexpr static size_t BIT_POW = 9; 

    union {
        __m512i val;
        uint64_t u64[8];
        uint8_t u8[64];
    };

    static void *aligned_malloc(size_t bytes) {
        return _mm_malloc(bytes, sizeof(__m512i));
    }
    static void aligned_free(void *ptr) {
        _mm_free(ptr);
    }

    inline bitword<512>() : val(__m512i{}) {
    }
    inline bitword<512>(__m512i val) : val(val) {
    }

    inline static bitword<512> tile8(uint8_t pattern) {
        return {_mm512_set1_epi8(pattern)};
    }

    inline static bitword<512> tile16(uint16_t pattern) {
        return {_mm512_set1_epi16(pattern)};
    }

    inline static bitword<512> tile32(uint32_t pattern) {
        return {_mm512_set1_epi32(pattern)};
    }

    inline static bitword<512> tile64(uint64_t pattern) {
        return {_mm512_set1_epi64x(pattern)};
    }

    inline operator bool() const {  // NOLINT(hicpp-explicit-conversions)
        return u64[0] | u64[1] | u64[2] | u64[3] | u64[4] | u64[5] | u64[6] | u64[7];
    }

    inline bitword<512> &operator^=(const bitword<512> &other) {
        val = _mm512_xor_si512(val, other.val);
        return *this;
    }

    inline bitword<512> &operator&=(const bitword<512> &other) {
        val = _mm512_and_si512(val, other.val);
        return *this;
    }

    inline bitword<512> &operator|=(const bitword<512> &other) {
        val = _mm512_or_si512(val, other.val);
        return *this;
    }

    inline bitword<512> operator^(const bitword<512> &other) const {
        return {_mm512_xor_si512(val, other.val)};
    }

    inline bitword<512> operator&(const bitword<512> &other) const {
        return {_mm512_and_si512(val, other.val)};
    }

    inline bitword<512> operator|(const bitword<512> &other) const {
        return {_mm512_or_si512(val, other.val)};
    }

    inline bitword<512> andnot(const bitword<512> &other) const {
        return {_mm512_andnot_si512(val, other.val)};
    }

    inline uint16_t popcount() const {
        return stim::popcnt64(u64[0]) + stim::popcnt64(u64[1]) + stim::popcnt64(u64[2]) +
               stim::popcnt64(u64[3]) + stim::popcnt64(u64[4]) + stim::popcnt64(u64[5]) +
               stim::popcnt64(u64[6]) + (uint16_t)stim::popcnt64(u64[7]);
    }

    template <uint64_t shift>
    static void inplace_transpose_block_pass(bitword<512> *data, size_t stride, __m512i mask) {
        for (size_t k = 0; k < 512; k++) {
            if (k & shift) {
                continue;
            }
            bitword<512> &x = data[stride * k];
            bitword<512> &y = data[stride * (k + shift)];
            bitword<512> a = x & mask;
            bitword<512> b = x & ~mask;
            bitword<512> c = y & mask;
            bitword<512> d = y & ~mask;
            x = a | bitword<512>(_mm512_slli_epi64(c.val, shift));
            y = bitword<512>(_mm512_srli_epi64(b.val, shift)) | d;
        }
    }

    //TODO: this is just a copy of the same function of avx2
    static void inplace_transpose_block_pass_64_and_128_and_256(bitword<512> *data, size_t stride) {
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

    static void inplace_transpose_square(bitword<512> *data, size_t stride) {
        inplace_transpose_block_pass<1>(data, stride, _mm512_set1_epi8(0x55));
        inplace_transpose_block_pass<2>(data, stride, _mm512_set1_epi8(0x33));
        inplace_transpose_block_pass<4>(data, stride, _mm512_set1_epi8(0xF));
        inplace_transpose_block_pass<8>(data, stride, _mm512_set1_epi16(0xFF));
        inplace_transpose_block_pass<16>(data, stride, _mm512_set1_epi32(0xFFFF));
        inplace_transpose_block_pass<32>(data, stride, _mm512_set1_epi64x(0xFFFFFFFF));
        inplace_transpose_block_pass_64_and_128(data, stride);
    }
};
#endif

}  // namespace stim

#endif
