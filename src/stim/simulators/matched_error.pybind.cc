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

#include "stim/simulators/matched_error.pybind.h"

#include "stim/circuit/gate_target.h"
#include "stim/circuit/gate_target.pybind.h"
#include "stim/dem/detector_error_model_target.pybind.h"
#include "stim/gates/gates.h"
#include "stim/py/base.pybind.h"
#include "stim/simulators/matched_error.h"
#include "stim/util_bot/str_util.h"

using namespace stim;
using namespace stim_pybind;

std::string CircuitErrorLocationStackFrame_repr(const CircuitErrorLocationStackFrame &self) {
    std::stringstream out;
    out << "stim.CircuitErrorLocationStackFrame(";
    out << "\n    instruction_offset=" << self.instruction_offset << ",";
    out << "\n    iteration_index=" << self.iteration_index << ",";
    out << "\n    instruction_repetitions_arg=" << self.instruction_repetitions_arg << ",";
    out << "\n)";
    return out.str();
}

std::string DemTargetWithCoords_repr(const DemTargetWithCoords &self) {
    std::stringstream out;
    out << "stim.DemTargetWithCoords";
    out << "(dem_target=" << ExposedDemTarget(self.dem_target).repr();
    out << ", coords=[" << comma_sep(self.coords) << "]";
    out << ")";
    return out.str();
}

pybind11::ssize_t CircuitTargetsInsideInstruction_hash(const CircuitTargetsInsideInstruction &self) {
    return pybind11::hash(
        pybind11::make_tuple(
            "CircuitTargetsInsideInstruction",
            self.gate_type == GateType::NOT_A_GATE ? std::string_view("") : GATE_DATA[self.gate_type].name,
            self.target_range_start,
            self.target_range_end,
            tuple_tree(self.targets_in_range),
            tuple_tree(self.args)));
}

std::string GateTargetWithCoords_repr(const GateTargetWithCoords &self) {
    std::stringstream out;
    out << "stim.GateTargetWithCoords";
    out << "(" << self.gate_target;
    out << ", [" << comma_sep(self.coords) << "]";
    out << ")";
    return out.str();
}

std::string FlippedMeasurement_repr(const FlippedMeasurement &self) {
    std::stringstream out;
    out << "stim.FlippedMeasurement(";
    out << "\n    record_index=";
    if (self.measurement_record_index == UINT64_MAX) {
        out << "None";
    } else {
        out << self.measurement_record_index;
    }
    out << ",\n    observable=(";
    for (const auto &e : self.measured_observable) {
        out << GateTargetWithCoords_repr(e) << ",";
    }
    out << "),\n)";
    return out.str();
}

std::string CircuitTargetsInsideInstruction_repr(const CircuitTargetsInsideInstruction &self) {
    std::stringstream out;
    out << "stim.CircuitTargetsInsideInstruction";
    out << "(gate='" << (self.gate_type == GateType::NOT_A_GATE ? "NULL" : GATE_DATA[self.gate_type].name) << "'";
    out << ", args=[" << comma_sep(self.args) << "]";
    out << ", target_range_start=" << self.target_range_start;
    out << ", target_range_end=" << self.target_range_end;
    out << ", targets_in_range=(";
    for (const auto &e : self.targets_in_range) {
        out << GateTargetWithCoords_repr(e) << ",";
    }
    out << "))";
    return out.str();
}

std::string CircuitErrorLocation_repr(const CircuitErrorLocation &self) {
    std::stringstream out;
    out << "stim.CircuitErrorLocation";
    out << "(tick_offset=" << self.tick_offset;

    out << ", flipped_pauli_product=(";
    for (const auto &e : self.flipped_pauli_product) {
        out << GateTargetWithCoords_repr(e) << ",";
    }
    out << ")";

    out << ", flipped_measurement=" << FlippedMeasurement_repr(self.flipped_measurement);
    out << ", instruction_targets=" << CircuitTargetsInsideInstruction_repr(self.instruction_targets);

    out << ", stack_frames=(";
    for (const auto &e : self.stack_frames) {
        out << CircuitErrorLocationStackFrame_repr(e) << ",";
    }
    out << ")";

    out << ")";
    return out.str();
}

std::string MatchedError_repr(const ExplainedError &self) {
    std::stringstream out;
    out << "stim.ExplainedError";
    out << "(dem_error_terms=(";
    for (const auto &e : self.dem_error_terms) {
        out << DemTargetWithCoords_repr(e) << ",";
    }
    out << ")";

    out << ", circuit_error_locations=(";
    for (const auto &e : self.circuit_error_locations) {
        out << CircuitErrorLocation_repr(e) << ",";
    }
    out << ")";

    out << ")";
    return out.str();
}

