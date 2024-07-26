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

#ifndef _STIM_MEM_SIMD_BITS_RANGE_REF_H
#define _STIM_MEM_SIMD_BITS_RANGE_REF_H

#include <array>
#include <iostream>
#include <random>
#include <vector>

#include "stim/mem/bit_ref.h"
#include "stim/mem/simd_word.h"

namespace stim {

template <size_t W>
constexpr size_t min_bits_to_num_bits_padded(size_t min_bits) {
    return (min_bits + (sizeof(bitword<W>) * 8 - 1)) & ~(sizeof(bitword<W>) * 8 - 1);
}

template <size_t W>
constexpr size_t min_bits_to_num_simd_words(size_t min_bits) {
    return (min_bits_to_num_bits_padded<W>(min_bits) / sizeof(bitword<W>)) >> 3;
}

/// A reference to a range of bits that support SIMD operations (e.g. they are aligned and padded correctly).
///
/// Conceptually behaves the same as a reference like `int &`, as opposed to a pointer like `int *`. For example, the
/// `=` operator overwrites the contents of the range being referenced instead of changing which range is pointed to.
template <size_t W>
struct simd_bits_range_ref {
    union {
        // It is fair to say that this is the most dangerous block, or danger-enabling block, in the entire codebase.
        // C++ is very particular when it comes to touching the same memory as if it had multiple different types.
        // If you know how to make something *for sure work as a flexibly-accessible bag of bits*, please fix this.
        // In the meantime, always build with `-fno-strict-aliasing` and a short ritual prayer to the compiler gods.
        uint8_t *const u8;
        uint64_t *const u64;
        bitword<W> *const ptr_simd;
    };
    const size_t num_simd_words;

    /// Construct a simd_bits_range_ref from a given pointer and word count.
    simd_bits_range_ref(bitword<W> *ptr_simd, size_t num_simd_words);

    /// Overwrite assignment.
    simd_bits_range_ref operator=(
        const simd_bits_range_ref
            other);  // NOLINT(cppcoreguidelines-c-copy-assignment-signature,misc-unconventional-assign-operator)
    /// Xor assignment.
    simd_bits_range_ref operator^=(const simd_bits_range_ref other);
    /// Mask assignment.
    simd_bits_range_ref operator&=(const simd_bits_range_ref other);
    simd_bits_range_ref operator|=(const simd_bits_range_ref other);
    // Addition assigment
    simd_bits_range_ref operator+=(const simd_bits_range_ref<W> other);
    simd_bits_range_ref operator-=(const simd_bits_range_ref<W> other);
    // Shift assigment
    simd_bits_range_ref operator>>=(int offset);
    simd_bits_range_ref operator<<=(int offset);
    /// Swap assignment.
    void swap_with(simd_bits_range_ref other);

    /// Equality.
    bool operator==(const simd_bits_range_ref<W> &other) const;
    /// Inequality.
    bool operator!=(const simd_bits_range_ref<W> &other) const;
    /// Determines whether or not any of the bits in the referenced range are non-zero.
    bool not_zero() const;
    /// Treats the referenced range as an unsigned integer, and returns it as a uint64_t.
    /// If the integer is too large  to fit, an exception is raised.
    uint64_t as_u64() const;

    /// Returns a reference to a given bit within the referenced range.
    inline bit_ref operator[](size_t k) {
        return bit_ref(u8, k);
    }
    /// Returns a const reference to a given bit within the referenced range.
    inline const bit_ref operator[](size_t k) const {
        return bit_ref(u8, k);
    }
    /// Returns a reference to a sub-range of the bits at the start of this simd_bits.
    inline simd_bits_range_ref prefix_ref(size_t unpadded_bit_length) {
        return simd_bits_range_ref(ptr_simd, min_bits_to_num_simd_words<W>(unpadded_bit_length));
    }
    /// Returns a reference to a sub-range of the bits in the referenced range.
    inline simd_bits_range_ref word_range_ref(size_t word_offset, size_t sub_num_simd_words) {
        return simd_bits_range_ref(ptr_simd + word_offset, sub_num_simd_words);
    }
    /// Returns a const reference to a sub-range of the bits in the referenced range.
    inline const simd_bits_range_ref word_range_ref(size_t word_offset, size_t sub_num_simd_words) const {
        return simd_bits_range_ref(ptr_simd + word_offset, sub_num_simd_words);
    }

