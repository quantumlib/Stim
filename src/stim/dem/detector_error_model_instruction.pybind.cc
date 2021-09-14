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

#include "stim/dem/detector_error_model_instruction.pybind.h"

#include "stim/dem/detector_error_model.pybind.h"
#include "stim/py/base.pybind.h"

using namespace stim;

DemInstruction ExposedDemInstruction::as_dem_instruction() const {
    return DemInstruction{arguments, targets, type};
}

std::string ExposedDemInstruction::type_name() const {
    std::stringstream out;
    out << type;
    return out.str();
}

std::string ExposedDemInstruction::str() const {
    return as_dem_instruction().str();
}

std::string ExposedDemInstruction::repr() const {
    std::stringstream out;
    out << "stim.DemInstruction('" << type << "', [";
    out << comma_sep(arguments);
    out << "], [";
    bool first = true;
    for (const auto &e : targets) {
        if (first) {
            first = false;
        } else {
            out << ", ";
        }
        if (type == DEM_SHIFT_DETECTORS) {
            out << e.data;
        } else if (e.is_relative_detector_id()) {
            out << "stim.target_relative_detector_id(" << e.raw_id() << ")";
        } else if (e.is_separator()) {
            out << "stim.target_separator()";
        } else {
            out << "stim.target_logical_observable_id(" << e.raw_id() << ")";
        }
    }
    out << "])";
    return out.str();
}

bool ExposedDemInstruction::operator==(const ExposedDemInstruction &other) const {
    return type == other.type && arguments == other.arguments && targets == other.targets;
}
bool ExposedDemInstruction::operator!=(const ExposedDemInstruction &other) const {
    return !(*this == other);
}
std::vector<pybind11::object> ExposedDemInstruction::targets_copy() const {
    std::vector<pybind11::object> result;
    if (type == DEM_SHIFT_DETECTORS) {
        for (const auto &e : targets) {
            result.push_back(pybind11::cast(e.data));
        }
    } else {
        for (const auto &e : targets) {
            result.push_back(pybind11::cast(ExposedDemTarget{e}));
        }
    }
    return result;
}
std::vector<double> ExposedDemInstruction::args_copy() const {
    return arguments;
}

void pybind_detector_error_model_instruction(pybind11::module &m) {
    auto c = pybind11::class_<ExposedDemInstruction>(
        m,
        "DemInstruction",
        clean_doc_string(u8R"DOC(
            An instruction from a detector error model.

            Examples:
                >>> import stim
                >>> model = stim.DetectorErrorModel('''
                ...     error(0.125) D0
                ...     error(0.125) D0 D1 L0
                ...     error(0.125) D1 D2
                ...     error(0.125) D2 D3
                ...     error(0.125) D3
                ... ''')
                >>> instruction = model[0]
                >>> instruction
                stim.DemInstruction('error', [0.125], [stim.target_relative_detector_id(0)])
        )DOC")
            .data());

    c.def(
        pybind11::init(
            [](const char *type, const std::vector<double> &arguments, const std::vector<pybind11::object> &targets) {
                std::string lower;
                for (const char *c = type; *c != '\0'; c++) {
                    lower.push_back(tolower(*c));
                }
                DemInstructionType conv_type;
                std::vector<DemTarget> conv_targets;
                if (lower == "error") {
                    conv_type = DEM_ERROR;
                } else if (lower == "shift_detectors") {
                    conv_type = DEM_SHIFT_DETECTORS;
                } else if (lower == "detector") {
                    conv_type = DEM_DETECTOR;
                } else if (lower == "logical_observable") {
                    conv_type = DEM_LOGICAL_OBSERVABLE;
                } else {
                    throw std::invalid_argument("Unrecognized instruction name '" + lower + "'.");
                }
                if (conv_type == DEM_SHIFT_DETECTORS) {
                    for (const auto &e : targets) {
                        try {
                            conv_targets.push_back(DemTarget{pybind11::cast<uint64_t>(e)});
                        } catch (pybind11::cast_error &ex) {
                            throw std::invalid_argument(
                                "Instruction '" + lower + "' only takes unsigned integer targets.");
                        }
                    }
                } else {
                    for (const auto &e : targets) {
                        try {
                            conv_targets.push_back(pybind11::cast<ExposedDemTarget>(e).internal());
                        } catch (pybind11::cast_error &ex) {
                            throw std::invalid_argument(
                                "Instruction '" + lower +
                                "' only takes stim.target_relative_detector_id(k), "
                                "stim.target_logical_observable_id(k), "
                                "stim.target_separator() targets.");
                        }
                    }
                }

                ExposedDemInstruction result{arguments, std::move(conv_targets), conv_type};
                result.as_dem_instruction().validate();
                return result;
            }),
        pybind11::arg("type"),
        pybind11::arg("args"),
        pybind11::arg("targets"),
        clean_doc_string(u8R"DOC(
            Creates a stim.DemInstruction.

            Args:
                type: The name of the instruction type (e.g. "error" or "shift_detectors").
                args: Numeric values parameterizing the instruction (e.g. the 0.1 in "error(0.1)").
                targets: The objects the instruction involves (e.g. the "D0" and "L1" in "error(0.1) D0 L1").

            Examples:
                >>> import stim
                >>> instruction = stim.DemInstruction('error', [0.125], [stim.target_relative_detector_id(5)])
                >>> print(instruction)
                error(0.125) D5
        )DOC")
            .data());

    c.def(
        "args_copy",
        &ExposedDemInstruction::args_copy,
        "Returns a copy of the list of numbers parameterizing the instruction (e.g. the probability of an error).");
    c.def(
        "targets_copy",
        &ExposedDemInstruction::targets_copy,
        "Returns a copy of the list of objects the instruction applies to (e.g. affected detectors.");
    c.def_property_readonly(
        "type",
        &ExposedDemInstruction::type_name,
        clean_doc_string(u8R"DOC(
            The name of the instruction type (e.g. "error" or "shift_detectors").
        )DOC")
            .data());
    c.def(pybind11::self == pybind11::self, "Determines if two instructions have identical contents.");
    c.def(pybind11::self != pybind11::self, "Determines if two instructions have non-identical contents.");

    c.def(
        "__str__",
        &ExposedDemInstruction::str,
        "Returns detector error model (.dem) instructions (that can be parsed by stim) for the model.");
    c.def(
        "__repr__",
        &ExposedDemInstruction::repr,
        "Returns text that is a valid python expression evaluating to an equivalent `stim.DetectorErrorModel`.");
}
