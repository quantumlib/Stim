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

#include "stim/circuit/gate_target.pybind.h"

#include "stim/circuit/circuit.h"
#include "stim/py/base.pybind.h"

using namespace stim;
using namespace stim_pybind;

GateTarget handle_to_gate_target(const pybind11::handle &obj) {
    try {
        std::string_view text = pybind11::cast<std::string_view>(obj);
        return GateTarget::from_target_str(text);
    } catch (const pybind11::cast_error &ex) {
    }
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
        clean_doc_string(R"DOC(
            Represents a gate target, like `0` or `rec[-1]`, from a circuit.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     M 0 !1
                ... ''')
                >>> circuit[0].targets_copy()[0]
                stim.GateTarget(0)
                >>> circuit[0].targets_copy()[1]
                stim.target_inv(1)
        )DOC")
            .data());
}

void stim_pybind::pybind_circuit_gate_target_methods(pybind11::module &m, pybind11::class_<stim::GateTarget> &c) {
    c.def(
        pybind11::init(&obj_to_gate_target),
        pybind11::arg("value"),
        clean_doc_string(R"DOC(
            Initializes a `stim.GateTarget`.

            Args:
                value: A value to convert into a gate target, like an integer
                    to interpret as a qubit target or a string to parse.

            Examples:
                >>> import stim
                >>> stim.GateTarget(stim.GateTarget(5))
                stim.GateTarget(5)
                >>> stim.GateTarget("X7")
                stim.target_x(7)
                >>> stim.GateTarget("rec[-3]")
                stim.target_rec(-3)
                >>> stim.GateTarget("!Z7")
                stim.target_z(7, invert=True)
                >>> stim.GateTarget("*")
                stim.GateTarget.combiner()
        )DOC")
            .data());

    c.def_property_readonly(
        "value",
        &GateTarget::value,
        clean_doc_string(R"DOC(
            The numeric part of the target.

            This is non-negative integer for qubit targets, and a negative integer for
            measurement record targets.

            Examples:
                >>> import stim
                >>> stim.GateTarget(6).value
                6
                >>> stim.target_inv(7).value
                7
                >>> stim.target_x(8).value
                8
                >>> stim.target_y(2).value
                2
                >>> stim.target_z(3).value
                3
                >>> stim.target_sweep_bit(9).value
                9
                >>> stim.target_rec(-5).value
                -5
        )DOC")
            .data());

    c.def_property_readonly(
        "qubit_value",
        [](const GateTarget &self) -> pybind11::object {
            if (self.data & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT | TARGET_COMBINER)) {
                return pybind11::none();
            }
            return pybind11::cast(self.qubit_value());
        },
        clean_doc_string(R"DOC(
            @signature def qubit_value(self) -> Optional[int]:
            Returns the integer value of the targeted qubit, or else None.

            Examples:
                >>> import stim
                >>> stim.GateTarget(6).qubit_value
                6
                >>> stim.target_inv(7).qubit_value
                7
                >>> stim.target_x(8).qubit_value
                8
                >>> stim.target_y(2).qubit_value
                2
                >>> stim.target_z(3).qubit_value
                3
                >>> print(stim.target_sweep_bit(9).qubit_value)
                None
                >>> print(stim.target_rec(-5).qubit_value)
                None
        )DOC")
            .data());

    c.def_property_readonly(
        "is_qubit_target",
        &GateTarget::is_qubit_target,
        clean_doc_string(R"DOC(
            Returns whether or not this is a qubit target like `5` or `!6`.

            Examples:
                >>> import stim
                >>> stim.GateTarget(6).is_qubit_target
                True
                >>> stim.target_inv(7).is_qubit_target
                True
                >>> stim.target_x(8).is_qubit_target
                False
                >>> stim.target_y(2).is_qubit_target
                False
                >>> stim.target_z(3).is_qubit_target
                False
                >>> stim.target_sweep_bit(9).is_qubit_target
                False
                >>> stim.target_rec(-5).is_qubit_target
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "is_x_target",
        &GateTarget::is_x_target,
        clean_doc_string(R"DOC(
            Returns whether or not this is an X pauli target like `X2` or `!X7`.

            Examples:
                >>> import stim
                >>> stim.GateTarget(6).is_x_target
                False
                >>> stim.target_inv(7).is_x_target
                False
                >>> stim.target_x(8).is_x_target
                True
                >>> stim.target_y(2).is_x_target
                False
                >>> stim.target_z(3).is_x_target
                False
                >>> stim.target_sweep_bit(9).is_x_target
                False
                >>> stim.target_rec(-5).is_x_target
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "is_y_target",
        &GateTarget::is_y_target,
        clean_doc_string(R"DOC(
            Returns whether or not this is a Y pauli target like `Y2` or `!Y7`.

            Examples:
                >>> import stim
                >>> stim.GateTarget(6).is_y_target
                False
                >>> stim.target_inv(7).is_y_target
                False
                >>> stim.target_x(8).is_y_target
                False
                >>> stim.target_y(2).is_y_target
                True
                >>> stim.target_z(3).is_y_target
                False
                >>> stim.target_sweep_bit(9).is_y_target
                False
                >>> stim.target_rec(-5).is_y_target
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "is_z_target",
        &GateTarget::is_z_target,
        clean_doc_string(R"DOC(
            Returns whether or not this is a Z pauli target like `Z2` or `!Z7`.

            Examples:
                >>> import stim
                >>> stim.GateTarget(6).is_z_target
                False
                >>> stim.target_inv(7).is_z_target
                False
                >>> stim.target_x(8).is_z_target
                False
                >>> stim.target_y(2).is_z_target
                False
                >>> stim.target_z(3).is_z_target
                True
                >>> stim.target_sweep_bit(9).is_z_target
                False
                >>> stim.target_rec(-5).is_z_target
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "pauli_type",
        &GateTarget::pauli_type,
        clean_doc_string(R"DOC(
            Returns whether this is an 'X', 'Y', or 'Z' target.

            For non-pauli targets, this property evaluates to 'I'.

            Examples:
                >>> import stim
                >>> stim.GateTarget(6).pauli_type
                'I'
                >>> stim.target_inv(7).pauli_type
                'I'
                >>> stim.target_x(8).pauli_type
                'X'
                >>> stim.target_y(2).pauli_type
                'Y'
                >>> stim.target_z(3).pauli_type
                'Z'
                >>> stim.target_sweep_bit(9).pauli_type
                'I'
                >>> stim.target_rec(-5).pauli_type
                'I'
        )DOC")
            .data());

    c.def_property_readonly(
        "is_inverted_result_target",
        &GateTarget::is_inverted_result_target,
        clean_doc_string(R"DOC(
            Returns whether or not this is an inverted target like `!5` or `!X4`.

            Examples:
                >>> import stim
                >>> stim.GateTarget(6).is_inverted_result_target
                False
                >>> stim.target_inv(7).is_inverted_result_target
                True
                >>> stim.target_x(8).is_inverted_result_target
                False
                >>> stim.target_x(8, invert=True).is_inverted_result_target
                True
                >>> stim.target_y(2).is_inverted_result_target
                False
                >>> stim.target_z(3).is_inverted_result_target
                False
                >>> stim.target_sweep_bit(9).is_inverted_result_target
                False
                >>> stim.target_rec(-5).is_inverted_result_target
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "is_measurement_record_target",
        &GateTarget::is_measurement_record_target,
        clean_doc_string(R"DOC(
            Returns whether or not this is a measurement record target like `rec[-5]`.

            Examples:
                >>> import stim
                >>> stim.GateTarget(6).is_measurement_record_target
                False
                >>> stim.target_inv(7).is_measurement_record_target
                False
                >>> stim.target_x(8).is_measurement_record_target
                False
                >>> stim.target_y(2).is_measurement_record_target
                False
                >>> stim.target_z(3).is_measurement_record_target
                False
                >>> stim.target_sweep_bit(9).is_measurement_record_target
                False
                >>> stim.target_rec(-5).is_measurement_record_target
                True
        )DOC")
            .data());

    c.def_property_readonly(
        "is_combiner",
        &GateTarget::is_combiner,
        clean_doc_string(R"DOC(
            Returns whether or not this is a combiner target like `*`.

            Examples:
                >>> import stim
                >>> stim.GateTarget(6).is_combiner
                False
                >>> stim.target_inv(7).is_combiner
                False
                >>> stim.target_x(8).is_combiner
                False
                >>> stim.target_y(2).is_combiner
                False
                >>> stim.target_z(3).is_combiner
                False
                >>> stim.target_sweep_bit(9).is_combiner
                False
                >>> stim.target_rec(-5).is_combiner
                False
                >>> stim.target_combiner().is_combiner
                True
        )DOC")
            .data());

    c.def_property_readonly(
        "is_sweep_bit_target",
        &GateTarget::is_sweep_bit_target,
        clean_doc_string(R"DOC(
            Returns whether or not this is a sweep bit target like `sweep[4]`.

            Examples:
                >>> import stim
                >>> stim.GateTarget(6).is_sweep_bit_target
                False
                >>> stim.target_inv(7).is_sweep_bit_target
                False
                >>> stim.target_x(8).is_sweep_bit_target
                False
                >>> stim.target_y(2).is_sweep_bit_target
                False
                >>> stim.target_z(3).is_sweep_bit_target
                False
                >>> stim.target_sweep_bit(9).is_sweep_bit_target
                True
                >>> stim.target_rec(-5).is_sweep_bit_target
                False
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
