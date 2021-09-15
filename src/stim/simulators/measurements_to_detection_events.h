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

#ifndef _STIM_SIMULATORS_MEASUREMENTS_TO_DETECTION_EVENTS_H
#define _STIM_SIMULATORS_MEASUREMENTS_TO_DETECTION_EVENTS_H

#include <cassert>
#include <functional>
#include <iostream>
#include <new>
#include <random>
#include <sstream>

#include "stim/circuit/circuit.h"
#include "stim/io/measure_record.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/stabilizers/tableau.h"
#include "stim/stabilizers/tableau_transposed_raii.h"

namespace stim {

/// Reads measurement data from a file, converts it to detection event data, and writes that out to another file.
///
/// Args:
///     measurements_in: The file to read measurement data from.
///     input_format: The format of the measurement data in the file.
///     results_out: The file to write detection event data to.
///     output_format: The format to use when writing the detection event data.
///     circuit: The circuit that the measurement data corresponds to, with DETECTOR and OBSERVABLE_INCLUDE annotations
///         indicating how to perform the conversion.
///     append_observables: Whether or not to include observable flip data in the detection event data.
void measurements_to_detection_events(
    FILE *measurements_in,
    SampleFormat input_format,
    FILE *results_out,
    SampleFormat output_format,
    const Circuit &circuit,
    bool append_observables,
    bool skip_reference_sample);

/// Converts measurement data into detection event data based on a circuit.
///
/// Args:
///     measurement_results_minor_shots: Measurement data. Major axis is measurement index. Minor axis is shot index.
///     circuit: The circuit that the measurement data corresponds to, with DETECTOR and OBSERVABLE_INCLUDE annotations
///         indicating how to perform the conversion.
///     append_observables: Whether or not to include observable flip data in the detection event data.
///
/// Returns:
///     Detection event data. Major axis is detector index (+ observable index). Minor axis is shot index.
simd_bit_table measurements_to_detection_events(
    const simd_bit_table &measurement_results_minor_shots,
    const Circuit &circuit,
    bool append_observables,
    bool skip_reference_sample);

}  // namespace stim

#endif
