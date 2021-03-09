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

#include "circuit.h"

#include "../py/base.pybind.h"
#include "../py/compiled_detector_sampler.pybind.h"
#include "../py/compiled_measurement_sampler.pybind.h"
#include "circuit.pybind.h"

void pybind_circuit(pybind11::module &m) {
    pybind11::class_<Circuit>(m, "Circuit", "A mutable stabilizer circuit.")
        .def(pybind11::init())
        .def_readonly("num_measurements", &Circuit::num_measurements, R"DOC(
            The number of measurement bits produced when sampling from the circuit.
         )DOC")
        .def_readonly("num_qubits", &Circuit::num_qubits, R"DOC(
            The number of qubits used when simulating the circuit.
         )DOC")
        .def(
            "compile_sampler",
            [](const Circuit &self) {
                return CompiledMeasurementSampler(self);
            },
            R"DOC(
            Returns a CompiledMeasurementSampler, which can quickly batch sample measurements, for the circuit.
         )DOC")
        .def(
            "compile_detector_sampler",
            [](Circuit &self) {
                return CompiledDetectorSampler(self);
            },
            R"DOC(
            Returns a CompiledDetectorSampler, which can quickly batch sample detection events, for the circuit.
         )DOC")
        .def("__iadd__", &Circuit::operator+=, R"DOC(
            Appends a circuit into the receiving circuit (mutating it).
         )DOC")
        .def("__imul__", &Circuit::operator*=, R"DOC(
            Mutates the circuit into multiple copies of itself.
         )DOC")
        .def("__add__", &Circuit::operator+, R"DOC(
            Creates a circuit by appending two circuits.
         )DOC")
        .def("__mul__", &Circuit::operator*, R"DOC(
            Creates a circuit by repeating a circuit multiple times.
         )DOC")
        .def("__rmul__", &Circuit::operator*, R"DOC(
            Creates a circuit by repeating a circuit multiple times.
         )DOC")
        .def(
            "append_operation", &Circuit::append_op, R"DOC(
            Appends an operation into the circuit.

            Args:
                name: The name of the operation's gate (e.g. "H" or "M" or "CNOT").
                targets: The gate targets. Gates implicitly broadcast over their targets.
                arg: A modifier for the gate, e.g. the probability of an error. Defaults to 0.
         )DOC",
            pybind11::arg("name"), pybind11::arg("targets"), pybind11::arg("arg") = 0.0)
        .def(
            "append_from_stim_program_text", &Circuit::append_from_text, R"DOC(
            Appends operations described by a STIM format program into the circuit.

            Example STIM program:

                H 0  # comment
                CNOT 0 2
                M 2
                CNOT rec[-1] 1

            Args:
                text: The STIM program text containing the circuit operations to append.
         )DOC",
            pybind11::arg("stim_program_text"))
        .def("__str__", &Circuit::str);
}
