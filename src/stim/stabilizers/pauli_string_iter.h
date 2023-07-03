/*
 * Copyright 2023 Google LLC
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

#ifndef _STIM_STABILIZERS_PAULI_STRING_ITER_H
#define _STIM_STABILIZERS_PAULI_STRING_ITER_H

#include <stack>

#include "stim/mem/fixed_cap_vector.h"
#include "stim/mem/span_ref.h"
#include "stim/stabilizers/pauli_string.h"

namespace stim {

/// Iterates over PauliStrings of a given size.
template <size_t W>
struct PauliStringIterator {
    PauliString<W> result;  // Pre-allocated result storage.
    size_t min_weight;
    size_t max_weight;

    // Fields tracking the progress of iteration.
    size_t cur_k;  // current index
    size_t cur_w;  // current weight
    size_t cur_p;  // current permutation index
    std::vector<int> set_bits;
    // Needs to be a list of integers
    simd_bits<W> cur_perm;  // current permutation for given weight
    simd_bits<W> terminal;  // Terminal value of current weight permutation

    PauliStringIterator(size_t num_qubits, size_t min_weight, size_t max_weight);
    PauliStringIterator(const PauliStringIterator &);
    PauliStringIterator &operator=(const PauliStringIterator &);
    PauliStringIterator &operator=(PauliStringIterator &&) = delete;

    // Finds set bits in a string and stores their location in a vector.
    void find_set_bits(simd_bits<W> &cur_perm, std::vector<int> &set_bits);
    // Get next permutation of qubit indices.
    void next_qubit_permutation(simd_bits<W> &cur_perm);
    size_t count_trailing_zeros(simd_bits<W> &cur_perm);
    void ones_mask_with_val(simd_bits<W> &mask, uint64_t val, size_t loc);
    // Yield the next Pauli String
    bool iter_next();
    // Iterate over Pauli strings of a given weight
    bool iter_next_weight();
    // For a given weight and qubit permutation iterate over 3^w Pauli Strings.
    bool iter_all_cur_perm(std::vector<int> &set_bits);

    // Restarts iteration.
    void restart();
};

}  // namespace stim

#include "stim/stabilizers/pauli_string_iter.inl"

#endif