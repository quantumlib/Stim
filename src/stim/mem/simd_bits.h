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

#ifndef _STIM_MEM_SIMD_BITS_H
#define _STIM_MEM_SIMD_BITS_H

#include <cstdint>
#include <random>

#include "stim/mem/bit_ref.h"
#include "stim/mem/simd_bits_range_ref.h"

namespace stim {

/// Densely packed bits, allocated with alignment and padding enabling SIMD operations.
///
/// Note that, due to the padding, the smallest simd_bits you can have is 256 bits (32 bytes) long.
///
/// For performance, simd_bits does not store the "intended" size of the data, only the padded size. Any intended size
/// has to be tracked separately.
template <size_t W>
struct simd_bits {
    size_t num_simd_words;
    union {
        // It is fair to say that this is the most dangerous block, or danger-enabling block, in the entire codebase.
        // C++ is very particular when it comes to touching the same memory as if it had multiple different types.
        // If you know how to make something *for sure work as a flexibly-accessible bag of bits*, please fix this.
        // In the meantime, always build with `-fno-strict-aliasing` and a short ritual prayer to the compiler gods.
        uint8_t *u8;
        uint64_t *u64;
        bitword<W> *ptr_simd;
    };

    /// Constructs a zero-initialized simd_bits with at least the given number of bits.
    explicit simd_bits(size_t min_bits);
    /// Frees allocated bits.
    ~simd_bits();
    /// Copy constructor.
    simd_bits(const simd_bits &other);
    /// Copy constructor from range reference.
    simd_bits(const simd_bits_range_ref<W> other);
    /// Move constructor.
    simd_bits(simd_bits &&other) noexcept;

    /// Copy assignment.
    simd_bits &operator=(const simd_bits &other);
    /// Copy assignment from range reference.
    simd_bits &operator=(const simd_bits_range_ref<W> other);
    /// Move assignment.
    simd_bits &operator=(simd_bits &&other) noexcept;
    // Xor assignment.
    simd_bits &operator^=(const simd_bits_range_ref<W> other);
    // Mask assignment.
    simd_bits &operator&=(const simd_bits_range_ref<W> other);
    simd_bits &operator|=(const simd_bits_range_ref<W> other);
    // Addition assigment
    simd_bits &operator+=(const simd_bits_range_ref<W> other);
    simd_bits &operator-=(const simd_bits_range_ref<W> other);
    // right shift assignment
    simd_bits &operator>>=(int offset);
    // left shift assignment
    simd_bits &operator<<=(int offset);
    // Swap assignment.
    simd_bits &swap_with(simd_bits_range_ref<W> other);

    // Equality.
    bool operator==(const simd_bits_range_ref<W> &other) const;
    bool operator==(const simd_bits<W> &other) const;
    // Inequality.
    bool operator!=(const simd_bits_range_ref<W> &other) const;
    bool operator!=(const simd_bits<W> &other) const;
    /// Determines whether or not any of the bits in the simd_bits are non-zero.
    bool not_zero() const;

    // Arbitrary ordering.
    bool operator<(const simd_bits_range_ref<W> other) const;

    void destructive_resize(size_t new_min_bits);

    /// Returns a reference to the bit at offset k.
    bit_ref operator[](size_t k);
    /// Returns a const reference to the bit at offset k.
    const bit_ref operator[](size_t k) const;
    /// Returns a reference to the bits in this simd_bits.
    operator simd_bits_range_ref<W>();
    /// Returns a const reference to the bits in this simd_bits.
    operator const simd_bits_range_ref<W>() const;
    /// Returns a reference to a sub-range of the bits in this simd_bits.
    inline simd_bits_range_ref<W> word_range_ref(size_t word_offset, size_t sub_num_simd_words) {
        return simd_bits_range_ref<W>(ptr_simd + word_offset, sub_num_simd_words);
    }
    /// Returns a reference to a sub-range of the bits at the start of this simd_bits.
    inline simd_bits_range_ref<W> prefix_ref(size_t unpadded_bit_length) {
        return simd_bits_range_ref<W>(ptr_simd, min_bits_to_num_simd_words<W>(unpadded_bit_length));
    }
    /// Returns a const reference to a sub-range of the bits in this simd_bits.
    inline const simd_bits_range_ref<W> word_range_ref(size_t word_offset, size_t sub_num_simd_words) const {
        return simd_bits_range_ref<W>(ptr_simd + word_offset, sub_num_simd_words);
    }

