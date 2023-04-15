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
#include "stim/simulators/frame_simulator.h"

namespace stim {

/// A convenience method for sampling detection events from the circuit.
///
/// This method assumes the circuit is small enough that it's reasonable to simply store the
/// measurements and detection event data of all samples in memory simultaneously. It also
/// assumes that it's' acceptable to create a fresh FrameSimulator with all its various
/// buffers. To stream data, or to re-use a FrameSimulator, drive a FrameSimulator directly
/// instead of using this method.
///
/// Args:
///     circuit: The circuit to sample.
///     circuit_det_stats: A precomputed value saying how many detectors and observables are
///         in the circuit. The method could compute this on its own, but you need to know
///         these numbers in order to understand the result so forcing it to be passed in
///         avoids duplicate work.
///     num_shots: The number of samples to take.
///     rng: Random number generator to use.
///
/// Returns:
///     A pair of simd_bit_tables. The first is the detection event data. The second is the
///     observable data.
std::pair<simd_bit_table<MAX_BITWORD_WIDTH>, simd_bit_table<MAX_BITWORD_WIDTH>> sample_detection_events_simple(
    const Circuit &circuit,
    CircuitDetectorStats circuit_det_stats,
    size_t num_shots,
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