pybind11::class_<CircuitErrorLocationStackFrame> stim_pybind::pybind_circuit_error_location_stack_frame(
    pybind11::module &m) {
    return pybind11::class_<CircuitErrorLocationStackFrame>(
        m,
        "CircuitErrorLocationStackFrame",
        clean_doc_string(R"DOC(
            Describes the location of an instruction being executed within a
            circuit or loop, distinguishing between separate loop iterations.

            The full location of an instruction is a list of these frames,
            drilling down from the top level circuit to the inner-most loop
            that the instruction is within.


            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     REPEAT 5 {
                ...         R 0
                ...         Y_ERROR(0.125) 0
                ...         M 0
                ...     }
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].circuit_error_locations[0].stack_frames[0]
                stim.CircuitErrorLocationStackFrame(
                    instruction_offset=0,
                    iteration_index=0,
                    instruction_repetitions_arg=5,
                )
                >>> err[0].circuit_error_locations[0].stack_frames[1]
                stim.CircuitErrorLocationStackFrame(
                    instruction_offset=1,
                    iteration_index=4,
                    instruction_repetitions_arg=0,
                )
            )DOC")
            .data());
}
void stim_pybind::pybind_circuit_error_location_stack_frame_methods(
    pybind11::module &m, pybind11::class_<stim::CircuitErrorLocationStackFrame> &c) {
    c.def_readonly(
        "instruction_offset",
        &CircuitErrorLocationStackFrame::instruction_offset,
        clean_doc_string(R"DOC(
            The index of the instruction within the circuit, or within the
            instruction's parent REPEAT block. This is slightly different
            from the line number, because blank lines and commented lines
            don't count and also because the offset of the first instruction
            is 0 instead of 1.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0
                ...     TICK
                ...     Y_ERROR(0.125) 0
                ...     M 0
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].circuit_error_locations[0].stack_frames[0].instruction_offset
                2
        )DOC")
            .data());

    c.def_readonly(
        "iteration_index",
        &CircuitErrorLocationStackFrame::iteration_index,
        clean_doc_string(R"DOC(
            Disambiguates which iteration of the loop containing this instruction
            is being referred to. If the instruction isn't in a REPEAT block, this
            field defaults to 0.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     REPEAT 5 {
                ...         R 0
                ...         Y_ERROR(0.125) 0
                ...         M 0
                ...     }
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> full = err[0].circuit_error_locations[0].stack_frames[0]
                >>> loop = err[0].circuit_error_locations[0].stack_frames[1]
                >>> full.iteration_index
                0
                >>> loop.iteration_index
                4
        )DOC")
            .data());

    c.def_readonly(
        "instruction_repetitions_arg",
        &CircuitErrorLocationStackFrame::instruction_repetitions_arg,
        clean_doc_string(R"DOC(
            If the instruction being referred to is a REPEAT block,
            this is the repetition count of that REPEAT block. Otherwise
            this field defaults to 0.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     REPEAT 5 {
                ...         R 0
                ...         Y_ERROR(0.125) 0
                ...         M 0
                ...     }
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> full = err[0].circuit_error_locations[0].stack_frames[0]
                >>> loop = err[0].circuit_error_locations[0].stack_frames[1]
                >>> full.instruction_repetitions_arg
                5
                >>> loop.instruction_repetitions_arg
                0
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self);
    c.def(pybind11::self != pybind11::self);
    c.def("__hash__", [](const CircuitErrorLocationStackFrame &self) {
        return pybind11::hash(
            pybind11::make_tuple(
                "CircuitErrorLocationStackFrame",
                self.instruction_offset,
                self.iteration_index,
                self.instruction_repetitions_arg));
    });
    c.def(
        pybind11::init(
            [](uint64_t instruction_offset,
               uint64_t iteration_index,
               uint64_t instruction_repetitions_arg) -> CircuitErrorLocationStackFrame {
                return CircuitErrorLocationStackFrame{instruction_offset, iteration_index, instruction_repetitions_arg};
            }),
        pybind11::kw_only(),
        pybind11::arg("instruction_offset"),
        pybind11::arg("iteration_index"),
        pybind11::arg("instruction_repetitions_arg"),
        clean_doc_string(R"DOC(
            Creates a stim.CircuitErrorLocationStackFrame.

            Examples:
                >>> import stim
                >>> frame = stim.CircuitErrorLocationStackFrame(
                ...     instruction_offset=1,
                ...     iteration_index=2,
                ...     instruction_repetitions_arg=3,
                ... )
        )DOC")
            .data());
    c.def("__str__", &CircuitErrorLocationStackFrame_repr);
    c.def("__repr__", &CircuitErrorLocationStackFrame_repr);
}

