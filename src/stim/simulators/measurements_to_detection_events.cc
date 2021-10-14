// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/simulators/measurements_to_detection_events.h"

#include <cassert>

#include "frame_simulator.h"
#include "stim/circuit/gate_data.h"
#include "stim/io/measure_record_batch_writer.h"
#include "stim/io/measure_record_reader.h"
#include "stim/io/stim_data_formats.h"
#include "stim/mem/simd_util.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/stabilizers/pauli_string.h"

using namespace stim;

void stim::measurements_to_detection_events_helper(
    const simd_bit_table &measurements__minor_shot_index,
    const simd_bit_table &sweep_bits__minor_shot_index,
    simd_bit_table &out_detection_results__minor_shot_index,
    const Circuit &noiseless_circuit,
    const simd_bits &reference_sample,
    bool append_observables,
    size_t num_measurements,
    size_t num_detectors,
    size_t num_observables,
    size_t num_qubits) {
    // Tables should agree on the batch size.
    size_t batch_size = out_detection_results__minor_shot_index.num_minor_bits_padded();
    assert(measurements__minor_shot_index.num_minor_bits_padded() == batch_size);
    assert(sweep_bits__minor_shot_index.num_minor_bits_padded() == batch_size);
    // Tables should have the right number of bits per shot.
    assert(
        out_detection_results__minor_shot_index.num_major_bits_padded() >=
        num_detectors + num_observables * append_observables);
    assert(measurements__minor_shot_index.num_major_bits_padded() >= num_measurements);

    // The frame simulator is used to account for flips in the measurement results that originate from the sweep data.
    // Eg. a `CNOT sweep[5] 0` can bit flip qubit 0, which can invert later measurement results, which will invert the
    // expected parity of detectors involving that measurement. This can vary from shot to shot.
    std::mt19937_64 rng1(0);
    std::mt19937_64 rng2(0);
    FrameSimulator frame_sim(num_qubits, batch_size, num_measurements, rng1);
    frame_sim.sweep_table = sweep_bits__minor_shot_index;
    frame_sim.guarantee_anticommutation_via_frame_randomization = false;

    uint64_t measure_count_so_far = 0;
    uint64_t detector_offset = 0;
    const auto det_id = gate_name_to_id("DETECTOR");
    const auto obs_id = gate_name_to_id("OBSERVABLE_INCLUDE");
    noiseless_circuit.for_each_operation([&](const Operation &op) {
        uint64_t out_index;
        if (op.gate->id == det_id) {
            // Detectors go into next slot.
            out_index = detector_offset;
            detector_offset++;
        } else if (append_observables && op.gate->id == obs_id) {
            // Observables accumulate at end of output, if desired.
            assert(!op.target_data.args.empty());  // Circuit validation should guarantee this.
            out_index = num_detectors + (uint64_t)op.target_data.args[0];
        } else {
            measure_count_so_far += op.count_measurement_results();
            (frame_sim.*op.gate->frame_simulator_function)(op.target_data);
            return;
        }

        // XOR together the appropriate measurement inversions, using the reference sample as a baseline.
        out_detection_results__minor_shot_index[out_index].clear();
        for (const auto &t : op.target_data.targets) {
            assert(t.is_measurement_record_target());  // Circuit validation should guarantee this.
            uint32_t lookback = t.data & TARGET_VALUE_MASK;
            if (lookback > measure_count_so_far) {
                throw std::invalid_argument("Referred to a measurement result before the beginning of time.");
            }
            out_detection_results__minor_shot_index[out_index] ^=
                measurements__minor_shot_index[measure_count_so_far - lookback];
            out_detection_results__minor_shot_index[out_index] ^= frame_sim.m_record.lookback(lookback);
            if (reference_sample[measure_count_so_far - lookback]) {
                out_detection_results__minor_shot_index[out_index].invert_bits();
            }
        }
    });

    // Safety check verifying no randomness was used by the frame simulator.
    assert(rng1() == rng2());
}

simd_bit_table stim::measurements_to_detection_events(
    const simd_bit_table &measurements__minor_shot_index,
    const simd_bit_table &sweep_bits__minor_shot_index,
    const Circuit &circuit,
    bool append_observables,
    bool skip_reference_sample) {
    size_t num_measurements = circuit.count_measurements();
    size_t num_detectors = circuit.count_detectors();
    size_t num_observables = circuit.count_observables();
    size_t num_qubits = circuit.count_qubits();
    simd_bits reference_sample(num_measurements);
    if (!skip_reference_sample) {
        reference_sample = TableauSimulator::reference_sample_circuit(circuit);
    }
    simd_bit_table out(
        num_detectors + num_observables * append_observables, measurements__minor_shot_index.num_minor_bits_padded());
    measurements_to_detection_events_helper(
        measurements__minor_shot_index,
        sweep_bits__minor_shot_index,
        out,
        circuit.aliased_noiseless_circuit(),
        reference_sample,
        append_observables,
        num_measurements,
        num_detectors,
        num_observables,
        num_qubits);
    return out;
}

