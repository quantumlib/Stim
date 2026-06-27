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

#ifndef _STIM_SIMULATORS_TABLEAU_SIMULATOR_H
#define _STIM_SIMULATORS_TABLEAU_SIMULATOR_H

#include <cassert>
#include <functional>
#include <iostream>
#include <new>
#include <random>
#include <sstream>

#include "stim/circuit/circuit.h"
#include "stim/io/measure_record.h"
#include "stim/stabilizers/tableau.h"
#include "stim/stabilizers/tableau_transposed_raii.h"

namespace stim {

/// A stabilizer circuit simulator that tracks an inverse stabilizer tableau
/// and allows interactive usage, where gates and measurements are applied
/// on demand.
///
/// The template parameter, W, represents the SIMD width
template <size_t W>
struct TableauSimulator {
    Tableau<W> inv_state;
    std::mt19937_64 rng;
    int8_t sign_bias;
    MeasureRecord measurement_record;
    bool last_correlated_error_occurred;

    /// Args:
    ///     num_qubits: The initial number of qubits in the simulator state.
    ///     rng: The random number generator to use for random operations.
    ///     sign_bias: 0 means collapse randomly, -1 means collapse towards True, +1 means collapse towards False.
    ///     record: Measurement record configuration.
    explicit TableauSimulator(
        std::mt19937_64 &&rng, size_t num_qubits = 0, int8_t sign_bias = 0, MeasureRecord record = MeasureRecord());
    /// Args:
    ///     other: TableauSimulator to copy state from.
    ///     rng: The random number generator to use for random operations.
    TableauSimulator(const TableauSimulator &other, std::mt19937_64 &&rng);

    /// Samples the given circuit in a deterministic fashion.
    ///
    /// Discards all noisy operations, and biases all collapse events towards +Z instead of randomly +Z/-Z.
    static simd_bits<W> reference_sample_circuit(const Circuit &circuit);
    static simd_bits<W> sample_circuit(const Circuit &circuit, std::mt19937_64 &rng, int8_t sign_bias = 0);
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

    /// Returns a state vector satisfying the current stabilizer generators.
    std::vector<std::complex<float>> to_state_vector(bool little_endian) const;

    /// Collapses then records an observable.
    ///
    /// Args:
    ///     pauli_string: The observable to measure.
    bool measure_pauli_string(const PauliStringRef<W> pauli_string, double flip_probability);

    /// Determines if a qubit's X observable commutes (vs anti-commutes) with the current stabilizer generators.
    bool is_deterministic_x(size_t target) const;
    /// Determines if a qubit's Y observable commutes (vs anti-commutes) with the current stabilizer generators.
    bool is_deterministic_y(size_t target) const;
    /// Determines if a qubit's Z observable commutes (vs anti-commutes) with the current stabilizer generators.
    bool is_deterministic_z(size_t target) const;

    /// Runs all of the operations in the given circuit.
    ///
    /// Automatically expands the tableau simulator's state, if needed.
    void safe_do_circuit(const Circuit &circuit, uint64_t reps = 1);
    void do_operation_ensure_size(const CircuitInstruction &operation);

    void apply_tableau(const Tableau<W> &tableau, const std::vector<size_t> &targets);

    std::vector<PauliString<W>> canonical_stabilizers() const;

    void do_gate(const CircuitInstruction &inst);

