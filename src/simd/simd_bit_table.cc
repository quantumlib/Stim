#include <algorithm>
#include <cassert>
#include <cstring>
#include <immintrin.h>
#include <random>
#include <sstream>

#include "simd_bit_table.h"
#include "simd_util.h"

simd_bit_table::simd_bit_table(size_t min_bits_major, size_t min_bits_minor) :
    num_simd_words_major(simd_bits::min_bits_to_num_simd_words(min_bits_major)),
    num_simd_words_minor(simd_bits::min_bits_to_num_simd_words(min_bits_minor)),
    data(simd_bits::min_bits_to_num_bits_padded(min_bits_minor) * simd_bits::min_bits_to_num_bits_padded(min_bits_major)) {
}

simd_bit_table simd_bit_table::identity(size_t n) {
    simd_bit_table result(n, n);
    for (size_t k = 0; k < n; k++) {
        result[k][k] = true;
    }
    return result;
}

simd_bits_range_ref simd_bit_table::operator[](size_t major_index) {
    return data.word_range_ref(major_index * num_simd_words_minor, num_simd_words_minor);
}

const simd_bits_range_ref simd_bit_table::operator[](size_t major_index) const {
    return data.word_range_ref(major_index * num_simd_words_minor, num_simd_words_minor);
}

void simd_bit_table::clear() {
    data.clear();
}

bool simd_bit_table::operator==(const simd_bit_table &other) const {
    return num_simd_words_minor == other.num_simd_words_minor
        && num_simd_words_major == other.num_simd_words_major
        && data == other.data;
}
bool simd_bit_table::operator!=(const simd_bit_table &other) const {
    return !(*this == other);
}

simd_bit_table simd_bit_table::square_mat_mul(const simd_bit_table &rhs, size_t n) const {
    assert(num_major_bits_padded() >= n && num_minor_bits_padded() >= n);
    assert(rhs.num_major_bits_padded() >= n && rhs.num_minor_bits_padded() >= n);

    simd_bit_table result(n, n);
    for (size_t row = 0; row < n; row++) {
        for (size_t col = 0; col < n; col++) {
            bool b = false;
            for (size_t mid = 0; mid < n; mid++) {
                b ^= (*this)[row][mid] & rhs[mid][col];
            }
            result[row][col] = b;
        }
    }
    return result;
}

simd_bit_table simd_bit_table::inverse_assuming_lower_triangular(size_t n) const {
    assert(num_major_bits_padded() >= n && num_minor_bits_padded() >= n);

    simd_bit_table copy = *this;
    simd_bit_table result = simd_bit_table::identity(n);
    for (size_t pivot = 0; pivot < n; pivot++) {
        for (size_t target = pivot + 1; target < n; target++) {
            if (copy[target][pivot]) {
                copy[target] ^= copy[pivot];
                result[target] ^= result[pivot];
            }
        }
    }
    return result;
}

void simd_bit_table::do_square_transpose() {
    assert(num_major_bits_padded() == num_minor_bits_padded());
    transpose_bit_matrix(data.u64, num_major_bits_padded());
}

simd_bit_table simd_bit_table::from_quadrants(
        size_t n,
        const simd_bit_table &upper_left,
        const simd_bit_table &upper_right,
        const simd_bit_table &lower_left,
        const simd_bit_table &lower_right) {
    assert(upper_left.num_minor_bits_padded() >= n && upper_left.num_major_bits_padded() >= n);
    assert(upper_right.num_minor_bits_padded() >= n && upper_right.num_major_bits_padded() >= n);
    assert(lower_left.num_minor_bits_padded() >= n && lower_left.num_major_bits_padded() >= n);
    assert(lower_right.num_minor_bits_padded() >= n && lower_right.num_major_bits_padded() >= n);

    simd_bit_table result(n << 1, n << 1);
    for (size_t row = 0; row < n; row++) {
        for (size_t col = 0; col < n; col++) {
            result[row][col] = upper_left[row][col];
            result[row][col + n] = upper_right[row][col];
            result[row + n][col] = lower_left[row][col];
            result[row + n][col + n] = lower_right[row][col];
        }
    }
    return result;
}

std::string simd_bit_table::str(size_t n) const {
    std::stringstream out;
    for (size_t row = 0; row < n; row++) {
        if (row) {
            out << "\n";
        }
        for (size_t col = 0; col < n; col++) {
            out << ".1"[(*this)[row][col]];
        }
    }
    return out.str();
}
