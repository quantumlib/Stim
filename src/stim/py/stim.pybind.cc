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

#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>

#include "stim/circuit/circuit.pybind.h"
#include "stim/circuit/circuit_instruction.pybind.h"
#include "stim/circuit/circuit_repeat_block.pybind.h"
#include "stim/circuit/gate_target.pybind.h"
#include "stim/cmd/command_diagram.pybind.h"
#include "stim/dem/dem_instruction.pybind.h"
#include "stim/dem/detector_error_model.pybind.h"
#include "stim/dem/detector_error_model_repeat_block.pybind.h"
#include "stim/dem/detector_error_model_target.pybind.h"
#include "stim/gates/gates.pybind.h"
#include "stim/io/read_write.pybind.h"
#include "stim/main_namespaced.h"
#include "stim/py/base.pybind.h"
#include "stim/py/compiled_detector_sampler.pybind.h"
#include "stim/py/compiled_measurement_sampler.pybind.h"
#include "stim/py/march.pybind.h"
#include "stim/simulators/dem_sampler.pybind.h"
#include "stim/simulators/frame_simulator.pybind.h"
#include "stim/simulators/matched_error.pybind.h"
#include "stim/simulators/measurements_to_detection_events.pybind.h"
#include "stim/simulators/tableau_simulator.pybind.h"
#include "stim/stabilizers/flow.pybind.h"
#include "stim/stabilizers/pauli_string.pybind.h"
#include "stim/stabilizers/pauli_string_iter.pybind.h"
#include "stim/stabilizers/tableau.h"
#include "stim/stabilizers/tableau.pybind.h"
#include "stim/stabilizers/tableau_iter.pybind.h"

#define xstr_literal(s) str_literal(s)
#define str_literal(s) #s

using namespace stim;
using namespace stim_pybind;

GateTarget target_rec(int32_t lookback) {
    return GateTarget::rec(lookback);
}

GateTarget target_inv(const pybind11::object &qubit) {
    if (pybind11::isinstance<GateTarget>(qubit)) {
        return !pybind11::cast<GateTarget>(qubit);
    }
    return GateTarget::qubit(pybind11::cast<uint32_t>(qubit), true);
}

GateTarget target_x(const pybind11::object &qubit, bool invert) {
    if (pybind11::isinstance<GateTarget>(qubit)) {
        auto t = pybind11::cast<GateTarget>(qubit);
        if (!t.is_qubit_target()) {
            throw std::invalid_argument("result of stim.target_x(" + t.str() + ") is not defined");
        }
        return GateTarget::x(t.qubit_value(), t.is_inverted_result_target() ^ invert);
    }
    return GateTarget::x(pybind11::cast<uint32_t>(qubit), invert);
}

GateTarget target_y(const pybind11::object &qubit, bool invert) {
    if (pybind11::isinstance<GateTarget>(qubit)) {
        auto t = pybind11::cast<GateTarget>(qubit);
        if (!t.is_qubit_target()) {
            throw std::invalid_argument("result of stim.target_y(" + t.str() + ") is not defined");
        }
        return GateTarget::y(t.qubit_value(), t.is_inverted_result_target() ^ invert);
    }
    return GateTarget::y(pybind11::cast<uint32_t>(qubit), invert);
}

GateTarget target_z(const pybind11::object &qubit, bool invert) {
    if (pybind11::isinstance<GateTarget>(qubit)) {
        auto t = pybind11::cast<GateTarget>(qubit);
        if (!t.is_qubit_target()) {
            throw std::invalid_argument("result of stim.target_z(" + t.str() + ") is not defined");
        }
        return GateTarget::z(t.qubit_value(), t.is_inverted_result_target() ^ invert);
    }
    return GateTarget::z(pybind11::cast<uint32_t>(qubit), invert);
}

