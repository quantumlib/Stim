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

#include "stim/circuit/circuit.h"
#include "stim/mem/simd_word.h"
#include "stim/util_bot/probability_util.h"

namespace stim {

template <size_t W>
struct PauliStringRef;

template <size_t W>
struct PauliString;

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
    template <size_t W>
    static VectorSimulator from_stabilizers(const std::vector<PauliStringRef<W>> &stabilizers) {
        VectorSimulator result(0);
        result.state = state_vector_from_stabilizers(stabilizers, 1);
        return result;
    }

    template <size_t W>
    static std::vector<std::complex<float>> state_vector_from_stabilizers(
        const std::vector<PauliStringRef<W>> &stabilizers, float norm2 = 1) {
        size_t num_qubits = stabilizers.empty() ? 0 : stabilizers[0].num_qubits;
        VectorSimulator sim(num_qubits);

        // Create an initial state $|A\rangle^{\otimes n}$ which overlaps with all possible stabilizers.
        std::uniform_real_distribution<float> dist(-1.0, +1.0);

        auto rng = externally_seeded_rng();
        for (auto &s : sim.state) {
            s = {dist(rng), dist(rng)};
        }

        // Project out the non-overlapping parts.
        for (const auto &p : stabilizers) {
            sim.project<W>(p);
        }
        if (stabilizers.empty()) {
            sim.project<W>(PauliString<W>(0));
        }

        sim.canonicalize_assuming_stabilizer_state(norm2);

        return sim.state;
    }

    /// Applies a unitary operation to the given qubits, updating the state vector.
    void apply(const std::vector<std::vector<std::complex<float>>> &matrix, const std::vector<size_t> &qubits);

    /// Helper method for applying named single qubit gates.
    void apply(GateType gate, size_t qubit);
    /// Helper method for applying named two qubit gates.
    void apply(GateType gate, size_t qubit1, size_t qubit2);

    /// Helper method for applying the gates in a Pauli string.
    template <size_t W>
    void apply(const PauliStringRef<W> &gate, size_t qubit_offset) {
        if (gate.sign) {
            for (auto &e : state) {
                e *= -1;
            }
        }
        for (size_t k = 0; k < gate.num_qubits; k++) {
            bool x = gate.xs[k];
            bool z = gate.zs[k];
            size_t q = qubit_offset + k;
            if (x && z) {
                apply(GateType::Y, q);
            } else if (x) {
                apply(GateType::X, q);
            } else if (z) {
                apply(GateType::Z, q);
            }
        }
    }

    /// Applies the unitary operations within a circuit to the simulator's state.
    void do_unitary_circuit(const Circuit &circuit);

    /// Modifies the state vector to be EXACTLY entries of 0, 1, -1, i, or -i.
    ///
    /// Each entry is a ratio relative to the given base value.
    /// If any entry has a ratio not near the desired set, an exception is raised.
    void smooth_stabilizer_state(std::complex<float> base_value);

    /// Projects the state vector into the +1 eigenstate of the given observable, and renormalizes.
    ///
    /// Returns:
    ///     The 2-norm of the component of the state vector that was already in the +1 eigenstate.
    ///     In other words, the probability that measuring the observable would have returned +1 instead of -1.
    template <size_t W>
    float project(const PauliStringRef<W> &observable) {
        assert(1ULL << observable.num_qubits == state.size());
        auto basis_change = [&]() {
            for (size_t k = 0; k < observable.num_qubits; k++) {
                if (observable.xs[k]) {
                    if (observable.zs[k]) {
                        apply(GateType::H_YZ, k);
                    } else {
                        apply(GateType::H, k);
                    }
                }
            }
        };

        uint64_t mask = 0;
        for (size_t k = 0; k < observable.num_qubits; k++) {
            if (observable.xs[k] || observable.zs[k]) {
                mask |= 1ULL << k;
            }
        }

        basis_change();
        float mag2 = 0;
        for (size_t i = 0; i < state.size(); i++) {
            bool reject = observable.sign;
            reject ^= (std::popcount(i & mask) & 1) != 0;
            if (reject) {
                state[i] = 0;
            } else {
                mag2 += state[i].real() * state[i].real() + state[i].imag() * state[i].imag();
            }
        }
        assert(mag2 > 1e-8);
        auto w = sqrtf(mag2);
        for (size_t i = 0; i < state.size(); i++) {
            state[i] /= w;
        }
        basis_change();
        return mag2;
    }

    /// Determines if two vector simulators have similar state vectors.
    bool approximate_equals(const VectorSimulator &other, bool up_to_global_phase = false) const;

    /// A description of the state vector's state.
    std::string str() const;

    void canonicalize_assuming_stabilizer_state(double norm2);
};

/// Writes a description of the state vector's state to an output stream.
std::ostream &operator<<(std::ostream &out, const VectorSimulator &sim);

}  // namespace stim

#endif