    /// Inverts all bits in the referenced range.
    void invert_bits();
    /// Sets all bits in the referenced range to zero.
    void clear();
    /// Randomizes the bits in the referenced range, up to the given bit count. Leaves further bits unchanged.
    void randomize(size_t num_bits, std::mt19937_64 &rng);
    /// Returns the number of bits that are 1 in the bit range.
    size_t popcnt() const;
    /// Returns the power-of-two-ness of the number, or SIZE_MAX if the number has no 1s.
    size_t countr_zero() const;
    /// Returns whether or not the two ranges have set bits in common.
    bool intersects(const simd_bits_range_ref other) const;

    /// Writes bits from another location.
    /// Bits not part of the write are unchanged.
    void truncated_overwrite_from(simd_bits_range_ref other, size_t num_bits);

    /// Returns a description of the contents of the range.
    std::string str() const;

    /// Number of 64 bit words in the referenced range.
    inline size_t num_u64_padded() const {
        return num_simd_words * (sizeof(bitword<W>) / sizeof(uint64_t));
    }
    /// Number of 32 bit words in the referenced range.
    inline size_t num_u32_padded() const {
        return num_simd_words * (sizeof(bitword<W>) / sizeof(uint32_t));
    }
    /// Number of 16 bit words in the referenced range.
    inline size_t num_u16_padded() const {
        return num_simd_words * (sizeof(bitword<W>) / sizeof(uint16_t));
    }
    /// Number of 8 bit words in the referenced range.
    inline size_t num_u8_padded() const {
        return num_simd_words * (sizeof(bitword<W>) / sizeof(uint8_t));
    }
    /// Number of bits in the referenced range.
    inline size_t num_bits_padded() const {
        return num_simd_words * sizeof(bitword<W>) << 3;
    }

    /// Runs a function on each word in the range, in sequential order.
    ///
    /// The words are passed by reference and have type bitword<W>.
    ///
    /// This is a boilerplate reduction method. It could be an iterator, but when experimenting I found that the
    /// compiler seemed much more amenable to inline the function in the way I wanted when using this approach rather
    /// than iterators.
    ///
    /// Example:
    ///     size_t simd_popcount(simd_bits_range_ref data) {
    ///         size_t popcount = 0;
    ///         data.for_each_word([&](auto &w) {
    ///             popcount += w.popcount();
    ///         });
    ///         return popcount;
    ///     }
    ///
    /// HACK: Templating the function type makes inlining significantly more likely.
    template <typename FUNC>
    inline void for_each_word(FUNC body) const {
        auto *v0 = ptr_simd;
        auto *v0_end = v0 + num_simd_words;
        while (v0 != v0_end) {
            body(*v0);
            v0++;
        }
    }

    /// Runs a function on paired up words from two ranges, in sequential order.
    ///
    /// The words are passed by reference and have type bitword<W>.
    ///
    /// This is a boilerplate reduction method. It could be an iterator, but when experimenting I found that the
    /// compiler seemed much more amenable to inline the function in the way I wanted when using this approach rather
    /// than iterators.
    ///
    /// Example:
    ///     void xor_left_into_right(simd_bits_range_ref data1, simd_bits_range_ref data2) {
    ///         data1.for_each_word(data2, [&](auto &w1, auto &w2) {
    ///             w2 ^= w1;
    ///         });
    ///     }
    ///
    /// HACK: Templating the function type makes inlining significantly more likely.
    template <typename FUNC>
    inline void for_each_word(simd_bits_range_ref<W> other, FUNC body) const {
        auto *v0 = ptr_simd;
        auto *v1 = other.ptr_simd;
        auto *v0_end = v0 + num_simd_words;
        while (v0 != v0_end) {
            body(*v0, *v1);
            v0++;
            v1++;
        }
    }

