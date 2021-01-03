#ifndef PAULI_STRING_H
#define PAULI_STRING_H

#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include <functional>
#include "simd_util.h"

struct PauliString {
    size_t size = 0;
    bool _sign = false;
    uint64_t *_x = nullptr;
    uint64_t *_y = nullptr;

    ~PauliString();
    explicit PauliString(size_t size);
    PauliString(const PauliString &other);
    PauliString(PauliString &&other) noexcept;

    // Computes the scalar term created when multiplying two Pauli strings together.
    // For example, in XZ = iY, the scalar byproduct is i.
    //
    // Returns:
    //     The logarithm, base i, of the scalar byproduct.
    //     0 if the scalar byproduct is 1.
    //     1 if the scalar byproduct is i.
    //     2 if the scalar byproduct is -1.
    //     3 if the scalar byproduct is -i.
    uint8_t log_i_scalar_byproduct(const PauliString &other) const;

    void gather_into(PauliString &out, const std::vector<size_t> &in_indices) const;
    void scatter_into(PauliString &out, const std::vector<size_t> &out_indices) const;

    std::string str() const;

    bool operator==(const PauliString &other) const;
    bool operator!=(const PauliString &other) const;
    PauliString& operator*=(const PauliString& rhs);
    uint8_t inplace_right_mul_with_scalar_output(const PauliString& rhs);

    bool get_x_bit(size_t k) const;
    bool get_y_bit(size_t k) const;
    void set_x_bit(size_t k, bool b);
    void set_y_bit(size_t k, bool b);
    void toggle_x_bit(size_t k);
    void toggle_y_bit(size_t k);

    static PauliString from_pattern(bool sign, size_t size, const std::function<char(size_t)> &func);
    static PauliString from_str(const char *text);
    static PauliString identity(size_t size);
};

std::ostream &operator<<(std::ostream &out, const PauliString &ps);

#endif
