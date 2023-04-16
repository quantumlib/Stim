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

#include "stim/simulators/frame_simulator_util.h"

#include "stim/simulators/frame_simulator.h"

using namespace stim;

std::pair<simd_bit_table<MAX_BITWORD_WIDTH>, simd_bit_table<MAX_BITWORD_WIDTH>> stim::sample_batch_detection_events(
    const Circuit &circuit, size_t num_shots, std::mt19937_64 &rng) {
    FrameSimulator sim(circuit.compute_stats(), FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, num_shots, rng);
    sim.reset_all_and_run(circuit);

    return std::pair<simd_bit_table<MAX_BITWORD_WIDTH>, simd_bit_table<MAX_BITWORD_WIDTH>>{
        std::move(sim.det_record.storage),
        std::move(sim.obs_record),
    };
}

void rerun_frame_sim_while_streaming_dets_to_disk(
    const Circuit &circuit,
    CircuitStats circuit_stats,
    FrameSimulator &sim,
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    FILE *out,
    SampleFormat format,
    FILE *obs_out,
    SampleFormat obs_out_format) {
    if (prepend_observables) {
        throw std::invalid_argument(
            "--prepend_observables isn't supported when sampling circuits so large that they require streaming the "
            "results");
    }

    MeasureRecordBatchWriter writer(out, num_shots, format);
    std::vector<simd_bits<MAX_BITWORD_WIDTH>> observables;
    sim.reset_all();
    writer.begin_result_type('D');
    circuit.for_each_operation([&](const CircuitInstruction &op) {
        sim.do_gate(op);
        sim.m_record.mark_all_as_written();
        if (op.gate_type == GateType::DETECTOR) {
            constexpr size_t WRITE_SIZE = 256;
            if (sim.det_record.unwritten >= WRITE_SIZE) {
                assert(sim.det_record.stored == WRITE_SIZE);
                assert(sim.det_record.unwritten == WRITE_SIZE);
                writer.batch_write_bytes(sim.det_record.storage, WRITE_SIZE >> 6);
                sim.det_record.clear();
            }
        }
    });
    for (size_t k = sim.det_record.stored - sim.det_record.unwritten; k < sim.det_record.stored; k++) {
        writer.batch_write_bit(sim.det_record.storage[k]);
    }
    if (append_observables) {
        writer.begin_result_type('L');
        for (size_t k = 0; k < circuit_stats.num_observables; k++) {
            writer.batch_write_bit(sim.obs_record[k]);
        }
    }
    writer.write_end();

    if (obs_out != nullptr) {
        write_table_data(
            obs_out,
            num_shots,
            circuit_stats.num_observables,
            simd_bits<MAX_BITWORD_WIDTH>(0),
            sim.obs_record,
            obs_out_format,
            'L',
            'L',
            circuit_stats.num_observables);
    }
}

void rerun_frame_sim_while_streaming_measurements_to_disk(
    const Circuit &circuit,
    FrameSimulator &sim,
    const simd_bits<MAX_BITWORD_WIDTH> &reference_sample,
    size_t num_shots,
    FILE *out,
    SampleFormat format) {
    MeasureRecordBatchWriter writer(out, num_shots, format);
    sim.reset_all();
    circuit.for_each_operation([&](const CircuitInstruction &op) {
        sim.do_gate(op);
        sim.m_record.intermediate_write_unwritten_results_to(writer, reference_sample);
    });
    sim.m_record.final_write_unwritten_results_to(writer, reference_sample);
}

void rerun_frame_sim_in_memory_and_write_dets_to_disk(
    const Circuit &circuit,
    const CircuitStats &circuit_stats,
    FrameSimulator &frame_sim,
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    FILE *out,
    SampleFormat format,
    FILE *obs_out,
    SampleFormat obs_out_format) {
    if (prepend_observables + append_observables + (obs_out != nullptr) > 1) {
        throw std::out_of_range("Can't combine --prepend_observables, --append_observables, or --obs_out");
    }

    frame_sim.reset_all_and_run(circuit);
    const auto &obs_data = frame_sim.obs_record;
    const auto &det_data = frame_sim.det_record.storage;

    if (obs_out != nullptr) {
        write_table_data(
            obs_out,
            num_shots,
            circuit_stats.num_observables,
            simd_bits<MAX_BITWORD_WIDTH>(0),
            obs_data,
            obs_out_format,
            'L',
            'L',
            circuit_stats.num_observables);
    }

    if (prepend_observables || append_observables) {
        simd_bit_table<MAX_BITWORD_WIDTH> concat_data(0, 0);
        if (prepend_observables) {
            assert(!append_observables);
            concat_data = obs_data.concat_major(det_data, circuit_stats.num_observables, circuit_stats.num_detectors);
        } else {
            assert(append_observables);
            concat_data = det_data.concat_major(obs_data, circuit_stats.num_detectors, circuit_stats.num_observables);
        }

        char c1 = append_observables ? 'D' : 'L';
        char c2 = append_observables ? 'L' : 'D';
        size_t ct = append_observables ? circuit_stats.num_detectors : circuit_stats.num_observables;
        write_table_data(
            out,
            num_shots,
            circuit_stats.num_observables + circuit_stats.num_detectors,
            simd_bits<MAX_BITWORD_WIDTH>(0),
            concat_data,
            format,
            c1,
            c2,
            ct);
    } else {
        write_table_data(
            out,
            num_shots,
            circuit_stats.num_detectors,
            simd_bits<MAX_BITWORD_WIDTH>(0),
            det_data,
            format,
            'D',
            'L',
            circuit_stats.num_detectors);
    }
}

