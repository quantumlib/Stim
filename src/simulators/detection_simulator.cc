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

#include "frame_simulator.h"

template <typename T>
void xor_measurement_set_into_result(
    const T &measurement_set, simd_bit_table &frame_samples, simd_bit_table &output, size_t &output_index_ticker) {
    simd_bits_range_ref dst = output[output_index_ticker++];
    for (auto i : measurement_set) {
        dst ^= frame_samples[i];
    }
}

simd_bit_table detector_samples(
    const Circuit &circuit, const DetectorsAndObservables &det_obs, size_t num_shots,
    bool append_observables, std::mt19937_64 &rng) {
    // Start from measurement samples.
    simd_bit_table frame_samples = FrameSimulator::sample_flipped_measurements(circuit, num_shots, rng);

    auto num_detectors = det_obs.detectors.size();
    auto num_obs = det_obs.observables.size();
    size_t num_results = num_detectors + num_obs * append_observables;
    simd_bit_table result(num_results, num_shots);

    // Xor together measurement samples to form detector samples.
    size_t offset = 0;
    for (const auto &det : det_obs.detectors) {
        xor_measurement_set_into_result(det, frame_samples, result, offset);
    }
    if (append_observables) {
        for (const auto &obs : det_obs.observables) {
            xor_measurement_set_into_result(obs, frame_samples, result, offset);
        }
    }

    return result;
}

simd_bit_table detector_samples(
    const Circuit &circuit, size_t num_shots, bool append_observables, std::mt19937_64 &rng) {
    return detector_samples(
        circuit, DetectorsAndObservables(circuit), num_shots, append_observables, rng);
}

void detector_sample_out_helper(
        const Circuit &circuit,
        FrameSimulator &sim,
        size_t num_samples,
        bool append_observables,
        FILE *out,
        SampleFormat format) {
    BatchResultWriter writer(out, num_samples, format);
    std::vector<simd_bits> observables;
    sim.reset_all();
    writer.set_result_type('D');
    simd_bit_table detector_buffer(1024, num_samples);
    size_t buffered_detectors = 0;
    circuit.for_each_operation([&](const Operation &op) {
        if (op.gate->id == gate_name_to_id("DETECTOR")) {
            simd_bits_range_ref result = detector_buffer[buffered_detectors];
            for (auto t : op.target_data.targets) {
                assert(t & TARGET_RECORD_BIT);
                result ^= sim.m_record.lookback(t ^ TARGET_RECORD_BIT);
            }
            buffered_detectors++;
            if (buffered_detectors == 1024) {
                writer.write_table_batch(detector_buffer, 1024 >> 6);
                buffered_detectors = 0;
            }
        } else if (op.gate->id == gate_name_to_id("OBSERVABLE_INCLUDE")) {
            if (append_observables) {
                size_t id = (size_t)op.target_data.arg;
                while (observables.size() <= id) {
                    observables.emplace_back(num_samples);
                }
                simd_bits_range_ref result = observables[id];

                for (auto t : op.target_data.targets) {
                    assert(t & TARGET_RECORD_BIT);
                    result ^= sim.m_record.lookback(t ^ TARGET_RECORD_BIT);
                }
            }
        } else {
            (sim.*op.gate->frame_simulator_function)(op.target_data);
            sim.m_record.mark_all_as_written();
        }
    });
    for (size_t k = 0; k < buffered_detectors; k++) {
        writer.write_bit_batch(detector_buffer[k]);
    }
    writer.set_result_type('L');
    for (const auto &result : observables) {
        writer.write_bit_batch(result);
    }
    writer.write_end();
}

void detector_samples_out(const Circuit &circuit, size_t num_shots, bool append_observables, FILE *out,
    SampleFormat format, std::mt19937_64 &rng) {

    constexpr size_t GOOD_BLOCK_SIZE = 1024;
    size_t num_qubits = circuit.count_qubits();
    size_t max_lookback = circuit.max_lookback();
    if (num_shots >= GOOD_BLOCK_SIZE) {
        auto sim = FrameSimulator(num_qubits, GOOD_BLOCK_SIZE, max_lookback, rng);
        while (num_shots > GOOD_BLOCK_SIZE) {
            detector_sample_out_helper(circuit, sim, GOOD_BLOCK_SIZE, append_observables, out, format);
            num_shots -= GOOD_BLOCK_SIZE;
        }
    }
    if (num_shots) {
        auto sim = FrameSimulator(num_qubits, num_shots, max_lookback, rng);
        detector_sample_out_helper(circuit, sim, num_shots, append_observables, out, format);
    }
}
