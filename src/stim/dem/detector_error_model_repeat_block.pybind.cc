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

#include "stim/dem/detector_error_model.pybind.h"
#include "stim/dem/detector_error_model_instruction.pybind.h"
#include "stim/py/base.pybind.h"

using namespace stim;

void pybind_detector_error_model_repeat_block(pybind11::module &m) {
    auto c = pybind11::class_<ExposedDemRepeatBlock>(
        m,
        "DemRepeatBlock",
        clean_doc_string(u8R"DOC(
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

    c.def(
        pybind11::init<uint64_t, DetectorErrorModel>(),
        pybind11::arg("repeat_count"),
        pybind11::arg("block"),
        clean_doc_string(u8R"DOC(
            Creates a stim.DemRepeatBlock.

            Args:
                repeat_count: The number of times the repeat block's body is supposed to execute.
                body: The body of the repeat block as a DetectorErrorModel containing the instructions to repeat.

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
        clean_doc_string(u8R"DOC(
            Returns a copy of the block's body, as a stim.DetectorErrorModel.
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
    out << "stim.DemRepeatBlock(" << repeat_count << ", " << detector_error_model_repr(body) << ")";
    return out.str();
}
bool ExposedDemRepeatBlock::operator==(const ExposedDemRepeatBlock &other) const {
    return repeat_count == other.repeat_count && body == other.body;
}
bool ExposedDemRepeatBlock::operator!=(const ExposedDemRepeatBlock &other) const {
    return !(*this == other);
}
