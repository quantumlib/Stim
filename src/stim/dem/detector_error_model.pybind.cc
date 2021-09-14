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

#include "stim/dem/detector_error_model.pybind.h"

#include "stim/dem/detector_error_model_instruction.pybind.h"
#include "stim/dem/detector_error_model_repeat_block.pybind.h"
#include "stim/dem/detector_error_model_target.pybind.h"
#include "stim/py/base.pybind.h"

using namespace stim;

std::string detector_error_model_repr(const DetectorErrorModel &self) {
    if (self.instructions.empty()) {
        return "stim.DetectorErrorModel()";
    }
    std::stringstream ss;
    ss << "stim.DetectorErrorModel('''\n";
    print_detector_error_model(ss, self, 4);
    ss << "\n''')";
    return ss.str();
}

void pybind_detector_error_model(pybind11::module &m) {
    auto c = pybind11::class_<DetectorErrorModel>(
        m,
        "DetectorErrorModel",
        clean_doc_string(u8R"DOC(
            A list of instructions describing error mechanisms in terms of the detection events they produce.

            Examples:
                >>> import stim
                >>> model = stim.DetectorErrorModel('''
                ...     error(0.125) D0
                ...     error(0.125) D0 D1 L0
                ...     error(0.125) D1 D2
                ...     error(0.125) D2 D3
                ...     error(0.125) D3
                ... ''')
                >>> len(model)
                5

                >>> stim.Circuit('''
                ...     X_ERROR(0.125) 0
                ...     X_ERROR(0.25) 1
                ...     CORRELATED_ERROR(0.375) X0 X1
                ...     M 0 1
                ...     DETECTOR rec[-2]
                ...     DETECTOR rec[-1]
                ... ''').detector_error_model()
                stim.DetectorErrorModel('''
                    error(0.125) D0
                    error(0.375) D0 D1
                    error(0.25) D1
                ''')
        )DOC")
            .data());

    pybind_detector_error_model_instruction(m);
    pybind_detector_error_model_target(m);
    pybind_detector_error_model_repeat_block(m);

    c.def(
        pybind11::init([](const char *detector_error_model_text) {
            DetectorErrorModel self;
            self.append_from_text(detector_error_model_text);
            return self;
        }),
        pybind11::arg("detector_error_model_text") = "",
        clean_doc_string(u8R"DOC(
            Creates a stim.DetectorErrorModel.

            Args:
                detector_error_model_text: Defaults to empty. Describes instructions to append into the circuit in the
                    detector error model (.dem) format.

            Examples:
                >>> import stim
                >>> empty = stim.DetectorErrorModel()
                >>> not_empty = stim.DetectorErrorModel('''
                ...    error(0.125) D0 L0
                ... ''')
        )DOC")
            .data());

    c.def_property_readonly(
        "num_detectors",
        &DetectorErrorModel::count_detectors,
        clean_doc_string(u8R"DOC(
            Counts the number of detectors (e.g. `D2`) in the error model.

            Detector indices are assumed to be contiguous from 0 up to whatever the maximum detector id is.
            If the largest detector's absolute id is n-1, then the number of detectors is n.

            Examples:
                >>> import stim

                >>> stim.Circuit('''
                ...     X_ERROR(0.125) 0
                ...     X_ERROR(0.25) 1
                ...     CORRELATED_ERROR(0.375) X0 X1
                ...     M 0 1
                ...     DETECTOR rec[-2]
                ...     DETECTOR rec[-1]
                ... ''').detector_error_model().num_detectors
                2

                >>> stim.DetectorErrorModel('''
                ...    error(0.1) D0 D199
                ... ''').num_detectors
                200

                >>> stim.DetectorErrorModel('''
                ...    shift_detectors 1000
                ...    error(0.1) D0 D199
                ... ''').num_detectors
                1200
        )DOC")
            .data());

    c.def_property_readonly(
        "num_observables",
        &DetectorErrorModel::count_observables,
        clean_doc_string(u8R"DOC(
            Counts the number of frame changes (e.g. `L2`) in the error model.

            Observable indices are assumed to be contiguous from 0 up to whatever the maximum observable id is.
            If the largest observable's id is n-1, then the number of observables is n.

            Examples:
                >>> import stim

                >>> stim.Circuit('''
                ...     X_ERROR(0.125) 0
                ...     M 0
                ...     OBSERVABLE_INCLUDE(99) rec[-1]
                ... ''').detector_error_model().num_observables
                100

                >>> stim.DetectorErrorModel('''
                ...    error(0.1) L399
                ... ''').num_observables
                400
        )DOC")
            .data());

    c.def(
        "clear",
        &DetectorErrorModel::clear,
        clean_doc_string(u8R"DOC(
            Clears the contents of the detector error model.

            Examples:
                >>> import stim
                >>> model = stim.DetectorErrorModel('''
                ...    error(0.1) D0 D1
                ... ''')
                >>> model.clear()
                >>> model
                stim.DetectorErrorModel()
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two detector error models have identical contents.");
    c.def(pybind11::self != pybind11::self, "Determines if two detector error models have non-identical contents.");

    c.def(
        "__str__",
        &DetectorErrorModel::str,
        clean_doc_string(u8R"DOC(
            "Returns detector error model (.dem) instructions (that can be parsed by stim) for the model.");
        )DOC")
            .data());
    c.def(
        "__repr__",
        &detector_error_model_repr,
        clean_doc_string(u8R"DOC(
            "Returns text that is a valid python expression evaluating to an equivalent `stim.DetectorErrorModel`."
        )DOC")
            .data());

    c.def(
        "copy",
        [](DetectorErrorModel &self) -> DetectorErrorModel {
            return self;
        },
        clean_doc_string(u8R"DOC(
            Returns a copy of the detector error model. An independent model with the same contents.

            Examples:
                >>> import stim

                >>> c1 = stim.DetectorErrorModel("error(0.1) D0 D1")
                >>> c2 = c1.copy()
                >>> c2 is c1
                False
                >>> c2 == c1
                True
        )DOC")
            .data());

    c.def(
        "__len__",
        [](const DetectorErrorModel &self) -> size_t {
            return self.instructions.size();
        },
        clean_doc_string(u8R"DOC(
            Returns the number of top-level instructions and blocks in the detector error model.

            Instructions inside of blocks are not included in this count.

            Examples:
                >>> import stim
                >>> len(stim.DetectorErrorModel())
                0
                >>> len(stim.DetectorErrorModel('''
                ...    error(0.1) D0 D1
                ...    shift_detectors 100
                ...    logical_observable L5
                ... '''))
                3
                >>> len(stim.DetectorErrorModel('''
                ...    REPEAT 100 {
                ...        error(0.1) D0 D1
                ...        error(0.1) D1 D2
                ...    }
                ... '''))
                1
        )DOC")
            .data());

    c.def(
        "__getitem__",
        [](const DetectorErrorModel &self, pybind11::object index_or_slice) -> pybind11::object {
            pybind11::ssize_t index, step, slice_length;
            if (normalize_index_or_slice(index_or_slice, self.instructions.size(), &index, &step, &slice_length)) {
                return pybind11::cast(self.py_get_slice(index, step, slice_length));
            }

            auto &op = self.instructions[index];
            if (op.type == DEM_REPEAT_BLOCK) {
                return pybind11::cast(
                    ExposedDemRepeatBlock{op.target_data[0].data, self.blocks[op.target_data[1].data]});
            }
            ExposedDemInstruction result;
            result.targets.insert(result.targets.begin(), op.target_data.begin(), op.target_data.end());
            result.arguments.insert(result.arguments.begin(), op.arg_data.begin(), op.arg_data.end());
            result.type = op.type;
            return pybind11::cast(result);
        },
        pybind11::arg("index_or_slice"),
        clean_doc_string(u8R"DOC(
            Returns copies of instructions from the detector error model.

            Args:
                index_or_slice: An integer index picking out an instruction to return, or a slice picking out a range
                    of instructions to return as a detector error model.

            Examples:
            Examples:
                >>> import stim
                >>> model = stim.DetectorErrorModel('''
                ...    error(0.125) D0
                ...    error(0.125) D1 L1
                ...    REPEAT 100 {
                ...        error(0.125) D1 D2
                ...        shift_detectors 1
                ...    }
                ...    error(0.125) D2
                ...    logical_observable L0
                ...    detector D5
                ... ''')
                >>> model[1]
                stim.DemInstruction('error', [0.125], [stim.target_relative_detector_id(1), stim.target_logical_observable_id(1)])
                >>> model[2]
                stim.DemRepeatBlock(100, stim.DetectorErrorModel('''
                    error(0.125) D1 D2
                    shift_detectors 1
                '''))
                >>> model[1::2]
                stim.DetectorErrorModel('''
                    error(0.125) D1 L1
                    error(0.125) D2
                    detector D5
                ''')
        )DOC")
            .data());
}
