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

#include "stim/simulators/detection_simulator.h"

#include "stim/simulators/frame_simulator.h"

using namespace stim;

std::pair<simd_bit_table<MAX_BITWORD_WIDTH>, simd_bit_table<MAX_BITWORD_WIDTH>> stim::sample_detection_events_simple(
        const Circuit &circuit,
        CircuitDetectorStats circuit_det_stats,
        size_t num_shots,
        std::mt19937_64 &rng) {
    FrameSimulator sim(
        circuit.count_qubits(),
        SIZE_MAX,
        circuit_det_stats.num_detectors,
        circuit_det_stats.num_observables,
        num_shots,
        rng);
    sim.reset_all_and_run(circuit);

    return std::pair<simd_bit_table<MAX_BITWORD_WIDTH>, simd_bit_table<MAX_BITWORD_WIDTH>>{
        std::move(sim.det_record.storage),
        std::move(sim.obs_record),
    };
}

void detector_sample_out_helper_stream(
    const Circuit &circuit,
    FrameSimulator &sim,
    size_t num_samples,
    bool append_observables,
    FILE *out,
    SampleFormat format) {
    MeasureRecordBatchWriter writer(out, num_samples, format);
    std::vector<simd_bits<MAX_BITWORD_WIDTH>> observables;
    sim.reset_all();
    writer.begin_result_type('D');
    simd_bit_table<MAX_BITWORD_WIDTH> detector_buffer(1024, num_samples);
    size_t buffered_detectors = 0;
    circuit.for_each_operation([&](const CircuitInstruction &op) {
        if (op.gate_type == GateType::DETECTOR) {
            simd_bits_range_ref<MAX_BITWORD_WIDTH> result = detector_buffer[buffered_detectors];
            result.clear();
            for (auto t : op.targets) {
                assert(t.data & TARGET_RECORD_BIT);
                result ^= sim.m_record.lookback(t.data ^ TARGET_RECORD_BIT);
            }
            buffered_detectors++;
            if (buffered_detectors == 1024) {
                writer.batch_write_bytes(detector_buffer, 1024 >> 6);
                buffered_detectors = 0;
            }
        } else if (op.gate_type == GateType::OBSERVABLE_INCLUDE) {
            if (append_observables) {
                size_t id = (size_t)op.args[0];
                while (observables.size() <= id) {
                    observables.emplace_back(num_samples);
                }
                simd_bits_range_ref<MAX_BITWORD_WIDTH> result = observables[id];

                for (auto t : op.targets) {
                    assert(t.data & TARGET_RECORD_BIT);
                    result ^= sim.m_record.lookback(t.data ^ TARGET_RECORD_BIT);
                }
            }
        } else {
            sim.do_gate(op);
            sim.m_record.mark_all_as_written();
        }
    });
    for (size_t k = 0; k < buffered_detectors; k++) {
        writer.batch_write_bit(detector_buffer[k]);
    }
    writer.begin_result_type('L');
    for (const auto &result : observables) {
        writer.batch_write_bit(result);
    }
    writer.write_end();
}

void detector_samples_out_in_memory(
    const Circuit &circuit,
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    FILE *out,
    SampleFormat format,
    std::mt19937_64 &rng,
    FILE *obs_out,
    SampleFormat obs_out_format) {
    if (prepend_observables + append_observables + (obs_out != nullptr) > 1) {
        throw std::out_of_range("Can't combine --prepend_observables, --append_observables, or --obs_out");
    }

    auto det_stats = circuit.compute_detector_stats();

    auto [det_data, obs_data] = sample_detection_events_simple(circuit, det_stats, num_shots, rng);
    if (obs_out != nullptr) {
        write_table_data(
            obs_out,
            num_shots,
            det_stats.num_observables,
            simd_bits<MAX_BITWORD_WIDTH>(0),
            obs_data,
            obs_out_format,
            'L',
            'L',
            det_stats.num_observables);
    }

    if (prepend_observables || append_observables) {
        simd_bit_table<MAX_BITWORD_WIDTH> concat_data(0, 0);
        if (prepend_observables) {
            assert(!append_observables);
            concat_data = obs_data.concat_major(det_data, det_stats.num_observables, det_stats.num_detectors);
        } else {
            assert(append_observables);
            concat_data = det_data.concat_major(obs_data, det_stats.num_detectors, det_stats.num_observables);
        }

        char c1 = append_observables ? 'D' : 'L';
        char c2 = append_observables ? 'L' : 'D';
        size_t ct = append_observables ? det_stats.num_detectors : det_stats.num_observables;
        write_table_data(
            out,
            num_shots,
            det_stats.num_observables + det_stats.num_detectors,
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
            det_stats.num_detectors,
            simd_bits<MAX_BITWORD_WIDTH>(0),
            det_data,
            format,
            'D',
            'L',
            det_stats.num_detectors);
    }
}

void detector_sample_out_helper(
    const Circuit &circuit,
    FrameSimulator &sim,
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    FILE *out,
    SampleFormat format,
    std::mt19937_64 &rng,
    FILE *obs_out,
    SampleFormat obs_out_format) {
    uint64_t d = circuit.count_detectors() + circuit.count_observables();
    uint64_t approx_mem_usage = std::max(num_shots, size_t{256}) * std::max(circuit.count_measurements(), d);
    if (!prepend_observables && obs_out == nullptr && should_use_streaming_instead_of_memory(approx_mem_usage)) {
        detector_sample_out_helper_stream(circuit, sim, num_shots, append_observables, out, format);
    } else {
        detector_samples_out_in_memory(
            circuit, num_shots, prepend_observables, append_observables, out, format, rng, obs_out, obs_out_format);
    }
}

void stim::detector_samples_out(
    const Circuit &circuit,
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    FILE *out,
    SampleFormat format,
    std::mt19937_64 &rng,
    FILE *obs_out,
    SampleFormat obs_out_format) {
    constexpr size_t GOOD_BLOCK_SIZE = 768;
    size_t num_qubits = circuit.count_qubits();
    size_t max_lookback = circuit.max_lookback();
    auto det_stats = circuit.compute_detector_stats();
    if (num_shots >= GOOD_BLOCK_SIZE) {
        auto sim = FrameSimulator(num_qubits, max_lookback, det_stats.num_detectors, det_stats.num_observables, GOOD_BLOCK_SIZE, rng);
        while (num_shots > GOOD_BLOCK_SIZE) {
            detector_sample_out_helper(
                circuit,
                sim,
                GOOD_BLOCK_SIZE,
                prepend_observables,
                append_observables,
                out,
                format,
                rng,
                obs_out,
                obs_out_format);
            num_shots -= GOOD_BLOCK_SIZE;
        }
    }
    if (num_shots) {
        auto sim = FrameSimulator(num_qubits, max_lookback, det_stats.num_detectors, det_stats.num_observables, num_shots, rng);
        detector_sample_out_helper(
            circuit,
            sim,
            num_shots,
            prepend_observables,
            append_observables,
            out,
            format,
            rng,
            obs_out,
            obs_out_format);
    }
}
