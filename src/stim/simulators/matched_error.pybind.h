// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _STIM_SIMULATORS_MATCHED_ERROR_PYBIND_H
#define _STIM_SIMULATORS_MATCHED_ERROR_PYBIND_H

#include <pybind11/pybind11.h>

#include "stim/simulators/matched_error.h"

namespace stim_pybind {

pybind11::class_<stim::CircuitErrorLocationStackFrame> pybind_circuit_error_location_stack_frame(pybind11::module &m);
pybind11::class_<stim::GateTargetWithCoords> pybind_gate_target_with_coords(pybind11::module &m);
pybind11::class_<stim::DemTargetWithCoords> pybind_dem_target_with_coords(pybind11::module &m);
pybind11::class_<stim::FlippedMeasurement> pybind_flipped_measurement(pybind11::module &m);
pybind11::class_<stim::CircuitTargetsInsideInstruction> pybind_circuit_targets_inside_instruction(pybind11::module &m);
pybind11::class_<stim::CircuitErrorLocation> pybind_circuit_error_location(pybind11::module &m);
pybind11::class_<stim::ExplainedError> pybind_explained_error(pybind11::module &m);

void pybind_circuit_error_location_stack_frame_methods(
    pybind11::module &m, pybind11::class_<stim::CircuitErrorLocationStackFrame> &c);

void pybind_gate_target_with_coords_methods(pybind11::module &m, pybind11::class_<stim::GateTargetWithCoords> &c);

void pybind_dem_target_with_coords_methods(pybind11::module &m, pybind11::class_<stim::DemTargetWithCoords> &c);
void pybind_flipped_measurement_methods(pybind11::module &m, pybind11::class_<stim::FlippedMeasurement> &c);
void pybind_circuit_targets_inside_instruction_methods(
    pybind11::module &m, pybind11::class_<stim::CircuitTargetsInsideInstruction> &c);

void pybind_circuit_error_location_methods(pybind11::module &m, pybind11::class_<stim::CircuitErrorLocation> &c);
void pybind_explained_error_methods(pybind11::module &m, pybind11::class_<stim::ExplainedError> &c);

}  // namespace stim_pybind

#endif
