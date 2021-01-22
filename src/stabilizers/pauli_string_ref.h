#ifndef PAULI_STRING_REF_H
#define PAULI_STRING_REF_H

#include <iostream>
#include <immintrin.h>
#include <new>
#include <cassert>
#include <sstream>
#include <functional>
#include "../simd/simd_bits.h"
#include "../simd/simd_bits_range_ref.h"
#include "../simd/bit_ref.h"
#include "../simd/simd_util.h"

struct PauliStringRef {
    size_t num_qubits;
    bit_ref sign_ref;
    simd_bits_range_ref x_ref;
    simd_bits_range_ref z_ref;

    PauliStringRef(size_t num_qubits, bit_ref sign_ref, simd_bits_range_ref x_ref, simd_bits_range_ref z_ref);
    PauliStringRef &operator=(const PauliStringRef &other);

    bool operator==(const PauliStringRef &other) const;
    bool operator!=(const PauliStringRef &other) const;

    void swap_with(PauliStringRef other);

    void gather_into(PauliStringRef out, const std::vector<size_t> &in_indices) const;
    void scatter_into(PauliStringRef out, const std::vector<size_t> &out_indices) const;

    std::string str() const;
    std::string sparse_str() const;

    // Multiplies a commuting Pauli string into this one.
    //
    // ASSERTS:
    //     The given Pauli strings have the same size.
    //     The given Pauli strings commute.
    PauliStringRef& operator*=(const PauliStringRef &commuting_rhs);

    // A more general version  of `*this *= rhs` which works for anti-commuting Paulis.
    //
    // Instead of updating the sign of `*this`, the base i logarithm of a scalar factor that still needs to be included
    // into the result is returned. For example, when multiplying XZ to get iY, the left hand side would become `Y`
    // and the returned value would be `1` (meaning a factor of `i**1 = i` is missing from the `Y`).
    //
    // Returns:
    //     The logarithm, base i, of a scalar byproduct from the multiplication.
    //     0 if the scalar byproduct is 1.
    //     1 if the scalar byproduct is i.
    //     2 if the scalar byproduct is -1.
    //     3 if the scalar byproduct is -i.
    //
    // ASSERTS:
    //     The given Pauli strings have the same size.
    uint8_t inplace_right_mul_returning_log_i_scalar(const PauliStringRef& rhs) noexcept;

    bool commutes(const PauliStringRef& other) const noexcept;
};

std::ostream &operator<<(std::ostream &out, const PauliStringRef &ps);


#endif