void rerun_frame_sim_in_memory_and_write_measurements_to_disk(
    const Circuit &circuit,
    CircuitStats circuit_stats,
    FrameSimulator &frame_sim,
    const simd_bits<MAX_BITWORD_WIDTH> &reference_sample,
    size_t num_shots,
    FILE *out,
    SampleFormat format) {
    frame_sim.reset_all_and_run(circuit);
    const auto &measure_data = frame_sim.m_record.storage;

    write_table_data(
        out, num_shots, circuit_stats.num_measurements, reference_sample, measure_data, format, 'M', 'M', 0);
}

void stim::sample_batch_detection_events_writing_results_to_disk(
    const Circuit &circuit,
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    FILE *out,
    SampleFormat format,
    std::mt19937_64 &rng,
    FILE *obs_out,
    SampleFormat obs_out_format) {
    if (num_shots == 0) {
        // Vacuously complete.
        return;
    }

    auto stats = circuit.compute_stats();

    // Pick a batch size that's not so large that it would cause memory issues.
    size_t batch_size = 0;
    while (batch_size < 1024 && batch_size < num_shots) {
        batch_size += MAX_BITWORD_WIDTH;
    }
    uint64_t memory_per_full_shot =
        2 * stats.num_qubits + 2 * stats.max_lookback + stats.num_observables + stats.num_detectors;
    while (batch_size > 0 &&
           should_use_streaming_because_bit_count_is_too_large_to_store(memory_per_full_shot * batch_size)) {
        batch_size -= MAX_BITWORD_WIDTH;
    }

    // If the batch size ended up at 0, the results won't fit in memory. Need to stream.
    bool streaming = batch_size == 0;
    if (streaming) {
        batch_size = MAX_BITWORD_WIDTH;
    }

    // Create a correctly sized frame simulator.
    FrameSimulator frame_sim(
        stats,
        streaming ? FrameSimulatorMode::STREAM_DETECTIONS_TO_DISK : FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY,
        batch_size,
        rng);

    // Run the frame simulator until as many shots as requested have been written.
    size_t shots_left = num_shots;
    while (shots_left) {
        size_t shots_performed = std::min(shots_left, batch_size);
        if (streaming) {
            rerun_frame_sim_while_streaming_dets_to_disk(
                circuit,
                stats,
                frame_sim,
                shots_performed,
                prepend_observables,
                append_observables,
                out,
                format,
                obs_out,
                obs_out_format);
        } else {
            rerun_frame_sim_in_memory_and_write_dets_to_disk(
                circuit,
                stats,
                frame_sim,
                shots_performed,
                prepend_observables,
                append_observables,
                out,
                format,
                obs_out,
                obs_out_format);
        }
        shots_left -= shots_performed;
    }
}

simd_bit_table<MAX_BITWORD_WIDTH> stim::sample_batch_measurements(
    const Circuit &circuit,
    const simd_bits<MAX_BITWORD_WIDTH> &reference_sample,
    size_t num_samples,
    std::mt19937_64 &rng,
    bool transposed) {
    FrameSimulator sim(circuit.compute_stats(), FrameSimulatorMode::STORE_MEASUREMENTS_TO_MEMORY, num_samples, rng);
    sim.reset_all_and_run(circuit);
    simd_bit_table<MAX_BITWORD_WIDTH> result = std::move(sim.m_record.storage);

    if (reference_sample.not_zero()) {
        result = transposed_vs_ref(num_samples, result, reference_sample);
        transposed = !transposed;
    }

    if (transposed) {
        result = result.transposed();
    }

    return result;
}

void stim::sample_batch_measurements_writing_results_to_disk(
    const Circuit &circuit,
    const simd_bits<MAX_BITWORD_WIDTH> &reference_sample,
    uint64_t num_shots,
    FILE *out,
    SampleFormat format,
    std::mt19937_64 &rng) {
    if (num_shots == 0) {
        // Vacuously complete.
        return;
    }

    auto stats = circuit.compute_stats();

    // Pick a batch size that's not so large that it would cause memory issues.
    size_t batch_size = 0;
    while (batch_size < 1024 && batch_size < num_shots) {
        batch_size += MAX_BITWORD_WIDTH;
    }
    uint64_t memory_per_full_shot = 2 * stats.num_qubits + stats.num_measurements;
    while (batch_size > 0 &&
           should_use_streaming_because_bit_count_is_too_large_to_store(memory_per_full_shot * batch_size)) {
        batch_size -= MAX_BITWORD_WIDTH;
    }

    // If the batch size ended up at 0, the results won't fit in memory. Need to stream.
    bool streaming = batch_size == 0;
    if (streaming) {
        batch_size = MAX_BITWORD_WIDTH;
    }

    // Create a correctly sized frame simulator.
    FrameSimulator frame_sim(
        circuit.compute_stats(),
        streaming ? FrameSimulatorMode::STREAM_MEASUREMENTS_TO_DISK : FrameSimulatorMode::STORE_MEASUREMENTS_TO_MEMORY,
        batch_size,
        rng);

    // Run the frame simulator until as many shots as requested have been written.
    size_t shots_left = num_shots;
    while (shots_left) {
        size_t shots_performed = std::min(shots_left, batch_size);
        if (streaming) {
            rerun_frame_sim_while_streaming_measurements_to_disk(
                circuit, frame_sim, reference_sample, shots_performed, out, format);
        } else {
            rerun_frame_sim_in_memory_and_write_measurements_to_disk(
                circuit, stats, frame_sim, reference_sample, shots_performed, out, format);
        }
        shots_left -= shots_performed;
    }
}
