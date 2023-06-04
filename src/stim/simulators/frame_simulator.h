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
#include "stim/circuit/gate_data_table.h"
#include "stim/io/measure_record_batch.h"
#include "stim/mem/simd_bit_table.h"
#include "stim/stabilizers/pauli_string.h"

namespace stim {

enum FrameSimulatorMode {
    STORE_MEASUREMENTS_TO_MEMORY,
    STREAM_MEASUREMENTS_TO_DISK,
    STORE_DETECTIONS_TO_MEMORY,
    STREAM_DETECTIONS_TO_DISK,
};

/// A Pauli Frame simulator that computes many samples simultaneously.
///
/// This simulator tracks, for each qubit, whether or not that qubit is bit flipped and/or phase flipped.
/// Instead of reporting qubit measurements, it reports whether a measurement is inverted or not.
/// This requires a set of reference measurements to diff against.
struct FrameSimulator {
    size_t num_qubits;  // Number of qubits being tracked.
    bool keeping_detection_data;
    size_t batch_size;  // Number of instances being tracked.
    simd_bit_table<MAX_BITWORD_WIDTH>
        x_table;  // x_table[q][k] is whether or not there's an X error on qubit q in instance k.
    simd_bit_table<MAX_BITWORD_WIDTH>
        z_table;                    // z_table[q][k] is whether or not there's a Z error on qubit q in instance k.
    MeasureRecordBatch<MAX_BITWORD_WIDTH> m_record;    // The measurement record.
    MeasureRecordBatch<MAX_BITWORD_WIDTH> det_record;  // Detection event record.
    simd_bit_table<MAX_BITWORD_WIDTH> obs_record;  // Accumulating observable flip record.
    simd_bits<MAX_BITWORD_WIDTH> rng_buffer;       // Workspace used when sampling error processes.
    simd_bits<MAX_BITWORD_WIDTH> tmp_storage;      // Workspace used when sampling compound error processes.
    simd_bits<MAX_BITWORD_WIDTH> last_correlated_error_occurred;  // correlated error flag for each instance.
    simd_bit_table<MAX_BITWORD_WIDTH> sweep_table;                // Shot-to-shot configuration data.
    std::mt19937_64 &rng;  // Random number generator used for generating entropy.

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
    FrameSimulator(CircuitStats circuit_stats, FrameSimulatorMode mode, size_t batch_size, std::mt19937_64 &rng);
    FrameSimulator() = delete;

    PauliString<MAX_BITWORD_WIDTH> get_frame(size_t sample_index) const;
    void set_frame(size_t sample_index, const PauliStringRef<MAX_BITWORD_WIDTH> &new_frame);
    void configure_for(CircuitStats new_circuit_stats, FrameSimulatorMode new_mode, size_t new_batch_size);

    void reset_all_and_run(const Circuit &circuit);
    void reset_all();

    void do_gate(const CircuitInstruction &data);

    void do_MX(const CircuitInstruction &target_data);
    void do_MY(const CircuitInstruction &target_data);
    void do_MZ(const CircuitInstruction &target_data);
    void do_RX(const CircuitInstruction &target_data);
    void do_RY(const CircuitInstruction &target_data);
    void do_RZ(const CircuitInstruction &target_data);
    void do_MRX(const CircuitInstruction &target_data);
    void do_MRY(const CircuitInstruction &target_data);
    void do_MRZ(const CircuitInstruction &target_data);

    void do_DETECTOR(const CircuitInstruction &target_data);
    void do_OBSERVABLE_INCLUDE(const CircuitInstruction &target_data);

    void do_I(const CircuitInstruction &target_data);
    void do_H_XZ(const CircuitInstruction &target_data);
    void do_H_XY(const CircuitInstruction &target_data);
    void do_H_YZ(const CircuitInstruction &target_data);
    void do_C_XYZ(const CircuitInstruction &target_data);
    void do_C_ZYX(const CircuitInstruction &target_data);
    void do_ZCX(const CircuitInstruction &target_data);
    void do_ZCY(const CircuitInstruction &target_data);
    void do_ZCZ(const CircuitInstruction &target_data);
    void do_XCX(const CircuitInstruction &target_data);
    void do_XCY(const CircuitInstruction &target_data);
    void do_XCZ(const CircuitInstruction &target_data);
    void do_YCX(const CircuitInstruction &target_data);
    void do_YCY(const CircuitInstruction &target_data);
    void do_YCZ(const CircuitInstruction &target_data);
    void do_SWAP(const CircuitInstruction &target_data);
    void do_ISWAP(const CircuitInstruction &target_data);
    void do_CXSWAP(const CircuitInstruction &target_data);
    void do_SWAPCX(const CircuitInstruction &target_data);
    void do_MPP(const CircuitInstruction &target_data);

    void do_SQRT_XX(const CircuitInstruction &target_data);
    void do_SQRT_YY(const CircuitInstruction &target_data);
    void do_SQRT_ZZ(const CircuitInstruction &target_data);

    void do_DEPOLARIZE1(const CircuitInstruction &target_data);
    void do_DEPOLARIZE2(const CircuitInstruction &target_data);
    void do_X_ERROR(const CircuitInstruction &target_data);
    void do_Y_ERROR(const CircuitInstruction &target_data);
    void do_Z_ERROR(const CircuitInstruction &target_data);
    void do_PAULI_CHANNEL_1(const CircuitInstruction &target_data);
    void do_PAULI_CHANNEL_2(const CircuitInstruction &target_data);
    void do_CORRELATED_ERROR(const CircuitInstruction &target_data);
    void do_ELSE_CORRELATED_ERROR(const CircuitInstruction &target_data);

   private:
    void xor_control_bit_into(uint32_t control, simd_bits_range_ref<MAX_BITWORD_WIDTH> target);
    void single_cx(uint32_t c, uint32_t t);
    void single_cy(uint32_t c, uint32_t t);
};

bool should_use_streaming_because_bit_count_is_too_large_to_store(uint64_t result_count);
struct DebugForceResultStreamingRaii {
    DebugForceResultStreamingRaii();
    ~DebugForceResultStreamingRaii();
};

}  // namespace stim

#endif