pybind11::class_<GateTargetWithCoords> stim_pybind::pybind_gate_target_with_coords(pybind11::module &m) {
    return pybind11::class_<GateTargetWithCoords>(
        m,
        "GateTargetWithCoords",
        clean_doc_string(R"DOC(
            A gate target with associated coordinate information.

            For example, if the gate target is a qubit from a circuit with
            QUBIT_COORDS instructions, the coords field will contain the
            coordinate data from the QUBIT_COORDS instruction for the qubit.

            This is helpful information to have available when debugging a
            problem in a circuit, instead of having to constantly manually
            look up the coordinates of a qubit index in order to understand
            what is happening.

            Examples:
                >>> import stim
                >>> t = stim.GateTargetWithCoords(0, [1.5, 2.0])
                >>> t.gate_target
                stim.GateTarget(0)
                >>> t.coords
                [1.5, 2.0]
        )DOC")
            .data());
}

void stim_pybind::pybind_gate_target_with_coords_methods(
    pybind11::module &m, pybind11::class_<stim::GateTargetWithCoords> &c) {
    c.def_readonly(
        "gate_target",
        &GateTargetWithCoords::gate_target,
        clean_doc_string(R"DOC(
            Returns the actual gate target as a `stim.GateTarget`.

            Examples:
                >>> import stim
                >>> t = stim.GateTargetWithCoords(0, [1.5, 2.0])
                >>> t.gate_target
                stim.GateTarget(0)
        )DOC")
            .data());

    c.def_readonly(
        "coords",
        &GateTargetWithCoords::coords,
        clean_doc_string(R"DOC(
            Returns the associated coordinate information as a list of floats.

            If there is no coordinate information, returns an empty list.

            Examples:
                >>> import stim
                >>> t = stim.GateTargetWithCoords(0, [1.5, 2.0])
                >>> t.coords
                [1.5, 2.0]
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self);
    c.def(pybind11::self != pybind11::self);
    c.def("__hash__", [](const GateTargetWithCoords &self) {
        return pybind11::hash(pybind11::make_tuple("GateTargetWithCoords", self.gate_target, tuple_tree(self.coords)));
    });
    c.def("__str__", &GateTargetWithCoords::str);
    c.def(
        pybind11::init(
            [](const pybind11::object &gate_target, const std::vector<double> &coords) -> GateTargetWithCoords {
                return GateTargetWithCoords{obj_to_gate_target(gate_target), coords};
            }),
        pybind11::arg("gate_target"),
        pybind11::arg("coords"),
        clean_doc_string(R"DOC(
            Creates a stim.GateTargetWithCoords.

            Examples:
                >>> import stim
                >>> t = stim.GateTargetWithCoords(0, [1.5, 2.0])
                >>> t.gate_target
                stim.GateTarget(0)
                >>> t.coords
                [1.5, 2.0]
        )DOC")
            .data());
    c.def("__repr__", &GateTargetWithCoords_repr);
}

pybind11::class_<DemTargetWithCoords> stim_pybind::pybind_dem_target_with_coords(pybind11::module &m) {
    return pybind11::class_<DemTargetWithCoords>(
        m,
        "DemTargetWithCoords",
        clean_doc_string(R"DOC(
            A detector error model instruction target with associated coords.

            It is also guaranteed that, if the type of the DEM target is a
            relative detector id, it is actually absolute (i.e. relative to
            0).

            For example, if the DEM target is a detector from a circuit with
            coordinate arguments given to detectors, the coords field will
            contain the coordinate data for the detector.

            This is helpful information to have available when debugging a
            problem in a circuit, instead of having to constantly manually
            look up the coordinates of a detector index in order to understand
            what is happening.

            Examples:
                >>> import stim
                >>> t = stim.DemTargetWithCoords(stim.DemTarget("D1"), [1.5, 2.0])
                >>> t.dem_target
                stim.DemTarget('D1')
                >>> t.coords
                [1.5, 2.0]
        )DOC")
            .data());
}

void stim_pybind::pybind_dem_target_with_coords_methods(
    pybind11::module &m, pybind11::class_<stim::DemTargetWithCoords> &c) {
    c.def_property_readonly(
        "dem_target",
        [](const DemTargetWithCoords &self) -> ExposedDemTarget {
            return ExposedDemTarget(self.dem_target);
        },
        clean_doc_string(R"DOC(
            Returns the actual DEM target as a `stim.DemTarget`.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0 1
                ...     X_ERROR(0.25) 0 1
                ...     M 0 1
                ...     DETECTOR(2, 3) rec[-1] rec[-2]
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].dem_error_terms[0].dem_target
                stim.DemTarget('D0')
        )DOC")
            .data());

    c.def_readonly(
        "coords",
        &DemTargetWithCoords::coords,
        clean_doc_string(R"DOC(
            Returns the associated coordinate information as a list of floats.

            If there is no coordinate information, returns an empty list.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0 1
                ...     X_ERROR(0.25) 0 1
                ...     M 0 1
                ...     DETECTOR(2, 3) rec[-1] rec[-2]
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].dem_error_terms[0].coords
                [2.0, 3.0]
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self);
    c.def(pybind11::self != pybind11::self);
    c.def("__hash__", [](const DemTargetWithCoords &self) {
        return pybind11::hash(
            pybind11::make_tuple("DemTargetWithCoords", self.dem_target.data, tuple_tree(self.coords)));
    });
    c.def("__str__", &DemTargetWithCoords::str);
    c.def(
        pybind11::init(
            [](const ExposedDemTarget &dem_target, const std::vector<double> &coords) -> DemTargetWithCoords {
                return DemTargetWithCoords{dem_target, coords};
            }),
        pybind11::arg("dem_target"),
        pybind11::arg("coords"),
        clean_doc_string(R"DOC(
            Creates a stim.DemTargetWithCoords.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0 1
                ...     X_ERROR(0.25) 0 1
                ...     M 0 1
                ...     DETECTOR(2, 3) rec[-1] rec[-2]
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].dem_error_terms[0]
                stim.DemTargetWithCoords(dem_target=stim.DemTarget('D0'), coords=[2, 3])
        )DOC")
            .data());
    c.def("__repr__", DemTargetWithCoords_repr);
}

