// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/stabilizers/tableau_iter.h"

#include "stim/stabilizers/pauli_string.h"

using namespace stim;

CommutingPauliStringIterator::CommutingPauliStringIterator(size_t num_qubits)
    : num_qubits(num_qubits),
      cur_desired_commutators(),
      cur_desired_anticommutators(),
      current(num_qubits),
      next_output_index(0),
      filled_output(0),
      output_buf() {
    if (num_qubits < 1) {
        throw std::invalid_argument("Too few qubits (num_qubits < 1).");
    }
    if (num_qubits >= 64) {
        throw std::invalid_argument("Too many qubits to iterate tableaus (num_qubits > 64).");
    }
    while (output_buf.size() < 64) {
        output_buf.push_back(PauliString(num_qubits));
    }
}

uint64_t CommutingPauliStringIterator::mass_anticommute_check(const PauliStringRef versus) {
    constexpr uint64_t x0 = 0xAAAAAAAAAAAAAAAAUL;
    constexpr uint64_t x1 = 0xCCCCCCCCCCCCCCCCUL;
    constexpr uint64_t x2 = 0xF0F0F0F0F0F0F0F0UL;
    constexpr uint64_t z0 = 0xFF00FF00FF00FF00UL;
    constexpr uint64_t z1 = 0xFFFF0000FFFF0000UL;
    constexpr uint64_t z2 = 0xFFFFFFFF00000000UL;

    uint64_t result = 0;
    if (versus.zs[0])
        result ^= x0;
    if (versus.zs[1])
        result ^= x1;
    if (versus.zs[2])
        result ^= x2;
    if (versus.xs[0])
        result ^= z0;
    if (versus.xs[1])
        result ^= z1;
    if (versus.xs[2])
        result ^= z2;
    if (versus.num_qubits > 3 && !versus.commutes(current)) {
        result ^= -1;
    }
    return result;
}

void CommutingPauliStringIterator::restart_iter_same_constraints() {
    current.xs.u64[0] = 0;
    current.zs.u64[0] = 0;
    next_output_index = 0;
    filled_output = 0;
}

void CommutingPauliStringIterator::restart_iter(
    ConstPointerRange<PauliStringRef> commutators, ConstPointerRange<PauliStringRef> anticommutators) {
    restart_iter_same_constraints();
    cur_desired_commutators = commutators;
    cur_desired_anticommutators = anticommutators;
}

const PauliString *CommutingPauliStringIterator::iter_next() {
    if (next_output_index >= filled_output) {
        load_more();
    }
    if (next_output_index >= filled_output) {
        return nullptr;
    }
    return &output_buf[next_output_index++];
}

void CommutingPauliStringIterator::load_more() {
    next_output_index = 0;
    filled_output = 0;

    uint64_t start_pass_mask = -1;
    if (num_qubits < 2) {
        // Drop bits corresponding to cases where x2 or z2 or x1 or z1 are set.
        start_pass_mask &= 0x0000000000000303UL;
    } else if (num_qubits < 3) {
        // Drop bits corresponding to cases where x2 or z2 are set.
        start_pass_mask &= 0x000000000F0F0F0FUL;
    }

    uint64_t num_bit_strings = 1 << num_qubits;
    while (filled_output == 0 && current.zs.u64[0] < num_bit_strings) {
        uint64_t pass_mask = start_pass_mask;
        if (current.xs.u64[0] == 0 && current.zs.u64[0] == 0) {
            pass_mask &= ~1;  // Don't search the identity.
        }
        for (const auto &p : cur_desired_commutators) {
            pass_mask &= ~mass_anticommute_check(p);
        }
        for (const auto &p : cur_desired_anticommutators) {
            pass_mask &= mass_anticommute_check(p);
        }
        if (pass_mask) {
            for (size_t b = 0; b < 64; b++) {
                if ((pass_mask >> b) & 1) {
                    output_buf[filled_output] = current;
                    output_buf[filled_output].xs.u64[0] |= b & 7;
                    output_buf[filled_output].zs.u64[0] |= (b >> 3) & 7;
                    filled_output++;
                }
            }
        }

        current.xs.u64[0] += 8;
        if (current.xs.u64[0] >= num_bit_strings) {
            current.xs.u64[0] = 0;
            current.zs.u64[0] += 8;
        }
    }
}

