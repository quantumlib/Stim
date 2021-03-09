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
#include "../simulators/tableau_simulator.pybind.h"
#include "compiled_detector_sampler.pybind.h"
#include "compiled_measurement_sampler.pybind.h"

#define STRINGIFY(x) #x

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
    m.attr("__version__") = STRINGIFY(VERSION_INFO);
    m.doc() = R"pbdoc(
        Stim: A stabilizer circuit simulator.
    )pbdoc";

    m.def(
        "target_rec", &target_rec, R"DOC(
        Returns a record target that can be passed into Circuit.append_operation.
        For example, the 'rec[-2]' in 'DETECTOR rec[-2]' is a record target.
    )DOC",
        pybind11::arg("lookback_index"));
    m.def(
        "target_inv", &target_inv, R"DOC(
        Returns a target flagged as inverted that can be passed into Circuit.append_operation
        For example, the '!1' in 'M 0 !1 2' is qubit 1 flagged as inverted,
        meaning the measurement result from qubit 1 should be inverted when reported.
    )DOC",
        pybind11::arg("qubit_index"));
    m.def(
        "target_x", &target_x, R"DOC(
        Returns a target flagged as Pauli X that can be passed into Circuit.append_operation
        For example, the 'X1' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 1 flagged as Pauli X.
    )DOC",
        pybind11::arg("qubit_index"));
    m.def(
        "target_y", &target_y, R"DOC(
        Returns a target flagged as Pauli Y that can be passed into Circuit.append_operation
        For example, the 'Y2' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 2 flagged as Pauli Y.
    )DOC",
        pybind11::arg("qubit_index"));
    m.def(
        "target_z", &target_z, R"DOC(
        Returns a target flagged as Pauli Z that can be passed into Circuit.append_operation
        For example, the 'Z3' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 3 flagged as Pauli Z.
    )DOC",
        pybind11::arg("qubit_index"));

    pybind_circuit(m);
    pybind_compiled_detector_sampler(m);
    pybind_compiled_measurement_sampler(m);
    pybind_tableau_simulator(m);
}
