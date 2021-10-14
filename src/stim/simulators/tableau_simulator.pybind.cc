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

#include "stim/simulators/tableau_simulator.pybind.h"

#include "stim/py/base.pybind.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/stabilizers/pauli_string.pybind.h"
#include "stim/stabilizers/tableau.h"

using namespace stim;

struct TempViewableData {
    std::vector<GateTarget> targets;
    TempViewableData(std::vector<GateTarget> targets) : targets(std::move(targets)) {
    }
    operator OperationData() {
        return {{}, targets};
    }
};

TableauSimulator create_tableau_simulator() {
    std::shared_ptr<std::mt19937_64> rng = PYBIND_SHARED_RNG(pybind11::none());
    // Note: there is a global shared_ptr to the unseeded rng so it won't deallocate.
    return TableauSimulator(*rng, 0);
}

TempViewableData args_to_targets(TableauSimulator &self, const pybind11::args &args) {
    std::vector<GateTarget> arguments;
    uint32_t max_q = 0;
    try {
        for (const auto &e : args) {
            uint32_t q = e.cast<uint32_t>();
            max_q = std::max(max_q, q & TARGET_VALUE_MASK);
            arguments.push_back(GateTarget{q});
        }
    } catch (const pybind11::cast_error &) {
        throw std::out_of_range("Target qubits must be non-negative integers.");
    }

    // Note: quadratic behavior.
    self.ensure_large_enough_for_qubits(max_q + 1);

    return TempViewableData(arguments);
}

TempViewableData args_to_target_pairs(TableauSimulator &self, const pybind11::args &args) {
    if (pybind11::len(args) & 1) {
        throw std::invalid_argument("Two qubit operation requires an even number of targets.");
    }
    auto result = args_to_targets(self, args);
    for (size_t k = 0; k < result.targets.size(); k += 2) {
        if (result.targets[k] == result.targets[k + 1]) {
            throw std::invalid_argument("Two qubit operation can't target the same qubit twice.");
        }
    }
    return result;
}

