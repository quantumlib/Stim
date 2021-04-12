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

#ifndef SIMD_BITS_H
#define SIMD_BITS_H

#include <cstdint>
#include <random>

#include "bit_ref.h"
#include "simd_bits_range_ref.h"

/// Densely packed bits, allocated with alignment and padding enabling SIMD operations.
///
/// A backing store for a `simd_bits_range_ref`.
struct simd_bits {
    size_t num_simd_words;
    union {
        uint8_t *u8;
        uint16_t *u16;
        uint32_t *u32;
        uint64_t *u64;
        simd_word *ptr_simd;
    };
    static size_t min_bits_to_num_simd_words(size_t min_bits);
    static size_t min_bits_to_num_bits_padded(size_t min_bits);

    /// Constructs a zero-initialized simd_bits with at least the given number of bits.
    explicit simd_bits(size_t min_bits);
    /// Frees allocated bits.
    ~simd_bits();
    /// Copy constructor.
    simd_bits(const simd_bits &other);
    /// Copy constructor from range reference.
    simd_bits(const simd_bits_range_ref other);
    /// Move constructor.
    simd_bits(simd_bits &&other) noexcept;

    /// Copy assignment.
    simd_bits &operator=(const simd_bits &other);
    /// Copy assignment from range reference.
    simd_bits &operator=(const simd_bits_range_ref other);
    /// Move assignment.
    simd_bits &operator=(simd_bits &&other) noexcept;
    // Xor assignment.
    simd_bits &operator^=(const simd_bits_range_ref other);
    // Mask assignment.
    simd_bits &operator&=(const simd_bits_range_ref other);
    simd_bits &operator|=(const simd_bits_range_ref other);
    // Swap assignment.
    simd_bits &swap_with(simd_bits_range_ref other);

    // Equality.
    bool operator==(const simd_bits_range_ref &other) const;
    // Inequality.
    bool operator!=(const simd_bits_range_ref &other) const;
    /// Determines whether or not any of the bits in the simd_bits are non-zero.
    bool not_zero() const;

    /// Returns a reference to the bit at offset k.
    bit_ref operator[](size_t k);
    /// Returns a const reference to the bit at offset k.
    const bit_ref operator[](size_t k) const;
    /// Returns a reference to the bits in this simd_bits.
    operator simd_bits_range_ref();
    /// Returns a const reference to the bits in this simd_bits.
    operator const simd_bits_range_ref() const;
    /// Returns a reference to a sub-range of the bits in this simd_bits.
    inline simd_bits_range_ref word_range_ref(size_t word_offset, size_t sub_num_simd_words) {
        return simd_bits_range_ref(ptr_simd + word_offset, sub_num_simd_words);
    }
    /// Returns a const reference to a sub-range of the bits in this simd_bits.
    inline const simd_bits_range_ref word_range_ref(size_t word_offset, size_t sub_num_simd_words) const {
        return simd_bits_range_ref(ptr_simd + word_offset, sub_num_simd_words);
    }

    /// Inverts all bits in the range.
    void invert_bits();
    /// Sets all bits in the range to zero.
    void clear();
    /// Randomizes the contents of this simd_bits using the given random number generator, up to the given bit position.
    void randomize(size_t num_bits, std::mt19937_64 &rng);
    /// Returns a simd_bits with at least the given number of bits, with bits up to the given number of bits randomized.
    /// Padding bits beyond the minimum number of bits are not randomized.
    static simd_bits random(size_t min_bits, std::mt19937_64 &rng);

    /// Writes bits from another location.
    /// Bits not part of the write are unchanged.
    void truncated_overwrite_from(simd_bits_range_ref other, size_t num_bits);

    /// Returns a description of the contents of the simd_bits.
    std::string str() const;

    /// Number of 64 bit words in the range.
    inline size_t num_u64_padded() const {
        return num_simd_words * (sizeof(simd_word) / sizeof(uint64_t));
    }
    /// Number of 32 bit words in the range.
    inline size_t num_u32_padded() const {
        return num_simd_words * (sizeof(simd_word) / sizeof(uint32_t));
    }
    /// Number of 16 bit words in the range.
    inline size_t num_u16_padded() const {
        return num_simd_words * (sizeof(simd_word) / sizeof(uint16_t));
    }
    /// Number of 8 bit words in the range.
    inline size_t num_u8_padded() const {
        return num_simd_words * (sizeof(simd_word) / sizeof(uint8_t));
    }
    /// Number of bits in the range.
    inline size_t num_bits_padded() const {
        return num_simd_words * sizeof(simd_word) << 3;
    }
};

#endif
