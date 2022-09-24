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

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "stim/circuit/circuit_instruction.pybind.h"
#include "stim/circuit/circuit_gate_target.pybind.h"
#include "stim/circuit/circuit_repeat_block.pybind.h"
#include "stim/circuit/circuit.pybind.h"
#include "stim/dem/detector_error_model_instruction.pybind.h"
#include "stim/dem/detector_error_model_repeat_block.pybind.h"
#include "stim/dem/detector_error_model_target.pybind.h"
#include "stim/dem/detector_error_model.pybind.h"
#include "stim/io/read_write.pybind.h"
#include "stim/main_namespaced.h"
#include "stim/py/base.pybind.h"
#include "stim/py/compiled_detector_sampler.pybind.h"
#include "stim/py/compiled_measurement_sampler.pybind.h"
#include "stim/py/march.pybind.h"
#include "stim/simulators/dem_sampler.pybind.h"
#include "stim/simulators/matched_error.pybind.h"
#include "stim/simulators/measurements_to_detection_events.pybind.h"
#include "stim/simulators/tableau_simulator.pybind.h"
#include "stim/stabilizers/pauli_string.pybind.h"
#include "stim/stabilizers/tableau.h"
#include "stim/stabilizers/tableau.pybind.h"
#include "stim/stabilizers/tableau_iter.pybind.h"

#define xstr(s) str(s)
#define str(s) #s

using namespace stim;
using namespace stim_pybind;

GateTarget target_rec(int32_t lookback) {
    return GateTarget::rec(lookback);
}

GateTarget target_inv(uint32_t qubit) {
    return GateTarget::qubit(qubit, true);
}

GateTarget target_x(uint32_t qubit, bool invert) {
    return GateTarget::x(qubit, invert);
}

GateTarget target_y(uint32_t qubit, bool invert) {
    return GateTarget::y(qubit, invert);
}

GateTarget target_z(uint32_t qubit, bool invert) {
    return GateTarget::z(qubit, invert);
}

GateTarget target_sweep_bit(uint32_t qubit) {
    return GateTarget::sweep_bit(qubit);
}

int stim_main(const std::vector<std::string> &args) {
    std::vector<const char *> argv;
    argv.push_back("stim.main");
    for (const auto &arg : args) {
        argv.push_back(arg.data());
    }
    return stim::main(argv.size(), argv.data());
}

pybind11::object raw_gate_data_solo(const Gate &gate) {
    pybind11::dict result;
    auto f = gate.extra_data_func;
    if (f == nullptr) {
        f = GATE_DATA.at(gate.name).extra_data_func;
    }
    auto extra = f();
    result["name"] = gate.name;
    result["category"] = extra.category;
    result["help"] = extra.help;
    if (gate.flags & GATE_IS_UNITARY) {
        result["unitary_matrix"] = gate.unitary();
        result["stabilizer_tableau"] = gate.tableau();
    }
    if (extra.h_s_cx_m_r_decomposition != nullptr) {
        result["h_s_cx_m_r_decomposition"] = Circuit(extra.h_s_cx_m_r_decomposition);
    }
    return result;
}

pybind11::object raw_format_data_solo(const FileFormatData &data) {
    pybind11::dict result;
    result["name"] = data.name;
    result["parse_example"] = data.help_python_parse;
    result["save_example"] = data.help_python_save;
    result["help"] = data.help;
    return result;
}

pybind11::dict raw_gate_data() {
    pybind11::dict result;
    for (const auto &gate : GATE_DATA.gates()) {
        result[gate.name] = raw_gate_data_solo(gate);
    }
    return result;
}

pybind11::dict raw_format_data() {
    pybind11::dict result;
    for (const auto &kv : format_name_to_enum_map) {
        result[kv.first.data()] = raw_format_data_solo(kv.second);
    }
    return result;
}