void stim::stream_measurements_to_detection_events(
    FILE *measurements_in,
    SampleFormat measurements_in_format,
    FILE *optional_initial_error_frames_in,
    SampleFormat initial_error_frames_in_format,
    FILE *results_out,
    SampleFormat results_out_format,
    const Circuit &circuit,
    bool append_observables,
    bool skip_reference_sample) {
    // Circuit metadata.
    size_t num_measurements = circuit.count_measurements();
    size_t num_observables = circuit.count_observables();
    size_t num_detectors = circuit.count_detectors();
    size_t num_qubits = circuit.count_qubits();
    size_t num_sweep_bits = circuit.count_sweep_bits();
    simd_bits reference_sample(num_measurements);
    Circuit noiseless_circuit = circuit.aliased_noiseless_circuit();
    if (!skip_reference_sample) {
        reference_sample = TableauSimulator::reference_sample_circuit(circuit);
    }

    stream_measurements_to_detection_events_helper(
        measurements_in,
        measurements_in_format,
        optional_initial_error_frames_in,
        initial_error_frames_in_format,
        results_out,
        results_out_format,
        noiseless_circuit,
        append_observables,
        reference_sample,
        num_measurements,
        num_observables,
        num_detectors,
        num_qubits,
        num_sweep_bits);
}

void stim::stream_measurements_to_detection_events_helper(
    FILE *measurements_in,
    SampleFormat measurements_in_format,
    FILE *optional_sweep_bits_in,
    SampleFormat sweep_bits_in_format,
    FILE *results_out,
    SampleFormat results_out_format,
    const Circuit &noiseless_circuit,
    bool append_observables,
    simd_bits_range_ref reference_sample,
    size_t num_measurements,
    size_t num_observables,
    size_t num_detectors,
    size_t num_qubits,
    size_t num_sweep_bits) {
    size_t num_out_bits = num_detectors + num_observables * append_observables;
    size_t num_sweep_bits_available = optional_sweep_bits_in == nullptr ? 0 : num_sweep_bits;
    size_t num_buffered_shots = 1024;

    // Readers / writers.
    auto reader = MeasureRecordReader::make(measurements_in, measurements_in_format, num_measurements);
    std::unique_ptr<MeasureRecordReader> sweep_data_reader;
    auto writer = MeasureRecordWriter::make(results_out, results_out_format);
    if (optional_sweep_bits_in != nullptr) {
        sweep_data_reader = MeasureRecordReader::make(optional_sweep_bits_in, sweep_bits_in_format, num_sweep_bits);
    }

    // Buffers and transposed buffers.
    simd_bit_table measurements__minor_shot_index(num_measurements, num_buffered_shots);
    simd_bit_table out__minor_shot_index(num_out_bits, num_buffered_shots);
    simd_bit_table out__major_shot_index(num_buffered_shots, num_out_bits);
    simd_bit_table sweep_bits__minor_shot_index(num_sweep_bits_available, num_buffered_shots);

    // Data streaming loop.
    size_t total_read = 0;
    while (true) {
        // Read measurement data and sweep data for a batch of shots.
        size_t record_count = reader->read_records_into(measurements__minor_shot_index, false);
        if (sweep_data_reader != nullptr) {
            size_t sweep_data_count = sweep_data_reader->read_records_into(sweep_bits__minor_shot_index, false);
            if (sweep_data_count != record_count) {
                std::stringstream ss;
                ss << "The sweep data contained a different number of shots than the measurement data.\n";
                ss << "There was " << (record_count + total_read) << " shot records total.\n";
                if (sweep_data_count < record_count) {
                    ss << "But there was " << (record_count + sweep_data_count) << " sweep records total.";
                } else {
                    ss << "But there was at least " << (record_count + sweep_data_count) << " sweep records.";
                }
                throw std::invalid_argument(ss.str());
            }
        }
        if (record_count == 0) {
            break;
        }
        total_read += record_count;

        // Convert measurement data into detection event data.
        measurements_to_detection_events_helper(
            measurements__minor_shot_index,
            sweep_bits__minor_shot_index,
            out__minor_shot_index,
            noiseless_circuit,
            reference_sample,
            append_observables,
            num_measurements,
            num_detectors,
            num_observables,
            num_qubits);
        out__minor_shot_index.transpose_into(out__major_shot_index);

        // Write detection event data.
        for (size_t k = 0; k < record_count; k++) {
            simd_bits_range_ref record = out__major_shot_index[k];
            writer->begin_result_type('D');
            writer->write_bits(record.u8, num_detectors);
            if (append_observables) {
                writer->begin_result_type('L');
                for (size_t k2 = 0; k2 < num_observables; k2++) {
                    writer->write_bit(record[num_detectors + k2]);
                }
            }
            writer->write_end();
        }
    }
}