    /// Returns the number of bits that are 1 in the bit range.
    size_t popcnt() const;
    /// Returns the power-of-two-ness of the number, or SIZE_MAX if the number has no 1s.
    size_t countr_zero() const;

    /// Inverts all bits in the range.
    void invert_bits();
    /// Sets all bits in the range to zero.
    void clear();
    /// Randomizes the contents of this simd_bits using the given random number generator, up to the given bit position.
    void randomize(size_t num_bits, std::mt19937_64 &rng);
    /// Returns a simd_bits with at least the given number of bits, with bits up to the given number of bits randomized.
    /// Padding bits beyond the minimum number of bits are not randomized.
    static simd_bits<W> random(size_t min_bits, std::mt19937_64 &rng);

    /// Returns whether or not the two ranges have set bits in common.
    bool intersects(const simd_bits_range_ref<W> other) const;

    /// Writes bits from another location.
    /// Bits not part of the write are unchanged.
    void truncated_overwrite_from(simd_bits_range_ref<W> other, size_t num_bits);

    /// Returns a description of the contents of the simd_bits.
    std::string str() const;

    /// Number of 64 bit words in the range.
    inline size_t num_u64_padded() const {
        return num_simd_words * (sizeof(bitword<W>) / sizeof(uint64_t));
    }
    /// Number of 32 bit words in the range.
    inline size_t num_u32_padded() const {
        return num_simd_words * (sizeof(bitword<W>) / sizeof(uint32_t));
    }
    /// Number of 16 bit words in the range.
    inline size_t num_u16_padded() const {
        return num_simd_words * (sizeof(bitword<W>) / sizeof(uint16_t));
    }
    /// Number of 8 bit words in the range.
    inline size_t num_u8_padded() const {
        return num_simd_words * (sizeof(bitword<W>) / sizeof(uint8_t));
    }
    /// Number of bits in the range.
    inline size_t num_bits_padded() const {
        return num_simd_words * W;
    }

    uint64_t as_u64() const;
};

template <size_t W>
simd_bits<W> operator^(const simd_bits_range_ref<W> a, const simd_bits_range_ref<W> b);
template <size_t W>
simd_bits<W> operator|(const simd_bits_range_ref<W> a, const simd_bits_range_ref<W> b);
template <size_t W>
simd_bits<W> operator&(const simd_bits_range_ref<W> a, const simd_bits_range_ref<W> b);
template <size_t W>
simd_bits<W> operator^(const simd_bits<W> a, const simd_bits_range_ref<W> b);
template <size_t W>
simd_bits<W> operator|(const simd_bits<W> a, const simd_bits_range_ref<W> b);
template <size_t W>
simd_bits<W> operator&(const simd_bits<W> a, const simd_bits_range_ref<W> b);
template <size_t W>
simd_bits<W> operator^(const simd_bits_range_ref<W> a, const simd_bits<W> b);
template <size_t W>
simd_bits<W> operator|(const simd_bits_range_ref<W> a, const simd_bits<W> b);
template <size_t W>
simd_bits<W> operator&(const simd_bits_range_ref<W> a, const simd_bits<W> b);
template <size_t W>
simd_bits<W> operator^(const simd_bits<W> a, const simd_bits<W> b);
template <size_t W>
simd_bits<W> operator|(const simd_bits<W> a, const simd_bits<W> b);
template <size_t W>
simd_bits<W> operator&(const simd_bits<W> a, const simd_bits<W> b);

template <size_t W>
std::ostream &operator<<(std::ostream &out, const simd_bits<W> m);

}  // namespace stim

#include "stim/mem/simd_bits.inl"

#endif
