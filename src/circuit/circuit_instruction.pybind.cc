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

#include "circuit_instruction.pybind.h"

#include "../py/base.pybind.h"
#include "circuit_gate_target.pybind.h"
#include "gate_data.h"

using namespace stim_internal;

CircuitInstruction::CircuitInstruction(const char *name, std::vector<GateTarget> targets, std::vector<double> gate_args)
    : gate(GATE_DATA.at(name)), targets(targets), gate_args(gate_args) {
}
CircuitInstruction::CircuitInstruction(const Gate &gate, std::vector<GateTarget> targets, std::vector<double> gate_args)
    : gate(gate), targets(targets), gate_args(gate_args) {
}

bool CircuitInstruction::operator==(const CircuitInstruction &other) const {
    return gate.id == other.gate.id && targets == other.targets && gate_args == other.gate_args;
}
bool CircuitInstruction::operator!=(const CircuitInstruction &other) const {
    return !(*this == other);
}

std::string CircuitInstruction::repr() const {
    std::stringstream result;
    result << "stim.CircuitInstruction('" << gate.name << "', [";
    bool first = true;
    for (const auto &t : targets) {
        if (first) {
            first = false;
        } else {
            result << ", ";
        }
        result << t.repr();
    }
    result << "], [" << comma_sep(gate_args) << "])";
    return result.str();
}
std::string CircuitInstruction::name() const {
    return gate.name;
}
std::vector<GateTarget> CircuitInstruction::targets_copy() const {
    return targets;
}
std::vector<double> CircuitInstruction::gate_args_copy() const {
    return gate_args;
}

void pybind_circuit_instruction(pybind11::module &m) {
    auto &&c = pybind11::class_<CircuitInstruction>(
        m,
        "CircuitInstruction",
        clean_doc_string(u8R"DOC(
            An instruction, like `H 0 1` or `CNOT rec[-1] 5`, from a circuit.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     H 0
                ...     M 0 !1
                ...     X_ERROR(0.125) 5 3
                ... ''')
                >>> circuit[0]
                stim.CircuitInstruction('H', [stim.GateTarget(0)], [])
                >>> circuit[1]
                stim.CircuitInstruction('M', [stim.GateTarget(0), stim.GateTarget(stim.target_inv(1))], [])
                >>> circuit[2]
                stim.CircuitInstruction('X_ERROR', [stim.GateTarget(5), stim.GateTarget(3)], [0.125])
        )DOC")
            .data());

    c.def(
        pybind11::init<const char *, std::vector<GateTarget>, std::vector<double>>(),
        pybind11::arg("name"),
        pybind11::arg("targets"),
        pybind11::arg("gate_args") = std::make_tuple(),
        clean_doc_string(u8R"DOC(
            Initializes a `stim.CircuitInstruction`.

            Args:
                name: The name of the instruction being applied.
                targets: The targets the instruction is being applied to. These can be raw values like `0` and
                    `stim.target_rec(-1)`, or instances of `stim.GateTarget`.
                gate_args: The sequence of numeric arguments parameterizing a gate. For noise gates this is their
                    probabilities. For OBSERVABLE_INCLUDE it's the logical observable's index.
        )DOC")
            .data());

    c.def_property_readonly(
        "name",
        &CircuitInstruction::name,
        clean_doc_string(u8R"DOC(
            The name of the instruction (e.g. `H` or `X_ERROR` or `DETECTOR`).
        )DOC")
            .data());

    c.def(
        "targets_copy",
        &CircuitInstruction::targets_copy,
        clean_doc_string(u8R"DOC(
            Returns a copy of the targets of the instruction.
        )DOC")
            .data());

    c.def(
        "gate_args_copy",
        &CircuitInstruction::gate_args_copy,
        clean_doc_string(u8R"DOC(
            Returns the gate's arguments (numbers parameterizing the instruction).

            For noisy gates this typically a list of probabilities.
            For OBSERVABLE_INCLUDE it's a singleton list containing the logical observable index.
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two `stim.CircuitInstruction`s are identical.");
    c.def(pybind11::self != pybind11::self, "Determines if two `stim.CircuitInstruction`s are different.");
    c.def(
        "__repr__",
        &CircuitInstruction::repr,
        "Returns text that is a valid python expression evaluating to an equivalent `stim.CircuitInstruction`.");
}
