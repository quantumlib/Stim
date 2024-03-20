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

#ifndef _STIM_STABILIZERS_CONVERSIONS_H
#define _STIM_STABILIZERS_CONVERSIONS_H

#include "stim/circuit/circuit.h"
#include "stim/dem/dem_instruction.h"
#include "stim/stabilizers/flex_pauli_string.h"
#include "stim/stabilizers/tableau.h"

namespace stim {

/// Computes destabilizers for the given stabilizers, and packages into a tableau.
///
/// Args:
///     stabilizers: The desired stabilizers for the tableau. Every stabilizer must have the same number of qubits.
///     allow_redundant: If false, including a redundant stabilizer will result in an error.
///         If true, redundant stabilizers are quietly dropped.
///     allow_underconstrained: If false, the number of independent stabilizers must equal the number of qubits in each
///         stabilizer. If true, the returned result will arbitrarily fill in missing stabilizers.
///     invert: Return the inverse tableau instead of the tableau with the stabilizers as its Z outputs.
///
/// Returns:
///     A tableau containing the given stabilizers, but extended to also include matching stabilizers.
///     The Z outputs of the tableau will be the given stabilizers (skipping any redundant ones).
template <size_t W>
Tableau<W> stabilizers_to_tableau(
    const std::vector<stim::PauliString<W>> &stabilizers,
    bool allow_redundant,
    bool allow_underconstrained,
    bool invert);

std::map<DemTarget, std::map<uint64_t, FlexPauliString>> circuit_to_detecting_regions(
    const Circuit &circuit,
    std::set<stim::DemTarget> included_targets,
    std::set<uint64_t> included_ticks,
    bool ignore_anticommutation_errors);

}  // namespace stim

#include "stim/stabilizers/conversions.inl"

#endif
