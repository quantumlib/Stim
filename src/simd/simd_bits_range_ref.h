#ifndef SIMD_RANGE_H
#define SIMD_RANGE_H

#include <immintrin.h>
#include <vector>
#include <iostream>
#include <random>
#include "bit_ref.h"
#include "simd_compat.h"

/// A reference to a range of bits that support SIMD operations (e.g. they are aligned and padded correctly).
///
/// Conceptually behaves the same as a reference like `int &`, as opposed to a pointer like `int *`. For example, the
/// `=` operator overwrites the contents of the range being referenced instead of changing which range is pointed to.
struct simd_bits_range_ref {
    union {
        uint8_t *const u8;
        uint16_t *const u16;
        uint32_t *const u32;
        uint64_t *const u64;
        SIMD_WORD_TYPE *const ptr_simd;
    };
    const size_t num_simd_words;

    /// Construct a simd_bits_range_ref from a given pointer and word count.
    simd_bits_range_ref(SIMD_WORD_TYPE *ptr_simd, size_t num_simd_words);

    /// Overwrite assignment.
    simd_bits_range_ref operator=(const simd_bits_range_ref other);
    /// Xor assignment.
    simd_bits_range_ref operator^=(const simd_bits_range_ref other);
    /// Swap assignment.
    void swap_with(simd_bits_range_ref other);

    /// Equality.
    bool operator==(const simd_bits_range_ref other) const;
    /// Inequality.
    bool operator!=(const simd_bits_range_ref other) const;
    /// Determines whether or not any of the bits in the referenced range are non-zero.
    bool not_zero() const;

    /// Returns a reference to a given bit within the referenced range.
    inline bit_ref operator[](size_t k) {
        return bit_ref(u8, k);
    }
    /// Returns a const reference to a given bit within the referenced range.
    inline const bit_ref operator[](size_t k) const {
        return bit_ref(u8, k);
    }
    /// Returns a reference to a sub-range of the bits in the referenced range.
    inline simd_bits_range_ref word_range_ref(size_t word_offset, size_t sub_num_simd_words) {
        return simd_bits_range_ref(ptr_simd + word_offset, sub_num_simd_words);
    }
    /// Returns a const reference to a sub-range of the bits in the referenced range.
    inline const simd_bits_range_ref word_range_ref(size_t word_offset, size_t sub_num_simd_words) const {
        return simd_bits_range_ref(ptr_simd + word_offset, sub_num_simd_words);
    }

    /// Sets all bits in the referenced range to zero.
    void clear();
    /// Randomizes the bits in the referenced range, up to the given bit count. Leaves further bits unchanged.
    void randomize(size_t num_bits, std::mt19937 &rng);

    /// Returns a description of the contents of the range.
    std::string str() const;

    /// Number of 64 bit words in the referenced range.
    inline size_t num_u64_padded() const { return num_simd_words << 2; }
    /// Number of 32 bit words in the referenced range.
    inline size_t num_u32_padded() const { return num_simd_words << 3; }
    /// Number of 16 bit words in the referenced range.
    inline size_t num_u16_padded() const { return num_simd_words << 4; }
    /// Number of 8 bit words in the referenced range.
    inline size_t num_u8_padded() const { return num_simd_words << 5; }
    /// Number of bits in the referenced range.
    inline size_t num_bits_padded() const { return num_simd_words << 8; }

    template <typename BODY>
    inline void for_each_word(BODY body) const {
        auto v0 = ptr_simd;
        auto v0_end = v0 + num_simd_words;
        while (v0 != v0_end) {
            body(*v0);
            v0++;
        }
    }

    template <typename BODY>
    inline void for_each_word(simd_bits_range_ref other, BODY body) const {
        auto v0 = ptr_simd;
        auto v1 = other.ptr_simd;
        auto v0_end = v0 + num_simd_words;
        while (v0 != v0_end) {
            body(*v0, *v1);
            v0++;
            v1++;
        }
    }

    template <typename BODY>
    inline void for_each_word(
            simd_bits_range_ref other1,
            simd_bits_range_ref other2,
            BODY body) const {
        auto v0 = ptr_simd;
        auto v1 = other1.ptr_simd;
        auto v2 = other2.ptr_simd;
        auto v0_end = v0 + num_simd_words;
        while (v0 != v0_end) {
            body(*v0, *v1, *v2);
            v0++;
            v1++;
            v2++;
        }
    }

    template <typename BODY>
    inline void for_each_word(
            simd_bits_range_ref other1,
            simd_bits_range_ref other2,
            simd_bits_range_ref other3,
            BODY body) const {
        auto v0 = ptr_simd;
        auto v1 = other1.ptr_simd;
        auto v2 = other2.ptr_simd;
        auto v3 = other3.ptr_simd;
        auto v0_end = v0 + num_simd_words;
        while (v0 != v0_end) {
            body(*v0, *v1, *v2, *v3);
            v0++;
            v1++;
            v2++;
            v3++;
        }
    }

    template <typename BODY>
    inline void for_each_word(
            simd_bits_range_ref other1,
            simd_bits_range_ref other2,
            simd_bits_range_ref other3,
            simd_bits_range_ref other4,
            BODY body) const {
        auto v0 = ptr_simd;
        auto v1 = other1.ptr_simd;
        auto v2 = other2.ptr_simd;
        auto v3 = other3.ptr_simd;
        auto v4 = other4.ptr_simd;
        auto v0_end = v0 + num_simd_words;
        while (v0 != v0_end) {
            body(*v0, *v1, *v2, *v3, *v4);
            v0++;
            v1++;
            v2++;
            v3++;
            v4++;
        }
    }
};

/// Writes a description of the contents of the range to `out`.
std::ostream &operator<<(std::ostream &out, const simd_bits_range_ref m);

#endif
