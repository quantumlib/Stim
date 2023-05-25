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

#ifndef _STIM_CIRCUIT_CIRCUIT_INSTRUCTION_PYBIND_H
#define _STIM_CIRCUIT_CIRCUIT_INSTRUCTION_PYBIND_H

#include <pybind11/pybind11.h>

#include "stim/circuit/circuit_instruction.h"
#include "stim/circuit/gate_data.h"
#include "stim/circuit/gate_target.h"

namespace stim_pybind {

struct PyCircuitInstruction {
    stim::GateType gate_type;
    std::vector<stim::GateTarget> targets;
    std::vector<double> gate_args;

    PyCircuitInstruction(
        const char *name, const std::vector<pybind11::object> &targets, const std::vector<double> &gate_args);
    PyCircuitInstruction(
        stim::GateType gate_type, std::vector<stim::GateTarget> targets, std::vector<double> gate_args);

    stim::CircuitInstruction as_operation_ref() const;
    operator stim::CircuitInstruction() const;
    std::string name() const;
    std::vector<stim::GateTarget> targets_copy() const;
    std::vector<double> gate_args_copy() const;
    std::vector<uint32_t> raw_targets() const;
    bool operator==(const PyCircuitInstruction &other) const;
    bool operator!=(const PyCircuitInstruction &other) const;

    std::string repr() const;
    std::string str() const;
};

pybind11::class_<PyCircuitInstruction> pybind_circuit_instruction(pybind11::module &m);
void pybind_circuit_instruction_methods(pybind11::module &m, pybind11::class_<PyCircuitInstruction> &c);

}  // namespace stim_pybind

#endif