TableauIterator::TableauIterator(size_t num_qubits, bool also_iter_signs)
    : also_iter_signs(also_iter_signs), result(num_qubits), cur_k(0) {
    for (size_t k = 0; k < num_qubits; k++) {
        // Iterator for X_k's output.
        pauli_string_iterators.push_back(CommutingPauliStringIterator(num_qubits));
        tableau_column_refs.push_back(result.xs[k]);

        // Iterator for Z_k's output.
        pauli_string_iterators.push_back(CommutingPauliStringIterator(num_qubits));
        tableau_column_refs.push_back(result.zs[k]);
    }

    for (size_t k = 0; k < 2 * num_qubits; k++) {
        auto constraints = constraints_for_pauli_iterator(k);
        pauli_string_iterators[k].cur_desired_commutators = constraints.first;
        pauli_string_iterators[k].cur_desired_anticommutators = constraints.second;
    }
}

TableauIterator::TableauIterator(const TableauIterator &other) : result(0) {
    *this = other;
}

TableauIterator &TableauIterator::operator=(const TableauIterator &other) {
    also_iter_signs = other.also_iter_signs;
    result = other.result;
    cur_k = other.cur_k;
    pauli_string_iterators = other.pauli_string_iterators;

    tableau_column_refs.clear();
    for (size_t k = 0; k < result.num_qubits; k++) {
        tableau_column_refs.push_back(result.xs[k]);
        tableau_column_refs.push_back(result.zs[k]);
    }

    for (size_t k = 0; k < 2 * result.num_qubits; k++) {
        auto constraints = constraints_for_pauli_iterator(k);
        pauli_string_iterators[k].cur_desired_commutators = constraints.first;
        pauli_string_iterators[k].cur_desired_anticommutators = constraints.second;
    }

    return *this;
}

std::pair<ConstPointerRange<PauliStringRef>, ConstPointerRange<PauliStringRef>>
TableauIterator::constraints_for_pauli_iterator(size_t k) const {
    const PauliStringRef *tab_obs_start = &tableau_column_refs[0];
    ConstPointerRange<PauliStringRef> commute_rng = {tab_obs_start, tab_obs_start + k};
    ConstPointerRange<PauliStringRef> anticommute_rng;
    if (k & 1) {
        anticommute_rng.ptr_end = commute_rng.ptr_end;
        commute_rng.ptr_end--;
        anticommute_rng.ptr_start = commute_rng.ptr_end;
    }
    return {commute_rng, anticommute_rng};
}

bool TableauIterator::iter_next() {
    if (result.xs.signs.u64[0] > 0) {
        result.xs.signs.u64[0]--;
        return true;
    }
    if (result.zs.signs.u64[0] > 0) {
        result.zs.signs.u64[0]--;
        result.xs.signs.u64[0] = (uint64_t{1} << result.num_qubits) - uint64_t{1};
        return true;
    }

    while (cur_k != SIZE_MAX) {
        const PauliString *out = pauli_string_iterators[cur_k].iter_next();
        if (out == nullptr) {
            // Exhausted all Paulis strings at this level; go back a level.
            cur_k--;  // At 0 this underflows to SIZE_MAX, exiting the loop.
            continue;
        }

        tableau_column_refs[cur_k] = *out;
        cur_k++;
        if (cur_k == 2 * result.num_qubits) {
            cur_k--;
            if (also_iter_signs) {
                // Okay, look, I admit this is technically wrong. It fails if num_qubits > 64 because
                // not all sign variations are explored. But no one is actually going to be able to finish
                // iterating the tableaus that are actually yielded, so good look demonstrating it's wrong.
                // :P
                result.xs.signs.u64[0] = (uint64_t{1} << result.num_qubits) - uint64_t{1};
                result.zs.signs.u64[0] = (uint64_t{1} << result.num_qubits) - uint64_t{1};
            }
            return true;
        }

        pauli_string_iterators[cur_k].restart_iter_same_constraints();
    }

    return false;
}

void TableauIterator::restart() {
    cur_k = 0;
    pauli_string_iterators[0].restart_iter({}, {});
    result.xs.signs.clear();
    result.zs.signs.clear();
}
