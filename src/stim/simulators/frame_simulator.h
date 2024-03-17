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

#ifndef _STIM_SIMULATORS_FRAME_SIMULATOR_H
#define _STIM_SIMULATORS_FRAME_SIMULATOR_H

#include <random>

#include "stim/circuit/circuit.h"
#include "stim/io/measure_record_batch.h"
#include "stim/mem/simd_bit_table.h"
#include "stim/stabilizers/pauli_string.h"

namespace stim {

enum class FrameSimulatorMode {
    STORE_MEASUREMENTS_TO_MEMORY,  // all measurements stored, detections not stored
    STREAM_MEASUREMENTS_TO_DISK,   // measurements stored up to lookback, detections not stored
    STORE_DETECTIONS_TO_MEMORY,    // measurements stored up to lookback, all detections stored
    STREAM_DETECTIONS_TO_DISK,     // measurements stored up to lookback, detections stored until write
    STORE_EVERYTHING_TO_MEMORY,    // all measurements stored and all detections stored
};

/// A Pauli Frame simulator that computes many samples simultaneously.
///
/// This simulator tracks, for each qubit, whether or not that qubit is bit flipped and/or phase flipped.
/// Instead of reporting qubit measurements, it reports whether a measurement is inverted or not.
/// This requires a set of reference measurements to diff against.
///
/// The template parameter, W, represents the SIMD width
template <size_t W>
struct FrameSimulator {
    size_t num_qubits;                 // Number of qubits being tracked.
    uint64_t num_observables;          // Number of observables being tracked.
    bool keeping_detection_data;       // Whether or not to store dets and obs data.
    size_t batch_size;                 // Number of instances being tracked.
    simd_bit_table<W> x_table;         // x_table[q][k] is whether or not there's an X error on qubit q in instance k.
    simd_bit_table<W> z_table;         // z_table[q][k] is whether or not there's a Z error on qubit q in instance k.
    MeasureRecordBatch<W> m_record;    // The measurement record.
    MeasureRecordBatch<W> det_record;  // Detection event record.
    simd_bit_table<W> obs_record;      // Accumulating observable flip record.
    simd_bits<W> rng_buffer;           // Workspace used when sampling error processes.
    simd_bits<W> tmp_storage;          // Workspace used when sampling compound error processes.
    simd_bits<W> last_correlated_error_occurred;  // correlated error flag for each instance.
    simd_bit_table<W> sweep_table;                // Shot-to-shot configuration data.
    std::mt19937_64 rng;                          // Random number generator used for generating entropy.

    // Determines whether e.g. 50% Z errors are multiplied into the frame when measuring in the Z basis.
    // This is necessary for correct sampling.
    // It should only be disabled when e.g. using the frame simulator to understand how a fixed set of errors will
    // propagate, without interference from other effects.
    bool guarantee_anticommutation_via_frame_randomization = true;

    /// Constructs a FrameSimulator capable of simulating a circuit with the given size stats.
    ///
    /// Args:
    ///     circuit_stats: Sizes that determine how large internal buffers must be. Get
    ///         this from stim::Circuit::compute_stats.
    ///     mode: Describes the intended usage of the simulator, which affects the sizing
    ///         of buffers.
    ///     batch_size: How many shots to simulate simultaneously.
    ///     rng: The random number generator to pull noise from.
    FrameSimulator(CircuitStats circuit_stats, FrameSimulatorMode mode, size_t batch_size, std::mt19937_64 &&rng);
    FrameSimulator() = delete;

    PauliString<W> get_frame(size_t sample_index) const;
    void set_frame(size_t sample_index, const PauliStringRef<W> &new_frame);
    void configure_for(CircuitStats new_circuit_stats, FrameSimulatorMode new_mode, size_t new_batch_size);
    void ensure_safe_to_do_circuit_with_stats(const CircuitStats &stats);

    void safe_do_instruction(const CircuitInstruction &instruction);
    void safe_do_circuit(const Circuit &circuit, uint64_t repetitions = 1);

