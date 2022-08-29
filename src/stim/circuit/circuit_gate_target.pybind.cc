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

#include "stim/circuit/circuit_gate_target.pybind.h"

#include "stim/circuit/circuit.h"
#include "stim/py/base.pybind.h"

using namespace stim;
using namespace stim_pybind;

GateTarget handle_to_gate_target(const pybind11::handle &obj) {
    try {
        return pybind11::cast<GateTarget>(obj);
    } catch (const pybind11::cast_error &ex) {
    }
    try {
        return GateTarget{pybind11::cast<uint32_t>(obj)};
    } catch (const pybind11::cast_error &ex) {
    }
    throw std::invalid_argument(
        "target argument wasn't a qubit index, a result from a `stim.target_*` method, or a `stim.GateTarget`.");
}

GateTarget obj_to_gate_target(const pybind11::object &obj) {
    return handle_to_gate_target(obj);
}

pybind11::class_<stim::GateTarget> stim_pybind::pybind_circuit_gate_target(pybind11::module &m) {
    return pybind11::class_<GateTarget>(
        m,
        "GateTarget",
        clean_doc_string(u8R"DOC(
            Represents a gate target, like `0` or `rec[-1]`, from a circuit.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     M 0 !1
                ... ''')
                >>> circuit[0].targets_copy()[0]
                stim.GateTarget(0)
                >>> circuit[0].targets_copy()[1]
                stim.GateTarget(stim.target_inv(1))
        )DOC")
            .data());
}

void stim_pybind::pybind_circuit_gate_target_methods(pybind11::module &m, pybind11::class_<stim::GateTarget> &c) {
    c.def(
        pybind11::init(&obj_to_gate_target),
        pybind11::arg("value"),
        clean_doc_string(u8R"DOC(
            Initializes a `stim.GateTarget`.

            Args:
                value: A target like `5` or `stim.target_rec(-1)`.
        )DOC")
            .data());

    c.def_property_readonly(
        "value",
        &GateTarget::value,
        clean_doc_string(u8R"DOC(
            The numeric part of the target.

            This is non-negative integer for qubit targets, and a negative integer for
            measurement record targets.
        )DOC")
            .data());

    c.def_property_readonly(
        "is_qubit_target",
        &GateTarget::is_qubit_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a (possibly inverted) qubit target.

            For example, `5` on its own in a stim circuit is a raw qubit target.
        )DOC")
            .data());

    c.def_property_readonly(
        "is_x_target",
        &GateTarget::is_x_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a (possibly inverted) X pauli target.

            For example, `X5` in a stim circuit is a X Pauli target. The value returned by
            `stim.target_x` is a X pauli target.
        )DOC")
            .data());

    c.def_property_readonly(
        "is_y_target",
        &GateTarget::is_y_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a (possibly inverted) Y pauli target.

            For example, `Y5` in a stim circuit is a Y Pauli target. The value returned by
            `stim.target_y` is a Y pauli target.
        )DOC")
            .data());

    c.def_property_readonly(
        "is_z_target",
        &GateTarget::is_z_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a (possibly inverted) Z pauli target.

            For example, `Z5` in a stim circuit is a Z Pauli target. The value returned by
            `stim.target_z` is a Z pauli target.
        )DOC")
            .data());

    c.def_property_readonly(
        "is_inverted_result_target",
        &GateTarget::is_inverted_result_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is an inverted target.

            Inverted targets include inverted qubit targets `stim.target_inv(5)`
            (`!5` in a circuit file) and inverted Pauli targets like
            `stim.target_x(4, invert=True)` (`!X4` in a circuit file).
        )DOC")
            .data());

    c.def_property_readonly(
        "is_measurement_record_target",
        &GateTarget::is_measurement_record_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a measurement record target.

            The value returned by `stim.target_rec(-5)`, which would be `rec[-5]` in a
            circuit file, is a measurement record target.
        )DOC")
            .data());

    c.def_property_readonly(
        "is_combiner",
        &GateTarget::is_combiner,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a `stim.target_combiner()`.
        )DOC")
            .data());

    c.def_property_readonly(
        "is_sweep_bit_target",
        &GateTarget::is_sweep_bit_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a sweep bit target.

            For example, `sweep[5]` in a circuit file is a sweep target, and the value
            returned by `stim.target_sweep_bit` is a sweep target.
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two `stim.GateTarget`s are identical.");
    c.def(pybind11::self != pybind11::self, "Determines if two `stim.GateTarget`s are different.");
    c.def("__hash__", [](const GateTarget &self) {
        return pybind11::hash(pybind11::make_tuple("GateTarget", self.data));
    });
    c.def(
        "__repr__",
        &GateTarget::repr,
        "Returns text that is a valid python expression evaluating to an equivalent `stim.GateTarget`.");
}
