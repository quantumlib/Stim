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

void compute_observables_into_offset(
    const Circuit &circuit, simd_bit_table &frame_samples, simd_bit_table &combined_samples, size_t &cs) {
    for (const auto &obs : circuit.observables) {
        simd_bits_range_ref dst = combined_samples[cs++];
        if (obs.expected_parity) {
            dst.invert_bits();
        }
        for (auto i : obs.indices) {
            dst ^= frame_samples[i];
        }
    }
}

simd_bit_table detector_samples(
    const Circuit &circuit, size_t num_shots, bool prepend_observables, bool append_observables, std::mt19937_64 &rng) {
    auto num_detectors = circuit.detectors.size();
    auto num_obs = circuit.observables.size();
    size_t num_sample_locations = num_detectors + num_obs * ((int)prepend_observables + (int)append_observables);
    simd_bit_table combined_samples(num_sample_locations, num_shots);
    size_t cs = 0;
    auto frame_samples = FrameSimulator::sample_flipped_measurements(circuit, num_shots, rng);

    if (prepend_observables) {
        compute_observables_into_offset(circuit, frame_samples, combined_samples, cs);
    }

    // Compute detector values.
    for (const auto &det : circuit.detectors) {
        simd_bits_range_ref dst = combined_samples[cs++];
        for (auto i : det.indices) {
            dst ^= frame_samples[i];
        }
    }

    if (append_observables) {
        compute_observables_into_offset(circuit, frame_samples, combined_samples, cs);
    }

    return combined_samples;
}

void detector_samples_out(
    const Circuit &circuit, size_t num_shots, bool prepend_observables, bool append_observables, FILE *out,
    SampleFormat format, std::mt19937_64 &rng) {
    size_t num_sample_locations =
        circuit.detectors.size() + circuit.observables.size() * ((int)prepend_observables + (int)append_observables);

    constexpr size_t GOOD_BLOCK_SIZE = 1024;
    simd_bits reference_sample(num_sample_locations);
    while (num_shots > GOOD_BLOCK_SIZE) {
        auto table = detector_samples(circuit, GOOD_BLOCK_SIZE, prepend_observables, append_observables, rng);
        write_table_data(out, GOOD_BLOCK_SIZE, num_sample_locations, reference_sample, table, format);
        num_shots -= GOOD_BLOCK_SIZE;
    }
    if (num_shots) {
        auto table = detector_samples(circuit, num_shots, prepend_observables, append_observables, rng);
        write_table_data(out, num_shots, num_sample_locations, reference_sample, table, format);
    }
}
