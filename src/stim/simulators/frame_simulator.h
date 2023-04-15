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

/// A Pauli Frame simulator that computes many samples simultaneously.
///
/// This simulator tracks, for each qubit, whether or not that qubit is bit flipped and/or phase flipped.
/// Instead of reporting qubit measurements, it reports whether a measurement is inverted or not.
/// This requires a set of reference measurements to diff against.
struct FrameSimulator {
    size_t num_qubits;  // Number of qubits being tracked.
    size_t batch_size;  // Number of instances being tracked.
    simd_bit_table<MAX_BITWORD_WIDTH>
        x_table;  // x_table[q][k] is whether or not there's an X error on qubit q in instance k.
    simd_bit_table<MAX_BITWORD_WIDTH>
        z_table;                  // z_table[q][k] is whether or not there's a Z error on qubit q in instance k.
    MeasureRecordBatch m_record;  // The measurement record.
    MeasureRecordBatch det_record;  // Detection event record.
    simd_bit_table<MAX_BITWORD_WIDTH> obs_record;  // Accumulating observable flip record.
    simd_bits<MAX_BITWORD_WIDTH> rng_buffer;   // Workspace used when sampling error processes.
    simd_bits<MAX_BITWORD_WIDTH> tmp_storage;  // Workspace used when sampling compound error processes.
    simd_bits<MAX_BITWORD_WIDTH> last_correlated_error_occurred;  // correlated error flag for each instance.
    simd_bit_table<MAX_BITWORD_WIDTH> sweep_table;                // Shot-to-shot configuration data.
    std::mt19937_64 &rng;  // Random number generator used for generating entropy.

    // Determines whether e.g. 50% Z errors are multiplied into the frame when measuring in the Z basis.
    // This is necessary for correct sampling.
    // It should only be disabled when e.g. using the frame simulator to understand how a fixed set of errors will
    // propagate, without interference from other effects.
    bool guarantee_anticommutation_via_frame_randomization = true;

    FrameSimulator() = delete;
    FrameSimulator(size_t num_qubits,
                   size_t max_measurement_lookback,
                   size_t max_detector_lookback,
                   size_t num_observables,
                   size_t batch_size,
                   std::mt19937_64 &rng);

    /// Returns a batch of measurement-flipped samples from the circuit.
    ///
    /// Args:
    ///     circuit: The circuit to sample from.
    ///     num_shots: The number of shots of the circuit to run.
    ///     rng: Random number generator.
    ///
    /// Returns:
    ///     A table of results. First index (major) is measurement index, second index (minor) is shot index.
    ///     Each bit in the table is whether a specific measurement was flipped in a specific shot.
    static simd_bit_table<MAX_BITWORD_WIDTH> sample_flipped_measurements(
        const Circuit &circuit, size_t num_shots, std::mt19937_64 &rng);

    /// Returns a batch of samples from the circuit.
    ///
    /// Args:
    ///     circuit: The circuit to sample from.
    ///     reference_sample: A known-good sample from the circuit, collected without any noise processes.
    ///     num_shots: The number of shots of the circuit to run.
    ///     rng: Random number generator.
    ///
    /// Returns:
    ///     A table of results. First index (major) is measurement index, second index (minor) is shot index.
    ///     Each bit in the table is a measurement result.
    static simd_bit_table<MAX_BITWORD_WIDTH> sample(
        const Circuit &circuit,
        const simd_bits<MAX_BITWORD_WIDTH> &reference_sample,
        size_t num_samples,
        std::mt19937_64 &rng);

    static void sample_out(
        const Circuit &circuit,
        const simd_bits<MAX_BITWORD_WIDTH> &reference_sample,
        uint64_t num_shots,
        FILE *out,
        SampleFormat format,
        std::mt19937_64 &rng);

    PauliString get_frame(size_t sample_index) const;
    void set_frame(size_t sample_index, const PauliStringRef &new_frame);

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

bool should_use_streaming_instead_of_memory(uint64_t result_count);
struct DebugForceResultStreamingRaii {
    DebugForceResultStreamingRaii();
    ~DebugForceResultStreamingRaii();
};

}  // namespace stim

#endif