void pybind_tableau_simulator(pybind11::module &m) {
    auto c = pybind11::class_<TableauSimulator>(
        m,
        "TableauSimulator",
        clean_doc_string(u8R"DOC(
            A quantum stabilizer circuit simulator whose internal state is an inverse stabilizer tableau.

            Supports interactive usage, where gates and measurements are applied on demand.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> if s.measure(0):
                ...     s.h(1)
                ...     s.cnot(1, 2)
                >>> s.measure(1) == s.measure(2)
                True

                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> s.cnot(0, 1)
                >>> s.current_inverse_tableau()
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+ZX"),
                        stim.PauliString("+_X"),
                    ],
                    zs=[
                        stim.PauliString("+X_"),
                        stim.PauliString("+XZ"),
                    ],
                )
        )DOC")
            .data());

    c.def(pybind11::init(&create_tableau_simulator));

    c.def(
        "current_inverse_tableau",
        [](TableauSimulator &self) {
            return self.inv_state;
        },
        clean_doc_string(u8R"DOC(
            Returns a copy of the internal state of the simulator as a stim.Tableau.

            Returns:
                A stim.Tableau copy of the simulator's state.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> s.current_inverse_tableau()
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Z"),
                    ],
                    zs=[
                        stim.PauliString("+X"),
                    ],
                )
                >>> s.cnot(0, 1)
                >>> s.current_inverse_tableau()
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+ZX"),
                        stim.PauliString("+_X"),
                    ],
                    zs=[
                        stim.PauliString("+X_"),
                        stim.PauliString("+XZ"),
                    ],
                )
        )DOC")
            .data());

    c.def(
        "state_vector",
        [](const TableauSimulator &self) {
            auto complex_vec = self.to_state_vector();
            std::vector<float> float_vec;
            float_vec.reserve(complex_vec.size() * 2);
            for (const auto &e : complex_vec) {
                float_vec.push_back(e.real());
                float_vec.push_back(e.imag());
            }
            void *ptr = float_vec.data();
            ssize_t itemsize = sizeof(float) * 2;
            std::vector<ssize_t> shape{(ssize_t)complex_vec.size()};
            std::vector<ssize_t> stride{itemsize};
            const std::string &format = pybind11::format_descriptor<std::complex<float>>::value;
            bool readonly = true;
            return pybind11::array_t<float>(
                pybind11::buffer_info(ptr, itemsize, format, shape.size(), shape, stride, readonly));
        },
        clean_doc_string(u8R"DOC(
            Returns a wavefunction that satisfies the stabilizers of the simulator's current state.

            This function takes O(n * 2**n) time and O(2**n) space, where n is the number of qubits. The computation is
            done by initialization a random state vector and iteratively projecting it into the +1 eigenspace of each
            stabilizer of the state. The global phase of the result is arbitrary (and will vary from call to call).

            The result is in little endian order. The amplitude at offset b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is
            the amplitude for the computational basis state where the qubit with index 0 is storing the bit b_0, the
            qubit with index 1 is storing the bit b_1, etc.

            Returns:
                A `numpy.ndarray[numpy.complex64]` of computational basis amplitudes in little endian order.

            Examples:
                >>> import stim
                >>> import numpy as np

                >>> # Check that the qubit-to-amplitude-index ordering is little-endian.
                >>> s = stim.TableauSimulator()
                >>> s.x(1)
                >>> s.x(4)
                >>> vector = s.state_vector()
                >>> np.abs(vector[0b_10010]).round(2)
                1.0
                >>> tensor = vector.reshape((2, 2, 2, 2, 2))
                >>> np.abs(tensor[1, 0, 0, 1, 0]).round(2)
                1.0
        )DOC")
            .data());

    c.def(
        "canonical_stabilizers",
        [](const TableauSimulator &self) {
            auto stabilizers = self.canonical_stabilizers();
            std::vector<PyPauliString> result;
            result.reserve(stabilizers.size());
            for (auto &s : stabilizers) {
                result.emplace_back(std::move(s), false);
            }
            return result;
        },
        clean_doc_string(u8R"DOC(
            Returns a list of the stabilizers of the simulator's current state in a standard form.

            Two simulators have the same canonical stabilizers if and only if their current quantum state is equal
            (and tracking the same number of qubits).

            The canonical form is computed as follows:

                1. Get a list of stabilizers using the `z_output`s of `simulator.current_inverse_tableau()**-1`.
                2. Perform Gaussian elimination on each generator g (ordered X0, Z0, X1, Z1, X2, Z2, etc).
                    2a) Pick any stabilizer that uses the generator g. If there are none, go to the next g.
                    2b) Multiply that stabilizer into all other stabilizers that use the generator g.
                    2c) Swap that stabilizer with the stabilizer at position `next_output` then increment `next_output`.

            Returns:
                A List[stim.PauliString] of the simulator's state's stabilizers.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> s.cnot(0, 1)
                >>> s.x(2)
                >>> s.canonical_stabilizers()
                [stim.PauliString("+XX_"), stim.PauliString("+ZZ_"), stim.PauliString("-__Z")]

                >>> # Scramble the stabilizers then check that the canonical form is unchanged.
                >>> s.set_inverse_tableau(s.current_inverse_tableau()**-1)
                >>> s.cnot(0, 1)
                >>> s.cz(0, 2)
                >>> s.s(0, 2)
                >>> s.cy(2, 1)
                >>> s.set_inverse_tableau(s.current_inverse_tableau()**-1)
                >>> s.canonical_stabilizers()
                [stim.PauliString("+XX_"), stim.PauliString("+ZZ_"), stim.PauliString("-__Z")]
        )DOC")
            .data());

    c.def(
        "current_measurement_record",
        [](TableauSimulator &self) {
            return self.measurement_record.storage;
        },
        clean_doc_string(u8R"DOC(
            Returns a copy of the record of all measurements performed by the simulator.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.current_measurement_record()
                []
                >>> s.measure(0)
                False
                >>> s.x(0)
                >>> s.measure(0)
                True
                >>> s.current_measurement_record()
                [False, True]
                >>> s.do(stim.Circuit("M 0"))
                >>> s.current_measurement_record()
                [False, True, True]

            Returns:
                A list of booleans containing the result of every measurement performed by the simulator so far.
        )DOC")
            .data());

    c.def(
        "do",
        &TableauSimulator::expand_do_circuit,
        pybind11::arg("circuit"),
        clean_doc_string(u8R"DOC(
            Applies all the operations in the given stim.Circuit to the simulator's state.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.do(stim.Circuit('''
                ...     X 0
                ...     M 0
                ... '''))
                >>> s.current_measurement_record()
                [True]

            Args:
                circuit: A stim.Circuit containing operations to apply.
        )DOC")
            .data());

    c.def(
        "do",
        [](TableauSimulator &self, const PyPauliString &pauli_string) {
            self.ensure_large_enough_for_qubits(pauli_string.value.num_qubits);
            self.paulis(pauli_string.value);
        },
        pybind11::arg("pauli_string"),
        clean_doc_string(u8R"DOC(
            Applies all the Pauli operations in the given stim.PauliString to the simulator's state.

            The Pauli at offset k is applied to the qubit with index k.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.do(stim.PauliString("IXYZ"))
                >>> s.measure_many(0, 1, 2, 3)
                [False, True, True, False]

            Args:
                pauli_string: A stim.PauliString containing Pauli operations to apply.
        )DOC")
            .data());

    c.def(
        "h",
        [](TableauSimulator &self, pybind11::args args) {
            self.H_XZ(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a Hadamard gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "h_xy",
        [](TableauSimulator &self, pybind11::args args) {
            self.H_XY(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a variant of the Hadamard gate that swaps the X and Y axes to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "h_yz",
        [](TableauSimulator &self, pybind11::args args) {
            self.H_YZ(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a variant of the Hadamard gate that swaps the Y and Z axes to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "x",
        [](TableauSimulator &self, pybind11::args args) {
            self.X(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a Pauli X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "y",
        [](TableauSimulator &self, pybind11::args args) {
            self.Y(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a Pauli Y gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "z",
        [](TableauSimulator &self, pybind11::args args) {
            self.Z(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a Pauli Z gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "s",
        [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_Z(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a SQRT_Z gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "s_dag",
        [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_Z_DAG(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a SQRT_Z_DAG gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "sqrt_x",
        [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_X(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a SQRT_X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "sqrt_x_dag",
        [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_X_DAG(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a SQRT_X_DAG gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "sqrt_y",
        [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_Y(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a SQRT_Y gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "sqrt_y_dag",
        [](TableauSimulator &self, pybind11::args args) {
            self.SQRT_Y_DAG(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a SQRT_Y_DAG gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
        )DOC")
            .data());

    c.def(
        "swap",
        [](TableauSimulator &self, pybind11::args args) {
            self.SWAP(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a swap gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "iswap",
        [](TableauSimulator &self, pybind11::args args) {
            self.ISWAP(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies an ISWAP gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "iswap_dag",
        [](TableauSimulator &self, pybind11::args args) {
            self.ISWAP_DAG(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies an ISWAP_DAG gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "cnot",
        [](TableauSimulator &self, pybind11::args args) {
            self.ZCX(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a controlled X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "cz",
        [](TableauSimulator &self, pybind11::args args) {
            self.ZCZ(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a controlled Z gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "cy",
        [](TableauSimulator &self, pybind11::args args) {
            self.ZCY(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a controlled Y gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "xcx",
        [](TableauSimulator &self, pybind11::args args) {
            self.XCX(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies an X-controlled X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "xcy",
        [](TableauSimulator &self, pybind11::args args) {
            self.XCY(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies an X-controlled Y gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "xcz",
        [](TableauSimulator &self, pybind11::args args) {
            self.XCZ(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies an X-controlled Z gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "ycx",
        [](TableauSimulator &self, pybind11::args args) {
            self.YCX(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a Y-controlled X gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "ycy",
        [](TableauSimulator &self, pybind11::args args) {
            self.YCY(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a Y-controlled Y gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "ycz",
        [](TableauSimulator &self, pybind11::args args) {
            self.YCZ(args_to_target_pairs(self, args));
        },
        clean_doc_string(u8R"DOC(
            Applies a Y-controlled Z gate to the simulator's state.

            Args:
                *targets: The indices of the qubits to target with the gate.
                    Applies the gate to the first two targets, then the next two targets, and so forth.
                    There must be an even number of targets.
        )DOC")
            .data());

    c.def(
        "reset",
        [](TableauSimulator &self, pybind11::args args) {
            self.reset_z(args_to_targets(self, args));
        },
        clean_doc_string(u8R"DOC(
            Resets qubits to zero (e.g. by swapping them for zero'd qubit from the environment).

            Args:
                *targets: The indices of the qubits to reset.
        )DOC")
            .data());

    c.def(
        "peek_bloch",
        [](TableauSimulator &self, size_t target) {
            self.ensure_large_enough_for_qubits(target + 1);
            return PyPauliString(self.peek_bloch(target));
        },
        pybind11::arg("target"),
        clean_doc_string(u8R"DOC(
            Returns the current bloch vector of the qubit, represented as a stim.PauliString.

            This is a non-physical operation. It reports information about the qubit without disturbing it.

            Args:
                target: The qubit to peek at.

            Returns:
                stim.PauliString("I"): The qubit is entangled. Its bloch vector is x=y=z=0.
                stim.PauliString("+Z"): The qubit is in the |0> state. Its bloch vector is z=+1, x=y=0.
                stim.PauliString("-Z"): The qubit is in the |1> state. Its bloch vector is z=-1, x=y=0.
                stim.PauliString("+Y"): The qubit is in the |i> state. Its bloch vector is y=+1, x=z=0.
                stim.PauliString("-Y"): The qubit is in the |-i> state. Its bloch vector is y=-1, x=z=0.
                stim.PauliString("+X"): The qubit is in the |+> state. Its bloch vector is x=+1, y=z=0.
                stim.PauliString("-X"): The qubit is in the |-> state. Its bloch vector is x=-1, y=z=0.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.peek_bloch(0)
                stim.PauliString("+Z")
                >>> s.x(0)
                >>> s.peek_bloch(0)
                stim.PauliString("-Z")
                >>> s.h(0)
                >>> s.peek_bloch(0)
                stim.PauliString("-X")
                >>> s.sqrt_x(1)
                >>> s.peek_bloch(1)
                stim.PauliString("-Y")
                >>> s.cz(0, 1)
                >>> s.peek_bloch(0)
                stim.PauliString("+_")
        )DOC")
            .data());

    c.def(
        "peek_observable_expectation",
        [](const TableauSimulator &self, const PyPauliString &observable) -> int8_t {
            if (observable.imag) {
                throw std::invalid_argument(
                    "Observable isn't Hermitian; it has imaginary sign. Need observable.sign in [1, -1].");
            }
            return self.peek_observable_expectation(observable.value);
        },
        pybind11::arg("observable"),
        clean_doc_string(u8R"DOC(
            Determines the expected value of an observable (which will always be -1, 0, or +1).

            This is a non-physical operation.
            It reports information about the quantum state without disturbing it.

            Args:
                observable: The observable to determine the expected value of.
                    This observable must have a real sign, not an imaginary sign.

            Returns:
                +1: Observable will be deterministically false when measured.
                -1: Observable will be deterministically true when measured.
                0: Observable will be random when measured.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> s.peek_observable_expectation(stim.PauliString("+Z"))
                1
                >>> s.peek_observable_expectation(stim.PauliString("+X"))
                0
                >>> s.peek_observable_expectation(stim.PauliString("-Z"))
                -1

                >>> s.do(stim.Circuit('''
                ...     H 0
                ...     CNOT 0 1
                ... '''))
                >>> queries = ['XX', 'YY', 'ZZ', '-ZZ', 'ZI', 'II', 'IIZ']
                >>> for q in queries:
                ...     print(q, s.peek_observable_expectation(stim.PauliString(q)))
                XX 1
                YY -1
                ZZ 1
                -ZZ -1
                ZI 0
                II 1
                IIZ 1
        )DOC")
            .data());

    c.def(
        "measure",
        [](TableauSimulator &self, uint32_t target) {
            self.ensure_large_enough_for_qubits(target + 1);
            self.measure_z(TempViewableData({GateTarget{target}}));
            return (bool)self.measurement_record.storage.back();
        },
        pybind11::arg("target"),
        clean_doc_string(u8R"DOC(
            Measures a single qubit.

            Unlike the other methods on TableauSimulator, this one does not broadcast
            over multiple targets. This is to avoid returning a list, which would
            create a pitfall where typing `if sim.measure(qubit)` would be a bug.

            To measure multiple qubits, use `TableauSimulator.measure_many`.

            Args:
                target: The index of the qubit to measure.

            Returns:
                The measurement result as a bool.
        )DOC")
            .data());

    c.def(
        "measure_many",
        [](TableauSimulator &self, pybind11::args args) {
            auto converted_args = args_to_targets(self, args);
            self.measure_z(converted_args);
            auto e = self.measurement_record.storage.end();
            return std::vector<bool>(e - converted_args.targets.size(), e);
        },
        clean_doc_string(u8R"DOC(
            Measures multiple qubits.

            Args:
                *targets: The indices of the qubits to measure.

            Returns:
                The measurement results as a list of bools.
        )DOC")
            .data());

    c.def(
        "set_num_qubits",
        [](TableauSimulator &self, uint32_t new_num_qubits) {
            self.set_num_qubits(new_num_qubits);
        },
        clean_doc_string(u8R"DOC(
            Forces the simulator's internal state to track exactly the qubits whose indices are in range(new_num_qubits).

            Note that untracked qubits are always assumed to be in the |0> state. Therefore, calling this method
            will effectively force any qubit whose index is outside range(new_num_qubits) to be reset to |0>.

            Note that this method does not prevent future operations from implicitly expanding the size of the
            tracked state (e.g. setting the number of qubits to 5 will not prevent a Hadamard from then being
            applied to qubit 100, increasing the number of qubits to 101).

            Args:
                new_num_qubits: The length of the range of qubits the internal simulator should be tracking.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> len(s.current_inverse_tableau())
                0

                >>> s.set_num_qubits(5)
                >>> len(s.current_inverse_tableau())
                5

                >>> s.x(0, 1, 2, 3)
                >>> s.set_num_qubits(2)
                >>> s.measure_many(0, 1, 2, 3)
                [True, True, False, False]
        )DOC")
            .data());

    c.def(
        "set_inverse_tableau",
        [](TableauSimulator &self, const Tableau &new_inverse_tableau) {
            self.inv_state = new_inverse_tableau;
        },
        clean_doc_string(u8R"DOC(
            Overwrites the simulator's internal state with a copy of the given inverse tableau.

            The inverse tableau specifies how Pauli product observables of qubits at the current time transform
            into equivalent Pauli product observables at the beginning of time, when all qubits were in the
            |0> state. For example, if the Z observable on qubit 5 maps to a product of Z observables at the
            start of time then a Z basis measurement on qubit 5 will be deterministic and equal to the sign
            of the product. Whereas if it mapped to a product of observables including an X or a Y then the Z
            basis measurement would be random.

            Any qubits not within the length of the tableau are implicitly in the |0> state.

            Args:
                new_inverse_tableau: The tableau to overwrite the internal state with.

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()
                >>> t = stim.Tableau.random(4)
                >>> s.set_inverse_tableau(t)
                >>> s.current_inverse_tableau() == t
                True
        )DOC")
            .data());

    c.def(
        "copy",
        [](TableauSimulator &self) {
            TableauSimulator copy = self;
            return copy;
        },
        clean_doc_string(u8R"DOC(
            Returns a copy of the simulator. A simulator with the same internal state.

            Examples:
                >>> import stim

                >>> s1 = stim.TableauSimulator()
                >>> s1.set_inverse_tableau(stim.Tableau.random(1))
                >>> s2 = s1.copy()
                >>> s2 is s1
                False
                >>> s2.current_inverse_tableau() == s1.current_inverse_tableau()
                True

                >>> s = stim.TableauSimulator()
                >>> def brute_force_post_select(qubit, desired_result):
                ...     global s
                ...     while True:
                ...         copy = s.copy()
                ...         if copy.measure(qubit) == desired_result:
                ...             s = copy
                ...             break
                >>> s.h(0)
                >>> brute_force_post_select(qubit=0, desired_result=True)
                >>> s.measure(0)
                True
        )DOC")
            .data());

    c.def(
        "measure_kickback",
        [](TableauSimulator &self, uint32_t target) {
            self.ensure_large_enough_for_qubits(target + 1);
            auto result = self.measure_kickback_z({target});
            if (result.second.num_qubits == 0) {
                return pybind11::make_tuple(result.first, pybind11::none());
            }
            return pybind11::make_tuple(result.first, PyPauliString(result.second));
        },
        pybind11::arg("target"),
        clean_doc_string(u8R"DOC(
            Measures a qubit and returns the result as well as its Pauli kickback (if any).

            The "Pauli kickback" of a stabilizer circuit measurement is a set of Pauli operations that
            flip the post-measurement system state between the two possible post-measurement states.
            For example, consider measuring one of the qubits in the state |00>+|11> in the Z basis.
            If the measurement result is False, then the system projects into the state |00>.
            If the measurement result is True, then the system projects into the state |11>.
            Applying a Pauli X operation to both qubits flips between |00> and |11>.
            Therefore the Pauli kickback of the measurement is `stim.PauliString("XX")`.
            Note that there are often many possible equivalent Pauli kickbacks. For example,
            if in the previous example there was a third qubit in the |0> state, then both
            `stim.PauliString("XX_")` and `stim.PauliString("XXZ")` are valid kickbacks.

            Measurements with determinist results don't have a Pauli kickback.

            Args:
                target: The index of the qubit to measure.

            Returns:
                A (result, kickback) tuple.
                The result is a bool containing the measurement's output.
                The kickback is either None (meaning the measurement was deterministic) or a stim.PauliString
                (meaning the measurement was random, and the operations in the Pauli string flip between the
                two possible post-measurement states).

            Examples:
                >>> import stim
                >>> s = stim.TableauSimulator()

                >>> s.measure_kickback(0)
                (False, None)

                >>> s.h(0)
                >>> s.measure_kickback(0)[1]
                stim.PauliString("+X")

                >>> def pseudo_post_select(qubit, desired_result):
                ...     m, kick = s.measure_kickback(qubit)
                ...     if m != desired_result:
                ...         if kick is None:
                ...             raise ValueError("Deterministic measurement differed from desired result.")
                ...         s.do(kick)
                >>> s = stim.TableauSimulator()
                >>> s.h(0)
                >>> s.cnot(0, 1)
                >>> s.cnot(0, 2)
                >>> pseudo_post_select(qubit=2, desired_result=True)
                >>> s.measure_many(0, 1, 2)
                [True, True, True]
        )DOC")
            .data());
}