pybind11::class_<FlippedMeasurement> stim_pybind::pybind_flipped_measurement(pybind11::module &m) {
    return pybind11::class_<FlippedMeasurement>(
        m,
        "FlippedMeasurement",
        clean_doc_string(R"DOC(
            Describes a measurement that was flipped.

            Gives the measurement's index in the measurement record, and also
            the observable of the measurement.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     M(0.25) 1 10
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].circuit_error_locations[0].flipped_measurement
                stim.FlippedMeasurement(
                    record_index=1,
                    observable=(stim.GateTargetWithCoords(stim.target_z(10), []),),
                )
        )DOC")
            .data());
}
void stim_pybind::pybind_flipped_measurement_methods(
    pybind11::module &m, pybind11::class_<stim::FlippedMeasurement> &c) {
    c.def_readonly(
        "record_index",
        &FlippedMeasurement::measurement_record_index,
        clean_doc_string(R"DOC(
            The measurement record index of the flipped measurement.
            For example, the fifth measurement in a circuit has a measurement
            record index of 4.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     M(0.25) 1 10
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].circuit_error_locations[0].flipped_measurement.record_index
                1
        )DOC")
            .data());

    c.def_readonly(
        "observable",
        &FlippedMeasurement::measured_observable,
        clean_doc_string(R"DOC(
            Returns the observable of the flipped measurement.

            For example, an `MX 5` measurement will have the observable X5.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     M(0.25) 1 10
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].circuit_error_locations[0].flipped_measurement.observable
                [stim.GateTargetWithCoords(stim.target_z(10), [])]
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self);
    c.def(pybind11::self != pybind11::self);
    c.def("__hash__", [](const FlippedMeasurement &self) {
        return pybind11::hash(
            pybind11::make_tuple(
                "FlippedMeasurement", self.measurement_record_index, tuple_tree(self.measured_observable)));
    });
    c.def(
        pybind11::init(
            [](const pybind11::object &measurement_record_index,
               const pybind11::object &measured_observable) -> FlippedMeasurement {
                uint64_t u;
                if (measurement_record_index.is_none()) {
                    u = UINT64_MAX;
                } else {
                    u = pybind11::cast<uint64_t>(measurement_record_index);
                }
                FlippedMeasurement result{u, {}};
                for (const auto &e : measured_observable) {
                    result.measured_observable.push_back(pybind11::cast<GateTargetWithCoords>(e));
                }
                return result;
            }),
        pybind11::kw_only(),
        pybind11::arg("record_index"),
        pybind11::arg("observable"),
        clean_doc_string(R"DOC(
            @signature def __init__(self, measurement_record_index: Optional[int], measured_observable: Iterable[stim.GateTargetWithCoords]):
            Creates a stim.FlippedMeasurement.

            Examples:
                >>> import stim
                >>> print(stim.FlippedMeasurement(
                ...     record_index=5,
                ...     observable=[],
                ... ))
                stim.FlippedMeasurement(
                    record_index=5,
                    observable=(),
                )
        )DOC")
            .data());
    c.def("__repr__", &FlippedMeasurement_repr);
    c.def("__str__", &FlippedMeasurement_repr);
}

