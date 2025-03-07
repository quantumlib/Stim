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

#ifndef _STIM_STABILIZERS_TABLEAU_H
#define _STIM_STABILIZERS_TABLEAU_H

#include <complex>
#include <iostream>
#include <unordered_map>

#include "stim/mem/simd_bit_table.h"
#include "stim/mem/simd_util.h"
#include "stim/mem/span_ref.h"
#include "stim/stabilizers/pauli_string.h"

namespace stim {

template <size_t W>
struct TableauHalf {
    size_t num_qubits;
    simd_bit_table<W> xt;
    simd_bit_table<W> zt;
    simd_bits<W> signs;
    PauliStringRef<W> operator[](size_t input_qubit);
    const PauliStringRef<W> operator[](size_t input_qubit) const;
    TableauHalf(size_t num_qubits);
};

/// A Tableau is a stabilizer tableau representation of a Clifford operation.
/// It stores, for each X and Z observable for each qubit, what is produced when
/// conjugating that observable by the operation. In other words, it explains how
/// to transform "input side" Pauli products into "output side" Pauli products.
///
/// The memory layout used by this class is column major, meaning iterating over
/// the output observable is iterating along the grain of memory. This makes
/// prepending operations cheap. To append operations, use TableauTransposedRaii.
///
/// The template parameter, W, represents the SIMD width.
template <size_t W>
struct Tableau {
    size_t num_qubits;
    TableauHalf<W> xs;
    TableauHalf<W> zs;

    explicit Tableau(size_t num_qubits);
    bool operator==(const Tableau &other) const;
    bool operator!=(const Tableau &other) const;

    PauliString<W> eval_y_obs(size_t qubit) const;

    std::string str() const;

    /// Grows the size of the tableau (or leaves it the same) by adding
    /// new rows and columns with identity elements along the diagonal.
    ///
    /// Requires:
    ///     new_num_qubits >= this.num_qubits
    ///     resize_pad_factor >= 1
    ///
    /// Args:
    ///     new_num_qubits: The new number of qubits the tableau represents.
    ///     resize_pad_factor: When resizing, memory will be overallocated
    ///          so that the tableau can be expanded to at least this many
    ///          times the number of requested qubits. Use this to avoid
    ///          quadratic overheads from constant slight expansions.
    void expand(size_t new_num_qubits, double resize_pad_factor);

    /// Creates a Tableau representing the identity operation.
    static Tableau<W> identity(size_t num_qubits);
    /// Creates a Tableau from a PauliString via conjugation
    static Tableau<W> from_pauli_string(const PauliString<W> &pauli_string);
    /// Creates a Tableau representing a randomly sampled Clifford operation from a uniform distribution.
    static Tableau<W> random(size_t num_qubits, std::mt19937_64 &rng);
    /// Returns the inverse Tableau.
    ///
    /// Args:
    ///     skip_signs: Instead of computing the signs, just set them all to positive.
    Tableau<W> inverse(bool skip_signs = false) const;
    /// Returns the Tableau raised to an integer power (using repeated squaring).
    Tableau<W> raised_to(int64_t exponent) const;

    std::vector<std::complex<float>> to_flat_unitary_matrix(bool little_endian) const;
    bool satisfies_invariants() const;

    /// If a Tableau fixes each pauli upto sign, then it is conjugation by a pauli
    bool is_pauli_product() const;

    /// If tableau is conjugation by a pauli, then return that pauli. Else throw exception.
    PauliString<W> to_pauli_string() const;

    /// Creates a Tableau representing a single qubit gate.
    ///
    /// All observables specified using the string format accepted by `PauliString::from_str`.
    /// For example: "-X" or "+Y".
    ///
    /// Args:
    ///    x: The output-side observable that the input-side X observable gets mapped to.
    ///    z: The output-side observable that the input-side Y observable gets mapped to.
    static Tableau<W> gate1(const char *x, const char *z);

    /// Creates a Tableau representing a two qubit gate.
    ///
    /// All observables specified using the string format accepted by `PauliString::from_str`.
    /// For example: "-IX" or "+YZ".
    ///
    /// Args:
    ///    x1: The output-side observable that the input-side XI observable gets mapped to.
    ///    z1: The output-side observable that the input-side YI observable gets mapped to.
    ///    x2: The output-side observable that the input-side IX observable gets mapped to.
    ///    z2: The output-side observable that the input-side IY observable gets mapped to.
    static Tableau<W> gate2(const char *x1, const char *z1, const char *x2, const char *z2);

    /// Returns the result of applying the tableau to the given Pauli string.
    ///
    /// Args:
    ///     p: The input-side Pauli string.
    ///
    /// Returns:
    ///     The output-side Pauli string.
    ///     Algebraically: $c p c^{-1}$ where $c$ is the tableau's Clifford operation.
    PauliString<W> operator()(const PauliStringRef<W> &p) const;

    /// Returns the result of applying the tableau to `gathered_input.scatter(scattered_indices)`.
    PauliString<W> scatter_eval(
        const PauliStringRef<W> &gathered_input, const std::vector<size_t> &scattered_indices) const;

    /// Returns a tableau equivalent to the composition of two tableaus of the same size.
    Tableau<W> then(const Tableau<W> &second) const;

    /// Applies the Tableau inplace to a subset of a Pauli string.
    void apply_within(PauliStringRef<W> &target, SpanRef<const size_t> target_qubits) const;

