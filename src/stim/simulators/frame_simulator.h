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

/// A Pauli Frame simulator that computes many samples simultaneously.
///
/// This simulator tracks, for each qubit, whether or not that qubit is bit flipped and/or phase flipped.
/// Instead of reporting qubit measurements, it reports whether a measurement is inverted or not.
/// This requires a set of reference measurements to diff against.
struct FrameSimulator {
    size_t num_qubits;            // Number of qubits being tracked.
    size_t batch_size;            // Number of instances being tracked.
    simd_bit_table x_table;       // x_table[q][k] is whether or not there's an X error on qubit q in instance k.
    simd_bit_table z_table;       // z_table[q][k] is whether or not there's a Z error on qubit q in instance k.
    MeasureRecordBatch m_record;  // The measurement record.
    simd_bits rng_buffer;         // Workspace used when sampling error processes.
    simd_bits tmp_storage;        // Workspace used when sampling compound error processes.
    simd_bits last_correlated_error_occurred;  // correlated error flag for each instance.
    simd_bit_table sweep_table;                // Shot-to-shot configuration data.
    std::mt19937_64 &rng;                      // Random number generator used for generating entropy.

    // Determines whether e.g. 50% Z errors are multiplied into the frame when measuring in the Z basis.
    // This is necessary for correct sampling.
    // It should only be disabled when e.g. using the frame simulator to understand how a fixed set of errors will
    // propagate, without interference from other effects.
    bool guarantee_anticommutation_via_frame_randomization = true;

    FrameSimulator(size_t num_qubits, size_t batch_size, size_t max_lookback, std::mt19937_64 &rng);

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
    static simd_bit_table sample_flipped_measurements(const Circuit &circuit, size_t num_shots, std::mt19937_64 &rng);

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
    static simd_bit_table sample(
        const Circuit &circuit, const simd_bits &reference_sample, size_t num_samples, std::mt19937_64 &rng);

    static void sample_out(
        const Circuit &circuit,
        const simd_bits &reference_sample,
        uint64_t num_shots,
        FILE *out,
        SampleFormat format,
        std::mt19937_64 &rng);

    PauliString get_frame(size_t sample_index) const;
    void set_frame(size_t sample_index, const PauliStringRef &new_frame);

    void reset_all_and_run(const Circuit &circuit);
    void reset_all();

    void measure_x(const OperationData &target_data);
    void measure_y(const OperationData &target_data);
    void measure_z(const OperationData &target_data);
    void reset_x(const OperationData &target_data);
    void reset_y(const OperationData &target_data);
    void reset_z(const OperationData &target_data);
    void measure_reset_x(const OperationData &target_data);
    void measure_reset_y(const OperationData &target_data);
    void measure_reset_z(const OperationData &target_data);

    void I(const OperationData &target_data);
    void H_XZ(const OperationData &target_data);
    void H_XY(const OperationData &target_data);
    void H_YZ(const OperationData &target_data);
    void C_XYZ(const OperationData &target_data);
    void C_ZYX(const OperationData &target_data);
    void ZCX(const OperationData &target_data);
    void ZCY(const OperationData &target_data);
    void ZCZ(const OperationData &target_data);
    void XCX(const OperationData &target_data);
    void XCY(const OperationData &target_data);
    void XCZ(const OperationData &target_data);
    void YCX(const OperationData &target_data);
    void YCY(const OperationData &target_data);
    void YCZ(const OperationData &target_data);
    void SWAP(const OperationData &target_data);
    void ISWAP(const OperationData &target_data);
    void MPP(const OperationData &target_data);

    void SQRT_XX(const OperationData &target_data);
    void SQRT_YY(const OperationData &target_data);
    void SQRT_ZZ(const OperationData &target_data);

    void DEPOLARIZE1(const OperationData &target_data);
    void DEPOLARIZE2(const OperationData &target_data);
    void X_ERROR(const OperationData &target_data);
    void Y_ERROR(const OperationData &target_data);
    void Z_ERROR(const OperationData &target_data);
    void PAULI_CHANNEL_1(const OperationData &target_data);
    void PAULI_CHANNEL_2(const OperationData &target_data);
    void CORRELATED_ERROR(const OperationData &target_data);
    void ELSE_CORRELATED_ERROR(const OperationData &target_data);

   private:
    void xor_control_bit_into(uint32_t control, simd_bits_range_ref target);
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