pybind11::class_<CircuitTargetsInsideInstruction> stim_pybind::pybind_circuit_targets_inside_instruction(
    pybind11::module &m) {
    return pybind11::class_<CircuitTargetsInsideInstruction>(
        m,
        "CircuitTargetsInsideInstruction",
        clean_doc_string(R"DOC(
            Describes a range of targets within a circuit instruction.
        )DOC")
            .data());
}

void stim_pybind::pybind_circuit_targets_inside_instruction_methods(
    pybind11::module &m, pybind11::class_<stim::CircuitTargetsInsideInstruction> &c) {
    c.def_property_readonly(
        "gate",
        [](const CircuitTargetsInsideInstruction &self) -> pybind11::object {
            if (self.gate_type == GateType::NOT_A_GATE) {
                return pybind11::none();
            }
            return pybind11::str(GATE_DATA[self.gate_type].name);
        },
        clean_doc_string(R"DOC(
            Returns the name of the gate / instruction that was being executed.
            @signature def gate(self) -> Optional[str]:

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0 1
                ...     X_ERROR(0.25) 0 1
                ...     M 0 1
                ...     DETECTOR(2, 3) rec[-1] rec[-2]
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> loc: stim.CircuitErrorLocation = err[0].circuit_error_locations[0]
                >>> loc.instruction_targets.gate
                'X_ERROR'
        )DOC")
            .data());

    c.def_property_readonly(
        "tag",
        [](const CircuitTargetsInsideInstruction &self) {
            return self.gate_tag;
        },
        clean_doc_string(R"DOC(
            Returns the tag of the gate / instruction that was being executed.
            @signature def tag(self) -> str:

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0 1
                ...     X_ERROR[look-at-me-imma-tag](0.25) 0 1
                ...     M 0 1
                ...     DETECTOR(2, 3) rec[-1] rec[-2]
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> loc: stim.CircuitErrorLocation = err[0].circuit_error_locations[0]
                >>> loc.instruction_targets.tag
                'look-at-me-imma-tag'
        )DOC")
            .data());

    c.def_readonly(
        "target_range_start",
        &CircuitTargetsInsideInstruction::target_range_start,
        clean_doc_string(R"DOC(
            Returns the inclusive start of the range of targets that were executing
            within the gate / instruction.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0 1
                ...     X_ERROR(0.25) 0 1
                ...     M 0 1
                ...     DETECTOR(2, 3) rec[-1] rec[-2]
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> loc: stim.CircuitErrorLocation = err[0].circuit_error_locations[0]
                >>> loc.instruction_targets.target_range_start
                0
                >>> loc.instruction_targets.target_range_end
                1
        )DOC")
            .data());

    c.def_readonly(
        "target_range_end",
        &CircuitTargetsInsideInstruction::target_range_end,
        clean_doc_string(R"DOC(
            Returns the exclusive end of the range of targets that were executing
            within the gate / instruction.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0 1
                ...     X_ERROR(0.25) 0 1
                ...     M 0 1
                ...     DETECTOR(2, 3) rec[-1] rec[-2]
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> loc: stim.CircuitErrorLocation = err[0].circuit_error_locations[0]
                >>> loc.instruction_targets.target_range_start
                0
                >>> loc.instruction_targets.target_range_end
                1
        )DOC")
            .data());

    c.def_readonly(
        "args",
        &CircuitTargetsInsideInstruction::args,
        clean_doc_string(R"DOC(
            Returns parens arguments of the gate / instruction that was being executed.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0 1
                ...     X_ERROR(0.25) 0 1
                ...     M 0 1
                ...     DETECTOR(2, 3) rec[-1] rec[-2]
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> loc: stim.CircuitErrorLocation = err[0].circuit_error_locations[0]
                >>> loc.instruction_targets.args
                [0.25]
        )DOC")
            .data());

    c.def_readonly(
        "targets_in_range",
        &CircuitTargetsInsideInstruction::targets_in_range,
        clean_doc_string(R"DOC(
            Returns the subset of targets of the gate/instruction that were being executed.

            Includes coordinate data with the targets.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0 1
                ...     X_ERROR(0.25) 0 1
                ...     M 0 1
                ...     DETECTOR(2, 3) rec[-1] rec[-2]
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> loc: stim.CircuitErrorLocation = err[0].circuit_error_locations[0]
                >>> loc.instruction_targets.targets_in_range
                [stim.GateTargetWithCoords(0, [])]
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self);
    c.def(pybind11::self != pybind11::self);
    c.def("__hash__", &CircuitTargetsInsideInstruction_hash);
    c.def(
        pybind11::init(
            [](std::string_view gate,
               std::string_view tag,
               const std::vector<double> &args,
               size_t target_range_start,
               size_t target_range_end,
               const std::vector<GateTargetWithCoords> &targets_in_range) -> CircuitTargetsInsideInstruction {
                CircuitTargetsInsideInstruction result{
                    GATE_DATA.at(gate).id, std::string(tag), args, target_range_start, target_range_end, targets_in_range};
                return result;
            }),
        pybind11::kw_only(),
        pybind11::arg("gate"),
        pybind11::arg("tag") = "",
        pybind11::arg("args"),
        pybind11::arg("target_range_start"),
        pybind11::arg("target_range_end"),
        pybind11::arg("targets_in_range"),
        clean_doc_string(R"DOC(
            Creates a stim.CircuitTargetsInsideInstruction.

            Examples:
                >>> import stim
                >>> val = stim.CircuitTargetsInsideInstruction(
                ...     gate='X_ERROR',
                ...     tag='',
                ...     args=[0.25],
                ...     target_range_start=0,
                ...     target_range_end=1,
                ...     targets_in_range=[stim.GateTargetWithCoords(0, [])],
                ... )
        )DOC")
            .data());
    c.def("__repr__", &CircuitTargetsInsideInstruction_repr);
    c.def("__str__", &CircuitTargetsInsideInstruction::str);
}

pybind11::class_<CircuitErrorLocation> stim_pybind::pybind_circuit_error_location(pybind11::module &m) {
    return pybind11::class_<CircuitErrorLocation>(
        m,
        "CircuitErrorLocation",
        clean_doc_string(R"DOC(
            Describes the location of an error mechanism from a stim circuit.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit.generated(
                ...     "repetition_code:memory",
                ...     distance=5,
                ...     rounds=5,
                ...     before_round_data_depolarization=1e-3,
                ... )
                >>> logical_error = circuit.shortest_graphlike_error()
                >>> error_location = logical_error[0].circuit_error_locations[0]
                >>> print(error_location)
                CircuitErrorLocation {
                    flipped_pauli_product: X0
                    Circuit location stack trace:
                        (after 1 TICKs)
                        at instruction #3 (DEPOLARIZE1) in the circuit
                        at target #1 of the instruction
                        resolving to DEPOLARIZE1(0.001) 0
                }
            )DOC")
            .data());
}
void stim_pybind::pybind_circuit_error_location_methods(
    pybind11::module &m, pybind11::class_<stim::CircuitErrorLocation> &c) {
    c.def_readonly(
        "tick_offset",
        &CircuitErrorLocation::tick_offset,
        clean_doc_string(R"DOC(
            The number of TICKs that executed before the error happened.

            This counts TICKs occurring multiple times during loops.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0
                ...     TICK
                ...     TICK
                ...     TICK
                ...     Y_ERROR(0.125) 0
                ...     M 0
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].circuit_error_locations[0].tick_offset
                3
        )DOC")
            .data());

    c.def_readonly(
        "noise_tag",
        &CircuitErrorLocation::noise_tag,
        clean_doc_string(R"DOC(
            The tag on the noise instruction that caused the error.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0
                ...     Y_ERROR[test-tag](0.125) 0
                ...     M 0
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].circuit_error_locations[0].noise_tag
                'test-tag'
        )DOC")
            .data());

    c.def_readonly(
        "flipped_pauli_product",
        &CircuitErrorLocation::flipped_pauli_product,
        clean_doc_string(R"DOC(
            The Pauli errors that the error mechanism applied to qubits.

            When the error is a measurement error, this will be an empty list.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0
                ...     Y_ERROR(0.125) 0
                ...     M 0
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].circuit_error_locations[0].flipped_pauli_product
                [stim.GateTargetWithCoords(stim.target_y(0), [])]
        )DOC")
            .data());

    c.def_property_readonly(
        "flipped_measurement",
        [](const CircuitErrorLocation &self) -> pybind11::object {
            if (self.flipped_measurement.measured_observable.empty()) {
                return pybind11::none();
            }
            return pybind11::cast(self.flipped_measurement);
        },
        clean_doc_string(R"DOC(
            @signature def flipped_measurement(self) -> Optional[stim.FlippedMeasurement]:
            The measurement that was flipped by the error mechanism.

            If the error isn't a measurement error, this will be None.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0
                ...     M(0.125) 0
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].circuit_error_locations[0].flipped_measurement
                stim.FlippedMeasurement(
                    record_index=0,
                    observable=(stim.GateTargetWithCoords(stim.target_z(0), []),),
                )

        )DOC")
            .data());

    c.def_readonly(
        "instruction_targets",
        &CircuitErrorLocation::instruction_targets,
        clean_doc_string(R"DOC(
            Within the error instruction, which may have hundreds of
            targets, which specific targets were being executed to
            produce the error.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0
                ...     TICK
                ...     Y_ERROR(0.125) 0
                ...     M 0
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> targets = err[0].circuit_error_locations[0].instruction_targets
                >>> targets == stim.CircuitTargetsInsideInstruction(
                ...     gate='Y_ERROR',
                ...     args=[0.125],
                ...     target_range_start=0,
                ...     target_range_end=1,
                ...     targets_in_range=(stim.GateTargetWithCoords(0, []),),
                ... )
                True
        )DOC")
            .data());

    c.def_readonly(
        "stack_frames",
        &CircuitErrorLocation::stack_frames,
        clean_doc_string(R"DOC(
            Describes where in the circuit's execution the error happened.

            Multiple frames are needed because the error may occur within a loop,
            or a loop nested inside a loop, or etc.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0
                ...     TICK
                ...     Y_ERROR(0.125) 0
                ...     M 0
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> err[0].circuit_error_locations[0].stack_frames
                [stim.CircuitErrorLocationStackFrame(
                    instruction_offset=2,
                    iteration_index=0,
                    instruction_repetitions_arg=0,
                )]
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self);
    c.def(pybind11::self != pybind11::self);
    c.def("__hash__", [](const CircuitErrorLocation &self) {
        return pybind11::hash(
            pybind11::make_tuple(
                "CircuitErrorLocation",
                self.tick_offset,
                tuple_tree(self.flipped_pauli_product),
                self.flipped_measurement,
                self.instruction_targets,
                tuple_tree(self.stack_frames)));
    });
    c.def(
        pybind11::init(
            [](uint64_t tick_offset,
               const std::vector<GateTargetWithCoords> &flipped_pauli_product,
               const pybind11::object &flipped_measurement,
               const CircuitTargetsInsideInstruction &instruction_targets,
               const std::vector<CircuitErrorLocationStackFrame> &stack_frames,
               std::string_view noise_tag) -> CircuitErrorLocation {
                FlippedMeasurement m{0, {}};
                if (!flipped_measurement.is_none()) {
                    m = pybind11::cast<FlippedMeasurement>(flipped_measurement);
                }
                CircuitErrorLocation result{std::string(noise_tag), tick_offset, flipped_pauli_product, m, instruction_targets, stack_frames};
                return result;
            }),
        pybind11::kw_only(),
        pybind11::arg("tick_offset"),
        pybind11::arg("flipped_pauli_product"),
        pybind11::arg("flipped_measurement"),
        pybind11::arg("instruction_targets"),
        pybind11::arg("stack_frames"),
        pybind11::arg("noise_tag") = "",
        clean_doc_string(R"DOC(
            Creates a stim.CircuitErrorLocation.

            Examples:
                >>> import stim
                >>> err = stim.CircuitErrorLocation(
                ...     tick_offset=1,
                ...     flipped_pauli_product=(
                ...         stim.GateTargetWithCoords(
                ...             gate_target=stim.target_x(0),
                ...             coords=[],
                ...         ),
                ...     ),
                ...     flipped_measurement=stim.FlippedMeasurement(
                ...         record_index=None,
                ...         observable=(),
                ...     ),
                ...     instruction_targets=stim.CircuitTargetsInsideInstruction(
                ...         gate='DEPOLARIZE1',
                ...         args=[0.001],
                ...         target_range_start=0,
                ...         target_range_end=1,
                ...         targets_in_range=(stim.GateTargetWithCoords(
                ...             gate_target=0,
                ...             coords=[],
                ...         ),)
                ...     ),
                ...     stack_frames=(
                ...         stim.CircuitErrorLocationStackFrame(
                ...             instruction_offset=2,
                ...             iteration_index=0,
                ...             instruction_repetitions_arg=0,
                ...         ),
                ...     ),
                ...     noise_tag='test-tag',
                ... )
                >>> print(err)
                CircuitErrorLocation {
                    noise_tag: test-tag
                    flipped_pauli_product: X0
                    Circuit location stack trace:
                        (after 1 TICKs)
                        at instruction #3 (DEPOLARIZE1) in the circuit
                        at target #1 of the instruction
                        resolving to DEPOLARIZE1(0.001) 0
                }
        )DOC")
            .data());
    c.def("__repr__", &CircuitErrorLocation_repr);
    c.def("__str__", &CircuitErrorLocation::str);
}

