// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "simd_bit_table.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>

#include "simd_util.h"

simd_bit_table::simd_bit_table(size_t min_bits_major, size_t min_bits_minor)
    : num_simd_words_major(simd_bits::min_bits_to_num_simd_words(min_bits_major)),
      num_simd_words_minor(simd_bits::min_bits_to_num_simd_words(min_bits_minor)),
      data(
          simd_bits::min_bits_to_num_bits_padded(min_bits_minor) *
          simd_bits::min_bits_to_num_bits_padded(min_bits_major)) {
}

simd_bit_table simd_bit_table::identity(size_t n) {
    simd_bit_table result(n, n);
    for (size_t k = 0; k < n; k++) {
        result[k][k] = true;
    }
    return result;
}

void simd_bit_table::clear() {
    data.clear();
}

bool simd_bit_table::operator==(const simd_bit_table &other) const {
    return num_simd_words_minor == other.num_simd_words_minor && num_simd_words_major == other.num_simd_words_major &&
           data == other.data;
}
bool simd_bit_table::operator!=(const simd_bit_table &other) const {
    return !(*this == other);
}

simd_bit_table simd_bit_table::square_mat_mul(const simd_bit_table &rhs, size_t n) const {
    assert(num_major_bits_padded() >= n && num_minor_bits_padded() >= n);
    assert(rhs.num_major_bits_padded() >= n && rhs.num_minor_bits_padded() >= n);

    auto tmp = rhs.transposed();

    simd_bit_table result(n, n);
    for (size_t row = 0; row < n; row++) {
        for (size_t col = 0; col < n; col++) {
            simd_word acc{};
            (*this)[row].for_each_word(tmp[col], [&](simd_word &w1, simd_word &w2) {
                acc ^= w1 & w2;
            });
            result[row][col] = acc.popcount() & 1;
        }
    }

    return result;
}

simd_bit_table simd_bit_table::inverse_assuming_lower_triangular(size_t n) const {
    assert(num_major_bits_padded() >= n && num_minor_bits_padded() >= n);

    simd_bit_table result = simd_bit_table::identity(n);
    simd_bits copy_row(num_minor_bits_padded());
    for (size_t target = 0; target < n; target++) {
        copy_row = (*this)[target];
        for (size_t pivot = 0; pivot < target; pivot++) {
            if (copy_row[pivot]) {
                copy_row ^= (*this)[pivot];
                result[target] ^= result[pivot];
            }
        }
    }
    return result;
}

template <uint8_t step>
void rc_address_bit_swap(simd_bit_table &table, size_t base, size_t end) {
    auto mask = simd_word::tile64(interleave_mask(step));
    for (size_t major = base; major < end; major++, major += major & step) {
        table[major].for_each_word(table[major + step], [&mask](simd_word &a, simd_word &b) {
            auto t0 = a ^ b.leftshift_tile64(step);
            auto t1 = a.rightshift_tile64(step) ^ b;
            a ^= mask.andnot(t0);
            b ^= mask & t1;
        });
    }
}

template <uint8_t step>
void rc3456_address_bit_rotate_swap(simd_bit_table &table, size_t m1, size_t m2) {
    for (size_t major = m1; major < m2; major++, major += major & step) {
        table[major].for_each_word(table[major + step], [](simd_word &a, simd_word &b) {
            a.do_interleave8_tile128(b);
        });
    }
}

void rc_address_word_swap(simd_bit_table &t) {
    constexpr uint16_t block_diameter = sizeof(uint64_t) << 4;
    constexpr uint8_t block_shift = lg(block_diameter);
    size_t n = t.num_major_bits_padded();
    size_t num_blocks = n >> block_shift;
    auto *words = &t.data.ptr_simd[0].u128[0];
    for (size_t block_row = 0; block_row < num_blocks; block_row++) {
        for (size_t block_col = block_row + 1; block_col < num_blocks; block_col++) {
            size_t rc = block_row * n + block_col;
            size_t cr = block_col * n + block_row;
            for (size_t k = 0; k < n; k += num_blocks) {
                std::swap(words[rc + k], words[cr + k]);
            }
        }
    }
}

void simd_bit_table::do_square_transpose() {
    assert(num_simd_words_minor == num_simd_words_major);

    size_t n = num_major_bits_padded();
    for (size_t base = 0; base < n; base += 128) {
        size_t end = base + 128;
        rc3456_address_bit_rotate_swap<64>(*this, base, end);
        rc3456_address_bit_rotate_swap<32>(*this, base, end);
        rc_address_bit_swap<1>(*this, base, end);
        rc_address_bit_swap<2>(*this, base, end);
        rc_address_bit_swap<4>(*this, base, end);
        rc3456_address_bit_rotate_swap<16>(*this, base, end);
        rc3456_address_bit_rotate_swap<8>(*this, base, end);
    }
    rc_address_word_swap(*this);
}

simd_bit_table simd_bit_table::transposed() const {
    simd_bit_table result(num_minor_bits_padded(), num_major_bits_padded());
    transpose_into(result);
    return result;
}

simd_bit_table simd_bit_table::slice_maj(size_t maj_start_bit, size_t maj_stop_bit) const {
    simd_bit_table result(maj_stop_bit - maj_start_bit, num_minor_bits_padded());
    for (size_t k = maj_start_bit; k < maj_stop_bit; k++) {
        result[k - maj_start_bit] = (*this)[k];
    }
    return result;
}

void simd_bit_table::transpose_into(simd_bit_table &out) const {
    assert(out.num_simd_words_minor == num_simd_words_major);
    assert(out.num_simd_words_major == num_simd_words_minor);

    auto n_maj = num_major_bits_padded();
    auto n_min = num_minor_bits_padded();
    for (size_t min = 0; min < n_min; min += 128) {
        for (size_t maj = 0; maj < n_maj; maj += 128) {
            for (size_t common = 0; common < 128; common++) {
                auto *dst = &out[min | common].ptr_simd[0].u128[0];
                auto *src = &(*this)[maj | common].ptr_simd[0].u128[0];
                dst[maj >> 7] = src[min >> 7];
            }
        }
        size_t end = min + 128;
        rc3456_address_bit_rotate_swap<64>(out, min, end);
        rc3456_address_bit_rotate_swap<32>(out, min, end);
        rc_address_bit_swap<1>(out, min, end);
        rc_address_bit_swap<2>(out, min, end);
        rc_address_bit_swap<4>(out, min, end);
        rc3456_address_bit_rotate_swap<16>(out, min, end);
        rc3456_address_bit_rotate_swap<8>(out, min, end);
    }
}

simd_bit_table simd_bit_table::from_quadrants(
    size_t n, const simd_bit_table &upper_left, const simd_bit_table &upper_right, const simd_bit_table &lower_left,
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

simd_bit_table simd_bit_table::random(
    size_t num_randomized_major_bits, size_t num_randomized_minor_bits, std::mt19937_64 &rng) {
    simd_bit_table result(num_randomized_major_bits, num_randomized_minor_bits);
    for (size_t maj = 0; maj < num_randomized_major_bits; maj++) {
        result[maj].randomize(num_randomized_minor_bits, rng);
    }
    return result;
}
