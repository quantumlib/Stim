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
struct DemSampler {
    DetectorErrorModel model;
    uint64_t num_detectors;
    uint64_t num_observables;
    std::mt19937_64 rng;
    simd_bit_table det_buffer;
    simd_bit_table obs_buffer;
    uint64_t num_stripes;

    /// Compiles a sampler for the given detector error model.
    DemSampler(DetectorErrorModel model, std::mt19937_64 rng, size_t min_stripes);

    /// Clears the buffers and refills them with sampled shot data.
    void resample();

    /// Samples from the dem, writing results to files.
    void sample_write(
        size_t num_shots, FILE *det_out, SampleFormat det_out_format, FILE *obs_out, SampleFormat obs_out_format);
};

}  // namespace stim

#endif
