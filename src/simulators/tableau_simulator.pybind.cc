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

#include "../simulators/tableau_simulator.h"

#include "../py/base.pybind.h"
#include "../stabilizers/tableau.h"
#include "tableau_simulator.pybind.h"

struct TempViewableData {
    std::vector<uint32_t> targets;
    TempViewableData(std::vector<uint32_t> targets) : targets(std::move(targets)) {
    }
    operator OperationData() const {
        // Temporarily remove const correctness but then immediately restore it.
        VectorView<uint32_t> v{(std::vector<uint32_t> *)&targets, 0, targets.size()};
        return {0, v};
    }
};

TableauSimulator create_tableau_simulator() {
    return TableauSimulator(0, PYBIND_SHARED_RNG());
}

TempViewableData args_to_targets(TableauSimulator &self, const pybind11::args &args) {
    std::vector<uint32_t> arguments;
    size_t max_q = 0;
    try {
        for (const auto &e : args) {
            size_t q = e.cast<uint32_t>();
            max_q = std::max(max_q, q);
            arguments.push_back(q);
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
        throw std::out_of_range("Two qubit operation requires an even number of targets.");
    }
    return args_to_targets(self, args);
}

void pybind_tableau_simulator(pybind11::module &m) {
    pybind11::class_<TableauSimulator>(
        m,
        "TableauSimulator",
        R"DOC(
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
        .def(
            "current_inverse_tableau",
            [](TableauSimulator &self) {
                return self.inv_state;
            },
            R"DOC(
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
        .def(
            "current_measurement_record",
            [](TableauSimulator &self) {
                return self.measurement_record;
            },
            R"DOC(
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
        .def(
            "do",
            [](TableauSimulator &self, const Circuit &circuit) {
                self.ensure_large_enough_for_qubits(circuit.num_qubits);
                for (const auto &op : circuit.operations) {
                    (self.*op.gate->tableau_simulator_function)(op.target_data);
                }
            },
            pybind11::arg("circuit"),
            R"DOC(
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
        .def(
            "h",
            [](TableauSimulator &self, pybind11::args args) {
                self.H_XZ(args_to_targets(self, args));
            },
            R"DOC(
                Applies a Hadamard gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "h_xy",
            [](TableauSimulator &self, pybind11::args args) {
                self.H_XY(args_to_targets(self, args));
            },
            R"DOC(
                Applies a variant of the Hadamard gate that swaps the X and Y axes to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "h_yz",
            [](TableauSimulator &self, pybind11::args args) {
                self.H_YZ(args_to_targets(self, args));
            },
            R"DOC(
                Applies a variant of the Hadamard gate that swaps the Y and Z axes to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "x",
            [](TableauSimulator &self, pybind11::args args) {
                self.X(args_to_targets(self, args));
            },
            R"DOC(
                Applies a Pauli X gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "y",
            [](TableauSimulator &self, pybind11::args args) {
                self.Y(args_to_targets(self, args));
            },
            R"DOC(
                Applies a Pauli Y gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "z",
            [](TableauSimulator &self, pybind11::args args) {
                self.Z(args_to_targets(self, args));
            },
            R"DOC(
                Applies a Pauli Z gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "s",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_Z(args_to_targets(self, args));
            },
            R"DOC(
                Applies a SQRT_Z gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "s_dag",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_Z_DAG(args_to_targets(self, args));
            },
            R"DOC(
                Applies a SQRT_Z_DAG gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "sqrt_x",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_X(args_to_targets(self, args));
            },
            R"DOC(
                Applies a SQRT_X gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "sqrt_x_dag",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_X_DAG(args_to_targets(self, args));
            },
            R"DOC(
                Applies a SQRT_X_DAG gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "sqrt_y",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_Y(args_to_targets(self, args));
            },
            R"DOC(
                Applies a SQRT_Y gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "sqrt_y_dag",
            [](TableauSimulator &self, pybind11::args args) {
                self.SQRT_Y_DAG(args_to_targets(self, args));
            },
            R"DOC(
                Applies a SQRT_Y_DAG gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
            )DOC")
        .def(
            "swap",
            [](TableauSimulator &self, pybind11::args args) {
                self.SWAP(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies a swap gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "iswap",
            [](TableauSimulator &self, pybind11::args args) {
                self.ISWAP(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies an ISWAP gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "iswap_dag",
            [](TableauSimulator &self, pybind11::args args) {
                self.ISWAP_DAG(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies an ISWAP_DAG gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "cnot",
            [](TableauSimulator &self, pybind11::args args) {
                self.ZCX(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies a controlled X gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "cz",
            [](TableauSimulator &self, pybind11::args args) {
                self.ZCZ(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies a controlled Z gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "cy",
            [](TableauSimulator &self, pybind11::args args) {
                self.ZCY(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies a controlled Y gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "xcx",
            [](TableauSimulator &self, pybind11::args args) {
                self.XCX(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies an X-controlled X gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "xcy",
            [](TableauSimulator &self, pybind11::args args) {
                self.XCY(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies an X-controlled Y gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "xcz",
            [](TableauSimulator &self, pybind11::args args) {
                self.XCZ(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies an X-controlled Z gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "ycx",
            [](TableauSimulator &self, pybind11::args args) {
                self.YCX(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies a Y-controlled X gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "ycy",
            [](TableauSimulator &self, pybind11::args args) {
                self.YCY(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies a Y-controlled Y gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "ycz",
            [](TableauSimulator &self, pybind11::args args) {
                 self.YCZ(args_to_target_pairs(self, args));
            },
            R"DOC(
                Applies a Y-controlled Z gate to the simulator's state.

                Args:
                    *targets: The indices of the qubits to target with the gate.
                        Applies the gate to the first two targets, then the next two targets, and so forth.
                        There must be an even number of targets.
            )DOC")
        .def(
            "reset",
            [](TableauSimulator &self, pybind11::args args) {
                self.reset(args_to_targets(self, args));
            },
            R"DOC(
                Resets qubits to zero (e.g. by swapping them for zero'd qubit from the environment).

                Args:
                    *targets: The indices of the qubits to reset.
            )DOC")
        .def(
            "measure",
            [](TableauSimulator &self, uint32_t target) {
                self.measure(TempViewableData({target}));
                return (bool)self.measurement_record.back();
            },
            pybind11::arg("target"),
            R"DOC(
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
        .def(
            "measure_many",
            [](TableauSimulator &self, pybind11::args args) {
                auto converted_args = args_to_targets(self, args);
                self.measure(converted_args);
                auto e = self.measurement_record.end();
                return std::vector<bool>(e - converted_args.targets.size(), e);
            },
            R"DOC(
                Measures multiple qubits.

                Args:
                    *targets: The indices of the qubits to measure.

                Returns:
                    The measurement results as a list of bools.
            )DOC")
        .def(pybind11::init(&create_tableau_simulator));
}
