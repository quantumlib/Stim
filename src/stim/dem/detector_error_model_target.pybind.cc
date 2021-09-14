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

void pybind_detector_error_model_target(pybind11::module &m) {
    auto c = pybind11::class_<ExposedDemTarget>(
        m, "DemTarget", "An instruction target from a detector error model (.dem) file.");

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
        )DOC")
            .data());

    m.def(
        "target_logical_observable_id",
        &ExposedDemTarget::observable_id,
        pybind11::arg("index"),
        clean_doc_string(u8R"DOC(
            Returns a logical observable id identifying a frame change (e.g. "L5" in a .dem file).

            Args:
                index: The index of the observable.

            Returns:
                The logical observable target.
        )DOC")
            .data());

    m.def(
        "target_separator",
        &ExposedDemTarget::separator,
        clean_doc_string(u8R"DOC(
            Returns a target separator (e.g. "^" in a .dem file).
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two `stim.DemTarget`s are identical.");
    c.def(pybind11::self != pybind11::self, "Determines if two `stim.DemTarget`s are different.");

    c.def(
        "__repr__",
        &ExposedDemTarget::repr,
        "Returns text that is a valid python expression evaluating to an equivalent `stim.DemTarget`.");

    c.def("__str__", &ExposedDemTarget::str, "Returns a text description of the detector error model target.");

    c.def(
        "is_relative_detector_id",
        &ExposedDemTarget::is_relative_detector_id,
        clean_doc_string(u8R"DOC(
            Determines if the detector error model target is a relative detector id target (like "D4" in a .dem file).
        )DOC")
            .data());

    c.def(
        "is_logical_observable_id",
        &ExposedDemTarget::is_observable_id,
        clean_doc_string(u8R"DOC(
            Determines if the detector error model target is a logical observable id target (like "L5" in a .dem file).
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
            Determines if the detector error model target is a separator (like "^" in a .dem file).
        )DOC")
            .data());
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
