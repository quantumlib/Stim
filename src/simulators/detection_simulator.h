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

#ifndef DETECTION_SIMULATOR_H
#define DETECTION_SIMULATOR_H

#include <random>

#include "../circuit/circuit.h"
#include "../simd/simd_bit_table.h"

namespace stim_internal {

simd_bit_table detector_samples(
    const Circuit &circuit,
    const DetectorsAndObservables &det_obs,
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    std::mt19937_64 &rng);

simd_bit_table detector_samples(
    const Circuit &circuit, size_t num_shots, bool prepend_observables, bool append_observables, std::mt19937_64 &rng);

void detector_samples_out(
    const Circuit &circuit,
    size_t num_shots,
    bool prepend_observables,
    bool append_observables,
    FILE *out,
    SampleFormat format,
    std::mt19937_64 &rng);

}  // namespace stim_internal

#endif
