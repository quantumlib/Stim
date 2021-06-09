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

#include "circuit_gate_target.pybind.h"

#include "../py/base.pybind.h"
#include "circuit.h"

GateTarget::GateTarget(uint32_t target) : target(target) {
}

GateTarget::GateTarget(pybind11::object init_target) {
    try {
        target = pybind11::cast<GateTarget>(init_target).target;
        return;
    } catch (const pybind11::cast_error &ex) {
    }
    try {
        target = pybind11::cast<uint32_t>(init_target);
        return;
    } catch (const pybind11::cast_error &ex) {
    }
    throw std::invalid_argument(
        "target argument wasn't a qubit index, a result from a `stim.target_*` method, or a `stim.GateTarget`.");
}
int32_t GateTarget::value() const {
    ssize_t result = target & TARGET_VALUE_MASK;
    if (is_measurement_record_target()) {
        return -result;
    }
    return result;
}
bool GateTarget::is_x_target() const {
    return (target & TARGET_PAULI_X_BIT) && !(target & TARGET_PAULI_Z_BIT);
}
bool GateTarget::is_y_target() const {
    return (target & TARGET_PAULI_X_BIT) && (target & TARGET_PAULI_Z_BIT);
}
bool GateTarget::is_z_target() const {
    return !(target & TARGET_PAULI_X_BIT) && (target & TARGET_PAULI_Z_BIT);
}
bool GateTarget::is_inverted_result_target() const {
    return target & TARGET_INVERTED_BIT;
}
bool GateTarget::is_measurement_record_target() const {
    return target & TARGET_RECORD_BIT;
}
bool GateTarget::operator==(const GateTarget &other) const {
    return target == other.target;
}
bool GateTarget::operator!=(const GateTarget &other) const {
    return target != other.target;
}
std::string GateTarget::repr_inner() const {
    if (is_measurement_record_target()) {
        return "stim.target_rec(" + std::to_string(value()) + ")";
    }
    if (is_inverted_result_target()) {
        return "stim.target_inv(" + std::to_string(value()) + ")";
    }
    if (is_x_target()) {
        return "stim.target_x(" + std::to_string(value()) + ")";
    }
    if (is_y_target()) {
        return "stim.target_y(" + std::to_string(value()) + ")";
    }
    if (is_z_target()) {
        return "stim.target_z(" + std::to_string(value()) + ")";
    }
    return std::to_string(value());
}
std::string GateTarget::repr() const {
    return "stim.GateTarget(" + repr_inner() + ")";
}

void pybind_circuit_gate_target(pybind11::module &m) {
    auto &&c = pybind11::class_<GateTarget>(
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

    c.def(
        pybind11::init<pybind11::object>(),
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
            The numeric part of the target. Positive for qubit targets, negative for measurement record targets.
        )DOC")
            .data());

    c.def_property_readonly(
        "is_x_target",
        &GateTarget::is_x_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a `stim.target_x` target (e.g. `X5` in a circuit file).
        )DOC")
            .data());

    c.def_property_readonly(
        "is_y_target",
        &GateTarget::is_y_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a `stim.target_y` target (e.g. `Y5` in a circuit file).
        )DOC")
            .data());

    c.def_property_readonly(
        "is_z_target",
        &GateTarget::is_z_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a `stim.target_z` target (e.g. `Z5` in a circuit file).
        )DOC")
            .data());

    c.def_property_readonly(
        "is_inverted_result_target",
        &GateTarget::is_inverted_result_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a `stim.target_inv` target (e.g. `!5` in a circuit file).
        )DOC")
            .data());

    c.def_property_readonly(
        "is_measurement_record_target",
        &GateTarget::is_measurement_record_target,
        clean_doc_string(u8R"DOC(
            Returns whether or not this is a `stim.target_rec` target (e.g. `rec[-5]` in a circuit file).
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two `stim.GateTarget`s are identical.");
    c.def(pybind11::self != pybind11::self, "Determines if two `stim.GateTarget`s are different.");
    c.def(
        "__repr__",
        &GateTarget::repr,
        "Returns text that is a valid python expression evaluating to an equivalent `stim.GateTarget`.");
}
