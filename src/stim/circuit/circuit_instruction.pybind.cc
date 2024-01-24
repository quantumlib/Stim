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

#include "stim/circuit/circuit_instruction.pybind.h"

#include "stim/circuit/gate_target.pybind.h"
#include "stim/gates/gates.h"
#include "stim/py/base.pybind.h"
#include "stim/str_util.h"

using namespace stim;
using namespace stim_pybind;

PyCircuitInstruction::PyCircuitInstruction(
    const char *name, const std::vector<pybind11::object> &init_targets, const std::vector<double> &gate_args)
    : gate_type(GATE_DATA.at(name).id), gate_args(gate_args) {
    for (const auto &obj : init_targets) {
        targets.push_back(obj_to_gate_target(obj));
    }
}
PyCircuitInstruction::PyCircuitInstruction(
    GateType gate_type, std::vector<GateTarget> targets, std::vector<double> gate_args)
    : gate_type(gate_type), targets(targets), gate_args(gate_args) {
}

bool PyCircuitInstruction::operator==(const PyCircuitInstruction &other) const {
    return gate_type == other.gate_type && targets == other.targets && gate_args == other.gate_args;
}
bool PyCircuitInstruction::operator!=(const PyCircuitInstruction &other) const {
    return !(*this == other);
}

std::string PyCircuitInstruction::repr() const {
    std::stringstream result;
    result << "stim.CircuitInstruction('" << name() << "', [";
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

std::string PyCircuitInstruction::str() const {
    std::stringstream result;
    result << as_operation_ref();
    return result.str();
}

CircuitInstruction PyCircuitInstruction::as_operation_ref() const {
    return CircuitInstruction{
        gate_type,
        gate_args,
        targets,
    };
}
PyCircuitInstruction::operator CircuitInstruction() const {
    return as_operation_ref();
}
std::string PyCircuitInstruction::name() const {
    return GATE_DATA[gate_type].name;
}
std::vector<uint32_t> PyCircuitInstruction::raw_targets() const {
    std::vector<uint32_t> result;
    for (const auto &t : targets) {
        result.push_back(t.data);
    }
    return result;
}

std::vector<GateTarget> PyCircuitInstruction::targets_copy() const {
    return targets;
}
std::vector<double> PyCircuitInstruction::gate_args_copy() const {
    return gate_args;
}

pybind11::class_<PyCircuitInstruction> stim_pybind::pybind_circuit_instruction(pybind11::module &m) {
    return pybind11::class_<PyCircuitInstruction>(
        m,
        "CircuitInstruction",
        clean_doc_string(R"DOC(
            An instruction, like `H 0 1` or `CNOT rec[-1] 5`, from a circuit.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     H 0
                ...     M 0 1
                ...     X_ERROR(0.125) 5
                ... ''')
                >>> circuit[0]
                stim.CircuitInstruction('H', [stim.GateTarget(0)], [])
                >>> circuit[1]
                stim.CircuitInstruction('M', [stim.GateTarget(0), stim.GateTarget(1)], [])
                >>> circuit[2]
                stim.CircuitInstruction('X_ERROR', [stim.GateTarget(5)], [0.125])
        )DOC")
            .data());
}
void stim_pybind::pybind_circuit_instruction_methods(pybind11::module &m, pybind11::class_<PyCircuitInstruction> &c) {
    c.def(
        pybind11::init<const char *, std::vector<pybind11::object>, std::vector<double>>(),
        pybind11::arg("name"),
        pybind11::arg("targets"),
        pybind11::arg("gate_args") = std::make_tuple(),
        clean_doc_string(R"DOC(
            Initializes a `stim.CircuitInstruction`.

            Args:
                name: The name of the instruction being applied.
                targets: The targets the instruction is being applied to. These can be raw
                    values like `0` and `stim.target_rec(-1)`, or instances of
                    `stim.GateTarget`.
                gate_args: The sequence of numeric arguments parameterizing a gate. For
                    noise gates this is their probabilities. For `OBSERVABLE_INCLUDE`
                    instructions it's the index of the logical observable to affect.
        )DOC")
            .data());

    c.def_property_readonly(
        "name",
        &PyCircuitInstruction::name,
        clean_doc_string(R"DOC(
            The name of the instruction (e.g. `H` or `X_ERROR` or `DETECTOR`).
        )DOC")
            .data());

    c.def(
        "targets_copy",
        &PyCircuitInstruction::targets_copy,
        clean_doc_string(R"DOC(
            Returns a copy of the targets of the instruction.
        )DOC")
            .data());

    c.def(
        "gate_args_copy",
        &PyCircuitInstruction::gate_args_copy,
        clean_doc_string(R"DOC(
            Returns the gate's arguments (numbers parameterizing the instruction).

            For noisy gates this typically a list of probabilities.
            For OBSERVABLE_INCLUDE it's a singleton list containing the logical observable
            index.
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two `stim.CircuitInstruction`s are identical.");
    c.def(pybind11::self != pybind11::self, "Determines if two `stim.CircuitInstruction`s are different.");
    c.def(
        "__repr__",
        &PyCircuitInstruction::repr,
        "Returns text that is a valid python expression evaluating to an equivalent `stim.CircuitInstruction`.");
    c.def(
        "__str__",
        &PyCircuitInstruction::str,
        "Returns a text description of the instruction as a stim circuit file line.");

    c.def("__hash__", [](const PyCircuitInstruction &self) {
        return pybind11::hash(pybind11::str(self.str()));
    });
}
