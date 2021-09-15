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

#include "stim/circuit/circuit_gate_target.pybind.h"
#include "stim/circuit/gate_data.h"
#include "stim/circuit/gate_target.h"

void pybind_circuit_instruction(pybind11::module &m);

struct CircuitInstruction {
    const stim::Gate &gate;
    std::vector<stim::GateTarget> targets;
    std::vector<double> gate_args;

    CircuitInstruction(
        const char *name, const std::vector<pybind11::object> &targets, const std::vector<double> &gate_args);
    CircuitInstruction(const stim::Gate &gate, std::vector<stim::GateTarget> targets, std::vector<double> gate_args);

    std::string name() const;
    std::vector<stim::GateTarget> targets_copy() const;
    std::vector<double> gate_args_copy() const;
    std::vector<uint32_t> raw_targets() const;
    bool operator==(const CircuitInstruction &other) const;
    bool operator!=(const CircuitInstruction &other) const;

    std::string repr() const;
    std::string str();
};

#endif
