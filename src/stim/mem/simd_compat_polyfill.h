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

#ifndef _STIM_MEM_SIMD_COMPAT_POLYFILL_H
#define _STIM_MEM_SIMD_COMPAT_POLYFILL_H

/// Implements `simd_word` using no architecture-specific instructions, just raw C++11 code.

#include <stdlib.h>

#include "stim/mem/simd_util.h"

namespace stim {

struct emu_u128 {
    uint64_t a;
    uint64_t b;
};

#define simd_word simd_word_polyfill
struct simd_word_polyfill {
    union {
        uint64_t u64[2];
        uint8_t u8[16];
        emu_u128 u128[1];
    };

    static void *aligned_malloc(size_t bytes) {
        return malloc(bytes);
    }
    static void aligned_free(void *ptr) {
        free(ptr);
    }

    inline constexpr simd_word() : u64{} {
    }
    inline constexpr simd_word(uint64_t v1, uint64_t v2) : u64{v1, v2} {
    }

    constexpr inline static simd_word tile64(uint64_t pattern) {
        return {pattern, pattern};
    }

    constexpr inline static simd_word tile8(uint8_t pattern) {
        return simd_word::tile64(tile64_helper(pattern, 8));
    }

    constexpr inline static simd_word tile16(uint16_t pattern) {
        return simd_word::tile64(tile64_helper(pattern, 16));
    }

    constexpr inline static simd_word tile32(uint32_t pattern) {
        return simd_word::tile64(tile64_helper(pattern, 32));
    }

    inline operator bool() const {  // NOLINT(hicpp-explicit-conversions)
        return u64[0] | u64[1];
    }

    inline simd_word &operator^=(const simd_word &other) {
        u64[0] ^= other.u64[0];
        u64[1] ^= other.u64[1];
        return *this;
    }

    inline simd_word &operator&=(const simd_word &other) {
        u64[0] &= other.u64[0];
        u64[1] &= other.u64[1];
        return *this;
    }

    inline simd_word &operator|=(const simd_word &other) {
        u64[0] |= other.u64[0];
        u64[1] |= other.u64[1];
        return *this;
    }

    inline simd_word operator^(const simd_word &other) const {
        return {u64[0] ^ other.u64[0], u64[1] ^ other.u64[1]};
    }

    inline simd_word operator&(const simd_word &other) const {
        return {u64[0] & other.u64[0], u64[1] & other.u64[1]};
    }

    inline simd_word operator|(const simd_word &other) const {
        return {u64[0] | other.u64[0], u64[1] | other.u64[1]};
    }

    inline simd_word andnot(const simd_word &other) const {
        return {~u64[0] & other.u64[0], ~u64[1] & other.u64[1]};
    }

    inline uint16_t popcount() const {
        return popcnt64(u64[0]) + popcnt64(u64[1]);
    }

    inline simd_word leftshift_tile64(uint8_t offset) const {
        return {u64[0] << offset, u64[1] << offset};
    }

    inline simd_word rightshift_tile64(uint8_t offset) const {
        return {u64[0] >> offset, u64[1] >> offset};
    }

    /// For each 128 bit word pair between the two registers, the byte order goes from this:
    /// [a0 a1 a2 a3 ... a14 a15] [b0 b1 b2 b3 ... b14 b15]
    /// to this:
    /// [a0 b0 a1 b1 ...  a7  b7] [a8 b8 a9 b9 ... a15 b15]
    inline void do_interleave8_tile128(simd_word &other) {
        auto a1 = spread_bytes_32_to_64((uint32_t)u64[0]);
        auto a2 = spread_bytes_32_to_64((uint32_t)(u64[0] >> 32));
        auto b1 = spread_bytes_32_to_64((uint32_t)u64[1]);
        auto b2 = spread_bytes_32_to_64((uint32_t)(u64[1] >> 32));
        auto c1 = spread_bytes_32_to_64((uint32_t)other.u64[0]);
        auto c2 = spread_bytes_32_to_64((uint32_t)(other.u64[0] >> 32));
        auto d1 = spread_bytes_32_to_64((uint32_t)other.u64[1]);
        auto d2 = spread_bytes_32_to_64((uint32_t)(other.u64[1] >> 32));
        u64[0] = a1 | (c1 << 8);
        u64[1] = a2 | (c2 << 8);
        other.u64[0] = b1 | (d1 << 8);
        other.u64[1] = b2 | (d2 << 8);
    }
};

}  // namespace stim

#endif
