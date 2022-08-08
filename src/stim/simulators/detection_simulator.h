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

#ifndef _STIM_SIMULATORS_DETECTION_SIMULATOR_H
#define _STIM_SIMULATORS_DETECTION_SIMULATOR_H

#include <random>

#include "stim/circuit/circuit.h"
#include "stim/io/stim_data_formats.h"
#include "stim/mem/simd_bit_table.h"

namespace stim {

/// Samples detection events from the circuit and returns them in a simd_bit_table.
///
/// Args:
///     circuit: The circuit to sample.
///     num_shots: The number of samples to take.
///     prepend_observables: Include the observables in the output, before the detectors.
///     append_observables: Include the observables in the output, after the detectors.
///     rng: Random number generator to use.
///
/// Returns:
///     A simd_bit_table with detector/observable index as the major index and shot index as the minor index.
simd_bit_table<MAX_BITWORD_WIDTH> detector_samples(
    const Circuit &circuit, size_t num_shots, bool prepend_observables, bool append_observables, std::mt19937_64 &rng);

/// Samples detection events from the circuit and returns them in a simd_bit_table.
///
/// This is a specialization of the method that takes pre-analyzed detector and observable data, so that it does not
/// need to be recomputed repeatedly when taking multiple batches of shots.
///
/// Args:
///     circuit: The circuit to sample.
///     det_obs: Pre-analyzed detector and observable data for the circuit.
///     num_shots: The number of samples to take.
///     prepend_observables: Include the observables in the output, before the detectors.
///     append_observables: Include the observables in the output, after the detectors.
///     rng: Random number generator to use.
///
/// Returns:
///     A simd_bit_table with detector/observable index as the major index and shot index as the minor index.
simd_bit_table<MAX_BITWORD_WIDTH> detector_samples(
    const Circuit &circuit,
    const DetectorsAndObservables &det_obs,
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    std::mt19937_64 &rng);

/// Samples detection events from the circuit and writes them to a file.
///
/// Args:
///     circuit: The circuit to sample.
///     num_shots: The number of samples to take.
///     prepend_observables: Include the observables in the output, before the detectors.
///     append_observables: Include the observables in the output, after the detectors.
///     out: The file to write the result data to.
///     format: The format to use when encoding the data into the file.
///     rng: Random number generator to use.
///
/// Returns:
///     A simd_bit_table with detector/observable index as the major index and shot index as the minor index.
void detector_samples_out(
    const Circuit &circuit,
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    FILE *out,
    SampleFormat format,
    std::mt19937_64 &rng,
    FILE *obs_out,
    SampleFormat obs_out_format);

}  // namespace stim

#endif
