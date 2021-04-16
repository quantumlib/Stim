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

#ifndef SIM_TABLEAU_H
#define SIM_TABLEAU_H

#include <cassert>
#include <functional>
#include <iostream>
#include <new>
#include <random>
#include <sstream>

#include "../circuit/circuit.h"
#include "../stabilizers/tableau.h"
#include "../stabilizers/tableau_transposed_raii.h"
#include "measure_record.h"
#include "vector_simulator.h"

struct TableauSimulator {
    Tableau inv_state;
    std::mt19937_64 &rng;
    int8_t sign_bias;
    MeasureRecord measurement_record;
    bool last_correlated_error_occurred;

    /// Args:
    ///     num_qubits: The initial number of qubits in the simulator state.
    ///     rng: The random number generator to use for random operations.
    ///     sign_bias: 0 means collapse randomly, -1 means collapse towards True, +1 means collapse towards False.
    ///     record: Measurement record configuration.
    explicit TableauSimulator(
        size_t num_qubits, std::mt19937_64 &rng, int8_t sign_bias = 0, MeasureRecord record = MeasureRecord());

    /// Samples the given circuit in a deterministic fashion.
    ///
    /// Discards all noisy operations, and biases all collapse events towards +Z instead of randomly +Z/-Z.
    static simd_bits reference_sample_circuit(const Circuit &circuit);
    static simd_bits sample_circuit(const Circuit &circuit, std::mt19937_64 &rng, int8_t sign_bias = 0);
    static void sample_stream(FILE *in, FILE *out, SampleFormat format, bool interactive, std::mt19937_64 &rng);

    /// Expands the internal state of the simulator (if needed) to ensure the given qubit exists.
    ///
    /// Failing to ensure the state is large enough for a qubit, before that qubit is acted on for the first time,
    /// results in undefined behavior.
    void ensure_large_enough_for_qubits(size_t num_qubits);

    /// Forces the size of the internal state of the simulator.
    ///
    /// Shrinking the size will result in qubits beyond the size threshold being collapsed and discarded.
    void set_num_qubits(size_t new_num_qubits);

    /// Finds a state vector satisfying the current stabilizer generators, and returns a vector simulator in that state.
    VectorSimulator to_vector_sim() const;

    /// Collapses then records the Z signs of the target qubits. Supports flipping the result.
    ///
    /// Args:
    ///     target_data: The qubits to target, with flag data indicating whether to invert results.
    void measure(const OperationData &target_data);

    /// Collapses then clears the target qubits.
    ///
    /// Args:
    ///     target_data: The qubits to target, with flag data indicating whether to reset to 1 instead of 0.
    ///     sign_bias: 0 means collapse randomly, -1 means collapse towards True, +1 means collapse towards False.
    void reset(const OperationData &target_data);

    /// Collapses then records and clears the Z signs of the target qubits. Supports flipping the result.
    ///
    /// Args:
    ///     target_data: The qubits to target, with flag data indicating whether to invert results.
    void measure_reset(const OperationData &target_data);

    /// Determines if a qubit's Z observable commutes (vs anti-commutes) with the current stabilizer generators.
    bool is_deterministic(size_t target) const;

    std::vector<PauliString> canonical_stabilizers() const;

    /// === SPECIALIZED VECTORIZED OPERATION IMPLEMENTATIONS ===
    void I(const OperationData &target_data);
    void H_XZ(const OperationData &target_data);
    void H_YZ(const OperationData &target_data);
    void H_XY(const OperationData &target_data);
    void SQRT_X(const OperationData &target_data);
    void SQRT_Y(const OperationData &target_data);
    void SQRT_Z(const OperationData &target_data);
    void SQRT_X_DAG(const OperationData &target_data);
    void SQRT_Y_DAG(const OperationData &target_data);
    void SQRT_Z_DAG(const OperationData &target_data);
    void ZCX(const OperationData &target_data);
    void ZCY(const OperationData &target_data);
    void ZCZ(const OperationData &target_data);
    void SWAP(const OperationData &target_data);
    void X(const OperationData &target_data);
    void Y(const OperationData &target_data);
    void Z(const OperationData &target_data);
    void ISWAP(const OperationData &target_data);
    void ISWAP_DAG(const OperationData &target_data);
    void XCX(const OperationData &target_data);
    void XCY(const OperationData &target_data);
    void XCZ(const OperationData &target_data);
    void YCX(const OperationData &target_data);
    void YCY(const OperationData &target_data);
    void YCZ(const OperationData &target_data);
    void DEPOLARIZE1(const OperationData &target_data);
    void DEPOLARIZE2(const OperationData &target_data);
    void X_ERROR(const OperationData &target_data);
    void Y_ERROR(const OperationData &target_data);
    void Z_ERROR(const OperationData &target_data);
    void CORRELATED_ERROR(const OperationData &target_data);
    void ELSE_CORRELATED_ERROR(const OperationData &target_data);

    /// Returns the single-qubit stabilizer of a target or, if it is entangled, the identity operation.
    PauliString peek_bloch(uint32_t target) const;

    /// Applies all of the Pauli operations in the given PauliString to the simulator's state.
    void paulis(const PauliString &paulis);

    /// Performs a measurement and returns a kickback that flips between the possible post-measurement states.
    ///
    /// Deterministic measurements have no kickback.
    /// This is represented by setting the kickback to the empty Pauli string.
    std::pair<bool, PauliString> measure_kickback(uint32_t target);

    bool read_measurement_record(uint32_t encoded_target) const;
    void single_cx(uint32_t c, uint32_t t);
    void single_cy(uint32_t c, uint32_t t);

    /// Forces a qubit to have a collapsed Z observable.
    ///
    /// If the qubit already has a collapsed Z observable, this method has no effect.
    /// Other, the qubit's Z observable anticommutes with the current stabilizers and this method will apply state
    /// transformations that pick out a single stabilizer generator to destroy and replace with the measurement's
    /// stabilizer.
    ///
    /// Args:
    ///     target: The index of the qubit to collapse.
    ///     transposed_raii: A RAII value whose existence certifies the tableau data is currently transposed
    ///         (to make operations efficient).
    ///
    /// Returns:
    ///    SIZE_MAX: Already collapsed.
    ///    Else: The pivot index. The start-of-time qubit whose X flips the measurement.
    size_t collapse_qubit(size_t target, TableauTransposedRaii &transposed_raii);

    /// Collapses the given qubits.
    ///
    /// Args:
    ///     targets: The qubits to collapse.
    void collapse(ConstPointerRange<uint32_t> targets);

    /// Completely isolates a qubit from the other qubits tracked by the simulator, so it can be safely discarded.
    ///
    /// After this runs, it is guaranteed that the inverse tableau maps the target qubit's X and Z observables to
    /// themselves (possibly negated) and that it maps all other qubits to Pauli products not involving the target
    /// qubit.
    void collapse_isolate_qubit(size_t target, TableauTransposedRaii &transposed_raii);
};

#endif
