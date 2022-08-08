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

#include <stdlib.h>

#include "stim/mem/simd_util.h"

namespace stim {

template <size_t bit_size>
struct bitword;

/// Implements a 64 bit bitword using no architecture-specific instructions, just standard C++.
template <>
struct bitword<64> {
    constexpr static size_t BIT_SIZE = 64;
    constexpr static size_t BIT_POW = 6;

    union {
        uint64_t u64[1];
        uint8_t u8[8];
    };

    static void *aligned_malloc(size_t bytes) {
        return malloc(bytes);
    }
    static void aligned_free(void *ptr) {
        free(ptr);
    }

    inline constexpr bitword<64>() : u64{} {
    }
    inline constexpr explicit bitword<64>(uint64_t v) : u64{v} {
    }

    constexpr inline static bitword<64> tile64(uint64_t pattern) {
        return bitword<64>(pattern);
    }

    constexpr inline static bitword<64> tile8(uint8_t pattern) {
        return bitword<64>(tile64_helper(pattern, 8));
    }

    inline operator bool() const {  // NOLINT(hicpp-explicit-conversions)
        return (bool)u64[0];
    }

    inline bitword<64> &operator^=(const bitword<64> &other) {
        u64[0] ^= other.u64[0];
        return *this;
    }

    inline bitword<64> &operator&=(const bitword<64> &other) {
        u64[0] &= other.u64[0];
        return *this;
    }

    inline bitword<64> &operator|=(const bitword<64> &other) {
        u64[0] |= other.u64[0];
        return *this;
    }

    inline bitword<64> operator^(const bitword<64> &other) const {
        return bitword<64>(u64[0] ^ other.u64[0]);
    }

    inline bitword<64> operator&(const bitword<64> &other) const {
        return bitword<64>(u64[0] & other.u64[0]);
    }

    inline bitword<64> operator|(const bitword<64> &other) const {
        return bitword<64>(u64[0] | other.u64[0]);
    }

    inline bitword<64> andnot(const bitword<64> &other) const {
        return bitword<64>(~u64[0] & other.u64[0]);
    }

    inline uint16_t popcount() const {
        return popcnt64(u64[0]);
    }

    static void inplace_transpose_square(bitword<64> *data_block, size_t stride) {
        inplace_transpose_64x64((uint64_t *)data_block, stride);
    }
};

}  // namespace stim

#endif
