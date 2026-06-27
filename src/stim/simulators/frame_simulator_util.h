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

/// A convenience method for batch sampling detection events from a circuit.
///
/// Uses the frame simulator.
///
/// This method is intentionally inefficient to make it easier to use. In particular, it
/// assumes the circuit is small enough that it's reasonable to simply store the
/// detection event data of all samples in memory simultaneously. It also assumes that it's
/// acceptable to recreate a fresh FrameSimulator with all its various buffers each time the
/// method is called.
///
/// Args:
///     circuit: The circuit to sample.
///     num_shots: The number of samples to take.
///     rng: Random number generator to use.
///
/// Returns:
///     A pair of simd_bit_tables. The first is the detection event data. The second is the
///     observable data. Each table is arranged as follows:
///         major axis (first index): detector index (or observable index)
///         minor axis (second index): shot index
template <size_t W>
std::pair<simd_bit_table<W>, simd_bit_table<W>> sample_batch_detection_events(
    const Circuit &circuit, size_t num_shots, std::mt19937_64 &rng);

/// Samples detection events from a circuit and writes them to a file.
///
/// Uses the frame simulator.
///
/// Args:
///     circuit: The circuit to sample.
///     num_shots: The number of samples to take.
///     prepend_observables: Include the observables in the output, before the detectors.
///     append_observables: Include the observables in the output, after the detectors.
///     out: The file to write the result data to.
///     format: The format to use when encoding the data into the file.
///     rng: Random number generator to use.
///     obs_out: An optional secondary file to write observable data to. Set to nullptr to
///         not use.
///     obs_out_format: The format to use when writing to the secondary file.
template <size_t W>
void sample_batch_detection_events_writing_results_to_disk(
    const Circuit &circuit,
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    FILE *out,
    SampleFormat format,
    std::mt19937_64 &rng,
    FILE *obs_out,
    SampleFormat obs_out_format);

/// A convenience method for batch sampling measurements from a circuit.
///
/// Uses the frame simulator.
///
/// This method is intentionally inefficient to make it easier to use. In particular, it
/// assumes the circuit is small enough that it's reasonable to simply store the
/// measurement of all samples in memory simultaneously. It also assumes that it's
/// acceptable to recreate a fresh FrameSimulator with all its various buffers each time the
/// method is called.
///
/// Args:
///     circuit: The circuit to sample.
///     num_shots: The number of samples to take.
///     rng: Random number generator to use.
///     transposed: Whether or not to exchange the axes of the resulting table.
///
/// Returns:
///     A simd_bit_table containing the sampled measurement data.
///     If not transposed:
///         major axis (first index): measurement index
///         minor axis (second index): shot index
///     If transposed:
///         major axis (first index): shot index
///         minor axis (second index): measurement index
template <size_t W>
simd_bit_table<W> sample_batch_measurements(
    const Circuit &circuit,
    const simd_bits<W> &reference_sample,
    size_t num_samples,
    std::mt19937_64 &rng,
    bool transposed);

/// Samples measurements from a circuit and writes them to a file.
///
/// Uses the frame simulator.
///
/// Args:
///     circuit: The circuit to sample.
///     num_shots: The number of samples to take.
///     reference_sample: A noiseless sample from the circuit, acquired via other means
///         (for example, via TableauSimulator::reference_sample_circuit).
///     out: The file to write the result data to.
///     format: The format to use when encoding the data into the file.
///     rng: Random number generator to use.
template <size_t W>
void sample_batch_measurements_writing_results_to_disk(
    const Circuit &circuit,
    const simd_bits<W> &reference_sample,
    uint64_t num_shots,
    FILE *out,
    SampleFormat format,
    std::mt19937_64 &rng);

}  // namespace stim

#include "stim/simulators/frame_simulator_util.inl"

#endif
