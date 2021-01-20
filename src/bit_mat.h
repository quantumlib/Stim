#ifndef BIT_MAT_H
#define BIT_MAT_H

#include "simd/simd_bits.h"
#include <iostream>
#include <string>

struct BitMat {
    size_t n;
    simd_bits data;

    explicit BitMat(size_t n);
    static BitMat zero(size_t n);
    static BitMat identity(size_t n);
    static BitMat from_quadrants(
            const BitMat &upper_left,
            const BitMat &upper_right,
            const BitMat &lower_left,
            const BitMat &lower_right);

    [[nodiscard]] bool get(size_t row, size_t col) const;
    void set(size_t row, size_t col, bool new_value);
    [[nodiscard]] __m256i *row(size_t k) const;
    void xor_row_into(size_t src_row, size_t dst_row);
    [[nodiscard]] BitMat operator*(const BitMat &rhs);
    [[nodiscard]] BitMat inv_lower_triangular() const;
    [[nodiscard]] BitMat transposed() const;

    bool operator==(const BitMat &other) const;
    bool operator!=(const BitMat &other) const;
    std::string str() const;
};

std::ostream &operator<<(std::ostream &out, const BitMat &m);

#endif