    /// Runs a function on paired up words from three ranges, in sequential order.
    ///
    /// The words are passed by reference and have type bitword<W>.
    ///
    /// This is a boilerplate reduction method. It could be an iterator, but when experimenting I found that the
    /// compiler seemed much more amenable to inline the function in the way I wanted when using this approach rather
    /// than iterators.
    ///
    /// Example:
    ///     void xor_intersection_into_last(simd_bits_range_ref data1, simd_bits_range_ref data2, simd_bits_range_ref
    ///     data3) {
    ///         data1.for_each_word(data2, data3, [&](auto &w1, auto &w2, auto &w3) {
    ///             w3 ^= w1 & w2;
    ///         });
    ///     }
    ///
    /// HACK: Templating the function type makes inlining significantly more likely.
    template <typename FUNC>
    inline void for_each_word(simd_bits_range_ref<W> other1, simd_bits_range_ref<W> other2, FUNC body) const {
        auto *v0 = ptr_simd;
        auto *v1 = other1.ptr_simd;
        auto *v2 = other2.ptr_simd;
        auto *v0_end = v0 + num_simd_words;
        while (v0 != v0_end) {
            body(*v0, *v1, *v2);
            v0++;
            v1++;
            v2++;
        }
    }

    /// Runs a function on paired up words from four ranges, in sequential order.
    ///
    /// The words are passed by reference and have type bitword<W>.
    ///
    /// This is a boilerplate reduction method. It could be an iterator, but when experimenting I found that the
    /// compiler seemed much more amenable to inline the function in the way I wanted when using this approach rather
    /// than iterators.
    ///
    /// Example:
    ///     void xor_union_into_last(
    ///             simd_bits_range_ref data1, simd_bits_range_ref data2, simd_bits_range_ref data3, simd_bits_range_ref
    ///             data4) {
    ///         data1.for_each_word(data2, data3, data4, [&](auto &w1, auto &w2, auto &w3, auto &w4) {
    ///             w4 ^= w1 | w2 | w3;
    ///         });
    ///     }
    ///
    /// HACK: Templating the function type makes inlining significantly more likely.
    template <typename FUNC>
    inline void for_each_word(
        simd_bits_range_ref<W> other1, simd_bits_range_ref<W> other2, simd_bits_range_ref<W> other3, FUNC body) const {
        auto *v0 = ptr_simd;
        auto *v1 = other1.ptr_simd;
        auto *v2 = other2.ptr_simd;
        auto *v3 = other3.ptr_simd;
        auto *v0_end = v0 + num_simd_words;
        while (v0 != v0_end) {
            body(*v0, *v1, *v2, *v3);
            v0++;
            v1++;
            v2++;
            v3++;
        }
    }

    /// Runs a function on paired up words from five ranges, in sequential order.
    ///
    /// The words are passed by reference and have type bitword<W>.
    ///
    /// This is a boilerplate reduction method. It could be an iterator, but when experimenting I found that the
    /// compiler seemed much more amenable to inline the function in the way I wanted when using this approach rather
    /// than iterators.
    ///
    /// Example:
    ///     void xor_union_into_last(
    ///             simd_bits_range_ref data1,
    ///             simd_bits_range_ref data2,
    ///             simd_bits_range_ref data3,
    ///             simd_bits_range_ref data4,
    ///             simd_bits_range_ref data5) {
    ///         data1.for_each_word(data2, data3, data4, data5, [&](auto &w1, auto &w2, auto &w3, auto &w4, auto &w5) {
    ///             w5 ^= w1 | w2 | w3 | w4;
    ///         });
    ///     }
    ///
    /// HACK: Templating the function type makes inlining significantly more likely.
    template <typename FUNC>
    inline void for_each_word(
        simd_bits_range_ref<W> other1,
        simd_bits_range_ref<W> other2,
        simd_bits_range_ref<W> other3,
        simd_bits_range_ref<W> other4,
        FUNC body) const {
        auto *v0 = ptr_simd;
        auto *v1 = other1.ptr_simd;
        auto *v2 = other2.ptr_simd;
        auto *v3 = other3.ptr_simd;
        auto *v4 = other4.ptr_simd;
        auto *v0_end = v0 + num_simd_words;
        while (v0 != v0_end) {
            body(*v0, *v1, *v2, *v3, *v4);
            v0++;
            v1++;
            v2++;
            v3++;
            v4++;
        }
    }

    template <typename CALLBACK>
    void for_each_set_bit(CALLBACK callback) {
        size_t n = num_u64_padded();
        for (size_t w = 0; w < n; w++) {
            uint64_t v = u64[w];
            while (v) {
                size_t q = w * 64 + std::countr_zero(v);
                v &= v - 1;
                callback(q);
            }
        }
    }
};

/// Writes a description of the contents of the range to `out`.
template <size_t W>
std::ostream &operator<<(std::ostream &out, const simd_bits_range_ref<W> m);

}  // namespace stim

#include "stim/mem/simd_bits_range_ref.inl"

#endif
