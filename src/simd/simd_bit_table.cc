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

template <uint8_t step>
void rc_address_bit_swap(simd_bit_table &t, size_t base, size_t len) {
    size_t end = base + len;
    auto mask4 = SIMD_WORD_TYPE::tile(interleave_mask(step));
    for (size_t major = base; major < end; major++, major += major & step) {
        t[major].for_each_word(t[major + step], [&mask4](auto &a, auto &b) {
            auto a1 = mask4.andnot(a);
            auto b0 = b & mask4;
            a = (a & mask4) | b0.leftshift_tile64(step);
            b = mask4.andnot(b) | a1.rightshift_tile64(step);
        });
    }
}

template <uint8_t step>
void rc3456_address_bit_rotate_swap(simd_bit_table &t, size_t m1, size_t m2) {
    for (size_t major = m1; major < m2; major++, major += major & step) {
        t[major].for_each_word(t[major + step], [](auto &a, auto &b){
            a.interleave8_128_with(b);
        });
    }
}

constexpr uint8_t lg(size_t k) {
    size_t t = 0;
    while (k > 1) {
        k >>= 1;
        t += 1;
    }
    return t;
}

template <typename word_t>
void rc_address_word_swap(simd_bit_table &t) {
    constexpr uint16_t block_diameter = sizeof(word_t) << 3;
    constexpr uint8_t block_shift = lg(block_diameter);
    size_t n = t.num_major_bits_padded();
    size_t num_blocks = n >> block_shift;
    word_t *words = (word_t *)t.data.ptr_simd;
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
    // Bit order = C0 C1 C2 C3 C4 C5 C6 C7 ... r0 r1 r2 r3 r4 r5 r6 r7 ...
    for (size_t base = 0; base < n; base += 128) {
        auto end = base + 128;
        // Bit order = C0 C1 C2 C3 C4 C5 C6 C7 ... r0 r1 r2 r3 r4 r5 r6 r7 ...
        rc3456_address_bit_rotate_swap<64>(*this, base, end);
        // Bit order = C0 C1 C2 r6 C3 C4 C5 C7 ... r0 r1 r2 r3 r4 r5 C6 r7 ...
        rc3456_address_bit_rotate_swap<32>(*this, base, end);
        // Bit order = C0 C1 C2 r5 r6 C3 C4 C7 ... r0 r1 r2 r3 r4 C5 C6 r7 ...
        for (size_t local_base = base; local_base < end; local_base += 8) {
            // Bit order = C0 C1 C2 r5 r6 C3 C4 C7 ... r0 r1 r2 r3 r4 C5 C6 r7 ...
            rc_address_bit_swap<1>(*this, local_base, 4);
            rc_address_bit_swap<2>(*this, local_base, 8);
            rc_address_bit_swap<1>(*this, local_base + 4, 4);
            // Bit order = r0 r1 C2 r5 r6 C3 C4 C7 ... C0 C1 r2 r3 r4 C5 C6 r7 ...
            rc_address_bit_swap<4>(*this, local_base, 8);
            // Bit order = r0 r1 r2 r5 r6 C3 C4 C7 ... C0 C1 C2 r3 r4 C5 C6 r7 ...
        }
        // Bit order = r0 r1 r2 r5 r6 C3 C4 C7 ... C0 C1 C2 r3 r4 C5 C6 r7 ...
        rc3456_address_bit_rotate_swap<16>(*this, base, end);
        // Bit order = r0 r1 r2 r4 r5 r6 C3 C7 ... C0 C1 C2 r3 C4 C5 C6 r7 ...
        rc3456_address_bit_rotate_swap<8>(*this, base, end);
        // Bit order = r0 r1 r2 r3 r4 r5 r6 C7 ... C0 C1 C2 C3 C4 C5 C6 r7 ...
    }
    // Bit order = r0 r1 r2 r3 r4 r5 r6 C7 ... C0 C1 C2 C3 C4 C5 C6 r7 ...
    rc_address_word_swap<__m128i>(*this);
    // Bit order = r0 r1 r2 r3 r4 r5 r6 r7 ... C0 C1 C2 C3 C4 C5 C6 C7 ...
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
