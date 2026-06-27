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
///
/// The template parameter, W, represents the SIMD width.
template <size_t W>
struct PauliString {
    /// The length of the Pauli string.
    size_t num_qubits;
    /// Whether or not the Pauli string is negated. True means -1, False means +1. Imaginary phase is not permitted.
    bool sign;
    /// The Paulis in the Pauli string, densely bit packed in a fashion enabling the use vectorized instructions.
    /// Paulis are xz-encoded (P=xz: I=00, X=10, Y=11, Z=01) pairwise across the two bit vectors.
    simd_bits<W> xs, zs;

    /// Standard constructors.
    explicit PauliString(size_t num_qubits);
    PauliString(const PauliStringRef<W> &other);  // NOLINT(google-explicit-constructor)
    PauliString(const PauliString<W> &other);
    PauliString(PauliString<W> &&other) noexcept;
    PauliString &operator=(const PauliStringRef<W> &other);
    PauliString &operator=(const PauliString<W> &other);
    PauliString &operator=(PauliString<W> &&other);

    /// Parse constructor.
    explicit PauliString(std::string_view text);
    /// Factory method for creating a PauliString whose Pauli entries are returned by a function.
    static PauliString<W> from_func(bool sign, size_t num_qubits, const std::function<char(size_t)> &func);
    /// Factory method for creating a PauliString by parsing a string (e.g. "-XIIYZ").
    static PauliString<W> from_str(std::string_view text);
    /// Factory method for creating a PauliString with uniformly random sign and Pauli entries.
    static PauliString<W> random(size_t num_qubits, std::mt19937_64 &rng);

    /// Equality.
    bool operator==(const PauliStringRef<W> &other) const;
    bool operator==(const PauliString<W> &other) const;
    /// Inequality.
    bool operator!=(const PauliStringRef<W> &other) const;
    bool operator!=(const PauliString<W> &other) const;
    bool operator<(const PauliStringRef<W> &other) const;
    bool operator<(const PauliString<W> &other) const;

    /// Implicit conversion to a reference.
    operator PauliStringRef<W>();
    /// Implicit conversion to a const reference.
    operator const PauliStringRef<W>() const;
    /// Explicit conversion to a reference.
    PauliStringRef<W> ref();
    /// Explicit conversion to a const reference.
    const PauliStringRef<W> ref() const;

    /// Returns a python-style slice of the Paulis in the Pauli string.
    PauliString<W> py_get_slice(int64_t start, int64_t step, int64_t slice_length) const;
    /// Returns a Pauli from the pauli string, allowing python-style negative indices, using IXYZ encoding.
    uint8_t py_get_item(int64_t index) const;

    /// Returns a string describing the given Pauli string, with one character per qubit.
    std::string str() const;

    /// Grows the pauli string to be at least as large as the given number
    /// of qubits.
    ///
    /// Requires:
    ///     resize_pad_factor >= 1
    ///
    /// Args:
    ///     min_num_qubits: A minimum number of qubits that will be needed.
    ///     resize_pad_factor: When resizing, memory will be overallocated
    ///          so that the pauli string can be expanded to at least this
    ///          many times the number of requested qubits. Use this to
    ///          avoid quadratic overheads from constant slight expansions.
    void ensure_num_qubits(size_t min_num_qubits, double resize_pad_factor);

    void mul_pauli_term(GateTarget t, bool *imag, bool right_mul);
    void left_mul_pauli(GateTarget t, bool *imag);
    void right_mul_pauli(GateTarget t, bool *imag);
};

/// Writes a string describing the given Pauli string to an output stream.
///
/// The template parameter, W, represents the SIMD width.
template <size_t W>
std::ostream &operator<<(std::ostream &out, const PauliString<W> &ps);

}  // namespace stim

#include "stim/stabilizers/pauli_string.inl"

#endif
