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

#ifndef STIM_H
#define STIM_H

#include "../arg_parse.h"
#include "../benchmark_util.h"
#include "../circuit/circuit.h"
#include "../circuit/gate_data.h"
#include "../circuit/gate_target.h"
#include "../dem/detector_error_model.h"
#include "../gen/circuit_gen_main.h"
#include "../gen/circuit_gen_params.h"
#include "../gen/gen_color_code.h"
#include "../gen/gen_rep_code.h"
#include "../gen/gen_surface_code.h"
#include "../help.h"
#include "../io/measure_record.h"
#include "../io/measure_record_batch.h"
#include "../io/measure_record_batch_writer.h"
#include "../io/measure_record_reader.h"
#include "../io/measure_record_writer.h"
#include "../io/stim_data_formats.h"
#include "../main_namespaced.h"
#include "../probability_util.h"
#include "../simd/bit_ref.h"
#include "../simd/fixed_cap_vector.h"
#include "../simd/monotonic_buffer.h"
#include "../simd/pointer_range.h"
#include "../simd/simd_bit_table.h"
#include "../simd/simd_bits.h"
#include "../simd/simd_bits_range_ref.h"
#include "../simd/simd_compat.h"
#include "../simd/simd_util.h"
#include "../simd/sparse_xor_vec.h"
#include "../simulators/detection_simulator.h"
#include "../simulators/error_analyzer.h"
#include "../simulators/frame_simulator.h"
#include "../simulators/tableau_simulator.h"
#include "../simulators/vector_simulator.h"
#include "../stabilizers/pauli_string.h"
#include "../stabilizers/pauli_string_ref.h"
#include "../stabilizers/tableau.h"
#include "../stabilizers/tableau_transposed_raii.h"
#include "../str_util.h"

namespace stim {
using Circuit = stim_internal::Circuit;
using ErrorAnalyzer = stim_internal::ErrorAnalyzer;
using FrameSimulator = stim_internal::FrameSimulator;
using PauliString = stim_internal::PauliString;
using Tableau = stim_internal::Tableau;
using TableauSimulator = stim_internal::TableauSimulator;
using DetectorErrorModel = stim_internal::DetectorErrorModel;
extern const stim_internal::GateDataMap &GATE_DATA;
}  // namespace stim

#endif