    /// === SPECIALIZED VECTORIZED OPERATION IMPLEMENTATIONS ===
    void do_I(const CircuitInstruction &inst);
    void do_H_XZ(const CircuitInstruction &inst);
    void do_H_YZ(const CircuitInstruction &inst);
    void do_H_XY(const CircuitInstruction &inst);
    void do_H_NXY(const CircuitInstruction &inst);
    void do_H_NXZ(const CircuitInstruction &inst);
    void do_H_NYZ(const CircuitInstruction &inst);
    void do_C_XYZ(const CircuitInstruction &inst);
    void do_C_NXYZ(const CircuitInstruction &inst);
    void do_C_XNYZ(const CircuitInstruction &inst);
    void do_C_XYNZ(const CircuitInstruction &inst);
    void do_C_ZYX(const CircuitInstruction &inst);
    void do_C_NZYX(const CircuitInstruction &inst);
    void do_C_ZNYX(const CircuitInstruction &inst);
    void do_C_ZYNX(const CircuitInstruction &inst);
    void do_SQRT_X(const CircuitInstruction &inst);
    void do_SQRT_Y(const CircuitInstruction &inst);
    void do_SQRT_Z(const CircuitInstruction &inst);
    void do_SQRT_X_DAG(const CircuitInstruction &inst);
    void do_SQRT_Y_DAG(const CircuitInstruction &inst);
    void do_SQRT_Z_DAG(const CircuitInstruction &inst);
    void do_SQRT_XX(const CircuitInstruction &inst);
    void do_SQRT_XX_DAG(const CircuitInstruction &inst);
    void do_SQRT_YY(const CircuitInstruction &inst);
    void do_SQRT_YY_DAG(const CircuitInstruction &inst);
    void do_SQRT_ZZ(const CircuitInstruction &inst);
    void do_SQRT_ZZ_DAG(const CircuitInstruction &inst);
    void do_ZCX(const CircuitInstruction &inst);
    void do_ZCY(const CircuitInstruction &inst);
    void do_ZCZ(const CircuitInstruction &inst);
    void do_SWAP(const CircuitInstruction &inst);
    void do_X(const CircuitInstruction &inst);
    void do_Y(const CircuitInstruction &inst);
    void do_Z(const CircuitInstruction &inst);
    void do_ISWAP(const CircuitInstruction &inst);
    void do_ISWAP_DAG(const CircuitInstruction &inst);
    void do_CXSWAP(const CircuitInstruction &inst);
    void do_CZSWAP(const CircuitInstruction &inst);
    void do_SWAPCX(const CircuitInstruction &inst);
    void do_XCX(const CircuitInstruction &inst);
    void do_XCY(const CircuitInstruction &inst);
    void do_XCZ(const CircuitInstruction &inst);
    void do_YCX(const CircuitInstruction &inst);
    void do_YCY(const CircuitInstruction &inst);
    void do_YCZ(const CircuitInstruction &inst);
    void do_DEPOLARIZE1(const CircuitInstruction &inst);
    void do_DEPOLARIZE2(const CircuitInstruction &inst);
    void do_HERALDED_ERASE(const CircuitInstruction &inst);
    void do_HERALDED_PAULI_CHANNEL_1(const CircuitInstruction &inst);
    void do_X_ERROR(const CircuitInstruction &inst);
    void do_Y_ERROR(const CircuitInstruction &inst);
    void do_Z_ERROR(const CircuitInstruction &inst);
    void do_PAULI_CHANNEL_1(const CircuitInstruction &inst);
    void do_PAULI_CHANNEL_2(const CircuitInstruction &inst);
    void do_CORRELATED_ERROR(const CircuitInstruction &inst);
    void do_ELSE_CORRELATED_ERROR(const CircuitInstruction &inst);
    void do_MPP(const CircuitInstruction &inst);
    void do_SPP(const CircuitInstruction &inst);
    void do_SPP_DAG(const CircuitInstruction &inst);
    void do_MXX(const CircuitInstruction &inst);
    void do_MYY(const CircuitInstruction &inst);
    void do_MZZ(const CircuitInstruction &inst);
    void do_MPAD(const CircuitInstruction &inst);
    void do_MX(const CircuitInstruction &inst);
    void do_MY(const CircuitInstruction &inst);
    void do_MZ(const CircuitInstruction &inst);
    void do_RX(const CircuitInstruction &inst);
    void do_RY(const CircuitInstruction &inst);
    void do_RZ(const CircuitInstruction &inst);
    void do_MRX(const CircuitInstruction &inst);
    void do_MRY(const CircuitInstruction &inst);
    void do_MRZ(const CircuitInstruction &inst);

    /// Returns the single-qubit stabilizer of a target or, if it is entangled, the identity operation.
    PauliString<W> peek_bloch(uint32_t target) const;

    /// Returns the expectation value of measuring the qubit in the X basis.
    int8_t peek_x(uint32_t target) const;
    /// Returns the expectation value of measuring the qubit in the Y basis.
    int8_t peek_y(uint32_t target) const;
    /// Returns the expectation value of measuring the qubit in the Z basis.
    int8_t peek_z(uint32_t target) const;

    /// Projects the system into a desired qubit X observable, or raises an exception if it was impossible.
    void postselect_x(SpanRef<const GateTarget> targets, bool desired_result);
    /// Projects the system into a desired qubit Y observable, or raises an exception if it was impossible.
    void postselect_y(SpanRef<const GateTarget> targets, bool desired_result);
    /// Projects the system into a desired qubit Z observable, or raises an exception if it was impossible.
    void postselect_z(SpanRef<const GateTarget> targets, bool desired_result);

    /// Applies all of the Pauli operations in the given PauliString to the simulator's state.
    void paulis(const PauliString<W> &paulis);

