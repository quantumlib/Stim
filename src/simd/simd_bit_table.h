#ifndef SIMD_BIT_TABLE_H
#define SIMD_BIT_TABLE_H

#include "simd_bits.h"

struct simd_bit_table {
    size_t num_simd_words_major;
    size_t num_simd_words_minor;
    simd_bits data;

    /// Creates zero initialized table.
    simd_bit_table(size_t min_bits_major, size_t min_bits_minor);
    /// Creates a square table with 1s down the diagonal.
    static simd_bit_table identity(size_t n);
    /// Concatenates tables together to form a larger table.
    static simd_bit_table from_quadrants(
            size_t n,
            const simd_bit_table &upper_left,
            const simd_bit_table &upper_right,
            const simd_bit_table &lower_left,
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
    [[nodiscard]] simd_bit_table square_mat_mul(const simd_bit_table &rhs, size_t n) const;
    /// Square matrix inverse, assuming input is lower triangular. n is the diameter of the matrix.
    [[nodiscard]] simd_bit_table inverse_assuming_lower_triangular(size_t n) const;
    /// Transposes the table inplace.
    void do_square_transpose();

    /// Sets all bits in the table to zero.
    void clear();

    /// Number of 64 bit words in a column (row) assuming row (column) major indexing.
    [[nodiscard]] inline size_t num_major_u64_padded() const { return num_simd_words_major << 2; }
    /// Number of 32 bit words in a column (row) assuming row (column) major indexing.
    [[nodiscard]] inline size_t num_major_u32_padded() const { return num_simd_words_major << 3; }
    /// Number of 16 bit words in a column (row) assuming row (column) major indexing.
    [[nodiscard]] inline size_t num_major_u16_padded() const { return num_simd_words_major << 4; }
    /// Number of 8 bit words in a column (row) assuming row (column) major indexing.
    [[nodiscard]] inline size_t num_major_u8_padded() const { return num_simd_words_major << 5; }
    /// Number of bits in a column (row) assuming row (column) major indexing.
    [[nodiscard]] inline size_t num_major_bits_padded() const { return num_simd_words_major << 8; }

    /// Number of 64 bit words in a row (column) assuming row (column) major indexing.
    [[nodiscard]] inline size_t num_minor_u64_padded() const { return num_simd_words_minor << 2; }
    /// Number of 32 bit words in a row (column) assuming row (column) major indexing.
    [[nodiscard]] inline size_t num_minor_u32_padded() const { return num_simd_words_minor << 3; }
    /// Number of 16 bit words in a row (column) assuming row (column) major indexing.
    [[nodiscard]] inline size_t num_minor_u16_padded() const { return num_simd_words_minor << 4; }
    /// Number of 8 bit words in a row (column) assuming row (column) major indexing.
    [[nodiscard]] inline size_t num_minor_u8_padded() const { return num_simd_words_minor << 5; }
    /// Number of bits in a row (column) assuming row (column) major indexing.
    [[nodiscard]] inline size_t num_minor_bits_padded() const { return num_simd_words_minor << 8; }

    /// Returns a truncated description of the table's contents.
    [[nodiscard]] std::string str(size_t n) const;
};

#endif
