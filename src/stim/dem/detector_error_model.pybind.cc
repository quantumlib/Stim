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

#include <fstream>

#include "stim/circuit/circuit.pybind.h"
#include "stim/dem/detector_error_model_instruction.pybind.h"
#include "stim/dem/detector_error_model_repeat_block.pybind.h"
#include "stim/dem/detector_error_model_target.pybind.h"
#include "stim/io/raii_file.h"
#include "stim/py/base.pybind.h"
#include "stim/search/search.h"
#include "stim/simulators/dem_sampler.h"

using namespace stim;
using namespace stim_pybind;

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

std::vector<double> python_arg_to_instruction_arguments(const pybind11::object &arg) {
    if (arg.is(pybind11::none())) {
        return {};
    }
    try {
        return {pybind11::cast<double>(arg)};
    } catch (const pybind11::cast_error &ex) {
    }
    try {
        return pybind11::cast<std::vector<double>>(arg);
    } catch (const pybind11::cast_error &ex) {
    }
    throw std::invalid_argument("parens_arguments must be None, a double, or a list of doubles.");
}

DemInstructionType non_block_instruction_name_to_enum(const std::string &name) {
    std::string low;
    for (char c : name) {
        low.push_back(tolower(c));
    }
    if (low == "error") {
        return DemInstructionType::DEM_ERROR;
    } else if (low == "shift_detectors") {
        return DemInstructionType::DEM_SHIFT_DETECTORS;
    } else if (low == "detector") {
        return DemInstructionType::DEM_DETECTOR;
    } else if (low == "logical_observable") {
        return DemInstructionType::DEM_LOGICAL_OBSERVABLE;
    }
    throw std::invalid_argument("Not a non-block detector error model instruction name: " + name);
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
        "num_errors",
        &DetectorErrorModel::count_errors,
        clean_doc_string(u8R"DOC(
            Counts the number of errors (e.g. `error(0.1) D0`) in the error model.

            Error instructions inside repeat blocks count once per repetition.
            Redundant errors with the same targets count as separate errors.

            Examples:
                >>> import stim

                >>> stim.DetectorErrorModel('''
                ...     error(0.125) D0
                ...     repeat 100 {
                ...         repeat 5 {
                ...             error(0.25) D1
                ...         }
                ...     }
                ... ''').num_errors
                501
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
                ...    repeat 100 {
                ...        error(0.1) D0 D1
                ...        error(0.1) D1 D2
                ...    }
                ... '''))
                1
        )DOC")
            .data());

    c.def(
        "__getitem__",
        [](const DetectorErrorModel &self, const pybind11::object &index_or_slice) -> pybind11::object {
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
            @overload def __getitem__(self, index_or_slice: int) -> Union[stim.DemInstruction, stim.DemRepeatBlock]:
            @overload def __getitem__(self, index_or_slice: slice) -> stim.DetectorErrorModel:

            Args:
                index_or_slice: An integer index picking out an instruction to return, or a slice picking out a range
                    of instructions to return as a detector error model.

            Examples:
            Examples:
                >>> import stim
                >>> model = stim.DetectorErrorModel('''
                ...    error(0.125) D0
                ...    error(0.125) D1 L1
                ...    repeat 100 {
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

    c.def(
        "approx_equals",
        [](const DetectorErrorModel &self, const pybind11::object &obj, double atol) -> bool {
            try {
                return self.approx_equals(pybind11::cast<DetectorErrorModel>(obj), atol);
            } catch (const pybind11::cast_error &ex) {
                return false;
            }
        },
        pybind11::arg("other"),
        pybind11::kw_only(),
        pybind11::arg("atol"),
        clean_doc_string(u8R"DOC(
            Checks if a detector error model is approximately equal to another detector error model.

            Two detector error model are approximately equal if they are equal up to slight perturbations of instruction
            arguments such as probabilities. For example `error(0.100) D0` is approximately equal to `error(0.099) D0`
            within an absolute tolerance of 0.002. All other details of the models (such as the ordering of errors and
            their targets) must be exactly the same.

            Args:
                other: The detector error model, or other object, to compare to this one.
                atol: The absolute error tolerance. The maximum amount each probability may have been perturbed by.

            Returns:
                True if the given object is a detector error model approximately equal up to the receiving circuit up to
                the given tolerance, otherwise False.

            Examples:
                >>> import stim
                >>> base = stim.DetectorErrorModel('''
                ...    error(0.099) D0 D1
                ... ''')

                >>> base.approx_equals(base, atol=0)
                True

                >>> base.approx_equals(stim.DetectorErrorModel('''
                ...    error(0.101) D0 D1
                ... '''), atol=0)
                False

                >>> base.approx_equals(stim.DetectorErrorModel('''
                ...    error(0.101) D0 D1
                ... '''), atol=0.0001)
                False

                >>> base.approx_equals(stim.DetectorErrorModel('''
                ...    error(0.101) D0 D1
                ... '''), atol=0.01)
                True

                >>> base.approx_equals(stim.DetectorErrorModel('''
                ...    error(0.099) D0 D1 L0 L1 L2 L3 L4
                ... '''), atol=9999)
                False
        )DOC")
            .data());

    c.def(
        "append",
        [](DetectorErrorModel &self,
           const pybind11::object &instruction,
           const pybind11::object &parens_arguments,
           const std::vector<pybind11::object> &targets) {
            bool is_name = pybind11::isinstance<pybind11::str>(instruction);
            if (!is_name && (!targets.empty() || !parens_arguments.is_none())) {
                throw std::invalid_argument(
                    "Can't specify `parens_arguments` or `targets` when instruction is a "
                    "stim.DemInstruction (instead of an instruction name).");
            }
            if (is_name && (targets.empty() || parens_arguments.is_none())) {
                throw std::invalid_argument(
                    "Must specify `parens_arguments` and `targets` when instruction is an instruction name.");
            }

            if (is_name) {
                auto name = pybind11::cast<std::string>(instruction);
                auto type = non_block_instruction_name_to_enum(name);
                auto conv_args = python_arg_to_instruction_arguments(parens_arguments);
                std::vector<DemTarget> conv_targets;
                for (const auto &e : targets) {
                    try {
                        if (type == DemInstructionType::DEM_SHIFT_DETECTORS) {
                            conv_targets.push_back(DemTarget{pybind11::cast<uint64_t>(e)});
                        } else {
                            conv_targets.push_back(DemTarget{pybind11::cast<ExposedDemTarget>(e).data});
                        }
                    } catch (pybind11::cast_error &ex) {
                        auto str = pybind11::cast<std::string>(pybind11::str(e));
                        throw std::invalid_argument("Bad target '" + str + "' for instruction '" + name + "'.");
                    }
                }

                self.append_dem_instruction(DemInstruction{
                    conv_args,
                    conv_targets,
                    type,
                });
            } else if (pybind11::isinstance<ExposedDemInstruction>(instruction)) {
                const ExposedDemInstruction &exp = pybind11::cast<ExposedDemInstruction>(instruction);
                self.append_dem_instruction(DemInstruction{exp.arguments, exp.targets, exp.type});
            } else if (pybind11::isinstance<ExposedDemRepeatBlock>(instruction)) {
                const ExposedDemRepeatBlock &block = pybind11::cast<ExposedDemRepeatBlock>(instruction);
                self.append_repeat_block(block.repeat_count, block.body);
            } else {
                throw std::invalid_argument(
                    "First argument to stim.DetectorErrorModel.append must be a str (an instruction name), "
                    "a stim.DemInstruction, "
                    "or a stim.DemRepeatBlock");
            }
        },
        pybind11::arg("instruction"),
        pybind11::arg("parens_arguments") = pybind11::none(),
        pybind11::arg("targets") = pybind11::make_tuple(),
        clean_doc_string(u8R"DOC(
            Appends an instruction to the detector error model.

            Args:
                instruction: Either the name of an instruction, a stim.DemInstruction, or a stim.DemRepeatBlock.
                    The `parens_arguments` and `targets` arguments are given if and only if the instruction is a name.
                parens_arguments: Numeric values parameterizing the instruction. The numbers inside parentheses in a
                    detector error model file (eg. the `0.25` in `error(0.25) D0`). This argument can be given either
                    a list of doubles, or a single double (which will be implicitly wrapped into a list).
                targets: The instruction targets, such as the `D0` in `error(0.25) D0`.

            Examples:
                >>> import stim
                >>> m = stim.DetectorErrorModel()
                >>> m.append("error", 0.125, [
                ...     stim.DemTarget.relative_detector_id(1),
                ... ])
                >>> m.append("error", 0.25, [
                ...     stim.DemTarget.relative_detector_id(1),
                ...     stim.DemTarget.separator(),
                ...     stim.DemTarget.relative_detector_id(2),
                ...     stim.DemTarget.logical_observable_id(3),
                ... ])
                >>> print(repr(m))
                stim.DetectorErrorModel('''
                    error(0.125) D1
                    error(0.25) D1 ^ D2 L3
                ''')

                >>> m.append("shift_detectors", (1, 2, 3), [5])
                >>> print(repr(m))
                stim.DetectorErrorModel('''
                    error(0.125) D1
                    error(0.25) D1 ^ D2 L3
                    shift_detectors(1, 2, 3) 5
                ''')

                >>> m += m * 3
                >>> m.append(m[0])
                >>> m.append(m[-2])
                >>> print(repr(m))
                stim.DetectorErrorModel('''
                    error(0.125) D1
                    error(0.25) D1 ^ D2 L3
                    shift_detectors(1, 2, 3) 5
                    repeat 3 {
                        error(0.125) D1
                        error(0.25) D1 ^ D2 L3
                        shift_detectors(1, 2, 3) 5
                    }
                    error(0.125) D1
                    repeat 3 {
                        error(0.125) D1
                        error(0.25) D1 ^ D2 L3
                        shift_detectors(1, 2, 3) 5
                    }
                ''')
        )DOC")
            .data());

    c.def(
        "__imul__",
        &DetectorErrorModel::operator*=,
        pybind11::arg("repetitions"),
        clean_doc_string(u8R"DOC(
            Mutates the detector error model by putting its contents into a repeat block.

            Special case: if the repetition count is 0, the model is cleared.
            Special case: if the repetition count is 1, nothing happens.

            Args:
                repetitions: The number of times the repeat block should repeat.

            Examples:
                >>> import stim
                >>> m = stim.DetectorErrorModel('''
                ...    error(0.25) D0
                ...    shift_detectors 1
                ... ''')
                >>> m *= 3
                >>> print(m)
                repeat 3 {
                    error(0.25) D0
                    shift_detectors 1
                }
        )DOC")
            .data());

    c.def(
        "get_detector_coordinates",
        [](const DetectorErrorModel &self, const pybind11::object &obj) {
            return self.get_detector_coordinates(obj_to_abs_detector_id_set(obj, [&]() {
                return self.count_detectors();
            }));
        },
        pybind11::arg("only") = pybind11::none(),
        clean_doc_string(u8R"DOC(
            Returns the coordinate metadata of detectors in the detector error model.

            Args:
                only: Defaults to None (meaning include all detectors). A list of detector indices to include in the
                    result. Detector indices beyond the end of the detector error model cause an error.

            Returns:
                A dictionary mapping integers (detector indices) to lists of floats (coordinates).
                Detectors with no specified coordinate data are mapped to an empty tuple.
                If `only` is specified, then `set(result.keys()) == set(only)`.

            Examples:
                >>> import stim
                >>> dem = stim.DetectorErrorModel('''
                ...    error(0.25) D0 D1
                ...    detector(1, 2, 3) D1
                ...    shift_detectors(5) 1
                ...    detector(1, 2) D2
                ... ''')
                >>> dem.get_detector_coordinates()
                {0: [], 1: [1.0, 2.0, 3.0], 2: [], 3: [6.0, 2.0]}
                >>> dem.get_detector_coordinates(only=[1])
                {1: [1.0, 2.0, 3.0]}
        )DOC")
            .data());

    c.def(
        "__add__",
        &DetectorErrorModel::operator+,
        pybind11::arg("second"),
        clean_doc_string(u8R"DOC(
            Creates a detector error model by appending two models.

            Examples:
                >>> import stim
                >>> m1 = stim.DetectorErrorModel('''
                ...    error(0.125) D0
                ... ''')
                >>> m2 = stim.DetectorErrorModel('''
                ...    error(0.25) D1
                ... ''')
                >>> m1 + m2
                stim.DetectorErrorModel('''
                    error(0.125) D0
                    error(0.25) D1
                ''')
        )DOC")
            .data());

    c.def(
        "__iadd__",
        &DetectorErrorModel::operator+=,
        pybind11::arg("second"),
        clean_doc_string(u8R"DOC(
            Appends a detector error model into the receiving model (mutating it).

            Examples:
                >>> import stim
                >>> m1 = stim.DetectorErrorModel('''
                ...    error(0.125) D0
                ... ''')
                >>> m2 = stim.DetectorErrorModel('''
                ...    error(0.25) D1
                ... ''')
                >>> m1 += m2
                >>> print(repr(m1))
                stim.DetectorErrorModel('''
                    error(0.125) D0
                    error(0.25) D1
                ''')
        )DOC")
            .data());

    c.def(
        "__mul__",
        &DetectorErrorModel::operator*,
        pybind11::arg("repetitions"),
        clean_doc_string(u8R"DOC(
            Returns a detector error model with a repeat block containing the current model's instructions.

            Special case: if the repetition count is 0, an empty model is returned.
            Special case: if the repetition count is 1, an equal model with no repeat block is returned.

            Args:
                repetitions: The number of times the repeat block should repeat.

            Examples:
                >>> import stim
                >>> m = stim.DetectorErrorModel('''
                ...    error(0.25) D0
                ...    shift_detectors 1
                ... ''')
                >>> m * 3
                stim.DetectorErrorModel('''
                    repeat 3 {
                        error(0.25) D0
                        shift_detectors 1
                    }
                ''')
        )DOC")
            .data());

    c.def(
        "__rmul__",
        &DetectorErrorModel::operator*,
        pybind11::arg("repetitions"),
        clean_doc_string(u8R"DOC(
            Returns a detector error model with a repeat block containing the current model's instructions.

            Special case: if the repetition count is 0, an empty model is returned.
            Special case: if the repetition count is 1, an equal model with no repeat block is returned.

            Args:
                repetitions: The number of times the repeat block should repeat.

            Examples:
                >>> import stim
                >>> m = stim.DetectorErrorModel('''
                ...    error(0.25) D0
                ...    shift_detectors 1
                ... ''')
                >>> 3 * m
                stim.DetectorErrorModel('''
                    repeat 3 {
                        error(0.25) D0
                        shift_detectors 1
                    }
                ''')
        )DOC")
            .data());

    c.def(pybind11::pickle(
        [](const DetectorErrorModel &self) -> pybind11::str {
            return self.str();
        },
        [](const pybind11::str &text) -> DetectorErrorModel {
            return DetectorErrorModel(pybind11::cast<std::string>(text).data());
        }));

    c.def(
        "shortest_graphlike_error",
        &shortest_graphlike_undetectable_logical_error,
        pybind11::arg("ignore_ungraphlike_errors") = false,
        clean_doc_string(u8R"DOC(
            Finds a minimum sized set of graphlike errors that produce an undetected logical error.

            Note that this method does not pay attention to error probabilities (other than ignoring errors with
            probability 0). It searches for a logical error with the minimum *number* of physical errors, not the
            maximum probability of those physical errors all occurring.

            This method works by looking for errors that have frame changes (eg. "error(0.1) D0 D1 L5" flips the frame
            of observable 5). These errors are converted into one or two symptoms and a net frame change. The symptoms
            can then be moved around by following errors touching that symptom. Each symptom is moved until it
            disappears into a boundary or cancels against another remaining symptom, while leaving the other symptoms
            alone (ensuring only one symptom is allowed to move significantly reduces waste in the search space).
            Eventually a path or cycle of errors is found that cancels out the symptoms, and if there is still a frame
            change at that point then that path or cycle is a logical error (otherwise all that was found was a
            stabilizer of the system; a dead end). The search process advances like a breadth first search, seeded from
            all the frame-change errors and branching them outward in tandem, until one of them wins the race to find a
            solution.

            Args:
                ignore_ungraphlike_errors: Defaults to False. When False, an exception is raised if there are any
                    errors in the model that are not graphlike. When True, those errors are skipped as if they weren't
                    present.

                    A graphlike error is an error with at most two symptoms per decomposed component.
                        graphlike:
                            error(0.1) D0
                            error(0.1) D0 D1
                            error(0.1) D0 D1 L0
                            error(0.1) D0 D1 ^ D2
                        not graphlike:
                            error(0.1) D0 D1 D2
                            error(0.1) D0 D1 D2 ^ D3

            Returns:
                A detector error model containing just the error instructions corresponding to an undetectable logical
                error. There will be no other kinds of instructions (no `repeat`s, no `shift_detectors`, etc).
                The error probabilities will all be set to 1.

                The `len` of the returned model is the graphlike code distance of the circuit. But beware that in
                general the true code distance may be smaller. For example, in the XZ surface code with twists, the true
                minimum sized logical error is likely to use Y errors. But each Y error decomposes into two graphlike
                components (the X part and the Z part). As a result, the graphlike code distance in that context is
                likely to be nearly twice as large as the true code distance.

            Examples:
                >>> import stim

                >>> stim.DetectorErrorModel('''
                ...     error(0.125) D0
                ...     error(0.125) D0 D1
                ...     error(0.125) D1 L55
                ...     error(0.125) D1
                ... ''').shortest_graphlike_error()
                stim.DetectorErrorModel('''
                    error(1) D1
                    error(1) D1 L55
                ''')

                >>> stim.DetectorErrorModel('''
                ...     error(0.125) D0 D1 D2
                ...     error(0.125) L0
                ... ''').shortest_graphlike_error(ignore_ungraphlike_errors=True)
                stim.DetectorErrorModel('''
                    error(1) L0
                ''')

                >>> circuit = stim.Circuit.generated(
                ...     "repetition_code:memory",
                ...     rounds=10,
                ...     distance=7,
                ...     before_round_data_depolarization=0.01)
                >>> model = circuit.detector_error_model(decompose_errors=True)
                >>> len(model.shortest_graphlike_error())
                7
        )DOC")
            .data());

    c.def_static(
        "from_file",
        [](pybind11::object &obj) {
            try {
                auto path = pybind11::cast<std::string>(obj);
                RaiiFile f(path.data(), "r");
                return DetectorErrorModel::from_file(f.f);
            } catch (pybind11::cast_error &ex) {
            }

            auto py_path = pybind11::module::import("pathlib").attr("Path");
            if (pybind11::isinstance(obj, py_path)) {
                auto path = pybind11::cast<std::string>(pybind11::str(obj));
                RaiiFile f(path.data(), "r");
                return DetectorErrorModel::from_file(f.f);
            }

            auto py_text_io_base = pybind11::module::import("io").attr("TextIOBase");
            if (pybind11::isinstance(obj, py_text_io_base)) {
                auto contents = obj.attr("read")();
                return DetectorErrorModel(pybind11::cast<std::string>(pybind11::str(contents)).data());
            }

            throw std::invalid_argument(
                "Don't know how to read from " + pybind11::cast<std::string>(pybind11::str(obj)));
        },
        pybind11::arg("file"),
        clean_doc_string(u8R"DOC(
            @signature def from_file(file: Union[io.TextIOBase, str, pathlib.Path]) -> stim.DetectorErrorModel:
            Reads a detector error model from a file.

            The file format is defined at https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md

            Args:
                file: A file path or open file object to read from.

            Returns:
                The circuit parsed from the file.

            Examples:
                >>> import stim
                >>> import tempfile

                >>> with tempfile.TemporaryDirectory() as tmpdir:
                ...     path = tmpdir + '/tmp.stim'
                ...     with open(path, 'w') as f:
                ...         print('error(0.25) D2 D3', file=f)
                ...     circuit = stim.DetectorErrorModel.from_file(path)
                >>> circuit
                stim.DetectorErrorModel('''
                    error(0.25) D2 D3
                ''')

                >>> with tempfile.TemporaryDirectory() as tmpdir:
                ...     path = tmpdir + '/tmp.stim'
                ...     with open(path, 'w') as f:
                ...         print('error(0.25) D2 D3', file=f)
                ...     with open(path) as f:
                ...         circuit = stim.DetectorErrorModel.from_file(path)
                >>> circuit
                stim.DetectorErrorModel('''
                    error(0.25) D2 D3
                ''')
        )DOC")
            .data());

    c.def(
        "to_file",
        [](const DetectorErrorModel &self, pybind11::object &obj) {
            try {
                auto path = pybind11::cast<std::string>(obj);
                std::ofstream out(path, std::ofstream::out);
                if (!out.is_open()) {
                    throw std::invalid_argument("Failed to open " + path);
                }
                out << self << '\n';
                return;
            } catch (pybind11::cast_error &ex) {
            }

            auto py_path = pybind11::module::import("pathlib").attr("Path");
            if (pybind11::isinstance(obj, py_path)) {
                auto path = pybind11::cast<std::string>(pybind11::str(obj));
                std::ofstream out(path, std::ofstream::out);
                if (!out.is_open()) {
                    throw std::invalid_argument("Failed to open " + path);
                }
                out << self << '\n';
                return;
            }

            auto py_text_io_base = pybind11::module::import("io").attr("TextIOBase");
            if (pybind11::isinstance(obj, py_text_io_base)) {
                obj.attr("write")(pybind11::str(self.str()));
                obj.attr("write")(pybind11::str("\n"));
                return;
            }

            throw std::invalid_argument(
                "Don't know how to write to " + pybind11::cast<std::string>(pybind11::str(obj)));
        },
        pybind11::arg("file"),
        clean_doc_string(u8R"DOC(
            @signature def to_file(self, file: Union[io.TextIOBase, str, pathlib.Path]) -> None:
            Writes the detector error model to a file.

            The file format is defined at https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md

            Args:
                file: A file path or an open file to write to.

            Examples:
                >>> import stim
                >>> import tempfile
                >>> c = stim.DetectorErrorModel('error(0.25) D2 D3')

                >>> with tempfile.TemporaryDirectory() as tmpdir:
                ...     path = tmpdir + '/tmp.stim'
                ...     with open(path, 'w') as f:
                ...         c.to_file(f)
                ...     with open(path) as f:
                ...         contents = f.read()
                >>> contents
                'error(0.25) D2 D3\n'

                >>> with tempfile.TemporaryDirectory() as tmpdir:
                ...     path = tmpdir + '/tmp.stim'
                ...     c.to_file(path)
                ...     with open(path) as f:
                ...         contents = f.read()
                >>> contents
                'error(0.25) D2 D3\n'
        )DOC")
            .data());

    c.def(
        "compile_sampler",
        [](const DetectorErrorModel &self, const pybind11::object &seed) {
            return DemSampler(self, *make_py_seeded_rng(seed), 1024);
        },
        pybind11::kw_only(),
        pybind11::arg("seed") = pybind11::none(),
        clean_doc_string(u8R"DOC(
            Returns a CompiledDemSampler, which can quickly batch sample from detector error models.

            Args:
                seed: PARTIALLY determines simulation results by deterministically seeding the random number generator.
                    Must be None or an integer in range(2**64).

                    Defaults to None. When set to None, a prng seeded by system entropy is used.

                    When set to an integer, making the exact same series calls on the exact same machine with the exact
                    same version of Stim will produce the exact same simulation results.

                    CAUTION: simulation results *WILL NOT* be consistent between versions of Stim. This restriction is
                    present to make it possible to have future optimizations to the random sampling, and is enforced by
                    introducing intentional differences in the seeding strategy from version to version.

                    CAUTION: simulation results *MAY NOT* be consistent across machines that differ in the width of
                    supported SIMD instructions. For example, using the same seed on a machine that supports AVX
                    instructions and one that only supports SSE instructions may produce different simulation results.

                    CAUTION: simulation results *MAY NOT* be consistent if you vary how many shots are taken. For
                    example, taking 10 shots and then 90 shots will give different results from taking 100 shots in one
                    call.

            Returns:
                A seeded stim.CompiledDemSampler for the given detector error model.

            Examples:
                >>> import stim
                >>> dem = stim.DetectorErrorModel('''
                ...    error(0) D0
                ...    error(1) D1 D2 L0
                ... ''')
                >>> sampler = dem.compile_sampler()
                >>> det_data, obs_data, err_data = sampler.sample(shots=4, return_errors=True)
                >>> det_data
                array([[False,  True,  True],
                       [False,  True,  True],
                       [False,  True,  True],
                       [False,  True,  True]])
                >>> obs_data
                array([[ True],
                       [ True],
                       [ True],
                       [ True]])
                >>> err_data
                array([[False,  True],
                       [False,  True],
                       [False,  True],
                       [False,  True]])
        )DOC")
            .data());

    c.def(
        "flattened",
        &DetectorErrorModel::flattened,
        clean_doc_string(u8R"DOC(
            Creates an equivalent detector error model without repeat blocks or detector_shift instructions.

            Returns:
                A `stim.DetectorErrorModel` with the same errors in the same order,
                but with loops flattened into repeated instructions and with
                all coordinate/index shifts inlined.

            Examples:
                >>> import stim
                >>> stim.DetectorErrorModel('''
                ...     error(0.125) D0
                ...     REPEAT 5 {
                ...         error(0.25) D0 D1
                ...         shift_detectors 1
                ...     }
                ...     error(0.125) D0 L0
                ... ''').flattened()
                stim.DetectorErrorModel('''
                    error(0.125) D0
                    error(0.25) D0 D1
                    error(0.25) D1 D2
                    error(0.25) D2 D3
                    error(0.25) D3 D4
                    error(0.25) D4 D5
                    error(0.125) D5 L0
                ''')
        )DOC")
            .data());

    c.def(
        "rounded",
        &DetectorErrorModel::rounded,
        clean_doc_string(u8R"DOC(
            Creates an equivalent detector error model but with rounded error probabilities.

            Args:
                digits: The number of digits to round to.

            Returns:
                A `stim.DetectorErrorModel` with the same instructions in the same order,
                but with the parens arguments of error instructions rounded to the given
                precision.

                Instructions whose error probability was rounded to zero are still
                included in the output.

            Examples:
                >>> import stim
                >>> dem = stim.DetectorErrorModel('''
                ...     error(0.019499) D0
                ...     error(0.000001) D0 D1
                ... ''')

                >>> dem.rounded(2)
                stim.DetectorErrorModel('''
                    error(0.02) D0
                    error(0) D0 D1
                ''')

                >>> dem.rounded(3)
                stim.DetectorErrorModel('''
                    error(0.019) D0
                    error(0) D0 D1
                ''')
        )DOC")
            .data());
}