    /// Performs a measurement and returns a kickback that flips between the possible post-measurement states.
    ///
    /// Deterministic measurements have no kickback.
    /// This is represented by setting the kickback to the empty Pauli string.
    std::pair<bool, PauliString<W>> measure_kickback_z(GateTarget target);
    std::pair<bool, PauliString<W>> measure_kickback_y(GateTarget target);
    std::pair<bool, PauliString<W>> measure_kickback_x(GateTarget target);

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
    size_t collapse_qubit_z(size_t target, TableauTransposedRaii<W> &transposed_raii);

    /// Collapses the given qubits into the X basis.
    ///
    /// Args:
    ///     targets: The qubits to collapse.
    ///     stride: Defaults to 1. Set to 2 to skip over every other target.
    void collapse_x(SpanRef<const GateTarget> targets, size_t stride = 1);

    /// Collapses the given qubits into the Y basis.
    ///
    /// Args:
    ///     targets: The qubits to collapse.
    ///     stride: Defaults to 1. Set to 2 to skip over every other target.
    void collapse_y(SpanRef<const GateTarget> targets, size_t stride = 1);

    /// Collapses the given qubits into the Z basis.
    ///
    /// Args:
    ///     targets: The qubits to collapse.
    ///     stride: Defaults to 1. Set to 2 to skip over every other target.
    void collapse_z(SpanRef<const GateTarget> targets, size_t stride = 1);

    /// Completely isolates a qubit from the other qubits tracked by the simulator, so it can be safely discarded.
    ///
    /// After this runs, it is guaranteed that the inverse tableau maps the target qubit's X and Z observables to
    /// themselves (possibly negated) and that it maps all other qubits to Pauli products not involving the target
    /// qubit.
    void collapse_isolate_qubit_z(size_t target, TableauTransposedRaii<W> &transposed_raii);

    /// Determines the expected value of an observable (which will always be -1, 0, or +1).
    ///
    /// This is a non-physical operation.
    /// It reports information about the quantum state without disturbing it.
    ///
    /// Args:
    ///     observable: The observable to determine the expected value of.
    ///
    /// Returns:
    ///     +1: Observable will be deterministically false when measured.
    ///     -1: Observable will be deterministically true when measured.
    ///     0: Observable will be random when measured.
    int8_t peek_observable_expectation(const PauliString<W> &observable) const;

    /// Forces a desired measurement result, or raises an exception if it was impossible.
    void postselect_observable(PauliStringRef<W> observable, bool desired_result);

   private:
    uint32_t try_isolate_observable_to_qubit_z(PauliStringRef<W> observable, bool undo);
    void do_MXX_disjoint_controls_segment(const CircuitInstruction &inst);
    void do_MYY_disjoint_controls_segment(const CircuitInstruction &inst);
    void do_MZZ_disjoint_controls_segment(const CircuitInstruction &inst);
    void noisify_new_measurements(SpanRef<const double> args, size_t num_targets);
    void noisify_new_measurements(const CircuitInstruction &target_data);
    void postselect_helper(
        SpanRef<const GateTarget> targets,
        bool desired_result,
        GateType basis_change_gate,
        const char *false_name,
        const char *true_name);
};

template <size_t Q, typename RESET_FLAG, typename ELSE_CORR>
void perform_pauli_errors_via_correlated_errors(
    const CircuitInstruction &target_data, RESET_FLAG reset_flag, ELSE_CORR else_corr) {
    double target_p{};
    GateTarget target_t[Q];
    CircuitInstruction data{GateType::E, {&target_p}, {&target_t[0], &target_t[Q]}, ""};
    for (size_t k = 0; k < target_data.targets.size(); k += Q) {
        reset_flag();
        double used_probability = 0;
        for (size_t pauli = 1; pauli < 1 << (2 * Q); pauli++) {
            double p = target_data.args[pauli - 1];
            if (p == 0) {
                continue;
            }
            double remaining = 1 - used_probability;
            double conditional_prob = remaining <= 0 ? 0 : remaining <= p ? 1 : p / remaining;
            used_probability += p;

            for (size_t q = 0; q < Q; q++) {
                target_t[q] = target_data.targets[k + q];
                bool z = (pauli >> (2 * (Q - q - 1))) & 2;
                bool y = (pauli >> (2 * (Q - q - 1))) & 1;
                if (z ^ y) {
                    target_t[q].data |= TARGET_PAULI_X_BIT;
                }
                if (z) {
                    target_t[q].data |= TARGET_PAULI_Z_BIT;
                }
            }
            target_p = conditional_prob;
            else_corr(data);
        }
    }
}

}  // namespace stim

#include "stim/simulators/tableau_simulator.inl"

#endif
