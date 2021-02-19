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
    const Circuit &circuit, const DetectorsAndObservables &det_obs, size_t num_shots, bool prepend_observables,
    bool append_observables, std::mt19937_64 &rng) {
    // Start from measurement samples.
    simd_bit_table frame_samples = FrameSimulator::sample_flipped_measurements(circuit, num_shots, rng);

    auto num_detectors = det_obs.detectors.size();
    auto num_obs = det_obs.observables.size();
    size_t num_results = num_detectors + num_obs * ((int)prepend_observables + (int)append_observables);
    simd_bit_table result(num_results, num_shots);

    // Xor together measurement samples to form detector samples.
    size_t offset = 0;
    if (prepend_observables) {
        for (auto obs : det_obs.observables) {
            xor_measurement_set_into_result(obs, frame_samples, result, offset);
        }
    }
    for (auto det : det_obs.detectors) {
        xor_measurement_set_into_result(det, frame_samples, result, offset);
    }
    if (append_observables) {
        for (auto obs : det_obs.observables) {
            xor_measurement_set_into_result(obs, frame_samples, result, offset);
        }
    }

    return result;
}

simd_bit_table detector_samples(
    const Circuit &circuit, size_t num_shots, bool prepend_observables, bool append_observables, std::mt19937_64 &rng) {
    return detector_samples(
        circuit, DetectorsAndObservables(circuit), num_shots, prepend_observables, append_observables, rng);
}

void detector_samples_out(
    const Circuit &circuit, size_t num_shots, bool prepend_observables, bool append_observables, FILE *out,
    SampleFormat format, std::mt19937_64 &rng) {
    DetectorsAndObservables det_obs(circuit);
    size_t num_sample_locations =
        det_obs.detectors.size() + det_obs.observables.size() * ((int)prepend_observables + (int)append_observables);

    constexpr size_t GOOD_BLOCK_SIZE = 1024;
    simd_bits reference_sample(num_sample_locations);
    while (num_shots > GOOD_BLOCK_SIZE) {
        auto table = detector_samples(circuit, det_obs, GOOD_BLOCK_SIZE, prepend_observables, append_observables, rng);
        write_table_data(out, GOOD_BLOCK_SIZE, num_sample_locations, reference_sample, table, format);
        num_shots -= GOOD_BLOCK_SIZE;
    }
    if (num_shots) {
        auto table = detector_samples(circuit, det_obs, num_shots, prepend_observables, append_observables, rng);
        write_table_data(out, num_shots, num_sample_locations, reference_sample, table, format);
    }
}
