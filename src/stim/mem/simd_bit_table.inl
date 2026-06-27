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

#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>

namespace stim {

template <size_t W>
simd_bit_table<W>::simd_bit_table(size_t min_bits_major, size_t min_bits_minor)
    : num_simd_words_major(min_bits_to_num_simd_words<W>(min_bits_major)),
      num_simd_words_minor(min_bits_to_num_simd_words<W>(min_bits_minor)),
      data(min_bits_to_num_bits_padded<W>(min_bits_minor) * min_bits_to_num_bits_padded<W>(min_bits_major)) {
}

template <size_t W>
simd_bit_table<W> simd_bit_table<W>::identity(size_t n) {
    simd_bit_table<W> result(n, n);
    for (size_t k = 0; k < n; k++) {
        result[k][k] = true;
    }
    return result;
}

template <size_t W>
void simd_bit_table<W>::clear() {
    data.clear();
}

template <size_t W>
bool simd_bit_table<W>::operator==(const simd_bit_table<W> &other) const {
    return num_simd_words_minor == other.num_simd_words_minor && num_simd_words_major == other.num_simd_words_major &&
           data == other.data;
}

template <size_t W>
bool simd_bit_table<W>::operator!=(const simd_bit_table<W> &other) const {
    return !(*this == other);
}

template <size_t W>
simd_bit_table<W> simd_bit_table<W>::square_mat_mul(const simd_bit_table<W> &rhs, size_t n) const {
    assert(num_major_bits_padded() >= n && num_minor_bits_padded() >= n);
    assert(rhs.num_major_bits_padded() >= n && rhs.num_minor_bits_padded() >= n);

    auto tmp = rhs.transposed();

    simd_bit_table<W> result(n, n);
    for (size_t row = 0; row < n; row++) {
        for (size_t col = 0; col < n; col++) {
            bitword<W> acc{};
            (*this)[row].for_each_word(tmp[col], [&](bitword<W> &w1, bitword<W> &w2) {
                acc ^= w1 & w2;
            });
            result[row][col] = acc.popcount() & 1;
        }
    }

    return result;
}

template <size_t W>
simd_bit_table<W> simd_bit_table<W>::inverse_assuming_lower_triangular(size_t n) const {
    assert(num_major_bits_padded() >= n && num_minor_bits_padded() >= n);

    simd_bit_table<W> result = simd_bit_table<W>::identity(n);
    simd_bits<W> copy_row(num_minor_bits_padded());
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

template <size_t W>
void exchange_low_indices(simd_bit_table<W> &table) {
    for (size_t maj_high = 0; maj_high < table.num_simd_words_major; maj_high++) {
        for (size_t min_high = 0; min_high < table.num_simd_words_minor; min_high++) {
            size_t block_start = table.get_index_of_bitword(maj_high, 0, min_high);
            bitword<W>::inplace_transpose_square(table.data.ptr_simd + block_start, table.num_simd_words_minor);
        }
    }
}

template <size_t W>
void simd_bit_table<W>::destructive_resize(size_t new_min_bits_major, size_t new_min_bits_minor) {
    num_simd_words_minor = min_bits_to_num_simd_words<W>(new_min_bits_minor);
    num_simd_words_major = min_bits_to_num_simd_words<W>(new_min_bits_major);
    data.destructive_resize(num_simd_words_minor * num_simd_words_major * W * W);
}

template <size_t W>
void simd_bit_table<W>::copy_into_different_size_table(simd_bit_table<W> &other) const {
    size_t ni = num_simd_words_minor;
    size_t na = num_simd_words_major;
    size_t mi = other.num_simd_words_minor;
    size_t ma = other.num_simd_words_major;
    size_t num_min_bytes = std::min(ni, mi) * (W / 8);
    size_t num_maj = std::min(na, ma) * W;

    if (ni == mi) {
        memcpy(other.data.ptr_simd, data.ptr_simd, num_min_bytes * num_maj);
    } else {
        for (size_t maj = 0; maj < num_maj; maj++) {
            memcpy(other[maj].ptr_simd, (*this)[maj].ptr_simd, num_min_bytes);
        }
    }
}

template <size_t W>
void simd_bit_table<W>::resize(size_t new_min_bits_major, size_t new_min_bits_minor) {
    auto new_num_simd_words_minor = min_bits_to_num_simd_words<W>(new_min_bits_minor);
    auto new_num_simd_words_major = min_bits_to_num_simd_words<W>(new_min_bits_major);
    if (new_num_simd_words_major == num_simd_words_major && new_num_simd_words_minor == num_simd_words_minor) {
        return;
    }
    auto new_table = simd_bit_table<W>(new_min_bits_major, new_min_bits_minor);
    copy_into_different_size_table(new_table);
    *this = std::move(new_table);
}

template <size_t W>
void simd_bit_table<W>::do_square_transpose() {
    assert(num_simd_words_minor == num_simd_words_major);

    // Current address tensor indices: [...min_low ...min_high ...maj_low ...maj_high]

    exchange_low_indices(*this);

    // Current address tensor indices: [...maj_low ...min_high ...min_low ...maj_high]

    // Permute data such that high address bits of majors and minors are exchanged.
    for (size_t maj_high = 0; maj_high < num_simd_words_major; maj_high++) {
        for (size_t min_high = maj_high + 1; min_high < num_simd_words_minor; min_high++) {
            for (size_t maj_low = 0; maj_low < W; maj_low++) {
                std::swap(
                    data.ptr_simd[get_index_of_bitword(maj_high, maj_low, min_high)],
                    data.ptr_simd[get_index_of_bitword(min_high, maj_low, maj_high)]);
            }
        }
    }
    // Current address tensor indices: [...maj_low ...maj_high ...min_low ...min_high]
}

template <size_t W>
simd_bit_table<W> simd_bit_table<W>::transposed() const {
    simd_bit_table<W> result(num_minor_bits_padded(), num_major_bits_padded());
    transpose_into(result);
    return result;
}

template <size_t W>
simd_bits<W> simd_bit_table<W>::read_across_majors_at_minor_index(
    size_t major_start, size_t major_stop, size_t minor_index) const {
    assert(major_stop >= major_start);
    assert(major_stop <= num_major_bits_padded());
    assert(minor_index < num_minor_bits_padded());
    simd_bits<W> result(major_stop - major_start);
    for (size_t maj = major_start; maj < major_stop; maj++) {
        result[maj - major_start] = (*this)[maj][minor_index];
    }
    return result;
}

template <size_t W>
simd_bit_table<W> simd_bit_table<W>::slice_maj(size_t maj_start_bit, size_t maj_stop_bit) const {
    simd_bit_table<W> result(maj_stop_bit - maj_start_bit, num_minor_bits_padded());
    for (size_t k = maj_start_bit; k < maj_stop_bit; k++) {
        result[k - maj_start_bit] = (*this)[k];
    }
    return result;
}

template <size_t W>
void simd_bit_table<W>::transpose_into(simd_bit_table<W> &out) const {
    assert(out.num_simd_words_minor == num_simd_words_major);
    assert(out.num_simd_words_major == num_simd_words_minor);

    for (size_t maj_high = 0; maj_high < num_simd_words_major; maj_high++) {
        for (size_t min_high = 0; min_high < num_simd_words_minor; min_high++) {
            for (size_t maj_low = 0; maj_low < W; maj_low++) {
                size_t src_index = get_index_of_bitword(maj_high, maj_low, min_high);
                size_t dst_index = out.get_index_of_bitword(min_high, maj_low, maj_high);
                out.data.ptr_simd[dst_index] = data.ptr_simd[src_index];
            }
        }
    }

    exchange_low_indices(out);
}

template <size_t W>
simd_bit_table<W> simd_bit_table<W>::from_quadrants(
    size_t n,
    const simd_bit_table<W> &upper_left,
    const simd_bit_table<W> &upper_right,
    const simd_bit_table<W> &lower_left,
    const simd_bit_table<W> &lower_right) {
    assert(upper_left.num_minor_bits_padded() >= n && upper_left.num_major_bits_padded() >= n);
    assert(upper_right.num_minor_bits_padded() >= n && upper_right.num_major_bits_padded() >= n);
    assert(lower_left.num_minor_bits_padded() >= n && lower_left.num_major_bits_padded() >= n);
    assert(lower_right.num_minor_bits_padded() >= n && lower_right.num_major_bits_padded() >= n);

    simd_bit_table<W> result(n << 1, n << 1);
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

template <size_t W>
std::string simd_bit_table<W>::str(size_t rows, size_t cols) const {
    std::stringstream out;
    for (size_t row = 0; row < rows; row++) {
        if (row) {
            out << "\n";
        }
        for (size_t col = 0; col < cols; col++) {
            out << ".1"[(*this)[row][col]];
        }
    }
    return out.str();
}

template <size_t W>
std::string simd_bit_table<W>::str(size_t n) const {
    return str(n, n);
}

template <size_t W>
std::string simd_bit_table<W>::str() const {
    return str(num_major_bits_padded(), num_minor_bits_padded());
}

template <size_t W>
simd_bit_table<W> simd_bit_table<W>::concat_major(
    const simd_bit_table<W> &second, size_t n_first, size_t n_second) const {
    if (num_major_bits_padded() < n_first || second.num_major_bits_padded() < n_second ||
        num_minor_bits_padded() != second.num_minor_bits_padded()) {
        throw std::invalid_argument("Size mismatch");
    }
    simd_bit_table<W> result(n_first + n_second, num_minor_bits_padded());
    auto n1 = n_first * num_minor_u8_padded();
    auto n2 = n_second * num_minor_u8_padded();
    memcpy(result.data.u8, data.u8, n1);
    memcpy(result.data.u8 + n1, second.data.u8, n2);
    return result;
}

template <size_t W>
void simd_bit_table<W>::overwrite_major_range_with(
    size_t dst_major_start, const simd_bit_table<W> &src, size_t src_major_start, size_t num_major_indices) const {
    assert(src.num_minor_bits_padded() == num_minor_bits_padded());
    memcpy(
        data.u8 + dst_major_start * num_minor_u8_padded(),
        src.data.u8 + src_major_start * src.num_minor_u8_padded(),
        num_major_indices * num_minor_u8_padded());
}

template <size_t W>
simd_bit_table<W> simd_bit_table<W>::from_text(const char *text, size_t min_rows, size_t min_cols) {
    std::vector<std::vector<bool>> lines;
    lines.push_back({});

    // Skip indentation.
    while (*text == '\n' || *text == ' ') {
        text++;
    }

    for (const char *c = text; *c;) {
        if (*c == '\n') {
            lines.push_back({});
            c++;
            // Skip indentation.
            while (*c == ' ') {
                c++;
            }
        } else if (*c == '0' || *c == '.' || *c == '_') {
            lines.back().push_back(false);
            c++;
        } else if (*c == '1') {
            lines.back().push_back(true);
            c++;
        } else {
            throw std::invalid_argument(
                "Expected indented characters from \"10._\\n\". Got '" + std::string(1, *c) + "'.");
        }
    }

    // Remove trailing newline.
    if (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }

    size_t num_cols = min_cols;
    for (const auto &v : lines) {
        num_cols = std::max(v.size(), num_cols);
    }
    size_t num_rows = std::max(min_rows, lines.size());
    simd_bit_table<W> out(num_rows, num_cols);
    for (size_t row = 0; row < lines.size(); row++) {
        for (size_t col = 0; col < lines[row].size(); col++) {
            out[row][col] = lines[row][col];
        }
    }

    return out;
}

template <size_t W>
simd_bit_table<W> simd_bit_table<W>::random(
    size_t num_randomized_major_bits, size_t num_randomized_minor_bits, std::mt19937_64 &rng) {
    simd_bit_table<W> result(num_randomized_major_bits, num_randomized_minor_bits);
    for (size_t maj = 0; maj < num_randomized_major_bits; maj++) {
        result[maj].randomize(num_randomized_minor_bits, rng);
    }
    return result;
}

template <size_t W>
std::ostream &operator<<(std::ostream &out, const stim::simd_bit_table<W> &v) {
    for (size_t k = 0; k < v.num_major_bits_padded(); k++) {
        if (k) {
            out << '\n';
        }
        out << v[k];
    }
    return out;
}

}  // namespace stim
