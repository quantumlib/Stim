/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STIM_STABILIZERS_PAULI_STRING_H
#define _STIM_STABILIZERS_PAULI_STRING_H

#include <functional>
#include <iostream>

#include "stim/mem/simd_bits.h"
#include "stim/stabilizers/pauli_string_ref.h"

namespace stim {

/// Converts from the xz encoding
///
///     0b00: I
///     0b01: X
///     0b10: Z
///     0b11: Y
///
/// To the xyz encoding
///
///     0: I
///     1: X
///     2: Y
///     3: Z
inline uint8_t pauli_xz_to_xyz(bool x, bool z) {
    return (uint8_t)(x ^ z) | ((uint8_t)z << 1);
}

/// Converts from the xyz encoding
///
///     0: I
///     1: X
///     2: Y
///     3: Z
///
/// To the xz encoding
///
///     0b00: I
///     0b01: X
///     0b10: Z
///     0b11: Y
inline uint8_t pauli_xyz_to_xz(uint8_t xyz) {
    xyz ^= xyz >> 1;
    return xyz;
}

/// A Pauli string is a product of Pauli operations (I, X, Y, Z) to apply to various qubits.
///
/// In most cases, methods will take a PauliStringRef instead of a PauliString. This is because PauliStringRef can
/// have contents referring into densely packed table row data (or to a PauliString or to other sources). Basically,
/// PauliString is for the special somewhat-unusual case where you want to create data to back a PauliStringRef instead
/// of simply passing existing data along. It's a convenience class.
struct PauliString {
    /// The length of the Pauli string.
    size_t num_qubits;
    /// Whether or not the Pauli string is negated. True means -1, False means +1. Imaginary phase is not permitted.
    bool sign;
    /// The Paulis in the Pauli string, densely bit packed in a fashion enabling the use vectorized instructions.
    /// Paulis are xz-encoded (P=xz: I=00, X=10, Y=11, Z=01) pairwise across the two bit vectors.
    simd_bits xs, zs;

    /// Identity constructor.
    explicit PauliString(size_t num_qubits);
    /// Factory method for creating a PauliString whose Pauli entries are returned by a function.
    static PauliString from_func(bool sign, size_t num_qubits, const std::function<char(size_t)> &func);
    /// Factory method for creating a PauliString by parsing a string (e.g. "-XIIYZ").
    static PauliString from_str(const char *text);
    /// Factory method for creating a PauliString with uniformly random sign and Pauli entries.
    static PauliString random(size_t num_qubits, std::mt19937_64 &rng);

    /// Copy constructor.
    PauliString(const PauliStringRef &other);  // NOLINT(google-explicit-constructor)
    /// Overwrite assignment.
    PauliString &operator=(const PauliStringRef &other) noexcept;

    /// Equality.
    bool operator==(const PauliStringRef &other) const;
    /// Inequality.
    bool operator!=(const PauliStringRef &other) const;

    /// Implicit conversion to a reference.
    operator PauliStringRef();
    /// Implicit conversion to a const reference.
    operator const PauliStringRef() const;
    /// Explicit conversion to a reference.
    PauliStringRef ref();
    /// Explicit conversion to a const reference.
    const PauliStringRef ref() const;

    /// Returns a python-style slice of the Paulis in the Pauli string.
    PauliString py_get_slice(int64_t start, int64_t step, int64_t slice_length) const;
    /// Returns a Pauli from the pauli string, allowing python-style negative indices, using IXYZ encoding.
    uint8_t py_get_item(int64_t index) const;

    /// Returns a string describing the given Pauli string, with one character per qubit.
    std::string str() const;

    void ensure_num_qubits(size_t min_num_qubits);
};

/// Writes a string describing the given Pauli string to an output stream.
std::ostream &operator<<(std::ostream &out, const PauliString &ps);

}  // namespace stim

#endif
