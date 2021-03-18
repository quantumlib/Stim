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

#include "circuit.pybind.h"

#include "../py/base.pybind.h"
#include "../py/compiled_detector_sampler.pybind.h"
#include "../py/compiled_measurement_sampler.pybind.h"

std::string circuit_repr(const Circuit &self) {
    if (self.operations.empty()) {
        return "stim.Circuit()";
    }
    return "stim.Circuit('''\n" + self.str() + "\n''')";
}

void pybind_circuit(pybind11::module &m) {
    pybind11::class_<Circuit>(
        m, "Circuit",
        R"DOC(
            A mutable stabilizer circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit()
                >>> c.append_operation("X", [0])
                >>> c.append_operation("M", [0])
                >>> c.compile_sampler().sample(shots=1)
                array([[1]], dtype=uint8)

                >>> stim.Circuit('''
                ...    H 0
                ...    CNOT 0 1
                ...    M 0 1
                ...    DETECTOR rec[-1] rec[-2]
                ... ''').compile_detector_sampler().sample(shots=1)
                array([[0]], dtype=uint8)

        )DOC")
        .def(
            pybind11::init([](const char *stim_program_text) {
                Circuit self;
                self.append_from_text(stim_program_text);
                return self;
            }),
            pybind11::arg("stim_program_text") = "",
            R"DOC(
            Creates a stim.Circuit.

            Args:
                stim_program_text: Defaults to empty. Describes operations to append into the circuit.

            Examples:
                >>> import stim
                >>> empty = stim.Circuit()
                >>> not_empty = stim.Circuit('''
                ...    X 0
                ...    CNOT 0 1
                ...    M 1
                ... ''')
         )DOC")
        .def_property_readonly("num_measurements", &Circuit::count_measurements, R"DOC(
            Counts the number of measurement bits produced when sampling from the circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    M 0
                ...    M 0 1
                ... ''')
                >>> c.num_measurements
                3
         )DOC")
        .def_property_readonly("num_qubits", &Circuit::count_qubits, R"DOC(
            Counts the number of qubits used when simulating the circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    M 0
                ...    M 0 1
                ... ''')
                >>> c.num_qubits
                2
                >>> c.append_from_stim_program_text('''
                ...    X 100
                ... ''')
                >>> c.num_qubits
                101
         )DOC")
        .def(
            "compile_sampler",
            [](const Circuit &self) {
                return CompiledMeasurementSampler(self);
            },
            R"DOC(
                Returns a CompiledMeasurementSampler, which can quickly batch sample measurements, for the circuit.

                Examples:
                    >>> import stim
                    >>> c = stim.Circuit('''
                    ...    X 2
                    ...    M 0 1 2
                    ... ''')
                    >>> s = c.compile_sampler()
                    >>> s.sample(shots=1)
                    array([[0, 0, 1]], dtype=uint8)
            )DOC")
        .def(
            "compile_detector_sampler",
            [](Circuit &self) {
                return CompiledDetectorSampler(self);
            },
            R"DOC(
                Returns a CompiledDetectorSampler, which can quickly batch sample detection events, for the circuit.

                Examples:
                    >>> import stim
                    >>> c = stim.Circuit('''
                    ...    H 0
                    ...    CNOT 0 1
                    ...    M 0 1
                    ...    DETECTOR rec[-1] rec[-2]
                    ... ''')
                    >>> s = c.compile_detector_sampler()
                    >>> s.sample(shots=1)
                    array([[0]], dtype=uint8)
            )DOC")
        .def("clear", &Circuit::clear, R"DOC(
            Clears the contents of the circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> c.clear()
                >>> c
                stim.Circuit()
         )DOC")
        .def("__iadd__", &Circuit::operator+=, pybind11::arg("second"), R"DOC(
            Appends a circuit into the receiving circuit (mutating it).

            Examples:
                >>> import stim
                >>> c1 = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> c2 = stim.Circuit('''
                ...    M 0 1 2
                ... ''')
                >>> c1 += c2
                >>> print(c1)
                X 0
                Y 1 2
                M 0 1 2
         )DOC")
        .def(
            "__eq__",
            [](const Circuit &self, const Circuit &other) {
                return self == other;
            })
        .def(
            "__ne__",
            [](const Circuit &self, const Circuit &other) {
                return self != other;
            })
        .def("__add__", &Circuit::operator+, pybind11::arg("second"), R"DOC(
            Creates a circuit by appending two circuits.

            Examples:
                >>> import stim
                >>> c1 = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> c2 = stim.Circuit('''
                ...    M 0 1 2
                ... ''')
                >>> print(c1 + c2)
                X 0
                Y 1 2
                M 0 1 2
         )DOC")
        .def("__imul__", &Circuit::operator*=, pybind11::arg("repetitions"), R"DOC(
            Mutates the circuit into multiple copies of itself.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> c *= 3
                >>> print(c)
                X 0
                Y 1 2
                X 0
                Y 1 2
                X 0
                Y 1 2
         )DOC")
        .def("__mul__", &Circuit::operator*, pybind11::arg("repetitions"), R"DOC(
            Creates a circuit by repeating a circuit multiple times.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> print(c * 3)
                X 0
                Y 1 2
                X 0
                Y 1 2
                X 0
                Y 1 2
         )DOC")
        .def(
            "__rmul__", &Circuit::operator*, pybind11::arg("repetitions"),
            R"DOC(
                 Creates a circuit by repeating a circuit multiple times.

                 Examples:
                     >>> import stim
                     >>> c = stim.Circuit('''
                     ...    X 0
                     ...    Y 1 2
                     ... ''')
                     >>> print(3 * c)
                     REPEAT 3 {
                         X 0
                         Y 1 2
                     }
            )DOC")
        .def(
            "append_operation", &Circuit::append_op, R"DOC(
                Appends an operation into the circuit.

                Examples:
                    >>> import stim
                    >>> c = stim.Circuit()
                    >>> c.append_operation("X", [0])
                    >>> c.append_operation("H", [0, 1])
                    >>> c.append_operation("M", [0, stim.target_inv(1)])
                    >>> c.append_operation("CNOT", [stim.target_rec(-1), 0])
                    >>> c.append_operation("X_ERROR", [0], 0.125)
                    >>> c.append_operation("CORRELATED_ERROR", [stim.target_x(0), stim.target_y(2)], 0.25)
                    >>> print(c)
                    X 0
                    H 0 1
                    M 0 !1
                    CX rec[-1] 0
                    X_ERROR(0.125) 0
                    E(0.25) X0 Y2

                Args:
                    name: The name of the operation's gate (e.g. "H" or "M" or "CNOT").
                    targets: The gate targets. Gates implicitly broadcast over their targets.
                    arg: A modifier for the gate, e.g. the probability of an error. Defaults to 0.
            )DOC",
            pybind11::arg("name"), pybind11::arg("targets"), pybind11::arg("arg") = 0.0)
        .def(
            "append_from_stim_program_text",
            [](Circuit &self, const char *text) {
                self.append_from_text(text);
            },
            R"DOC(
                Appends operations described by a STIM format program into the circuit.

                Examples:
                    >>> import stim
                    >>> c = stim.Circuit()
                    >>> c.append_from_stim_program_text('''
                    ...    H 0  # comment
                    ...    CNOT 0 2
                    ...
                    ...    M 2
                    ...    CNOT rec[-1] 1
                    ... ''')
                    >>> print(c)
                    H 0
                    CX 0 2
                    M 2
                    CX rec[-1] 1

                Args:
                    text: The STIM program text containing the circuit operations to append.
            )DOC",
            pybind11::arg("stim_program_text"))
        .def("__str__", &Circuit::str)
        .def("__repr__", &circuit_repr);
}
