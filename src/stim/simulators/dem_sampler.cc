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

#include "stim/simulators/dem_sampler.h"

#include "stim/io/measure_record_writer.h"
#include "stim/probability_util.h"

using namespace stim;

DemSampler::DemSampler(DetectorErrorModel init_model, std::mt19937_64 rng, size_t min_stripes)
    : model(std::move(init_model)),
      num_detectors(model.count_detectors()),
      num_observables(model.count_observables()),
      rng(rng),
      det_buffer(num_detectors, min_stripes),
      obs_buffer(num_observables, min_stripes),
      num_stripes(det_buffer.num_minor_bits_padded()) {
}

void DemSampler::resample() {
    det_buffer.clear();
    obs_buffer.clear();
    simd_bits rng_buf(num_stripes);
    uint64_t *row_start_64 = rng_buf.u64;
    uint64_t *row_end_64 = row_start_64 + rng_buf.num_u64_padded();
    model.iter_flatten_error_instructions([&](const DemInstruction &op) {
        biased_randomize_bits(op.arg_data[0], row_start_64, row_end_64, rng);
        for (const auto &t : op.target_data) {
            if (t.is_relative_detector_id()) {
                det_buffer[t.raw_id()] ^= rng_buf;
            } else if (t.is_observable_id()) {
                obs_buffer[t.raw_id()] ^= rng_buf;
            }
        }
    });
}

void DemSampler::sample_write(
    size_t num_shots, FILE *det_out, SampleFormat det_out_format, FILE *obs_out, SampleFormat obs_out_format) {
    for (size_t k = 0; k < num_shots; k += num_stripes) {
        resample();
        size_t shots_left = std::min(num_stripes, num_shots - k);

        if (obs_out != nullptr) {
            write_table_data(
                obs_out, shots_left, num_observables, simd_bits(0), obs_buffer, obs_out_format, 'L', 'L', false);
        }

        write_table_data(det_out, shots_left, num_detectors, simd_bits(0), det_buffer, det_out_format, 'D', 'D', false);
    }
}
