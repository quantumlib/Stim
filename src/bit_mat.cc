#include <cassert>

#include <sstream>
#include "bit_mat.h"
#include "simd_util.h"

BitMat::BitMat(size_t n) : n(n), data(ceil256(n) * ceil256(n)) {
}

BitMat BitMat::zero(size_t n) {
    return BitMat(n);
}

BitMat BitMat::identity(size_t n) {
    BitMat result(n);
    for (size_t k = 0; k < n; k++) {
        result.set(k, k, true);
    }
    return result;
}

bool BitMat::get(size_t row, size_t col) const {
    return data.get_bit(row * ceil256(n) + col);
}

void BitMat::set(size_t row, size_t col, bool new_value) {
    return data.set_bit(row * ceil256(n) + col, new_value);
}

__m256i *BitMat::row(size_t k) const {
    return &data.u256[k * (ceil256(n) >> 8)];
}

void BitMat::xor_row_into(size_t src_row, size_t dst_row) {
    auto src = row(src_row);
    auto dst = row(dst_row);
    auto end = src + (ceil256(n) >> 8);
    while (src != end) {
        *dst ^= *src;
        src++;
        dst++;
    }
}

BitMat BitMat::operator*(const BitMat &rhs) {
    assert(n == rhs.n);
    BitMat result(n);
    for (size_t row = 0; row < n; row++) {
        for (size_t col = 0; col < n; col++) {
            bool b = false;
            for (size_t mid = 0; mid < n; mid++) {
                b ^= get(row, mid) & rhs.get(mid, col);
            }
            result.set(row, col, b);
        }
    }
    return result;
}

BitMat BitMat::inv_lower_triangular() const {
    auto m = *this;
    auto result = BitMat::identity(n);
    for (size_t pivot = 0; pivot < n; pivot++) {
        for (size_t target = pivot + 1; target < n; target++) {
            if (m.get(target, pivot)) {
                m.xor_row_into(pivot, target);
                result.xor_row_into(pivot, target);
            }
        }
    }
    return result;
}

BitMat BitMat::transposed() const {
    BitMat result(n);
    for (size_t row = 0; row < n; row++) {
        for (size_t col = 0; col < n; col++) {
            result.set(row, col, get(col, row));
        }
    }
    return result;
}

bool BitMat::operator==(const BitMat &other) const {
    return n == other.n && data == other.data;
}
bool BitMat::operator!=(const BitMat &other) const {
    return !(*this == other);
}

std::string BitMat::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

std::ostream &operator<<(std::ostream &out, const BitMat &m) {
    for (size_t row = 0; row < m.n; row++) {
        if (row) {
            out << "\n";
        }
        for (size_t col = 0; col < m.n; col++) {
            out << (m.get(row, col) ? '1' : '.');
        }
    }
    return out;
}

BitMat BitMat::from_quadrants(
            const BitMat &upper_left,
            const BitMat &upper_right,
            const BitMat &lower_left,
            const BitMat &lower_right) {
    auto n = upper_left.n;
    assert(n == upper_right.n);
    assert(n == lower_left.n);
    assert(n == lower_right.n);
    BitMat result(2*n);
    for (size_t row = 0; row < n; row++) {
        for (size_t col = 0; col < n; col++) {
            result.set(row, col, upper_left.get(row, col));
            result.set(row, col + n, upper_right.get(row, col));
            result.set(row + n, col, lower_left.get(row, col));
            result.set(row + n, col + n, lower_right.get(row, col));
        }
    }
    return result;
}