pybind11::class_<ExplainedError> stim_pybind::pybind_explained_error(pybind11::module &m) {
    return pybind11::class_<ExplainedError>(
        m,
        "ExplainedError",
        clean_doc_string(R"DOC(
            Describes the location of an error mechanism from a stim circuit.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0
                ...     TICK
                ...     Y_ERROR(0.125) 0
                ...     M 0
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> print(err[0])
                ExplainedError {
                    dem_error_terms: L0
                    CircuitErrorLocation {
                        flipped_pauli_product: Y0
                        Circuit location stack trace:
                            (after 1 TICKs)
                            at instruction #3 (Y_ERROR) in the circuit
                            at target #1 of the instruction
                            resolving to Y_ERROR(0.125) 0
                    }
                }
        )DOC")
            .data());
}
void stim_pybind::pybind_explained_error_methods(pybind11::module &m, pybind11::class_<stim::ExplainedError> &c) {
    c.def_readonly(
        "dem_error_terms",
        &ExplainedError::dem_error_terms,
        clean_doc_string(R"DOC(
            The detectors and observables flipped by this error mechanism.
        )DOC")
            .data());

    c.def_readonly(
        "circuit_error_locations",
        &ExplainedError::circuit_error_locations,
        clean_doc_string(R"DOC(
            The locations of circuit errors that produce the symptoms in dem_error_terms.

            Note: if this list contains a single entry, it may be because a result
            with a single representative error was requested (as opposed to all possible
            errors).

            Note: if this list is empty, it may be because there was a DEM error decomposed
            into parts where one of the parts is impossible to make on its own from a single
            circuit error.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0
                ...     TICK
                ...     Y_ERROR(0.125) 0
                ...     M 0
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> print(err[0].circuit_error_locations[0])
                CircuitErrorLocation {
                    flipped_pauli_product: Y0
                    Circuit location stack trace:
                        (after 1 TICKs)
                        at instruction #3 (Y_ERROR) in the circuit
                        at target #1 of the instruction
                        resolving to Y_ERROR(0.125) 0
                }
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self);
    c.def(pybind11::self != pybind11::self);
    c.def("__hash__", [](const ExplainedError &self) {
        return pybind11::hash(
            pybind11::make_tuple(
                "ExplainedError", tuple_tree(self.dem_error_terms), tuple_tree(self.circuit_error_locations)));
    });
    c.def(
        pybind11::init(
            [](const std::vector<DemTargetWithCoords> dem_error_terms,
               const std::vector<CircuitErrorLocation> &circuit_error_locations) -> ExplainedError {
                ExplainedError result{
                    dem_error_terms,
                    circuit_error_locations,
                };
                return result;
            }),
        pybind11::kw_only(),
        pybind11::arg("dem_error_terms"),
        pybind11::arg("circuit_error_locations"),
        clean_doc_string(R"DOC(
            Creates a stim.ExplainedError.

            Examples:
                >>> import stim
                >>> err = stim.Circuit('''
                ...     R 0
                ...     TICK
                ...     Y_ERROR(0.125) 0
                ...     M 0
                ...     OBSERVABLE_INCLUDE(0) rec[-1]
                ... ''').shortest_graphlike_error()
                >>> print(err[0])
                ExplainedError {
                    dem_error_terms: L0
                    CircuitErrorLocation {
                        flipped_pauli_product: Y0
                        Circuit location stack trace:
                            (after 1 TICKs)
                            at instruction #3 (Y_ERROR) in the circuit
                            at target #1 of the instruction
                            resolving to Y_ERROR(0.125) 0
                    }
                }
        )DOC")
            .data());
    c.def("__repr__", &MatchedError_repr);
    c.def("__str__", &ExplainedError::str);
}
