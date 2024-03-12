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

#ifndef _STIM_STABILIZERS_FLOW_H
#define _STIM_STABILIZERS_FLOW_H

#include <iostream>
#include <span>

#include "stim/circuit/gate_target.h"
#include "stim/stabilizers/pauli_string.h"

namespace stim {

template <size_t W>
struct Flow {
    stim::PauliString<W> input;
    stim::PauliString<W> output;
    /// Indexing follows python convention: -1 is the last element, 0 is the first element.
    std::vector<int32_t> measurements;

    static Flow<W> from_str(std::string_view text);
    bool operator==(const Flow<W> &other) const;
    bool operator!=(const Flow<W> &other) const;
    std::string str() const;
};

/// Probabilistically verifies that the given circuit has the specified flows.
///
/// Args:
///     num_samples: How many times to sample the circuit. Each sample has a 50/50 chance
///         of catching a bad stabilizer flow.
///     rng: Random number generator for the sampling process.
///     circuit: The circuit that should have the given flows.
///     flows: The flows that the circuit should have,
///
/// Returns:
///     A vector containing one boolean for each flow. The k'th boolean is true if the
///     k'th flow passed all checks.
template <size_t W>
std::vector<bool> sample_if_circuit_has_stabilizer_flows(
    size_t num_samples, std::mt19937_64 &rng, const Circuit &circuit, std::span<const Flow<W>> flows);

template <size_t W>
std::vector<bool> check_if_circuit_has_unsigned_stabilizer_flows(
    const Circuit &circuit, std::span<const Flow<W>> flows);

template <size_t W>
std::ostream &operator<<(std::ostream &out, const Flow<W> &flow);

}  // namespace stim

#include "stim/stabilizers/flow.inl"

#endif
