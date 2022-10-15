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

#include "stim/dem/detector_error_model_target.pybind.h"

#include "stim/dem/detector_error_model.pybind.h"
#include "stim/py/base.pybind.h"

using namespace stim;
using namespace stim_pybind;

pybind11::class_<ExposedDemTarget> stim_pybind::pybind_detector_error_model_target(pybind11::module &m) {
    return pybind11::class_<ExposedDemTarget>(
        m, "DemTarget", "An instruction target from a detector error model (.dem) file.");
}

void stim_pybind::pybind_detector_error_model_target_methods(
    pybind11::module &m,
    pybind11::class_<ExposedDemTarget> &c) {
    m.def(
        "target_relative_detector_id",
        &ExposedDemTarget::relative_detector_id,
        pybind11::arg("index"),
        clean_doc_string(u8R"DOC(
            Returns a relative detector id (e.g. "D5" in a .dem file).

            Args:
                index: The index of the detector, relative to the current detector offset.

            Returns:
                The relative detector target.

            Examples:
                >>> import stim
                >>> m = stim.DetectorErrorModel()
                >>> m.append("error", 0.25, [
                ...     stim.target_relative_detector_id(13)
                ... ])
                >>> print(repr(m))
                stim.DetectorErrorModel('''
                    error(0.25) D13
                ''')
        )DOC")
            .data());
    m.def(
        "target_logical_observable_id",
        &ExposedDemTarget::observable_id,
        pybind11::arg("index"),
        clean_doc_string(u8R"DOC(
            Returns a logical observable id identifying a frame change.

            Args:
                index: The index of the observable.

            Returns:
                The logical observable target.

            Examples:
                >>> import stim
                >>> m = stim.DetectorErrorModel()
                >>> m.append("error", 0.25, [
                ...     stim.target_logical_observable_id(13)
                ... ])
                >>> print(repr(m))
                stim.DetectorErrorModel('''
                    error(0.25) L13
                ''')
        )DOC")
            .data());

    m.def(
        "target_separator",
        &ExposedDemTarget::separator,
        clean_doc_string(u8R"DOC(
            Returns a target separator (e.g. "^" in a .dem file).

            Examples:
                >>> import stim
                >>> m = stim.DetectorErrorModel()
                >>> m.append("error", 0.25, [
                ...     stim.target_relative_detector_id(1),
                ...     stim.target_separator(),
                ...     stim.target_relative_detector_id(2),
                ... ])
                >>> print(repr(m))
                stim.DetectorErrorModel('''
                    error(0.25) D1 ^ D2
                ''')
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two `stim.DemTarget`s are identical.");
    c.def(pybind11::self != pybind11::self, "Determines if two `stim.DemTarget`s are different.");

    c.def_static(
        "relative_detector_id",
        &ExposedDemTarget::relative_detector_id,
        pybind11::arg("index"),
        clean_doc_string(u8R"DOC(
            Returns a relative detector id (e.g. "D5" in a .dem file).

            Args:
                index: The index of the detector, relative to the current detector offset.

            Returns:
                The relative detector target.

            Examples:
                >>> import stim
                >>> m = stim.DetectorErrorModel()
                >>> m.append("error", 0.25, [
                ...     stim.DemTarget.relative_detector_id(13)
                ... ])
                >>> print(repr(m))
                stim.DetectorErrorModel('''
                    error(0.25) D13
                ''')
        )DOC")
            .data());

    c.def_static(
        "logical_observable_id",
        &ExposedDemTarget::observable_id,
        pybind11::arg("index"),
        clean_doc_string(u8R"DOC(
            Returns a logical observable id identifying a frame change.

            Args:
                index: The index of the observable.

            Returns:
                The logical observable target.

            Examples:
                >>> import stim
                >>> m = stim.DetectorErrorModel()
                >>> m.append("error", 0.25, [
                ...     stim.DemTarget.logical_observable_id(13)
                ... ])
                >>> print(repr(m))
                stim.DetectorErrorModel('''
                    error(0.25) L13
                ''')
        )DOC")
            .data());

    c.def_static(
        "separator",
        &ExposedDemTarget::separator,
        clean_doc_string(u8R"DOC(
            Returns a target separator (e.g. "^" in a .dem file).

            Examples:
                >>> import stim
                >>> m = stim.DetectorErrorModel()
                >>> m.append("error", 0.25, [
                ...     stim.DemTarget.relative_detector_id(1),
                ...     stim.DemTarget.separator(),
                ...     stim.DemTarget.relative_detector_id(2),
                ... ])
                >>> print(repr(m))
                stim.DetectorErrorModel('''
                    error(0.25) D1 ^ D2
                ''')
        )DOC")
            .data());

    c.def(
        "__repr__",
        &ExposedDemTarget::repr,
        "Returns valid python code evaluating to an equivalent `stim.DemTarget`.");

    c.def("__str__", &ExposedDemTarget::str, "Returns a text description of the detector error model target.");

    c.def(
        "is_relative_detector_id",
        &ExposedDemTarget::is_relative_detector_id,
        clean_doc_string(u8R"DOC(
            Determines if the detector error model target is a relative detector id target.

            In a detector error model file, detectors are prefixed by `D`. For
            example, in `error(0.25) D0 L1` the `D0` is a relative detector target.
        )DOC")
            .data());

    c.def(
        "is_logical_observable_id",
        &ExposedDemTarget::is_observable_id,
        clean_doc_string(u8R"DOC(
            Determines if the detector error model target is a logical observable id target.

            In a detector error model file, observable targets are prefixed by `L`. For
            example, in `error(0.25) D0 L1` the `L1` is an observable target.
        )DOC")
            .data());

    c.def_property_readonly(
        "val",
        &ExposedDemTarget::val,
        clean_doc_string(u8R"DOC(
            Returns the target's integer value.

            Example:

                >>> import stim
                >>> stim.target_relative_detector_id(5).val
                5
                >>> stim.target_logical_observable_id(6).val
                6
        )DOC")
            .data());

    c.def(
        "is_separator",
        &ExposedDemTarget::is_separator,
        clean_doc_string(u8R"DOC(
            Determines if the detector error model target is a separator.

            Separates separate the components of a suggested decompositions within an error.
            For example, the `^` in `error(0.25) D1 D2 ^ D3 D4` is the separator.
        )DOC")
            .data());

    c.def("__hash__", [](const ExposedDemTarget &self) {
        return pybind11::hash(pybind11::make_tuple("DemInstruction", self.data));
    });
}

std::string ExposedDemTarget::repr() const {
    std::stringstream out;
    if (is_relative_detector_id()) {
        out << "stim.target_relative_detector_id(" << raw_id() << ")";
    } else if (is_separator()) {
        out << "stim.target_separator()";
    } else {
        out << "stim.target_logical_observable_id(" << raw_id() << ")";
    }
    return out.str();
}
ExposedDemTarget::ExposedDemTarget(DemTarget target) : DemTarget(target) {
}
ExposedDemTarget ExposedDemTarget::observable_id(uint32_t id) {
    return {DemTarget::observable_id(id)};
}
ExposedDemTarget ExposedDemTarget::relative_detector_id(uint64_t id) {
    return {DemTarget::relative_detector_id(id)};
}
ExposedDemTarget ExposedDemTarget::separator() {
    return ExposedDemTarget(DemTarget::separator());
}
stim::DemTarget ExposedDemTarget::internal() const {
    return {data};
}
