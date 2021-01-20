#ifndef SIMD_RANGE_H
#define SIMD_RANGE_H

#include <immintrin.h>
#include <vector>
#include <iostream>
#include <random>
#include "bit_ref.h"

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
        __m128i *const u128;
        __m256i *const u256;
    };
    const size_t num_simd_words;

    /// Construct a simd_bits_range_ref from a given pointer and word count.
    simd_bits_range_ref(__m256i *ptr, size_t num_simd_words);

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
    bit_ref operator[](size_t k);
    /// Returns a const reference to a given bit within the referenced range.
    const bit_ref operator[](size_t k) const;
    /// Returns a const reference to a sub-range of the bits in the referenced range.
    const simd_bits_range_ref word_range_ref(size_t word_offset, size_t sub_num_simd_words) const;
    /// Returns a reference to a sub-range of the bits in the referenced range.
    simd_bits_range_ref word_range_ref(size_t word_offset, size_t sub_num_simd_words);

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
};

/// Writes a description of the contents of the range to `out`.
std::ostream &operator<<(std::ostream &out, const simd_bits_range_ref m);

#endif
