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

#ifndef CIRCUIT_INSTRUCTION_PYBIND_H
#define CIRCUIT_INSTRUCTION_PYBIND_H

#include <pybind11/pybind11.h>
#include "circuit_gate_target.pybind.h"
#include "gate_data.h"

void pybind_circuit_instruction(pybind11::module &m);

struct CircuitInstruction {
    const stim_internal::Gate &gate;
    std::vector<GateTarget> targets;
    double gate_arg;

    CircuitInstruction(const char *name, std::vector<GateTarget> targets, double gate_arg);
    CircuitInstruction(const stim_internal::Gate &gate, std::vector<GateTarget> targets, double gate_arg);

    std::string name() const;
    std::vector<GateTarget> targets_copy() const;
    bool operator==(const CircuitInstruction &other) const;
    bool operator!=(const CircuitInstruction &other) const;

    std::string repr() const;
};

#endif
