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

#include "tableau.pybind.h"

#include "../py/base.pybind.h"
#include "../simulators/tableau_simulator.h"
#include "../stabilizers/pauli_string.h"
#include "../stabilizers/tableau.h"

void pybind_tableau(pybind11::module &m) {
    pybind11::class_<Tableau>(
        m, "Tableau",
        R"DOC(
            A stabilizer tableau.

            Represents a Clifford operation by explicitly storing how that operation conjugates a list of Pauli
            group generators into composite Pauli products.
        )DOC")
        .def(
            pybind11::init<size_t>(), pybind11::arg("num_qubits"),
            R"DOC(
                Creates an identity tableau over the given number of qubits.

                Examples:
                    >>> import stim
                    >>> t = stim.Tableau(3)
                    >>> print(t)
                    +-xz-xz-xz-
                    | ++ ++ ++
                    | XZ __ __
                    | __ XZ __
                    | __ __ XZ

                Args:
                    num_qubits: The number of qubits the tableau's operation acts on.
             )DOC")
        .def_static(
            "random",
            [](size_t num_qubits) {
                return Tableau::random(num_qubits, PYBIND_SHARED_RNG());
            },
            pybind11::arg("num_qubits"),
            R"DOC(
                Samples a uniformly random Clifford operation over the given number of qubits and returns its tableau.

                Args:
                    num_qubits: The number of qubits the tableau should act on.

                Returns:
                    The sampled tableau.

                Examples:
                    >>> import stim
                    >>> t = stim.Tableau.random(42)

                References:
                    "Hadamard-free circuits expose the structure of the Clifford group"
                    Sergey Bravyi, Dmitri Maslov
                    https://arxiv.org/abs/2003.09412
            )DOC")
        .def_static(
            "from_named_gate",
            [](const char *name) {
                const Gate &gate = GATE_DATA.at(name);
                if (!(gate.flags & GATE_IS_UNITARY)) {
                    throw std::out_of_range("Recognized name, but not unitary: " + std::string(name));
                }
                return gate.tableau();
            },
            pybind11::arg("name"),
            R"DOC(
                Returns the tableau of a named Clifford gate.

                Args:
                    name: The name of the Clifford gate.

                Returns:
                    The gate's tableau.

                Examples:
                    >>> import stim
                    >>> print(stim.Tableau.from_named_gate("H"))
                    +-xz-
                    | ++
                    | ZX
                    >>> print(stim.Tableau.from_named_gate("CNOT"))
                    +-xz-xz-
                    | ++ ++
                    | XZ _Z
                    | X_ XZ
                    >>> print(stim.Tableau.from_named_gate("S"))
                    +-xz-
                    | ++
                    | YZ
            )DOC")
        .def(
            "__len__",
            [](const Tableau &self) {
                return self.num_qubits;
            })
        .def("__str__", &Tableau::str)
        .def(
            "__eq__",
            [](const Tableau &self, const Tableau &other) {
                return self == other;
            })
        .def(
            "__ne__",
            [](const Tableau &self, const Tableau &other) {
                return self != other;
            })
        .def(
            "__pow__", &Tableau::raised_to, pybind11::arg("exponent"),
            R"DOC(
                Raises the tableau to an integer power using inversion and repeated squaring.

                Args:
                    exponent: The power to raise to. Can be negative, zero, or positive.

                Examples:
                    >>> import stim
                    >>> s = stim.Tableau.from_named_gate("S")
                    >>> s**0 == stim.Tableau(1)
                    True
                    >>> s**1 == s
                    True
                    >>> s**2 == stim.Tableau.from_named_gate("Z")
                    True
                    >>> s**-1 == s**3 == stim.Tableau.from_named_gate("S_DAG")
                    True
                    >>> s**5 == s
                    True
                    >>> s**(400000000 + 1) == s
                    True
                    >>> s**(-400000000 + 1) == s
                    True
            )DOC")
        .def(
            "append",
            [](Tableau &self, const Tableau &gate, const std::vector<size_t> targets) {
                std::vector<bool> use(self.num_qubits, false);
                if (targets.size() != gate.num_qubits) {
                    throw std::invalid_argument("len(targets) != len(gate)");
                }
                for (size_t k : targets) {
                    if (k >= self.num_qubits) {
                        throw std::invalid_argument("target >= len(tableau)");
                    }
                    if (use[k]) {
                        throw std::invalid_argument("target collision on qubit " + std::to_string(k));
                    }
                    use[k] = true;
                }
                self.inplace_scatter_append(gate, targets);
            },
            pybind11::arg("gate"), pybind11::arg("targets"),
            R"DOC(
                Appends an operation's effect into this tableau, mutating this tableau.

                Time cost is O(n*m*m) where n=len(self) and m=len(gate).

                Args:
                    gate: The tableau of the operation being appended into this tableau.
                    targets: The qubits being targeted by the gate.

                Examples:
                    >>> import stim
                    >>> cnot = stim.Tableau.from_named_gate("CNOT")
                    >>> t = stim.Tableau(2)
                    >>> t.append(cnot, [0, 1])
                    >>> t.append(cnot, [1, 0])
                    >>> t.append(cnot, [0, 1])
                    >>> t == stim.Tableau.from_named_gate("SWAP")
                    True
            )DOC")
        .def(
            "prepend",
            [](Tableau &self, const Tableau &gate, const std::vector<size_t> targets) {
                std::vector<bool> use(self.num_qubits, false);
                if (targets.size() != gate.num_qubits) {
                    throw std::invalid_argument("len(targets) != len(gate)");
                }
                for (size_t k : targets) {
                    if (k >= self.num_qubits) {
                        throw std::invalid_argument("target >= len(tableau)");
                    }
                    if (use[k]) {
                        throw std::invalid_argument("target collision on qubit " + std::to_string(k));
                    }
                    use[k] = true;
                }
                self.inplace_scatter_prepend(gate, targets);
            },
            pybind11::arg("gate"), pybind11::arg("targets"),
            R"DOC(
                Prepends an operation's effect into this tableau, mutating this tableau.

                Time cost is O(n*m*m) where n=len(self) and m=len(gate).

                Args:
                    gate: The tableau of the operation being prepended into this tableau.
                    targets: The qubits being targeted by the gate.

                Examples:
                    >>> import stim
                    >>> h = stim.Tableau.from_named_gate("H")
                    >>> cnot = stim.Tableau.from_named_gate("CNOT")
                    >>> t = stim.Tableau.from_named_gate("H")
                    >>> t.prepend(stim.Tableau.from_named_gate("X"), [0])
                    >>> t == stim.Tableau.from_named_gate("SQRT_Y_DAG")
                    True
            )DOC")
        .def(
            "x_output",
            [](Tableau &self, size_t target) {
                if (target >= self.num_qubits) {
                    throw std::invalid_argument("target >= len(tableau)");
                }
                PauliString copy = self.xs[target];
                return copy;
            },
            pybind11::arg("target"),
            R"DOC(
                Returns the result of conjugating a Pauli X by the tableau's Clifford operation.

                Args:
                    target: The qubit targeted by the Pauli X operation.

                Examples:
                    >>> import stim
                    >>> cnot = stim.Tableau.from_named_gate("CNOT")
                    >>> cnot.x_output(0)
                    stim.PauliString("+XX")
                    >>> cnot.x_output(1)
                    stim.PauliString("+_X")
            )DOC")
        .def(
            "z_output",
            [](Tableau &self, size_t target) {
                if (target >= self.num_qubits) {
                    throw std::invalid_argument("target >= len(tableau)");
                }
                PauliString copy = self.zs[target];
                return copy;
            },
            pybind11::arg("target"),
            R"DOC(
                Returns the result of conjugating a Pauli Z by the tableau's Clifford operation.

                Args:
                    target: The qubit targeted by the Pauli Z operation.

                Examples:
                    >>> import stim
                    >>> cnot = stim.Tableau.from_named_gate("CNOT")
                    >>> cnot.z_output(0)
                    stim.PauliString("+Z_")
                    >>> cnot.z_output(1)
                    stim.PauliString("+ZZ")
            )DOC")
        .def_static(
            "from_conjugated_generators",
            [](const std::vector<PauliString> &xs, const std::vector<PauliString> &zs) {
                size_t n = xs.size();
                if (n != zs.size()) {
                    throw std::invalid_argument("len(xs) != len(zs)");
                }
                for (const auto &p : xs) {
                    if (p.num_qubits != n) {
                        throw std::invalid_argument("not all(len(p) == len(xs) for p in xs)");
                    }
                }
                for (const auto &p : zs) {
                    if (p.num_qubits != n) {
                        throw std::invalid_argument("not all(len(p) == len(zs) for p in zs)");
                    }
                }
                Tableau result(n);
                for (size_t q = 0; q < n; q++) {
                    result.xs[q] = xs[q];
                    result.zs[q] = zs[q];
                }
                if (!result.satisfies_invariants()) {
                    throw std::invalid_argument(
                        "The given generator outputs don't describe a valid Clifford operation.\n"
                        "They don't preserve commutativity.\n"
                        "Everything must commute, except for X_k anticommuting with Z_k for each k.");
                }
                return result;
            },
            pybind11::kw_only(), pybind11::arg("xs"), pybind11::arg("zs"),
            R"DOC(
                Creates a tableau from the given outputs for each generator.

                Verifies that the tableau is well formed.

                Args:
                    xs: A List[stim.PauliString] with the results of conjugating X0, X1, etc.
                    zs: A List[stim.PauliString] with the results of conjugating Z0, Z1, etc.

                Returns:
                    The created tableau.

                Raises:
                    ValueError: The given outputs are malformed. Their lengths are inconsistent,
                        or they don't satisfy the required commutation relationships.

                Examples:
                    >>> import stim
                    >>> identity3 = stim.Tableau.from_conjugated_generators(
                    ...     xs=[
                    ...         stim.PauliString("X__"),
                    ...         stim.PauliString("_X_"),
                    ...         stim.PauliString("__X"),
                    ...     ],
                    ...     zs=[
                    ...         stim.PauliString("Z__"),
                    ...         stim.PauliString("_Z_"),
                    ...         stim.PauliString("__Z"),
                    ...     ],
                    ... )
                    >>> identity3 == stim.Tableau(3)
                    True
            )DOC")
        .def(
            "__repr__",
            [](const Tableau &self) {
                std::stringstream result;
                result << "stim.Tableau.from_conjugated_generators(\n    xs=[\n";
                for (size_t q = 0; q < self.num_qubits; q++) {
                    result << "        stim.PauliString(\"" << self.xs[q].str() << "\"),\n";
                }
                result << "    ],\n    zs=[\n";
                for (size_t q = 0; q < self.num_qubits; q++) {
                    result << "        stim.PauliString(\"" << self.zs[q].str() << "\"),\n";
                }
                result << "    ],\n)";
                return result.str();
            })
        .def(
            "__call__",
            [](const Tableau &self, const PauliString &pauli_string) {
                return self(pauli_string);
            },
            pybind11::arg("pauli_string"),
            R"DOC(
                 Returns the result of conjugating the given PauliString by the Tableau's Clifford operation.

                 Args:
                     pauli_string: The pauli string to conjugate.

                 Returns:
                     The new conjugated pauli string.

                 Examples:
                     >>> import stim
                     >>> t = stim.Tableau.from_named_gate("CNOT")
                     >>> p = stim.PauliString("XX")
                     >>> result = t(p)
                     >>> print(result)
                     +X_

                 References:
                     "Hadamard-free circuits expose the structure of the Clifford group"
                     Sergey Bravyi, Dmitri Maslov
                     https://arxiv.org/abs/2003.09412
             )DOC");
}
