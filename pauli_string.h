#ifndef PAULI_STRING_H
#define PAULI_STRING_H

#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include <functional>
#include "aligned_bits256.h"
#include "simd_util.h"

struct PauliStringVal;

struct PauliStringPtr {
    size_t size;
    bool *ptr_sign;
    uint64_t *_x;
    uint64_t *_z;
    size_t stride256;

    PauliStringPtr(size_t size, bool *sign_ptr, uint64_t *x, uint64_t *z, size_t stride256);
    PauliStringPtr(const PauliStringVal &other); // NOLINT(google-explicit-constructor)

    bool operator==(const PauliStringPtr &other) const;
    bool operator!=(const PauliStringPtr &other) const;

    void overwrite_with(const PauliStringPtr &other);

    // Computes the scalar term created when multiplying two Pauli strings together.
    // For example, in XZ = iY, the scalar byproduct is i.
    //
    // Returns:
    //     The logarithm, base i, of the scalar byproduct.
    //     0 if the scalar byproduct is 1.
    //     1 if the scalar byproduct is i.
    //     2 if the scalar byproduct is -1.
    //     3 if the scalar byproduct is -i.
    uint8_t log_i_scalar_byproduct(const PauliStringPtr &other) const;

    void gather_into(PauliStringPtr &out, const std::vector<size_t> &in_indices) const;
    void scatter_into(PauliStringPtr &out, const std::vector<size_t> &out_indices) const;

    std::string str() const;

    PauliStringPtr& operator*=(const PauliStringPtr &rhs);
    uint8_t inplace_right_mul_with_scalar_output(const PauliStringPtr& rhs);
    size_t num_words256() const;

    bool get_x_bit(size_t k) const;
    bool get_z_bit(size_t k) const;
    void set_x_bit(size_t k, bool b);
    void set_z_bit(size_t k, bool b);
    void toggle_x_bit(size_t k);
    void toggle_z_bit(size_t k);
};

struct PauliStringVal {
    bool val_sign;
    aligned_bits256 x_data;
    aligned_bits256 z_data;

    explicit PauliStringVal(size_t size);
    PauliStringVal(const PauliStringPtr &other); // NOLINT(google-explicit-constructor)
    PauliStringVal& operator=(const PauliStringPtr &other) noexcept;

    bool operator==(const PauliStringPtr &other) const;
    bool operator!=(const PauliStringPtr &other) const;

    static PauliStringVal from_pattern(bool sign, size_t size, const std::function<char(size_t)> &func);
    static PauliStringVal from_str(const char *text);
    static PauliStringVal identity(size_t size);
    PauliStringPtr ptr() const;

    std::string str() const;
};

std::ostream &operator<<(std::ostream &out, const PauliStringPtr &ps);
std::ostream &operator<<(std::ostream &out, const PauliStringVal &ps);

#endif
