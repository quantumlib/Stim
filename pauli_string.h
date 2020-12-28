#ifndef PAULI_STRING_H
#define PAULI_STRING_H

#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include <functional>
#include "simd_util.h"

struct PauliStringPtr {
    size_t size;
    bool *sign;
    __m256i *_x;
    __m256i *_y;

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

    std::string str() const;

    bool operator==(const PauliStringPtr &other) const;
    bool operator!=(const PauliStringPtr &other) const;
    PauliStringPtr& operator*=(const PauliStringPtr& rhs);
    void inplace_times_with_scalar_output(const PauliStringPtr& rhs, uint8_t *out_log_i) const;
};

std::ostream &operator<<(std::ostream &out, const PauliStringPtr &ps);

struct PauliStringStorage {
    size_t size;
    bool sign;
    std::vector<__m256i> x;
    std::vector<__m256i> y;

    static PauliStringStorage from_pattern(bool sign, size_t size, const std::function<char(size_t)> &func);
    static PauliStringStorage from_str(const char *text);
    PauliStringPtr ptr();
};

#endif
