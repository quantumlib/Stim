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

#include "stim/dem/detector_error_model_repeat_block.pybind.h"

#include "stim/dem/dem_instruction.pybind.h"
#include "stim/dem/detector_error_model.pybind.h"
#include "stim/py/base.pybind.h"

using namespace stim;
using namespace stim_pybind;

pybind11::class_<ExposedDemRepeatBlock> stim_pybind::pybind_detector_error_model_repeat_block(pybind11::module &m) {
    return pybind11::class_<ExposedDemRepeatBlock>(
        m,
        "DemRepeatBlock",
        clean_doc_string(R"DOC(
            A repeat block from a detector error model.

            Examples:
                >>> import stim
                >>> model = stim.DetectorErrorModel('''
                ...     repeat 100 {
                ...         error(0.125) D0 D1
                ...         shift_detectors 1
                ...     }
                ... ''')
                >>> model[0]
                stim.DemRepeatBlock(100, stim.DetectorErrorModel('''
                    error(0.125) D0 D1
                    shift_detectors 1
                '''))
        )DOC")
            .data());
}

void stim_pybind::pybind_detector_error_model_repeat_block_methods(
    pybind11::module &m, pybind11::class_<ExposedDemRepeatBlock> &c) {
    c.def(
        pybind11::init<uint64_t, DetectorErrorModel>(),
        pybind11::arg("repeat_count"),
        pybind11::arg("block"),
        clean_doc_string(R"DOC(
            Creates a stim.DemRepeatBlock.

            Args:
                repeat_count: The number of times the repeat block's body is supposed to
                    execute.
                block: The body of the repeat block as a DetectorErrorModel containing the
                    instructions to repeat.

            Examples:
                >>> import stim
                >>> repeat_block = stim.DemRepeatBlock(100, stim.DetectorErrorModel('''
                ...     error(0.125) D0 D1
                ...     shift_detectors 1
                ... '''))
        )DOC")
            .data());

    c.def_readonly(
        "repeat_count",
        &ExposedDemRepeatBlock::repeat_count,
        "The number of times the repeat block's body is supposed to execute.");
    c.def(
        "body_copy",
        &ExposedDemRepeatBlock::body_copy,
        clean_doc_string(R"DOC(
            Returns a copy of the block's body, as a stim.DetectorErrorModel.

            Examples:
                >>> import stim
                >>> body = stim.DetectorErrorModel('''
                ...     error(0.125) D0 D1
                ...     shift_detectors 1
                ... ''')
                >>> repeat_block = stim.DemRepeatBlock(100, body)
                >>> repeat_block.body_copy() == body
                True
                >>> repeat_block.body_copy() is repeat_block.body_copy()
                False
        )DOC")
            .data());
    c.def_property_readonly(
        "type",
        [](const ExposedDemRepeatBlock &self) -> pybind11::object {
            return pybind11::cast("repeat");
        },
        clean_doc_string(R"DOC(
            Returns the type name "repeat".

            This is a duck-typing convenience method. It exists so that code that doesn't
            know whether it has a `stim.DemInstruction` or a `stim.DemRepeatBlock`
            can check the type field without having to do an `instanceof` check first.

            Examples:
                >>> import stim
                >>> dem = stim.DetectorErrorModel('''
                ...     error(0.1) D0 L0
                ...     repeat 5 {
                ...         error(0.1) D0 D1
                ...         shift_detectors 1
                ...     }
                ...     logical_observable L0
                ... ''')
                >>> [instruction.type for instruction in dem]
                ['error', 'repeat', 'logical_observable']
        )DOC")
            .data());
    c.def(pybind11::self == pybind11::self, "Determines if two repeat blocks are identical.");
    c.def(pybind11::self != pybind11::self, "Determines if two repeat blocks are different.");

    c.def(
        "__repr__",
        &ExposedDemRepeatBlock::repr,
        "Returns text that is a valid python expression evaluating to an equivalent `stim.DemRepeatBlock`.");
}

stim::DetectorErrorModel ExposedDemRepeatBlock::body_copy() {
    return body;
}
std::string ExposedDemRepeatBlock::repr() const {
    std::stringstream out;
    out << "stim.DemRepeatBlock(" << repeat_count << ", " << detector_error_model_repr(body);
    if (!tag.empty()) {
        out << ", tag=" << pybind11::cast<std::string>(pybind11::repr(pybind11::cast(tag)));
    }
    out << ")";
    return out.str();
}
bool ExposedDemRepeatBlock::operator==(const ExposedDemRepeatBlock &other) const {
    return repeat_count == other.repeat_count && body == other.body && tag == other.tag;
}
bool ExposedDemRepeatBlock::operator!=(const ExposedDemRepeatBlock &other) const {
    return !(*this == other);
}
