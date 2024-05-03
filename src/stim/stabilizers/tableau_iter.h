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

#ifndef _STIM_STABILIZERS_TABLEAU_ITER_H
#define _STIM_STABILIZERS_TABLEAU_ITER_H

#include "stim/mem/fixed_cap_vector.h"
#include "stim/mem/span_ref.h"
#include "stim/stabilizers/tableau.h"

namespace stim {

/// Iterates over Pauli strings that match commutators and anticommutators.
///
/// The template parameter, W, represents the SIMD width.
template <size_t W>
struct CommutingPauliStringIterator {
    // Fields defining the pauli strings that will be iterated.
    size_t num_qubits;
    SpanRef<const PauliStringRef<W>> cur_desired_commutators;
    SpanRef<const PauliStringRef<W>> cur_desired_anticommutators;

    // Fields tracking progress of the iteration.
    PauliString<W> current;                  // The current Pauli string being considered.
    size_t next_output_index;                // Next thing to return from buffer.
    size_t filled_output;                    // Number of used entries in buffer.
    std::vector<PauliString<W>> output_buf;  // Pre-allocated buffer.

    CommutingPauliStringIterator(size_t num_qubits);

    /// Restarts iteration, and changes the target commutators/anticommutators.
    void restart_iter(SpanRef<const PauliStringRef<W>> commutators, SpanRef<const PauliStringRef<W>> anticommutators);
    void restart_iter_same_constraints();

    /// Yields the next iterated Pauli string (or nullptr if iteration over).
    const PauliString<W> *iter_next();

    /// Checks whether the given Pauli string (versus) commutes with 64 variants
    /// of `current` created by varying its first three Paulis. Assumes that the
    /// first three Paulis of current are set to I.
    ///
    /// Args:
    ///     versus: The Pauli string to compare to (to see if we commute or
    ///         anticommute.
    ///
    /// Returns:
    ///     A 64 bit integer where the k'th bit corresponds to whether the k'th
    ///     variant of the current Pauli string commuted or not. The bits of k
    ///     (where k is the index of a bit in the result) are x0, x1, x2, z0,
    ///     z1, z2 in little endian order.
    uint64_t mass_anticommute_check(const PauliStringRef<W> versus);

    /// Internal method used for refilling a buffer of results.
    void load_more();
};

/// Iterates over tableaus of a given size.
///
/// The template parameter, W, represents the SIMD width.
template <size_t W>
struct TableauIterator {
    bool also_iter_signs;                                // If false, only unsigned tableaus are yielded.
    Tableau<W> result;                                   // Pre-allocated result storage.
    std::vector<PauliStringRef<W>> tableau_column_refs;  // Quick access to tableau columns.

    // Fields tracking the progress of iteration.
    size_t cur_k;
    std::vector<CommutingPauliStringIterator<W>> pauli_string_iterators;

    TableauIterator(size_t num_qubits, bool also_iter_signs);
    TableauIterator(const TableauIterator<W> &);
    TableauIterator &operator=(const TableauIterator<W> &);
    TableauIterator &operator=(TableauIterator<W> &&) = delete;

    /// Updates the `result` field to point at the next yielded tableau.
    /// Returns true if this succeeded, or false if iteration has ended.
    bool iter_next();

    // Restarts iteration.
    void restart();
    std::pair<SpanRef<const PauliStringRef<W>>, SpanRef<const PauliStringRef<W>>> constraints_for_pauli_iterator(
        size_t k) const;
};

}  // namespace stim

#include "stim/stabilizers/tableau_iter.inl"

#endif
