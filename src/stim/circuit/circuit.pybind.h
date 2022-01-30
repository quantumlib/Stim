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

#ifndef _STIM_CIRCUIT_CIRCUIT_PYBIND_H
#define _STIM_CIRCUIT_CIRCUIT_PYBIND_H

#include <pybind11/pybind11.h>

#include "stim/circuit/circuit.h"

pybind11::class_<stim::Circuit> pybind_circuit(pybind11::module &m);
void pybind_circuit_after_types_all_defined(pybind11::class_<stim::Circuit> &c);
std::set<uint64_t> obj_to_abs_detector_id_set(
    const pybind11::object &obj, const std::function<size_t(void)> &get_num_detectors);
std::string circuit_repr(const stim::Circuit &self);

#endif