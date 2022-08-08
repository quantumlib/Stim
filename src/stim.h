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

#ifndef _STIM_H
#define _STIM_H

/// WARNING: THE STIM C++ API MAKES NO COMPATIBILITY GUARANTEES.
/// It may change arbitrarily and catastrophically from minor version to minor version.
/// If you need a stable API, use stim's Python API.

#include "stim/arg_parse.h"
#include "stim/circuit/circuit.h"
#include "stim/circuit/gate_data.h"
#include "stim/circuit/gate_target.h"
#include "stim/dem/detector_error_model.h"
#include "stim/gen/circuit_gen_main.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_color_code.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/help.h"
#include "stim/io/measure_record.h"
#include "stim/io/measure_record_batch.h"
#include "stim/io/measure_record_batch_writer.h"
#include "stim/io/measure_record_reader.h"
#include "stim/io/measure_record_writer.h"
#include "stim/io/stim_data_formats.h"
#include "stim/main_namespaced.h"
#include "stim/mem/bit_ref.h"
#include "stim/mem/fixed_cap_vector.h"
#include "stim/mem/monotonic_buffer.h"
#include "stim/mem/pointer_range.h"
#include "stim/mem/simd_bit_table.h"
#include "stim/mem/simd_bits.h"
#include "stim/mem/simd_bits_range_ref.h"
#include "stim/mem/simd_word.h"
#include "stim/mem/simd_util.h"
#include "stim/mem/sparse_xor_vec.h"
#include "stim/probability_util.h"
#include "stim/search/search.h"
#include "stim/simulators/detection_simulator.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/error_matcher.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/matched_error.h"
#include "stim/simulators/measurements_to_detection_events.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/pauli_string_ref.h"
#include "stim/stabilizers/tableau.h"
#include "stim/stabilizers/tableau_transposed_raii.h"
#include "stim/str_util.h"

#endif
