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

#include <algorithm>

#include "stim/io/measure_record_reader.h"
#include "stim/io/measure_record_writer.h"
#include "stim/probability_util.h"

using namespace stim;

DemSampler::DemSampler(DetectorErrorModel init_model, std::mt19937_64 rng, size_t min_stripes)
    : model(std::move(init_model)),
      num_detectors(model.count_detectors()),
      num_observables(model.count_observables()),
      num_errors(model.count_errors()),
      rng(rng),
      det_buffer((size_t)num_detectors, min_stripes),
      obs_buffer((size_t)num_observables, min_stripes),
      err_buffer((size_t)num_errors, min_stripes),
      num_stripes(det_buffer.num_minor_bits_padded()) {
}

void DemSampler::resample(bool replay_errors) {
    det_buffer.clear();
    obs_buffer.clear();
    if (!replay_errors) {
        err_buffer.clear();
    }
    size_t error_index = 0;
    model.iter_flatten_error_instructions([&](const DemInstruction &op) {
        simd_bits_range_ref err_row = err_buffer[error_index];
        if (!replay_errors) {
            biased_randomize_bits((float)op.arg_data[0], err_row.u64, err_row.u64 + err_row.num_u64_padded(), rng);
        }
        for (const auto &t : op.target_data) {
            if (t.is_relative_detector_id()) {
                det_buffer[(size_t)t.raw_id()] ^= err_row;
            } else if (t.is_observable_id()) {
                obs_buffer[(size_t)t.raw_id()] ^= err_row;
            }
        }
        error_index++;
    });
}

void DemSampler::sample_write(
    size_t num_shots,
    FILE *det_out,
    SampleFormat det_out_format,
    FILE *obs_out,
    SampleFormat obs_out_format,
    FILE *err_out,
    SampleFormat err_out_format,
    FILE *err_in,
    SampleFormat err_in_format) {
    for (size_t k = 0; k < num_shots; k += num_stripes) {
        size_t shots_left = std::min(num_stripes, num_shots - k);

        if (err_in != nullptr) {
            size_t errors_read = read_file_data_into_shot_table(
                err_in, shots_left, (size_t)num_errors, err_in_format, 'M', err_buffer, false);
            if (errors_read != shots_left) {
                throw std::invalid_argument("Expected more error data for the requested number of shots.");
            }
        }
        resample(err_in != nullptr);

        if (err_out != nullptr) {
            write_table_data(
                err_out, shots_left, (size_t)num_errors, simd_bits(0), err_buffer, err_out_format, 'M', 'M', false);
        }

        if (obs_out != nullptr) {
            write_table_data(
                obs_out,
                shots_left,
                (size_t)num_observables,
                simd_bits(0),
                obs_buffer,
                obs_out_format,
                'L',
                'L',
                false);
        }

        if (det_out != nullptr) {
            write_table_data(
                det_out, shots_left, (size_t)num_detectors, simd_bits(0), det_buffer, det_out_format, 'D', 'D', false);
        }
    }
}
