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

void xor_measurement_sets_into_result(
    const std::vector<MeasurementSet> &measurement_sets, bool include_expected_parity, simd_bit_table &frame_samples,
    simd_bit_table &combined_samples, size_t &offset) {
    for (const auto &obs : measurement_sets) {
        simd_bits_range_ref dst = combined_samples[offset++];
        if (include_expected_parity && obs.expected_parity) {
            dst.invert_bits();
        }
        for (auto i : obs.indices) {
            dst ^= frame_samples[i];
        }
    }
}

simd_bit_table detector_samples(
    const Circuit &circuit, const std::vector<MeasurementSet> &detectors,
    const std::vector<MeasurementSet> &observables, size_t num_shots, bool prepend_observables, bool append_observables,
    std::mt19937_64 &rng) {
    // Start from measurement samples.
    simd_bit_table frame_samples = FrameSimulator::sample_flipped_measurements(circuit, num_shots, rng);

    auto num_detectors = detectors.size();
    auto num_obs = observables.size();
    size_t num_results = num_detectors + num_obs * ((int)prepend_observables + (int)append_observables);
    simd_bit_table result(num_results, num_shots);

    // Xor together measurement samples to form detector samples.
    size_t offset = 0;
    if (prepend_observables) {
        xor_measurement_sets_into_result(observables, true, frame_samples, result, offset);
    }
    xor_measurement_sets_into_result(detectors, false, frame_samples, result, offset);
    if (append_observables) {
        xor_measurement_sets_into_result(observables, true, frame_samples, result, offset);
    }

    return result;
}

simd_bit_table detector_samples(
    const Circuit &circuit, size_t num_shots, bool prepend_observables, bool append_observables, std::mt19937_64 &rng) {
    auto ab = circuit.list_detectors_and_observables();
    return detector_samples(circuit, ab.first, ab.second, num_shots, prepend_observables, append_observables, rng);
}

void detector_samples_out(
    const Circuit &circuit, size_t num_shots, bool prepend_observables, bool append_observables, FILE *out,
    SampleFormat format, std::mt19937_64 &rng) {
    auto det_obs = circuit.list_detectors_and_observables();
    const auto &detectors = det_obs.first;
    const auto &observables = det_obs.second;
    size_t num_sample_locations =
        detectors.size() + observables.size() * ((int)prepend_observables + (int)append_observables);

    constexpr size_t GOOD_BLOCK_SIZE = 1024;
    simd_bits reference_sample(num_sample_locations);
    while (num_shots > GOOD_BLOCK_SIZE) {
        auto table = detector_samples(
            circuit, detectors, observables, GOOD_BLOCK_SIZE, prepend_observables, append_observables, rng);
        write_table_data(out, GOOD_BLOCK_SIZE, num_sample_locations, reference_sample, table, format);
        num_shots -= GOOD_BLOCK_SIZE;
    }
    if (num_shots) {
        auto table =
            detector_samples(circuit, detectors, observables, num_shots, prepend_observables, append_observables, rng);
        write_table_data(out, num_shots, num_sample_locations, reference_sample, table, format);
    }
}