    /// Appends a smaller operation into this tableau's operation.
    ///
    /// The new value T' of this tableau will equal the composition T o P = PT where T is the old
    /// value of this tableau and P is the operation to append.
    ///
    /// Args:
    ///     operation: The smaller operation to append into this tableau.
    ///     target_qubits: The qubits being acted on by `operation`.
    void inplace_scatter_append(const Tableau<W> &operation, const std::vector<size_t> &target_qubits);

    /// Prepends a smaller operation into this tableau's operation.
    ///
    /// The new value T' of this tableau will equal the composition P o T = TP where T is the old
    /// value of this tableau and P is the operation to append.
    ///
    /// Args:
    ///     operation: The smaller operation to prepend into this tableau.
    ///     target_qubits: The qubits being acted on by `operation`.
    void inplace_scatter_prepend(const Tableau<W> &operation, const std::vector<size_t> &target_qubits);

    /// Applies a transpose to the X2X, X2Z, Z2X, and Z2Z bit tables within the tableau.
    void do_transpose_quadrants();

    /// Returns the direct sum of two tableaus.
    Tableau<W> operator+(const Tableau<W> &second) const;
    /// Appends the other tableau onto this one, resulting in the direct sum.
    Tableau<W> &operator+=(const Tableau<W> &second);

    /// === Specialized vectorized methods for prepending operations onto the tableau === ///
    void prepend_SWAP(size_t q1, size_t q2);
    void prepend_X(size_t q);
    void prepend_Y(size_t q);
    void prepend_Z(size_t q);
    void prepend_H_XZ(size_t q);
    void prepend_H_YZ(size_t q);
    void prepend_H_XY(size_t q);
    void prepend_H_NXY(size_t q);
    void prepend_H_NXZ(size_t q);
    void prepend_H_NYZ(size_t q);
    void prepend_C_XYZ(size_t q);
    void prepend_C_NXYZ(size_t q);
    void prepend_C_XNYZ(size_t q);
    void prepend_C_XYNZ(size_t q);
    void prepend_C_ZYX(size_t q);
    void prepend_C_NZYX(size_t q);
    void prepend_C_ZNYX(size_t q);
    void prepend_C_ZYNX(size_t q);
    void prepend_SQRT_X(size_t q);
    void prepend_SQRT_X_DAG(size_t q);
    void prepend_SQRT_Y(size_t q);
    void prepend_SQRT_Y_DAG(size_t q);
    void prepend_SQRT_Z(size_t q);
    void prepend_SQRT_Z_DAG(size_t q);
    void prepend_SQRT_XX(size_t q1, size_t q2);
    void prepend_SQRT_XX_DAG(size_t q1, size_t q2);
    void prepend_SQRT_YY(size_t q1, size_t q2);
    void prepend_SQRT_YY_DAG(size_t q1, size_t q2);
    void prepend_SQRT_ZZ(size_t q1, size_t q2);
    void prepend_SQRT_ZZ_DAG(size_t q1, size_t q2);
    void prepend_ZCX(size_t control, size_t target);
    void prepend_ZCY(size_t control, size_t target);
    void prepend_ZCZ(size_t control, size_t target);
    void prepend_ISWAP(size_t q1, size_t q2);
    void prepend_ISWAP_DAG(size_t q1, size_t q2);
    void prepend_XCX(size_t control, size_t target);
    void prepend_XCY(size_t control, size_t target);
    void prepend_XCZ(size_t control, size_t target);
    void prepend_YCX(size_t control, size_t target);
    void prepend_YCY(size_t control, size_t target);
    void prepend_YCZ(size_t control, size_t target);
    void prepend_pauli_product(const PauliStringRef<W> &op);

    /// Builds the Y output by using Y = iXZ.
    PauliString<W> y_output(size_t input_index) const;

    /// Constant-time version of tableau.xs[input_index][output_index].
    uint8_t x_output_pauli_xyz(size_t input_index, size_t output_index) const;
    /// Constant-time version of tableau.y_output(input_index)[output_index].
    uint8_t y_output_pauli_xyz(size_t input_index, size_t output_index) const;
    /// Constant-time version of tableau.zs[input_index][output_index].
    uint8_t z_output_pauli_xyz(size_t input_index, size_t output_index) const;
    /// Constant-time version of tableau.inverse().xs[input_index][output_index].
    uint8_t inverse_x_output_pauli_xyz(size_t input_index, size_t output_index) const;
    /// Constant-time version of tableau.inverse().y_output(input_index)[output_index].
    uint8_t inverse_y_output_pauli_xyz(size_t input_index, size_t output_index) const;
    /// Constant-time version of tableau.inverse().zs[input_index][output_index].
    uint8_t inverse_z_output_pauli_xyz(size_t input_index, size_t output_index) const;
    /// Faster version of tableau.inverse().xs[input_index].
    PauliString<W> inverse_x_output(size_t input_index, bool skip_sign = false) const;
    /// Faster version of tableau.inverse().y_output(input_index).
    PauliString<W> inverse_y_output(size_t input_index, bool skip_sign = false) const;
    /// Faster version of tableau.inverse().zs[input_index].
    PauliString<W> inverse_z_output(size_t input_index, bool skip_sign = false) const;

    std::vector<PauliString<W>> stabilizers(bool canonical) const;
};

template <size_t W>
std::ostream &operator<<(std::ostream &out, const Tableau<W> &ps);

}  // namespace stim

#include "stim/stabilizers/tableau.inl"
#include "stim/stabilizers/tableau_specialized_prepend.inl"

#endif
