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

#ifndef SIM_FRAME_H
#define SIM_FRAME_H

#include <random>

#include "../circuit/circuit.h"
#include "../simd/simd_bit_table.h"
#include "../stabilizers/pauli_string.h"

/// A Pauli Frame simulator that computes many samples simultaneously.
///
/// This simulator tracks, for each qubit, whether or not that qubit is bit flipped and/or phase flipped.
/// Instead of reporting qubit measurements, it reports whether a measurement is inverted or not.
/// This requires a set of reference measurements to diff against.
struct FrameSimulator {
    size_t num_qubits;
    size_t num_samples_raw;
    size_t num_measurements_raw;
    size_t num_recorded_measurements;
    simd_bit_table x_table;
    simd_bit_table z_table;
    simd_bit_table m_table;
    simd_bits rng_buffer;
    simd_bits last_correlated_error_occurred;
    std::mt19937_64 &rng;

    FrameSimulator(size_t num_qubits, size_t num_samples, size_t num_measurements, std::mt19937_64 &rng);

    static simd_bit_table sample_flipped_measurements(const Circuit &circuit, size_t num_samples, std::mt19937_64 &rng);
    static simd_bit_table sample(
        const Circuit &circuit, const simd_bits &reference_sample, size_t num_samples, std::mt19937_64 &rng);
    static void sample_out(
        const Circuit &circuit, const simd_bits &reference_sample, size_t num_samples, FILE *out, SampleFormat format,
        std::mt19937_64 &rng);

    PauliString get_frame(size_t sample_index) const;
    void set_frame(size_t sample_index, const PauliStringRef &new_frame);

    void reset_all_and_run(const Circuit &circuit);
    void reset_all();

    void write_measurements(FILE *out, const simd_bits &reference_sample, SampleFormat format) const;

    void measure(const OperationData &target_data);
    void reset(const OperationData &target_data);
    void measure_reset(const OperationData &target_data);

    void I(const OperationData &target_data);
    void H_XZ(const OperationData &target_data);
    void H_XY(const OperationData &target_data);
    void H_YZ(const OperationData &target_data);
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

    void DEPOLARIZE1(const OperationData &target_data);
    void DEPOLARIZE2(const OperationData &target_data);
    void X_ERROR(const OperationData &target_data);
    void Y_ERROR(const OperationData &target_data);
    void Z_ERROR(const OperationData &target_data);
    void CORRELATED_ERROR(const OperationData &target_data);
    void ELSE_CORRELATED_ERROR(const OperationData &target_data);

   private:
    simd_bits_range_ref measurement_record_ref(uint32_t encoded_target);
    void single_cx(uint32_t c, uint32_t t);
    void single_cy(uint32_t c, uint32_t t);
};

void write_table_data(
        FILE *out,
        size_t num_shots_raw,
        size_t num_sample_locations_raw,
        const simd_bits &reference_sample,
        const simd_bit_table &table,
        SampleFormat format,
        char dets_prefix_1,
        char dets_prefix_2,
        size_t dets_prefix_transition);

#endif