std::vector<GateTarget> target_combined_paulis(const pybind11::object &paulis, bool invert) {
    std::vector<GateTarget> result;
    if (pybind11::isinstance<FlexPauliString>(paulis)) {
        const FlexPauliString &ps = pybind11::cast<const FlexPauliString &>(paulis);
        if (ps.imag) {
            std::stringstream ss;
            ss << "Imaginary sign: paulis=";
            ss << paulis;
            throw std::invalid_argument(ss.str());
        }
        invert ^= ps.value.sign;
        for (size_t q = 0; q < ps.value.num_qubits; q++) {
            bool x = ps.value.xs[q];
            bool z = ps.value.zs[q];
            if (x | z) {
                result.push_back(GateTarget::pauli_xz(q, x, z));
                result.push_back(GateTarget::combiner());
            }
        }
    } else {
        for (const auto &h : paulis) {
            if (pybind11::isinstance<GateTarget>(h)) {
                GateTarget g = pybind11::cast<GateTarget>(h);
                if (g.pauli_type() != 'I') {
                    if (g.is_inverted_result_target()) {
                        invert ^= true;
                        g.data ^= TARGET_INVERTED_BIT;
                    }
                    result.push_back(g);
                    result.push_back(GateTarget::combiner());
                    continue;
                }
            }

            std::stringstream ss;
            ss << "Expected a pauli string or iterable of stim.GateTarget but got this when iterating: ";
            ss << h;
            throw std::invalid_argument(ss.str());
        }
    }

    if (result.empty()) {
        std::stringstream ss;
        ss << "Identity pauli product: paulis=";
        ss << paulis;
        throw std::invalid_argument(ss.str());
    }
    result.pop_back();
    if (invert) {
        result[0].data ^= TARGET_INVERTED_BIT;
    }
    return result;
}

GateTarget target_pauli(uint32_t qubit_index, const pybind11::object &pauli, bool invert) {
    if ((qubit_index & TARGET_VALUE_MASK) != qubit_index) {
        std::stringstream ss;
        ss << "qubit_index=" << qubit_index << " is too large. Maximum qubit index is " << TARGET_VALUE_MASK << ".";
        throw std::invalid_argument(ss.str());
    }
    if (pybind11::isinstance<pybind11::str>(pauli)) {
        std::string_view p = pybind11::cast<std::string_view>(pauli);
        if (p == "X" || p == "x") {
            return GateTarget::x(qubit_index, invert);
        } else if (p == "Y" || p == "y") {
            return GateTarget::y(qubit_index, invert);
        } else if (p == "Z" || p == "z") {
            return GateTarget::z(qubit_index, invert);
        } else if (p == "I") {
            return GateTarget::qubit(qubit_index, invert);
        }
    } else {
        try {
            uint8_t p = pybind11::cast<uint8_t>(pauli);
            if (p == 1) {
                return GateTarget::x(qubit_index, invert);
            } else if (p == 2) {
                return GateTarget::y(qubit_index, invert);
            } else if (p == 3) {
                return GateTarget::z(qubit_index, invert);
            } else if (p == 0) {
                return GateTarget::qubit(qubit_index, invert);
            }
        } catch (const pybind11::cast_error &ex) {
            // Wasn't an integer.
        }
    }

    std::stringstream ss;
    ss << "Expected pauli in [0, 1, 2, 3, *'IXYZxyz'] but got pauli=" << pauli;
    throw std::invalid_argument(ss.str());
}

GateTarget target_sweep_bit(uint32_t qubit) {
    return GateTarget::sweep_bit(qubit);
}

int stim_main(const std::vector<std::string> &args) {
    pybind11::scoped_ostream_redirect redirect_out(std::cout, pybind11::module_::import("sys").attr("stdout"));
    pybind11::scoped_ostream_redirect redirect_err(std::cerr, pybind11::module_::import("sys").attr("stderr"));
    std::vector<const char *> argv;
    argv.push_back("stim.main");
    for (const auto &arg : args) {
        argv.push_back(arg.c_str());
    }
    return stim::main(argv.size(), argv.data());
}

pybind11::object raw_format_data_solo(const FileFormatData &data) {
    pybind11::dict result;
    result["name"] = data.name;
    result["parse_example"] = data.help_python_parse;
    result["save_example"] = data.help_python_save;
    result["help"] = data.help;
    return std::move(result);
}

