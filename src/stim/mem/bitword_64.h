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

#ifndef _STIM_MEM_SIMD_WORD_64_STD_H
#define _STIM_MEM_SIMD_WORD_64_STD_H

#include <array>
#include <bit>
#include <sstream>
#include <stdlib.h>

#include "stim/mem/bitword.h"
#include "stim/mem/simd_util.h"

namespace stim {

/// Implements a 64 bit bitword using no architecture-specific instructions, just standard C++.
template <>
struct bitword<64> {
    constexpr static size_t BIT_SIZE = 64;
    constexpr static size_t BIT_POW = 6;

    union {
        uint64_t val;
        uint8_t u8[8];
    };

    static void *aligned_malloc(size_t bytes) {
        return malloc(bytes);
    }
    static void aligned_free(void *ptr) {
        free(ptr);
    }

    inline constexpr bitword() : val{} {
    }
    inline bitword(std::array<uint64_t, 1> val) : val{val[0]} {
    }
    inline constexpr bitword(uint64_t v) : val{v} {
    }
    inline constexpr bitword(int64_t v) : val{(uint64_t)v} {
    }
    inline constexpr bitword(int v) : val{(uint64_t)v} {
    }

    constexpr inline static bitword<64> tile64(uint64_t pattern) {
        return bitword<64>(pattern);
    }

    constexpr inline static bitword<64> tile8(uint8_t pattern) {
        return bitword<64>(tile64_helper(pattern, 8));
    }

    inline std::array<uint64_t, 1> to_u64_array() const {
        return std::array<uint64_t, 1>{val};
    }
    inline operator bool() const {  // NOLINT(hicpp-explicit-conversions)
        return (bool)(val);
    }
    inline operator int() const {  // NOLINT(hicpp-explicit-conversions)
        return (int)val;
    }
    inline operator uint64_t() const {  // NOLINT(hicpp-explicit-conversions)
        return val;
    }
    inline operator int64_t() const {  // NOLINT(hicpp-explicit-conversions)
        return (int64_t)val;
    }

    inline bitword<64> &operator^=(const bitword<64> &other) {
        val ^= other.val;
        return *this;
    }

    inline bitword<64> &operator&=(const bitword<64> &other) {
        val &= other.val;
        return *this;
    }

    inline bitword<64> &operator|=(const bitword<64> &other) {
        val |= other.val;
        return *this;
    }

    inline bitword<64> operator^(const bitword<64> &other) const {
        return bitword<64>(val ^ other.val);
    }

    inline bitword<64> operator&(const bitword<64> &other) const {
        return bitword<64>(val & other.val);
    }

    inline bitword<64> operator|(const bitword<64> &other) const {
        return bitword<64>(val | other.val);
    }

    inline bitword<64> andnot(const bitword<64> &other) const {
        return bitword<64>(~val & other.val);
    }

    inline uint16_t popcount() const {
        return std::popcount(val);
    }

    inline std::string str() const {
        std::stringstream out;
        out << *this;
        return out.str();
    }

    inline bitword<64> shifted(int offset) const {
        uint64_t v = val;
        if (offset >= 64 || offset <= -64) {
            v = 0;
        } else if (offset > 0) {
            v <<= offset;
        } else {
            v >>= -offset;
        }
        return bitword<64>{v};
    }

    static void inplace_transpose_square(bitword<64> *data_block, size_t stride) {
        inplace_transpose_64x64((uint64_t *)data_block, stride);
    }
};

}  // namespace stim

#endif
