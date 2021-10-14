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
#include "stim/py/base.pybind.h"
#include "stim/py/compiled_detector_sampler.pybind.h"
#include "stim/py/compiled_measurement_sampler.pybind.h"
#include "stim/simulators/measurements_to_detection_events.pybind.h"
#include "stim/simulators/tableau_simulator.pybind.h"
#include "stim/stabilizers/pauli_string.pybind.h"
#include "stim/stabilizers/tableau.h"
#include "stim/stabilizers/tableau.pybind.h"

#define xstr(s) str(s)
#define str(s) #s

using namespace stim;

uint32_t target_rec(int32_t lookback) {
    if (lookback >= 0 || lookback <= -(1 << 24)) {
        throw std::out_of_range("Need -16777215 <= lookback <= -1");
    }
    return uint32_t(-lookback) | TARGET_RECORD_BIT;
}

uint32_t target_inv(uint32_t qubit) {
    return GateTarget::qubit(qubit, true).data;
}

uint32_t target_x(uint32_t qubit, bool invert) {
    return GateTarget::x(qubit, invert).data;
}

uint32_t target_y(uint32_t qubit, bool invert) {
    return GateTarget::y(qubit, invert).data;
}

uint32_t target_z(uint32_t qubit, bool invert) {
    return GateTarget::z(qubit, invert).data;
}

uint32_t target_sweep_bit(uint32_t qubit) {
    return GateTarget::sweep_bit(qubit).data;
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

PYBIND11_MODULE(stim, m) {
    m.attr("__version__") = xstr(VERSION_INFO);
    m.doc() = R"pbdoc(
        Stim: A fast stabilizer circuit simulator library.
    )pbdoc";

    // CAUTION: The ordering of these is important!
    // If a class references another before it is registered, method signatures can get messed up.
    // For example, if DetectorErrorModel is defined after Circuit then Circuit.detector_error_model's return type is
    // described as `stim::DetectorErrorModel` instead of `stim.DetectorErrorModel`.

    pybind_detector_error_model(m);

    auto c0 = pybind_compiled_detector_sampler_class(m);
    auto c1 = pybind_compiled_measurement_sampler_class(m);
    auto c2 = pybind_compiled_measurements_to_detection_events_converter_class(m);
    pybind_circuit(m);
    pybind_compiled_detector_sampler_methods(c0);
    pybind_compiled_measurement_sampler_methods(c1);
    pybind_compiled_measurements_to_detection_events_converter_methods(c2);
    pybind_pauli_string(m);
    pybind_tableau(m);
    pybind_tableau_simulator(m);

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

    m.def("_UNSTABLE_raw_gate_data", &raw_gate_data);
    m.def("_UNSTABLE_raw_format_data", &raw_format_data);
}
