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

simd_bit_table stim::initial_errors_to_flipped_measurements_raw(
    const simd_bit_table &errors_x,
    const simd_bit_table &errors_z,
    const Circuit &noiseless_circuit,
    size_t num_qubits,
    size_t num_measurements) {
    size_t batch_size = errors_x.num_minor_bits_padded();
    assert(errors_z.num_minor_bits_padded() == batch_size);
    assert(errors_x.num_major_bits_padded() >= num_qubits);
    assert(errors_x.num_major_bits_padded() == errors_z.num_major_bits_padded());

    // Not simulating, only propagating; no need to seed.
    std::mt19937_64 rng1(0);
    std::mt19937_64 rng2(0);
    FrameSimulator f(num_qubits, batch_size, num_measurements, rng1);

    f.guarantee_anticommutation_via_frame_randomization = false;
    f.m_record.clear();
    f.x_table = errors_x;
    f.z_table = errors_z;
    noiseless_circuit.for_each_operation([&](const Operation &op) {
        (f.*op.gate->frame_simulator_function)(op.target_data);
    });
    assert(rng1() == rng2()); // Verify no randomness used.
    return f.m_record.storage;
}

void measurements_to_detection_events_raw(
    const simd_bit_table &measurement_results_minor_shots,
    simd_bit_table &out_detection_results_minor_shots,
    const Circuit &circuit,
    const simd_bits &reference_sample,
    bool append_observables,
    size_t num_measurements,
    size_t num_detectors,
    size_t num_observables) {
    assert(
        out_detection_results_minor_shots.num_minor_bits_padded() ==
        measurement_results_minor_shots.num_minor_bits_padded());
    uint64_t measure_count_so_far = 0;
    uint64_t detector_offset = 0;
    assert(
        out_detection_results_minor_shots.num_major_bits_padded() >=
        num_detectors + num_observables * append_observables);
    assert(measurement_results_minor_shots.num_major_bits_padded() >= num_measurements);
    const auto det_id = gate_name_to_id("DETECTOR");
    const auto obs_id = gate_name_to_id("OBSERVABLE_INCLUDE");
    circuit.for_each_operation([&](const Operation &op) {
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
            // Keep track of measurement results so far.
            measure_count_so_far += op.count_measurement_results();
            return;
        }

        // XOR together the appropriate measurements.
        out_detection_results_minor_shots[out_index].clear();
        for (const auto &t : op.target_data.targets) {
            assert(t.is_measurement_record_target());  // Circuit validation should guarantee this.
            uint32_t lookback = t.data & TARGET_VALUE_MASK;
            if (lookback > measure_count_so_far) {
                throw std::invalid_argument("Referred to a measurement result before the beginning of time.");
            }
            out_detection_results_minor_shots[out_index] ^=
                measurement_results_minor_shots[measure_count_so_far - lookback];
            if (reference_sample[measure_count_so_far - lookback]) {
                out_detection_results_minor_shots[out_index].invert_bits();
            }
        }
    });
}

simd_bit_table stim::measurements_to_detection_events(
    const simd_bit_table &measurement_results_minor_shots,
    const Circuit &circuit,
    bool append_observables,
    bool skip_reference_sample) {
    size_t num_measurements = circuit.count_measurements();
    size_t num_detectors = circuit.count_detectors();
    size_t num_observables = circuit.num_observables();
    simd_bits reference_sample(num_measurements);
    if (!skip_reference_sample) {
        reference_sample = TableauSimulator::reference_sample_circuit(circuit);
    }
    simd_bit_table out(
        num_detectors + num_observables * append_observables, measurement_results_minor_shots.num_minor_bits_padded());
    measurements_to_detection_events_raw(
        measurement_results_minor_shots,
        out,
        circuit,
        reference_sample,
        append_observables,
        num_measurements,
        num_detectors,
        num_observables);
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
    size_t num_observables = circuit.num_observables();
    size_t num_detectors = circuit.count_detectors();
    size_t num_od = num_detectors + num_observables * append_observables;
    size_t num_f_qubits = optional_initial_error_frames_in == nullptr ? 0 : circuit.count_qubits();
    size_t num_buffered_shots = 1024;
    Circuit noiseless = circuit.aliased_noiseless_circuit();
    simd_bits reference_sample(num_measurements);
    if (!skip_reference_sample) {
        reference_sample = TableauSimulator::reference_sample_circuit(noiseless);
    }

    // Readers / writers.
    auto reader = MeasureRecordReader::make(measurements_in, measurements_in_format, num_measurements);
    std::unique_ptr<MeasureRecordReader> initial_frame_reader;
    auto writer = MeasureRecordWriter::make(results_out, results_out_format);
    if (optional_initial_error_frames_in != nullptr) {
        initial_frame_reader = MeasureRecordReader::make(optional_initial_error_frames_in, initial_error_frames_in_format, num_f_qubits);
    }

    // Buffers and transposed buffers.
    simd_bit_table m_buffer_minor_shots(num_measurements, num_buffered_shots);
    simd_bit_table d_buffer_minor_shots(num_od, num_buffered_shots);
    simd_bit_table d_buffer_major_shots(num_buffered_shots, num_od);
    simd_bit_table f_buffer_major_shots_x(num_buffered_shots, num_f_qubits);
    simd_bit_table f_buffer_minor_shots_x(num_f_qubits, num_buffered_shots);
    simd_bit_table f_buffer_major_shots_z(num_buffered_shots, num_f_qubits);
    simd_bit_table f_buffer_minor_shots_z(num_f_qubits, num_buffered_shots);

    // Data streaming loop.
    while (true) {
        // Read measurement data.
        size_t record_count = reader->read_records_into(m_buffer_minor_shots, false);
        if (record_count == 0) {
            break;
        }

        // Adjust measurement results based on errors injected into initial frame.
        if (initial_frame_reader != nullptr) {
            for (size_t k = 0; k < record_count; k++) {
                bool bx = initial_frame_reader->start_and_read_entire_record(f_buffer_major_shots_x[k]);
                bool bz = initial_frame_reader->start_and_read_entire_record(f_buffer_major_shots_z[k]);
                if (!bx || !bz) {
                    throw std::invalid_argument(
                        "The frame data contained fewer frames than the number of shots in the measurement data.");
                }
            }
            f_buffer_major_shots_x.transpose_into(f_buffer_minor_shots_x);
            f_buffer_major_shots_z.transpose_into(f_buffer_minor_shots_z);
            auto r = initial_errors_to_flipped_measurements_raw(f_buffer_minor_shots_x, f_buffer_minor_shots_z, noiseless, num_f_qubits, num_measurements);
            m_buffer_minor_shots.data ^= initial_errors_to_flipped_measurements_raw(f_buffer_minor_shots_x, f_buffer_minor_shots_z, noiseless, num_f_qubits, num_measurements).data;
        }

        // Convert measurement data into detection event data.
        measurements_to_detection_events_raw(
            m_buffer_minor_shots,
            d_buffer_minor_shots,
            circuit,
            reference_sample,
            append_observables,
            num_measurements,
            num_detectors,
            num_observables);
        d_buffer_minor_shots.transpose_into(d_buffer_major_shots);

        // Write detection event data.
        for (size_t k = 0; k < record_count; k++) {
            simd_bits_range_ref record = d_buffer_major_shots[k];
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
