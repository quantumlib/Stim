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

#ifndef SIMD_BIT_TABLE_H
#define SIMD_BIT_TABLE_H

#include "simd_bits.h"

struct simd_bit_table {
    size_t num_simd_words_major;
    size_t num_simd_words_minor;
    simd_bits data;

    /// Creates zero initialized table.
    simd_bit_table(size_t min_bits_major, size_t min_bits_minor);
    /// Creates a randomly initialized table.
    static simd_bit_table random(
        size_t num_randomized_major_bits, size_t num_randomized_minor_bits, std::mt19937_64 &rng);
    /// Creates a square table with 1s down the diagonal.
    static simd_bit_table identity(size_t n);
    /// Concatenates tables together to form a larger table.
    static simd_bit_table from_quadrants(
        size_t n, const simd_bit_table &upper_left, const simd_bit_table &upper_right, const simd_bit_table &lower_left,
        const simd_bit_table &lower_right);

    /// Equality.
    bool operator==(const simd_bit_table &other) const;
    /// Inequality.
    bool operator!=(const simd_bit_table &other) const;

    /// Returns a reference to a row (column) of the table, when using row (column) major indexing.
    inline simd_bits_range_ref operator[](size_t major_index) {
        return data.word_range_ref(major_index * num_simd_words_minor, num_simd_words_minor);
    }
    /// Returns a const reference to a row (column) of the table, when using row (column) major indexing.
    inline const simd_bits_range_ref operator[](size_t major_index) const {
        return data.word_range_ref(major_index * num_simd_words_minor, num_simd_words_minor);
    }

    /// Square matrix multiplication (assumes row major indexing). n is the diameter of the matrix.
    simd_bit_table square_mat_mul(const simd_bit_table &rhs, size_t n) const;
    /// Square matrix inverse, assuming input is lower triangular. n is the diameter of the matrix.
    simd_bit_table inverse_assuming_lower_triangular(size_t n) const;
    /// Transposes the table inplace.
    void do_square_transpose();
    /// Transposes the table out of place into a target location.
    void transpose_into(simd_bit_table &out) const;
    void transpose_into(simd_bit_table &out, size_t major_start_bit, size_t min_major_length_bits) const;
    /// Transposes the table out of place.
    simd_bit_table transposed() const;
    /// Returns a subset of the table.
    simd_bit_table slice_maj(size_t maj_start_bit, size_t maj_stop_bit) const;

    /// Sets all bits in the table to zero.
    void clear();

    /// Number of 64 bit words in a column (row) assuming row (column) major indexing.
    inline size_t num_major_u64_padded() const {
        return num_simd_words_major * (sizeof(simd_word) / sizeof(uint64_t));
    }
    /// Number of 32 bit words in a column (row) assuming row (column) major indexing.
    inline size_t num_major_u32_padded() const {
        return num_simd_words_major * (sizeof(simd_word) / sizeof(uint32_t));
    }
    /// Number of 16 bit words in a column (row) assuming row (column) major indexing.
    inline size_t num_major_u16_padded() const {
        return num_simd_words_major * (sizeof(simd_word) / sizeof(uint16_t));
    }
    /// Number of 8 bit words in a column (row) assuming row (column) major indexing.
    inline size_t num_major_u8_padded() const {
        return num_simd_words_major * (sizeof(simd_word) / sizeof(uint8_t));
    }
    /// Number of bits in a column (row) assuming row (column) major indexing.
    inline size_t num_major_bits_padded() const {
        return num_simd_words_major * sizeof(simd_word) << 3;
    }

    /// Number of 64 bit words in a row (column) assuming row (column) major indexing.
    inline size_t num_minor_u64_padded() const {
        return num_simd_words_minor * (sizeof(simd_word) / sizeof(uint64_t));
    }
    /// Number of 32 bit words in a row (column) assuming row (column) major indexing.
    inline size_t num_minor_u32_padded() const {
        return num_simd_words_minor * (sizeof(simd_word) / sizeof(uint32_t));
    }
    /// Number of 16 bit words in a row (column) assuming row (column) major indexing.
    inline size_t num_minor_u16_padded() const {
        return num_simd_words_minor * (sizeof(simd_word) / sizeof(uint16_t));
    }
    /// Number of 8 bit words in a row (column) assuming row (column) major indexing.
    inline size_t num_minor_u8_padded() const {
        return num_simd_words_minor * (sizeof(simd_word) / sizeof(uint8_t));
    }
    /// Number of bits in a row (column) assuming row (column) major indexing.
    inline size_t num_minor_bits_padded() const {
        return num_simd_words_minor * sizeof(simd_word) << 3;
    }

    /// Returns a truncated description of the table's contents.
    std::string str(size_t n) const;
};

constexpr uint8_t lg(size_t k) {
    return k <= 1 ? 0 : lg(k >> 1) + 1;
}

#endif