pybind11::dict raw_format_data() {
    pybind11::dict result;
    for (const auto &kv : format_name_to_enum_map()) {
        result[pybind11::str(kv.first)] = raw_format_data_solo(kv.second);
    }
    return result;
}

void top_level(pybind11::module &m) {
    m.def(
        "target_rec",
        &target_rec,
        pybind11::arg("lookback_index"),
        clean_doc_string(R"DOC(
            Returns a measurement record target with the given lookback.

            Measurement record targets are used to refer back to the measurement record;
            the list of measurements that have been performed so far. Measurement record
            targets always specify an index relative to the *end* of the measurement record.
            The latest measurement is `stim.target_rec(-1)`, the next most recent
            measurement is `stim.target_rec(-2)`, and so forth. Indexing is done this way
            in order to make it possible to write loops.

            Args:
                lookback_index: A negative integer indicating how far to look back, relative
                    to the end of the measurement record.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit()
                >>> circuit.append("M", [5, 7, 11])
                >>> circuit.append("CX", [stim.target_rec(-2), 3])
                >>> circuit
                stim.Circuit('''
                    M 5 7 11
                    CX rec[-2] 3
                ''')
        )DOC")
            .data());

    m.def(
        "target_inv",
        &target_inv,
        pybind11::arg("qubit_index"),
        clean_doc_string(R"DOC(
            @signature def target_inv(qubit_index: Union[int, stim.GateTarget]) -> stim.GateTarget:
            Returns a target flagged as inverted.

            Inverted targets are used to indicate measurement results should be flipped.

            Args:
                qubit_index: The underlying qubit index of the inverted target.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit()
                >>> circuit.append("M", [2, stim.target_inv(3)])
                >>> circuit
                stim.Circuit('''
                    M 2 !3
                ''')

            For example, the '!1' in 'M 0 !1 2' is qubit 1 flagged as inverted,
            meaning the measurement result from qubit 1 should be inverted when reported.
        )DOC")
            .data());

    m.def(
        "target_combiner",
        &GateTarget::combiner,
        clean_doc_string(R"DOC(
            Returns a target combiner that can be used to build Pauli products.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit()
                >>> circuit.append("MPP", [
                ...     stim.target_x(2),
                ...     stim.target_combiner(),
                ...     stim.target_y(3),
                ...     stim.target_combiner(),
                ...     stim.target_z(5),
                ... ])
                >>> circuit
                stim.Circuit('''
                    MPP X2*Y3*Z5
                ''')
        )DOC")
            .data());

    m.def(
        "target_x",
        &target_x,
        pybind11::arg("qubit_index"),
        pybind11::arg("invert") = false,
        clean_doc_string(R"DOC(
            @signature def target_x(qubit_index: Union[int, stim.GateTarget], invert: bool = False) -> stim.GateTarget:
            Returns a Pauli X target that can be passed into `stim.Circuit.append`.

            Args:
                qubit_index: The qubit that the Pauli applies to.
                invert: Defaults to False. If True, the target is inverted (indicating
                    that, for example, measurement results should be inverted).

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit()
                >>> circuit.append("MPP", [
                ...     stim.target_x(2),
                ...     stim.target_combiner(),
                ...     stim.target_y(3, invert=True),
                ...     stim.target_combiner(),
                ...     stim.target_z(5),
                ... ])
                >>> circuit
                stim.Circuit('''
                    MPP X2*!Y3*Z5
                ''')
        )DOC")
            .data());

    m.def(
        "target_y",
        &target_y,
        pybind11::arg("qubit_index"),
        pybind11::arg("invert") = false,
        clean_doc_string(R"DOC(
            @signature def target_y(qubit_index: Union[int, stim.GateTarget], invert: bool = False) -> stim.GateTarget:
            Returns a Pauli Y target that can be passed into `stim.Circuit.append`.

            Args:
                qubit_index: The qubit that the Pauli applies to.
                invert: Defaults to False. If True, the target is inverted (indicating
                    that, for example, measurement results should be inverted).

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit()
                >>> circuit.append("MPP", [
                ...     stim.target_x(2),
                ...     stim.target_combiner(),
                ...     stim.target_y(3, invert=True),
                ...     stim.target_combiner(),
                ...     stim.target_z(5),
                ... ])
                >>> circuit
                stim.Circuit('''
                    MPP X2*!Y3*Z5
                ''')
        )DOC")
            .data());

    m.def(
        "target_z",
        &target_z,
        pybind11::arg("qubit_index"),
        pybind11::arg("invert") = false,
        clean_doc_string(R"DOC(
            @signature def target_z(qubit_index: Union[int, stim.GateTarget], invert: bool = False) -> stim.GateTarget:
            Returns a Pauli Z target that can be passed into `stim.Circuit.append`.

            Args:
                qubit_index: The qubit that the Pauli applies to.
                invert: Defaults to False. If True, the target is inverted (indicating
                    that, for example, measurement results should be inverted).

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit()
                >>> circuit.append("MPP", [
                ...     stim.target_x(2),
                ...     stim.target_combiner(),
                ...     stim.target_y(3, invert=True),
                ...     stim.target_combiner(),
                ...     stim.target_z(5),
                ... ])
                >>> circuit
                stim.Circuit('''
                    MPP X2*!Y3*Z5
                ''')
        )DOC")
            .data());

    m.def(
        "target_pauli",
        &target_pauli,
        pybind11::arg("qubit_index"),
        pybind11::arg("pauli"),
        pybind11::arg("invert") = false,
        clean_doc_string(R"DOC(
            @signature def target_pauli(qubit_index: int, pauli: Union[str, int], invert: bool = False) -> stim.GateTarget:
            Returns a pauli target that can be passed into `stim.Circuit.append`.

            Args:
                qubit_index: The qubit that the Pauli applies to.
                pauli: The pauli gate to use. This can either be a string identifying the
                    pauli by name ("x", "X", "y", "Y", "z", or "Z") or an integer following
                    the convention (1=X, 2=Y, 3=Z). Setting this argument to "I" or to
                    0 will return a qubit target instead of a pauli target.
                invert: Defaults to False. If True, the target is inverted (like "!X10"),
                    indicating that, for example, measurement results should be inverted).

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit()
                >>> circuit.append("MPP", [
                ...     stim.target_pauli(2, "X"),
                ...     stim.target_combiner(),
                ...     stim.target_pauli(3, "y", invert=True),
                ...     stim.target_pauli(5, 3),
                ... ])
                >>> circuit
                stim.Circuit('''
                    MPP X2*!Y3 Z5
                ''')

                >>> circuit.append("M", [
                ...     stim.target_pauli(7, "I"),
                ... ])
                >>> circuit
                stim.Circuit('''
                    MPP X2*!Y3 Z5
                    M 7
                ''')
        )DOC")
            .data());

    m.def(
        "target_combined_paulis",
        &target_combined_paulis,
        pybind11::arg("paulis"),
        pybind11::arg("invert") = false,
        clean_doc_string(R"DOC(
            @signature def target_combined_paulis(paulis: Union[stim.PauliString, List[stim.GateTarget]], invert: bool = False) -> stim.GateTarget:
            Returns a list of targets encoding a pauli product for instructions like MPP.

            Args:
                paulis: The paulis to encode into the targets. This can be a
                    `stim.PauliString` or a list of pauli targets from `stim.target_x`,
                    `stim.target_pauli`, etc.
                invert: Defaults to False. If True, the product is inverted (like "!X2*Y3").
                    Note that this is in addition to any inversions specified by the
                    `paulis` argument.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit()
                >>> circuit.append("MPP", [
                ...     *stim.target_combined_paulis(stim.PauliString("-XYZ")),
                ...     *stim.target_combined_paulis([stim.target_x(2), stim.target_y(5)]),
                ...     *stim.target_combined_paulis([stim.target_z(9)], invert=True),
                ... ])
                >>> circuit
                stim.Circuit('''
                    MPP !X0*Y1*Z2 X2*Y5 !Z9
                ''')
        )DOC")
            .data());

    m.def(
        "target_sweep_bit",
        &target_sweep_bit,
        pybind11::arg("sweep_bit_index"),
        clean_doc_string(R"DOC(
            Returns a sweep bit target that can be passed into `stim.Circuit.append`.

            Args:
                sweep_bit_index: The index of the sweep bit to target.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit()
                >>> circuit.append("CX", [stim.target_sweep_bit(2), 5])
                >>> circuit
                stim.Circuit('''
                    CX sweep[2] 5
                ''')
        )DOC")
            .data());

    m.def(
        "main",
        &stim_main,
        pybind11::kw_only(),
        pybind11::arg("command_line_args"),
        clean_doc_string(R"DOC(
            Runs the command line tool version of stim on the given arguments.

            Note that by default any input will be read from stdin, any output
            will print to stdout (as opposed to being intercepted). For most
            commands, you can use arguments like `--out` to write to a file
            instead of stdout and `--in` to read from a file instead of stdin.

            Returns:
                An exit code (0 means success, not zero means failure).

            Raises:
                A large variety of errors, depending on what you are doing and
                how it failed! Beware that many errors are caught by the main
                method itself and printed to stderr, with the only indication
                that something went wrong being the return code.

            Example:
                >>> import stim
                >>> import tempfile
                >>> with tempfile.TemporaryDirectory() as d:
                ...     path = f'{d}/tmp.out'
                ...     return_code = stim.main(command_line_args=[
                ...         "gen",
                ...         "--code=repetition_code",
                ...         "--task=memory",
                ...         "--rounds=1000",
                ...         "--distance=2",
                ...         "--out",
                ...         path,
                ...     ])
                ...     assert return_code == 0
                ...     with open(path) as f:
                ...         print(f.read(), end='')
                # Generated repetition_code circuit.
                # task: memory
                # rounds: 1000
                # distance: 2
                # before_round_data_depolarization: 0
                # before_measure_flip_probability: 0
                # after_reset_flip_probability: 0
                # after_clifford_depolarization: 0
                # layout:
                # L0 Z1 d2
                # Legend:
                #     d# = data qubit
                #     L# = data qubit with logical observable crossing
                #     Z# = measurement qubit
                R 0 1 2
                TICK
                CX 0 1
                TICK
                CX 2 1
                TICK
                MR 1
                DETECTOR(1, 0) rec[-1]
                REPEAT 999 {
                    TICK
                    CX 0 1
                    TICK
                    CX 2 1
                    TICK
                    MR 1
                    SHIFT_COORDS(0, 1)
                    DETECTOR(1, 0) rec[-1] rec[-2]
                }
                M 0 2
                DETECTOR(1, 1) rec[-1] rec[-2] rec[-3]
                OBSERVABLE_INCLUDE(0) rec[-1]
        )DOC")
            .data());

    m.def("_UNSTABLE_raw_format_data", &raw_format_data);
}

PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.attr("__version__") = xstr_literal(VERSION_INFO);
    m.doc() = R"pbdoc(
        Stim: A fast stabilizer circuit library.
    )pbdoc";

    /// class registration happens before function/method
    /// registration. If a class references another before it is
    /// registered, method signatures can get messed up.  For example,
    /// if DetectorErrorModel is defined after Circuit then
    /// Circuit.detector_error_model's return type is described as
    /// `stim::DetectorErrorModel` instead of `stim.DetectorErrorModel`.

    /// class definitions
    auto c_dem_sampler = pybind_dem_sampler(m);
    auto c_compiled_detector_sampler = pybind_compiled_detector_sampler(m);
    auto c_compiled_measurement_sampler = pybind_compiled_measurement_sampler(m);
    auto c_compiled_m2d_converter = pybind_compiled_measurements_to_detection_events_converter(m);
    auto c_pauli_string = pybind_pauli_string(m);
    auto c_pauli_string_iter = pybind_pauli_string_iter(m);
    auto c_tableau = pybind_tableau(m);
    auto c_tableau_iter = pybind_tableau_iter(m);

    auto c_circuit_gate_target = pybind_circuit_gate_target(m);
    auto c_gate_data = pybind_gate_data(m);
    auto c_circuit_instruction = pybind_circuit_instruction(m);
    auto c_circuit_repeat_block = pybind_circuit_repeat_block(m);
    auto c_circuit = pybind_circuit(m);

    auto c_detector_error_model_instruction = pybind_detector_error_model_instruction(m);
    auto c_detector_error_model_target = pybind_detector_error_model_target(m);
    auto c_detector_error_model_repeat_block = pybind_detector_error_model_repeat_block(m);
    auto c_detector_error_model = pybind_detector_error_model(m);

    auto c_tableau_simulator = pybind_tableau_simulator(m);
    auto c_frame_simulator = pybind_frame_simulator(m);

    auto c_circuit_error_location_stack_frame = pybind_circuit_error_location_stack_frame(m);
    auto c_gate_target_with_coords = pybind_gate_target_with_coords(m);
    auto c_dem_target_with_coords = pybind_dem_target_with_coords(m);
    auto c_flipped_measurement = pybind_flipped_measurement(m);
    auto c_circuit_targets_inside_instruction = pybind_circuit_targets_inside_instruction(m);
    auto c_circuit_error_location = pybind_circuit_error_location(m);
    auto c_circuit_error_location_methods = pybind_explained_error(m);
    auto c_flow = pybind_flow(m);

    auto c_diagram_helper = pybind_diagram(m);

    /// top level function definitions
    top_level(m);
    pybind_read_write(m);

    // method definitions
    pybind_circuit_instruction_methods(m, c_circuit_instruction);
    pybind_circuit_gate_target_methods(m, c_circuit_gate_target);
    pybind_gate_data_methods(m, c_gate_data);
    pybind_circuit_repeat_block_methods(m, c_circuit_repeat_block);
    pybind_circuit_methods(m, c_circuit);
    pybind_circuit_methods_extra(m, c_circuit);

    pybind_tableau_iter_methods(m, c_tableau_iter);
    pybind_dem_sampler_methods(m, c_dem_sampler);

    pybind_detector_error_model_instruction_methods(m, c_detector_error_model_instruction);
    pybind_detector_error_model_repeat_block_methods(m, c_detector_error_model_repeat_block);
    pybind_detector_error_model_target_methods(m, c_detector_error_model_target);
    pybind_detector_error_model_methods(m, c_detector_error_model);

    pybind_tableau_methods(m, c_tableau);
    pybind_pauli_string_methods(m, c_pauli_string);
    pybind_pauli_string_iter_methods(m, c_pauli_string_iter);

    pybind_compiled_detector_sampler_methods(m, c_compiled_detector_sampler);
    pybind_compiled_measurement_sampler_methods(m, c_compiled_measurement_sampler);
    pybind_compiled_measurements_to_detection_events_converter_methods(m, c_compiled_m2d_converter);

    pybind_tableau_simulator_methods(m, c_tableau_simulator);
    pybind_frame_simulator_methods(m, c_frame_simulator);

    pybind_circuit_error_location_stack_frame_methods(m, c_circuit_error_location_stack_frame);
    pybind_gate_target_with_coords_methods(m, c_gate_target_with_coords);
    pybind_dem_target_with_coords_methods(m, c_dem_target_with_coords);
    pybind_flipped_measurement_methods(m, c_flipped_measurement);
    pybind_circuit_targets_inside_instruction_methods(m, c_circuit_targets_inside_instruction);
    pybind_circuit_error_location_methods(m, c_circuit_error_location);
    pybind_explained_error_methods(m, c_circuit_error_location_methods);
    pybind_flow_methods(m, c_flow);

    pybind_diagram_methods(m, c_diagram_helper);
}
