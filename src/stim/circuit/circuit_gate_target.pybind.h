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

#ifndef _STIM_CIRCUIT_CIRCUIT_GATE_TARGET_PYBIND_H
#define _STIM_CIRCUIT_CIRCUIT_GATE_TARGET_PYBIND_H

#include <cstdint>
#include <pybind11/pybind11.h>
#include <string>

#include "stim/circuit/gate_target.h"

namespace stim_pybind {

pybind11::class_<stim::GateTarget> pybind_circuit_gate_target(pybind11::module &m);
void pybind_circuit_gate_target_methods(pybind11::module &m, pybind11::class_<stim::GateTarget> &c);

}

stim::GateTarget obj_to_gate_target(const pybind11::object &obj);
stim::GateTarget handle_to_gate_target(const pybind11::handle &obj);

#endif
