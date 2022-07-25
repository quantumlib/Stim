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

#include "stim/circuit/circuit.pybind.h"
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

PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.attr("__version__") = xstr(VERSION_INFO);
    m.doc() = R"pbdoc(
        Stim: A fast stabilizer circuit library.
    )pbdoc";

    // CAUTION: The ordering of these is important!
    // If a class references another before it is registered, method signatures can get messed up.
    // For example, if DetectorErrorModel is defined after Circuit then Circuit.detector_error_model's return type is
    // described as `stim::DetectorErrorModel` instead of `stim.DetectorErrorModel`.

    pybind_detector_error_model(m);

    auto c_compiled_detector_sampler = pybind_compiled_detector_sampler_class(m);
    auto c_compiled_measurement_sampler = pybind_compiled_measurement_sampler_class(m);
    auto c_compiled_m2d_converter = pybind_compiled_measurements_to_detection_events_converter_class(m);
    auto c_tableau_iter = pybind_tableau_iter(m);
    auto c_circuit = pybind_circuit(m);
    auto c_dem_sampler = pybind_dem_sampler(m);
    pybind_compiled_detector_sampler_methods(c_compiled_detector_sampler);
    pybind_compiled_measurement_sampler_methods(c_compiled_measurement_sampler);
    pybind_compiled_measurements_to_detection_events_converter_methods(c_compiled_m2d_converter);
    pybind_pauli_string(m);
    pybind_tableau(m);
    pybind_tableau_simulator(m);
    pybind_matched_error(m);
    pybind_read_write(m);

    m.def(
        "target_rec",
        &target_rec,
        pybind11::arg("lookback_index"),
        clean_doc_string(u8R"DOC(
            Returns a record target that can be passed into Circuit.append_operation.
            For example, the 'rec[-2]' in 'DETECTOR rec[-2]' is a record target.
        )DOC")
            .data());

    m.def(
        "target_inv",
        &target_inv,
        pybind11::arg("qubit_index"),
        clean_doc_string(u8R"DOC(
            Returns a target flagged as inverted that can be passed into Circuit.append_operation
            For example, the '!1' in 'M 0 !1 2' is qubit 1 flagged as inverted,
            meaning the measurement result from qubit 1 should be inverted when reported.
        )DOC")
            .data());

    m.def(
        "target_combiner",
        &GateTarget::combiner,
        clean_doc_string(u8R"DOC(
            Returns a target combiner (`*` in circuit files) that can be used as an operation target.
        )DOC")
            .data());

    m.def(
        "target_x",
        &target_x,
        pybind11::arg("qubit_index"),
        pybind11::arg("invert") = false,
        clean_doc_string(u8R"DOC(
            Returns a target flagged as Pauli X that can be passed into Circuit.append_operation
            For example, the 'X1' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 1 flagged as Pauli X.
        )DOC")
            .data());

    m.def(
        "target_y",
        &target_y,
        pybind11::arg("qubit_index"),
        pybind11::arg("invert") = false,
        clean_doc_string(u8R"DOC(
            Returns a target flagged as Pauli Y that can be passed into Circuit.append_operation
            For example, the 'Y2' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 2 flagged as Pauli Y.
        )DOC")
            .data());

    m.def(
        "target_z",
        &target_z,
        pybind11::arg("qubit_index"),
        pybind11::arg("invert") = false,
        clean_doc_string(u8R"DOC(
            Returns a target flagged as Pauli Z that can be passed into Circuit.append_operation
            For example, the 'Z3' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 3 flagged as Pauli Z.
        )DOC")
            .data());

    m.def(
        "target_sweep_bit",
        &target_sweep_bit,
        pybind11::arg("sweep_bit_index"),
        clean_doc_string(u8R"DOC(
            Returns a sweep bit target that can be passed into Circuit.append_operation
            For example, the 'sweep[5]' in 'CNOT sweep[5] 7' is from `stim.target_sweep_bit(5)`.
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
    pybind_circuit_after_types_all_defined(c_circuit);
    pybind_tableau_iter_after_types_all_defined(m, c_tableau_iter);
    pybind_dem_sampler_after_types_all_defined(m, c_dem_sampler);
}