void top_level(pybind11::module &m) {
    m.def(
        "target_rec",
        &target_rec,
        pybind11::arg("lookback_index"),
        clean_doc_string(u8R"DOC(
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
        clean_doc_string(u8R"DOC(
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
        clean_doc_string(u8R"DOC(
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
        clean_doc_string(u8R"DOC(
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
        clean_doc_string(u8R"DOC(
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
        clean_doc_string(u8R"DOC(
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
        "target_sweep_bit",
        &target_sweep_bit,
        pybind11::arg("sweep_bit_index"),
        clean_doc_string(u8R"DOC(
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
        clean_doc_string(u8R"DOC(
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

    m.def("_UNSTABLE_raw_gate_data", &raw_gate_data);
    m.def("_UNSTABLE_raw_format_data", &raw_format_data);
}


PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.attr("__version__") = xstr(VERSION_INFO);
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
    auto c_tableau = pybind_tableau(m);
    auto c_tableau_iter = pybind_tableau_iter(m);

    auto c_circuit_gate_target = pybind_circuit_gate_target(m);
    auto c_circuit_instruction = pybind_circuit_instruction(m);
    auto c_circuit_repeat_block = pybind_circuit_repeat_block(m);
    auto c_circuit = pybind_circuit(m);

    auto c_detector_error_model_instruction = pybind_detector_error_model_instruction(m);
    auto c_detector_error_model_target = pybind_detector_error_model_target(m);
    auto c_detector_error_model_repeat_block = pybind_detector_error_model_repeat_block(m);
    auto c_detector_error_model = pybind_detector_error_model(m);

    auto c_tableau_simulator = pybind_tableau_simulator(m);


    auto c_circuit_error_location_stack_frame = pybind_circuit_error_location_stack_frame(m);
    auto c_gate_target_with_coords = pybind_gate_target_with_coords(m);
    auto c_dem_target_with_coords = pybind_dem_target_with_coords(m);
    auto c_flipped_measurement = pybind_flipped_measurement(m);
    auto c_circuit_targets_inside_instruction = pybind_circuit_targets_inside_instruction(m);
    auto c_circuit_error_location = pybind_circuit_error_location(m);
    auto c_circuit_error_location_methods = pybind_explained_error(m);


    /// top level function definitions
    top_level(m);
    pybind_read_write(m);


    // method definitions
    pybind_circuit_instruction_methods(m, c_circuit_instruction);
    pybind_circuit_gate_target_methods(m, c_circuit_gate_target);
    pybind_circuit_repeat_block_methods(m, c_circuit_repeat_block);
    pybind_circuit_methods(m, c_circuit);

    pybind_tableau_iter_methods(m, c_tableau_iter);
    pybind_dem_sampler_methods(m, c_dem_sampler);

    pybind_detector_error_model_instruction_methods(m, c_detector_error_model_instruction);
    pybind_detector_error_model_repeat_block_methods(m, c_detector_error_model_repeat_block);
    pybind_detector_error_model_target_methods(m, c_detector_error_model_target);
    pybind_detector_error_model_methods(m, c_detector_error_model);

    pybind_tableau_methods(m, c_tableau);
    pybind_pauli_string_methods(m, c_pauli_string);

    pybind_compiled_detector_sampler_methods(m, c_compiled_detector_sampler);
    pybind_compiled_measurement_sampler_methods(m, c_compiled_measurement_sampler);
    pybind_compiled_measurements_to_detection_events_converter_methods(m, c_compiled_m2d_converter);

    pybind_tableau_simulator_methods(m, c_tableau_simulator);

    pybind_circuit_error_location_stack_frame_methods(m, c_circuit_error_location_stack_frame);
    pybind_gate_target_with_coords_methods(m, c_gate_target_with_coords);
    pybind_dem_target_with_coords_methods(m, c_dem_target_with_coords);
    pybind_flipped_measurement_methods(m, c_flipped_measurement);
    pybind_circuit_targets_inside_instruction_methods(m, c_circuit_targets_inside_instruction);
    pybind_circuit_error_location_methods(m, c_circuit_error_location);
    pybind_explained_error_methods(m, c_circuit_error_location_methods);
}
