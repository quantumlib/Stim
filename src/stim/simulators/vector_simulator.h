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

#ifndef _STIM_SIMULATORS_VECTOR_SIMULATOR_H
#define _STIM_SIMULATORS_VECTOR_SIMULATOR_H

#include <complex>
#include <iostream>
#include <random>
#include <unordered_map>

#include "stim/stabilizers/pauli_string.h"

namespace stim {

/// A state vector quantum circuit simulator.
///
/// Not intended to be particularly performant. Mostly used as a reference when testing.
struct VectorSimulator {
    std::vector<std::complex<float>> state;

    /// Creates a state vector for the given number of qubits, initialized to the zero state.
    explicit VectorSimulator(size_t num_qubits);

    /// Returns a VectorSimulator with a state vector satisfying all the given stabilizers.
    ///
    /// Assumes the stabilizers commute. Works by generating a random state vector and projecting onto
    /// each of the given stabilizers. Global phase will vary.
    static VectorSimulator from_stabilizers(const std::vector<PauliStringRef> stabilizers, std::mt19937_64 &rng);

    /// Applies a unitary operation to the given qubits, updating the state vector.
    void apply(const std::vector<std::vector<std::complex<float>>> &matrix, const std::vector<size_t> &qubits);

    /// Helper method for applying named single qubit gates.
    void apply(const std::string &gate, size_t qubit);
    /// Helper method for applying named two qubit gates.
    void apply(const std::string &gate, size_t qubit1, size_t qubit2);
    /// Helper method for applying the gates in a Pauli string.
    void apply(const PauliStringRef &gate, size_t qubit_offset);

    /// Projects the state vector into the +1 eigenstate of the given observable, and renormalizes.
    ///
    /// Returns:
    ///     The 2-norm of the component of the state vector that was already in the +1 eigenstate.
    ///     In other words, the probability that measuring the observable would have returned +1 instead of -1.
    float project(const PauliStringRef &observable);

    /// Determines if two vector simulators have similar state vectors.
    bool approximate_equals(const VectorSimulator &other, bool up_to_global_phase = false) const;

    /// A description of the state vector's state.
    std::string str() const;
};

/// Writes a description of the state vector's state to an output stream.
std::ostream &operator<<(std::ostream &out, const VectorSimulator &sim);

}  // namespace stim

#endif
