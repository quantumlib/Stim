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

#include "../circuit/circuit.pybind.h"
#include "../dem/detector_error_model.pybind.h"
#include "../simulators/tableau_simulator.pybind.h"
#include "../stabilizers/pauli_string.pybind.h"
#include "../stabilizers/tableau.pybind.h"
#include "base.pybind.h"
#include "compiled_detector_sampler.pybind.h"
#include "compiled_measurement_sampler.pybind.h"

#define xstr(s) str(s)
#define str(s) #s

using namespace stim_internal;

uint32_t target_rec(int32_t lookback) {
    if (lookback >= 0 || lookback <= -(1 << 24)) {
        throw std::out_of_range("Need -16777215 <= lookback <= -1");
    }
    return uint32_t(-lookback) | TARGET_RECORD_BIT;
}

uint32_t target_inv(uint32_t qubit) {
    return qubit | TARGET_INVERTED_BIT;
}

uint32_t target_x(uint32_t qubit) {
    return qubit | TARGET_PAULI_X_BIT;
}

uint32_t target_y(uint32_t qubit) {
    return qubit | TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT;
}

uint32_t target_z(uint32_t qubit) {
    return qubit | TARGET_PAULI_Z_BIT;
}

PYBIND11_MODULE(stim, m) {
    m.attr("__version__") = xstr(VERSION_INFO);
    m.doc() = R"pbdoc(
        Stim: A fast stabilizer circuit simulator library.
    )pbdoc";

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
        "target_x",
        &target_x,
        pybind11::arg("qubit_index"),
        clean_doc_string(u8R"DOC(
            Returns a target flagged as Pauli X that can be passed into Circuit.append_operation
            For example, the 'X1' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 1 flagged as Pauli X.
        )DOC")
            .data());

    m.def(
        "target_y",
        &target_y,
        pybind11::arg("qubit_index"),
        clean_doc_string(u8R"DOC(
            Returns a target flagged as Pauli Y that can be passed into Circuit.append_operation
            For example, the 'Y2' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 2 flagged as Pauli Y.
        )DOC")
            .data());

    m.def(
        "target_z",
        &target_z,
        pybind11::arg("qubit_index"),
        clean_doc_string(u8R"DOC(
            Returns a target flagged as Pauli Z that can be passed into Circuit.append_operation
            For example, the 'Z3' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 3 flagged as Pauli Z.
        )DOC")
            .data());

    pybind_circuit(m);
    pybind_compiled_detector_sampler(m);
    pybind_compiled_measurement_sampler(m);
    pybind_detector_error_model(m);
    pybind_pauli_string(m);
    pybind_tableau_simulator(m);
    pybind_tableau(m);
}
