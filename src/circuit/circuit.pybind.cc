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
    auto &&c = pybind11::class_<Circuit>(
        m,
        "Circuit",
        clean_doc_string(u8R"DOC(
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

        )DOC").data()
    );

    c.def(
        pybind11::init([](const char *stim_program_text) {
            Circuit self;
            self.append_from_text(stim_program_text);
            return self;
        }),
        pybind11::arg("stim_program_text") = "",
        clean_doc_string(u8R"DOC(
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
        )DOC").data()
    );

    c.def_property_readonly(
        "num_measurements",
        &Circuit::count_measurements,
        clean_doc_string(u8R"DOC(
            Counts the number of measurement bits produced when sampling from the circuit.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    M 0
                ...    M 0 1
                ... ''')
                >>> c.num_measurements
                3
        )DOC").data()
    );

    c.def_property_readonly(
        "num_qubits",
        &Circuit::count_qubits,
        clean_doc_string(u8R"DOC(
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
        )DOC").data()
    );

    c.def(
        "compile_sampler",
        [](const Circuit &self) {
            return CompiledMeasurementSampler(self);
        },
        clean_doc_string(u8R"DOC(
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
        )DOC").data()
    );

    c.def(
        "compile_detector_sampler",
        [](Circuit &self) {
            return CompiledDetectorSampler(self);
        },
        clean_doc_string(u8R"DOC(
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
        )DOC").data()
    );

    c.def(
        "clear",
        &Circuit::clear,
        clean_doc_string(u8R"DOC(
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
        )DOC").data()
    );

    c.def(
        "__iadd__",
        &Circuit::operator+=,
        pybind11::arg("second"),
        clean_doc_string(u8R"DOC(
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
        )DOC").data()
    );

    c.def(
        "flattened_operations",
        [](Circuit &self){
            pybind11::list result;
            self.for_each_operation([&](const Operation &op){
                pybind11::list targets;
                for (auto t : op.target_data.targets) {
                    auto v = t & TARGET_VALUE_MASK;
                    if (t & TARGET_INVERTED_BIT) {
                        targets.append(pybind11::make_tuple("inv", v));
                    } else if (t & (TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT)) {
                        if (!(t & TARGET_PAULI_Z_BIT)) {
                            targets.append(pybind11::make_tuple("X", v));
                        } else if (!(t & TARGET_PAULI_X_BIT)) {
                            targets.append(pybind11::make_tuple("Z", v));
                        } else {
                            targets.append(pybind11::make_tuple("Y", v));
                        }
                    } else if (t & TARGET_RECORD_BIT) {
                        targets.append(pybind11::make_tuple("rec", -(long long)v));
                    } else {
                        targets.append(pybind11::int_(v));
                    }
                }
                result.append(pybind11::make_tuple(op.gate->name, targets, op.target_data.arg));
            });
            return result;
        },
        clean_doc_string(u8R"DOC(
            Flattens the circuit's operations into a list.

            The operations within repeat blocks are actually repeated in the output.

            Returns:
                A List[Tuple[name, targets, arg]] of the operations in the circuit.
                    name: A string with the gate's name.
                    targets: A list of things acted on by the gate. Each thing can be:
                        int: The index of a qubit.
                        Tuple["inv", int]: The index of a qubit to measure with an inverted result.
                        Tuple["rec", int]: A measurement record target like `rec[-1]`.
                        Tuple["X", int]: A Pauli X operation to apply during a correlated error.
                        Tuple["Y", int]: A Pauli Y operation to apply during a correlated error.
                        Tuple["Z", int]: A Pauli Z operation to apply during a correlated error.
                    arg: The gate's numeric argument. For most gates this is just 0. For noisy
                        gates this is the probability of the noise being applied.

            Examples:
                >>> import stim
                >>> stim.Circuit('''
                ...    H 0
                ...    X_ERROR(0.125) 1
                ...    M 0 !1
                ... ''').flattened_operations()
                [('H', [0], 0.0), ('X_ERROR', [1], 0.125), ('M', [0, ('inv', 1)], 0.0)]

                >>> stim.Circuit('''
                ...    REPEAT 2 {
                ...        H 6
                ...    }
                ... ''').flattened_operations()
                [('H', [6], 0.0), ('H', [6], 0.0)]
        )DOC").data()
    );

    c.def(pybind11::self == pybind11::self,
        "Determines if two circuits have identical contents.");
    c.def(pybind11::self != pybind11::self,
        "Determines if two circuits have non-identical contents.");

    c.def(
        "__add__",
        &Circuit::operator+,
        pybind11::arg("second"),
        clean_doc_string(u8R"DOC(
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
        )DOC").data()
    );

    c.def(
        "__imul__",
        &Circuit::operator*=,
        pybind11::arg("repetitions"),
        clean_doc_string(u8R"DOC(
            Mutates the circuit into multiple copies of itself.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> c *= 3
                >>> print(c)
                REPEAT 3 {
                    X 0
                    Y 1 2
                }
        )DOC").data()
    );

    c.def(
        "__mul__",
        &Circuit::operator*,
        pybind11::arg("repetitions"),
        clean_doc_string(u8R"DOC(
            Creates a circuit by repeating a circuit multiple times.

            Examples:
                >>> import stim
                >>> c = stim.Circuit('''
                ...    X 0
                ...    Y 1 2
                ... ''')
                >>> print(c * 3)
                REPEAT 3 {
                    X 0
                    Y 1 2
                }
        )DOC").data()
    );

    c.def(
        "__rmul__",
        &Circuit::operator*,
        pybind11::arg("repetitions"),
        clean_doc_string(u8R"DOC(
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
        )DOC").data()
    );

    c.def(
        "append_operation",
        &Circuit::append_op,
        pybind11::arg("name"),
        pybind11::arg("targets"),
        pybind11::arg("arg") = 0.0,
        clean_doc_string(u8R"DOC(
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
        )DOC").data()
    );

    c.def(
        "append_from_stim_program_text",
        [](Circuit &self, const char *text) {
            self.append_from_text(text);
        },
        pybind11::arg("stim_program_text"),
        clean_doc_string(u8R"DOC(
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
        )DOC").data()
    );

    c.def("__str__", &Circuit::str);
    c.def("__repr__", &circuit_repr);
}
