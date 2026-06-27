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

#include <cassert>

#include "stim/gates/gates.h"
#include "stim/io/measure_record_batch_writer.h"
#include "stim/io/measure_record_reader.h"
#include "stim/io/stim_data_formats.h"
#include "stim/mem/simd_util.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/measurements_to_detection_events.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/stabilizers/pauli_string.h"

namespace stim {

template <size_t W>
void measurements_to_detection_events_helper(
    const simd_bit_table<W> &measurements__minor_shot_index,
    const simd_bit_table<W> &sweep_bits__minor_shot_index,
    simd_bit_table<W> &out_detection_results__minor_shot_index,
    const Circuit &noiseless_circuit,
    CircuitStats circuit_stats,
    const simd_bits<W> &reference_sample,
    bool append_observables) {
    // Tables should agree on the batch size.
    size_t batch_size = out_detection_results__minor_shot_index.num_minor_bits_padded();
    if (measurements__minor_shot_index.num_minor_bits_padded() != batch_size) {
        throw std::invalid_argument("measurements__minor_shot_index.num_minor_bits_padded() != batch_size");
    }
    if (sweep_bits__minor_shot_index.num_minor_bits_padded() != batch_size) {
        throw std::invalid_argument("sweep_bits__minor_shot_index.num_minor_bits_padded() != batch_size");
    }
    // Tables should have the right number of bits per shot.
    if (out_detection_results__minor_shot_index.num_major_bits_padded() <
        circuit_stats.num_detectors + circuit_stats.num_observables * append_observables) {
        throw std::invalid_argument(
            "out_detection_results__minor_shot_index.num_major_bits_padded() < num_detectors + num_observables * "
            "append_observables");
    }
    if (measurements__minor_shot_index.num_major_bits_padded() < circuit_stats.num_measurements) {
        throw std::invalid_argument("measurements__minor_shot_index.num_major_bits_padded() < num_measurements");
    }

    // The frame simulator is used to account for flips in the measurement results that originate from the sweep data.
    // Eg. a `CNOT sweep[5] 0` can bit flip qubit 0, which can invert later measurement results, which will invert the
    // expected parity of detectors involving that measurement. This can vary from shot to shot.
    FrameSimulator<W> frame_sim(
        circuit_stats, FrameSimulatorMode::STREAM_DETECTIONS_TO_DISK, batch_size, std::mt19937_64(0));
    frame_sim.sweep_table = sweep_bits__minor_shot_index;
    frame_sim.guarantee_anticommutation_via_frame_randomization = false;

    uint64_t detector_offset = 0;
    uint64_t measure_count_so_far = 0;
    noiseless_circuit.for_each_operation([&](const CircuitInstruction &op) {
        frame_sim.do_gate(op);

        switch (op.gate_type) {
            case GateType::DETECTOR: {
                simd_bits_range_ref<W> out_row = out_detection_results__minor_shot_index[detector_offset];
                detector_offset++;

                // Include dependence from gates controlled by sweep bits.
                out_row ^= frame_sim.det_record.lookback(1);

                bool expectation = false;
                for (const auto &t : op.targets) {
                    uint32_t lookback = t.data & TARGET_VALUE_MASK;
                    // Include dependence from physical measurement results.
                    out_row ^= measurements__minor_shot_index[measure_count_so_far - lookback];
                    // Include dependence from reference sample expectation.
                    expectation ^= reference_sample[measure_count_so_far - lookback];
                }
                if (expectation) {
                    out_row.invert_bits();
                }

                frame_sim.det_record.clear();
                break;
            }
            case GateType::OBSERVABLE_INCLUDE: {
                simd_bits_range_ref<W> obs_row = frame_sim.obs_record[(uint64_t)op.args[0]];
                bool expectation = false;
                for (const auto &t : op.targets) {
                    if (t.is_classical_bit_target()) {
                        uint32_t lookback = t.data & TARGET_VALUE_MASK;
                        // Include dependence from physical measurement results.
                        obs_row ^= measurements__minor_shot_index[measure_count_so_far - lookback];
                        // Include dependence from reference sample expectation.
                        expectation ^= reference_sample[measure_count_so_far - lookback];
                    } else if (t.is_pauli_target()) {
                        // Ignored.
                    } else {
                        throw std::invalid_argument("Unexpected target for OBSERVABLE_INCLUDE: " + t.str());
                    }
                }
                if (expectation) {
                    obs_row.invert_bits();
                }
                break;
            }
            default:
                measure_count_so_far += op.count_measurement_results();
        }
    });

    if (append_observables) {
        for (size_t k = 0; k < circuit_stats.num_observables; k++) {
            // Include dependence from gates controlled by sweep bits.
            out_detection_results__minor_shot_index[circuit_stats.num_detectors + k] ^= frame_sim.obs_record[k];
        }
    }

    // Safety check verifying no randomness was used by the frame simulator.
    std::mt19937_64 fresh_rng(0);
    if (frame_sim.rng() != fresh_rng() || frame_sim.rng() != fresh_rng() || frame_sim.rng() != fresh_rng()) {
        throw std::invalid_argument("Something is wrong. Converting measurements consumed entropy, but it shouldn't.");
    }
}

template <size_t W>
simd_bit_table<W> measurements_to_detection_events(
    const simd_bit_table<W> &measurements__minor_shot_index,
    const simd_bit_table<W> &sweep_bits__minor_shot_index,
    const Circuit &circuit,
    bool append_observables,
    bool skip_reference_sample) {
    CircuitStats circuit_stats = circuit.compute_stats();
    simd_bits<W> reference_sample(circuit_stats.num_measurements);
    if (!skip_reference_sample) {
        reference_sample = TableauSimulator<W>::reference_sample_circuit(circuit);
    }
    simd_bit_table<W> out(
        circuit_stats.num_detectors + circuit_stats.num_observables * append_observables,
        measurements__minor_shot_index.num_minor_bits_padded());
    measurements_to_detection_events_helper(
        measurements__minor_shot_index,
        sweep_bits__minor_shot_index,
        out,
        circuit.aliased_noiseless_circuit(),
        circuit_stats,
        reference_sample,
        append_observables);
    return out;
}

template <size_t W>
void stream_measurements_to_detection_events(
    FILE *measurements_in,
    SampleFormat measurements_in_format,
    FILE *optional_sweep_bits_in,
    SampleFormat sweep_bits_format,
    FILE *results_out,
    SampleFormat results_out_format,
    const Circuit &circuit,
    bool append_observables,
    bool skip_reference_sample,
    FILE *obs_out,
    SampleFormat obs_out_format) {
    // Circuit metadata.
    CircuitStats circuit_stats = circuit.compute_stats();
    simd_bits<W> reference_sample(circuit_stats.num_measurements);
    Circuit noiseless_circuit = circuit.aliased_noiseless_circuit();
    if (!skip_reference_sample) {
        reference_sample = TableauSimulator<W>::reference_sample_circuit(circuit);
    }

    stream_measurements_to_detection_events_helper<W>(
        measurements_in,
        measurements_in_format,
        optional_sweep_bits_in,
        sweep_bits_format,
        results_out,
        results_out_format,
        noiseless_circuit,
        circuit_stats,
        append_observables,
        reference_sample,
        obs_out,
        obs_out_format);
}

template <size_t W>
void stream_measurements_to_detection_events_helper(
    FILE *measurements_in,
    SampleFormat measurements_in_format,
    FILE *optional_sweep_bits_in,
    SampleFormat sweep_bits_in_format,
    FILE *results_out,
    SampleFormat results_out_format,
    const Circuit &noiseless_circuit,
    CircuitStats circuit_stats,
    bool append_observables,
    simd_bits_range_ref<W> reference_sample,
    FILE *obs_out,
    SampleFormat obs_out_format) {
    bool internally_append_observables = append_observables || obs_out != nullptr;
    size_t num_out_bits_including_any_obs =
        circuit_stats.num_detectors + circuit_stats.num_observables * internally_append_observables;
    size_t num_sweep_bits_available = optional_sweep_bits_in == nullptr ? 0 : circuit_stats.num_sweep_bits;
    size_t num_buffered_shots = 1024;

    // Readers / writers.
    auto reader = MeasureRecordReader<W>::make(measurements_in, measurements_in_format, circuit_stats.num_measurements);
    std::unique_ptr<MeasureRecordReader<W>> sweep_data_reader;
    std::unique_ptr<MeasureRecordWriter> obs_writer;
    if (obs_out != nullptr) {
        obs_writer = MeasureRecordWriter::make(obs_out, obs_out_format);
    }
    auto writer = MeasureRecordWriter::make(results_out, results_out_format);
    if (optional_sweep_bits_in != nullptr) {
        sweep_data_reader =
            MeasureRecordReader<W>::make(optional_sweep_bits_in, sweep_bits_in_format, circuit_stats.num_sweep_bits);
    }

    // Buffers and transposed buffers.
    simd_bit_table<W> measurements__minor_shot_index(circuit_stats.num_measurements, num_buffered_shots);
    simd_bit_table<W> out__minor_shot_index(num_out_bits_including_any_obs, num_buffered_shots);
    simd_bit_table<W> out__major_shot_index(num_buffered_shots, num_out_bits_including_any_obs);
    simd_bit_table<W> sweep_bits__minor_shot_index(num_sweep_bits_available, num_buffered_shots);
    if (reader->expects_empty_serialized_data_for_each_shot()) {
        throw std::invalid_argument(
            "Can't tell how many shots are in the measurement data.\n"
            "The circuit has no measurements and the measurement format encodes empty shots into no bytes.");
    }

    // Data streaming loop.
    size_t total_read = 0;
    while (true) {
        // Read measurement data and sweep data for a batch of shots.
        size_t record_count = reader->read_records_into(measurements__minor_shot_index, false);
        if (sweep_data_reader != nullptr) {
            size_t sweep_data_count = sweep_data_reader->read_records_into(sweep_bits__minor_shot_index, false);
            if (sweep_data_count != record_count && !sweep_data_reader->expects_empty_serialized_data_for_each_shot()) {
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
        out__minor_shot_index.clear();
        measurements_to_detection_events_helper<W>(
            measurements__minor_shot_index,
            sweep_bits__minor_shot_index,
            out__minor_shot_index,
            noiseless_circuit,
            circuit_stats,
            reference_sample,
            internally_append_observables);
        out__minor_shot_index.transpose_into(out__major_shot_index);

        // Write detection event data.
        for (size_t k = 0; k < record_count; k++) {
            simd_bits_range_ref<W> record = out__major_shot_index[k];
            writer->begin_result_type('D');
            writer->write_bits(record.u8, circuit_stats.num_detectors);
            if (append_observables) {
                writer->begin_result_type('L');
                for (size_t k2 = 0; k2 < circuit_stats.num_observables; k2++) {
                    writer->write_bit(record[circuit_stats.num_detectors + k2]);
                }
            }
            writer->write_end();

            if (obs_out != nullptr) {
                obs_writer->begin_result_type('L');
                for (size_t k2 = 0; k2 < circuit_stats.num_observables; k2++) {
                    obs_writer->write_bit(record[circuit_stats.num_detectors + k2]);
                }
                obs_writer->write_end();
            }
        }
    }
}

}  // namespace stim