    void do_circuit(const Circuit &circuit);
    void reset_all();

    void do_gate(const CircuitInstruction &inst);

    void do_MX(const CircuitInstruction &inst);
    void do_MY(const CircuitInstruction &inst);
    void do_MZ(const CircuitInstruction &inst);
    void do_RX(const CircuitInstruction &inst);
    void do_RY(const CircuitInstruction &inst);
    void do_RZ(const CircuitInstruction &inst);
    void do_MRX(const CircuitInstruction &inst);
    void do_MRY(const CircuitInstruction &inst);
    void do_MRZ(const CircuitInstruction &inst);

    void do_DETECTOR(const CircuitInstruction &inst);
    void do_OBSERVABLE_INCLUDE(const CircuitInstruction &inst);

    void do_I(const CircuitInstruction &inst);
    void do_H_XZ(const CircuitInstruction &inst);
    void do_H_XY(const CircuitInstruction &inst);
    void do_H_YZ(const CircuitInstruction &inst);
    void do_C_XYZ(const CircuitInstruction &inst);
    void do_C_ZYX(const CircuitInstruction &inst);
    void do_ZCX(const CircuitInstruction &inst);
    void do_ZCY(const CircuitInstruction &inst);
    void do_ZCZ(const CircuitInstruction &inst);
    void do_XCX(const CircuitInstruction &inst);
    void do_XCY(const CircuitInstruction &inst);
    void do_XCZ(const CircuitInstruction &inst);
    void do_YCX(const CircuitInstruction &inst);
    void do_YCY(const CircuitInstruction &inst);
    void do_YCZ(const CircuitInstruction &inst);
    void do_SWAP(const CircuitInstruction &inst);
    void do_ISWAP(const CircuitInstruction &inst);
    void do_CXSWAP(const CircuitInstruction &inst);
    void do_CZSWAP(const CircuitInstruction &inst);
    void do_SWAPCX(const CircuitInstruction &inst);
    void do_MPP(const CircuitInstruction &inst);
    void do_SPP(const CircuitInstruction &inst);
    void do_SPP_DAG(const CircuitInstruction &inst);
    void do_MXX(const CircuitInstruction &inst);
    void do_MYY(const CircuitInstruction &inst);
    void do_MZZ(const CircuitInstruction &inst);
    void do_MPAD(const CircuitInstruction &inst);

    void do_SQRT_XX(const CircuitInstruction &inst);
    void do_SQRT_YY(const CircuitInstruction &inst);
    void do_SQRT_ZZ(const CircuitInstruction &inst);

    void do_DEPOLARIZE1(const CircuitInstruction &inst);
    void do_DEPOLARIZE2(const CircuitInstruction &inst);
    void do_X_ERROR(const CircuitInstruction &inst);
    void do_Y_ERROR(const CircuitInstruction &inst);
    void do_Z_ERROR(const CircuitInstruction &inst);
    void do_PAULI_CHANNEL_1(const CircuitInstruction &inst);
    void do_PAULI_CHANNEL_2(const CircuitInstruction &inst);
    void do_CORRELATED_ERROR(const CircuitInstruction &inst);
    void do_ELSE_CORRELATED_ERROR(const CircuitInstruction &inst);
    void do_HERALDED_ERASE(const CircuitInstruction &inst);
    void do_HERALDED_PAULI_CHANNEL_1(const CircuitInstruction &inst);

   private:
    void do_MXX_disjoint_controls_segment(const CircuitInstruction &inst);
    void do_MYY_disjoint_controls_segment(const CircuitInstruction &inst);
    void do_MZZ_disjoint_controls_segment(const CircuitInstruction &inst);
    void xor_control_bit_into(uint32_t control, simd_bits_range_ref<W> target);
    void single_cx(uint32_t c, uint32_t t);
    void single_cy(uint32_t c, uint32_t t);
};

}  // namespace stim

#include "stim/simulators/frame_simulator.inl"

#endif
