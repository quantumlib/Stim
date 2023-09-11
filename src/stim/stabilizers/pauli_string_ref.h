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

#ifndef _STIM_STABILIZERS_PAULI_STRING_REF_H
#define _STIM_STABILIZERS_PAULI_STRING_REF_H

#include <iostream>

#include "stim/mem/bit_ref.h"
#include "stim/mem/simd_bits_range_ref.h"
#include "stim/mem/span_ref.h"

namespace stim {

template <size_t W>
struct PauliString;
struct Circuit;
template <size_t W>
struct Tableau;
struct CircuitInstruction;

/// A Pauli string is a product of Pauli operations (I, X, Y, Z) to apply to various qubits.
///
/// A PauliStringRef is a Pauli string whose contents are backed by referenced memory, instead of memory owned by the
/// class instance. For example, the memory may be a row from the densely packed bits of a stabilizer tableau. This
/// avoids unnecessary copying, and allows for conveniently applying operations inplace on existing data.
///
/// The template parameter, W, represents the SIMD width.
template <size_t W>
struct PauliStringRef {
    /// The length of the Pauli string.
    size_t num_qubits;
    /// Whether or not the Pauli string is negated. True means -1, False means +1. Imaginary phase is not permitted.
    bit_ref sign;
    /// The Paulis in the Pauli string, densely bit packed in a fashion enabling the use of vectorized instructions.
    /// Paulis are xz-encoded (P=xz: I=00, X=10, Y=11, Z=01) pairwise across the two bit vectors.
    simd_bits_range_ref<W> xs, zs;

    /// Constructs a PauliStringRef pointing at the given sign, x, and z data.
    ///
    /// Requires:
    ///     xs.num_bits_padded() == zs.num_bits_padded()
    ///     xs.num_simd_words == ceil(num_qubits / W)
    PauliStringRef(size_t num_qubits, bit_ref sign, simd_bits_range_ref<W> xs, simd_bits_range_ref<W> zs);

    /// Equality.
    bool operator==(const PauliStringRef<W> &other) const;
    /// Inequality.
    bool operator!=(const PauliStringRef<W> &other) const;

    /// Overwrite assignment.
    PauliStringRef<W> &operator=(const PauliStringRef<W> &other);
    /// Swap assignment.
    void swap_with(PauliStringRef<W> other);

    /// Multiplies a commuting Pauli string into this one.
    ///
    /// If the two Pauli strings may anticommute, use `inplace_right_mul_returning_log_i_scalar` instead.
    ///
    /// ASSERTS:
    ///     The given Pauli strings have the same size.
    ///     The given Pauli strings commute.
    PauliStringRef<W> &operator*=(const PauliStringRef<W> &commuting_rhs);

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
    uint8_t inplace_right_mul_returning_log_i_scalar(const PauliStringRef<W> &rhs) noexcept;

    /// Overwrites the entire given Pauli string's contents with a subset of Paulis from this Pauli string.
    /// Does not affect the sign of the given Pauli string.
    ///
    /// Args:
    ///     out: The Pauli string to overwrite.
    ///     in_indices: For each qubit position in the output Pauli string, which qubit positions is read from in this
    ///         Pauli string.
    void gather_into(PauliStringRef<W> out, SpanRef<const size_t> in_indices) const;

    /// Overwrites part of the given Pauli string with the contents of this Pauli string.
    /// Also multiplies this Pauli string's sign into the given Pauli string's sign.
    ///
    /// Args:
    ///     out: The Pauli string to partially overwrite.
    ///     out_indices: For each qubit position in this Pauli string, which qubit position is overwritten in the output
    ///         Pauli string.
    void scatter_into(PauliStringRef<W> out, SpanRef<const size_t> out_indices) const;

    /// Determines if this Pauli string commutes with the given Pauli string.
    bool commutes(const PauliStringRef<W> &other) const noexcept;

    /// Returns a string describing the given Pauli string, with one character per qubit.
    std::string str() const;
    /// Returns a string describing the given Pauli string, indexing the Paulis so that identities can be omitted.
    std::string sparse_str() const;

    void after_inplace_broadcast(const Tableau<W> &tableau, SpanRef<const size_t> targets, bool inverse);
    void after_inplace(const Circuit &Circuit);
    void after_inplace(const CircuitInstruction &operation, bool inverse);
    PauliString<W> after(const Circuit &circuit) const;
    PauliString<W> after(const Tableau<W> &tableau, SpanRef<const size_t> indices) const;
    PauliString<W> after(const CircuitInstruction &operation) const;
    PauliString<W> before(const Circuit &circuit) const;
    PauliString<W> before(const Tableau<W> &tableau, SpanRef<const size_t> indices) const;
    PauliString<W> before(const CircuitInstruction &operation) const;

    size_t weight() const;
};

/// Writes a string describing the given Pauli string to an output stream.
template <size_t W>
std::ostream &operator<<(std::ostream &out, const PauliStringRef<W> &ps);

}  // namespace stim

#include "stim/stabilizers/pauli_string_ref.inl"

#endif
