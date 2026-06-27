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

#ifndef _STIM_SIMULATORS_DEM_SAMPLER_H
#define _STIM_SIMULATORS_DEM_SAMPLER_H

#include <random>

#include "stim/dem/detector_error_model.h"
#include "stim/io/stim_data_formats.h"
#include "stim/mem/simd_bit_table.h"

namespace stim {

/// Performs high performance bulk sampling of a detector error model.
///
/// The template parameter, W, represents the SIMD width
template <size_t W>
struct DemSampler {
    DetectorErrorModel model;
    uint64_t num_detectors;
    uint64_t num_observables;
    uint64_t num_errors;
    std::mt19937_64 rng;
    // TODO: allow these buffers to be streamed instead of entirely stored in memory.
    simd_bit_table<W> det_buffer;
    simd_bit_table<W> obs_buffer;
    simd_bit_table<W> err_buffer;
    size_t num_stripes;

    /// Compiles a sampler for the given detector error model.
    DemSampler(DetectorErrorModel model, std::mt19937_64 &&rng, size_t min_stripes);

    /// Clears the buffers and refills them with sampled shot data.
    void resample(bool replay_errors);

    /// Ensures the internal buffers are sized for a given number of shots.
    void set_min_stripes(size_t min_stripes);

    /// Samples from the dem, writing results to files.
    ///
    /// Args:
    ///     num_shots: The number of samples to take.
    ///     det_out: Where to write detection event data. Set to nullptr to not write detection event data.
    ///     det_out_format: The format to write detection event data in.
    ///     obs_out: Where to write observable data. Set to nullptr to not write observable data.
    ///     obs_out_format: The format to write observable data in.
    ///     err_out: Where to write recorded error data. Set to nullptr to not write recorded error data.
    ///     err_out_format: The format to write error data in.
    ///     replay_err_in: If this argument is given a non-null file, error data will be read from that file
    ///         and replayed (instead of generating new errors randomly).
    ///     replay_err_in_format: The format to read recorded error data to replay in.
    void sample_write(
        size_t num_shots,
        FILE *det_out,
        SampleFormat det_out_format,
        FILE *obs_out,
        SampleFormat obs_out_format,
        FILE *err_out,
        SampleFormat err_out_format,
        FILE *replay_err_in,
        SampleFormat replay_err_in_format);
};

}  // namespace stim

#include "stim/simulators/dem_sampler.inl"

#endif
