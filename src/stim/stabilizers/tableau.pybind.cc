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

#include "stim/stabilizers/tableau.pybind.h"

#include "stim/py/base.pybind.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/stabilizers/conversions.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/pauli_string.pybind.h"
#include "stim/stabilizers/tableau.h"
#include "stim/stabilizers/tableau_iter.h"

using namespace stim;
using namespace stim_pybind;

void stim_pybind::pybind_tableau(pybind11::module &m) {
    auto c = pybind11::class_<Tableau>(
        m,
        "Tableau",
        clean_doc_string(u8R"DOC(
            A stabilizer tableau.

            Represents a Clifford operation by explicitly storing how that operation conjugates a list of Pauli
            group generators into composite Pauli products.

            Examples:
                >>> import stim
                >>> stim.Tableau.from_named_gate("H")
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Z"),
                    ],
                    zs=[
                        stim.PauliString("+X"),
                    ],
                )

                >>> t = stim.Tableau.random(5)
                >>> t_inv = t**-1
                >>> print(t * t_inv)
                +-xz-xz-xz-xz-xz-
                | ++ ++ ++ ++ ++
                | XZ __ __ __ __
                | __ XZ __ __ __
                | __ __ XZ __ __
                | __ __ __ XZ __
                | __ __ __ __ XZ

                >>> x2z3 = t.x_output(2) * t.z_output(3)
                >>> t_inv(x2z3)
                stim.PauliString("+__XZ_")
        )DOC")
            .data());

    c.def(
        pybind11::init<size_t>(),
        pybind11::arg("num_qubits"),
        clean_doc_string(u8R"DOC(
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
            .data());

    c.def_static(
        "random",
        [](size_t num_qubits) {
            return Tableau::random(num_qubits, *make_py_seeded_rng(pybind11::none()));
        },
        pybind11::arg("num_qubits"),
        clean_doc_string(u8R"DOC(
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
            .data());

    c.def_static(
        "iter_all",
        [](size_t num_qubits, bool unsigned_only) {
            return TableauIterator(num_qubits, !unsigned_only);
        },
        pybind11::arg("num_qubits"),
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(u8R"DOC(
            Returns an iterator that iterates over all Tableaus of a given size.

            Args:
                num_qubits: The size of tableau to iterate over.
                unsigned: Defaults to False. If set to True, only tableaus where
                    all columns have positive sign are yielded by the iterator.
                    This substantially reduces the total number of tableaus to
                    iterate over.

            Returns:
                An Iterable[stim.Tableau] that yields the requested tableaus.

            Examples:
                >>> import stim
                >>> single_qubit_gate_reprs = set()
                >>> for t in stim.Tableau.iter_all(1):
                ...     single_qubit_gate_reprs.add(repr(t))
                >>> len(single_qubit_gate_reprs)
                24

                >>> num_2q_gates_mod_paulis = 0
                >>> for _ in stim.Tableau.iter_all(2, unsigned=True):
                ...     num_2q_gates_mod_paulis += 1
                >>> num_2q_gates_mod_paulis
                720
        )DOC")
            .data());

    c.def(
        "to_unitary_matrix",
        [](Tableau &self, const std::string &endian) {
            bool little_endian;
            if (endian == "little") {
                little_endian = true;
            } else if (endian == "big") {
                little_endian = false;
            } else {
                throw std::invalid_argument("endian not in ['little', 'big']");
            }
            auto data = self.to_flat_unitary_matrix(little_endian);

            void *ptr = data.data();
            pybind11::ssize_t itemsize = sizeof(float) * 2;
            pybind11::ssize_t n = 1 << self.num_qubits;
            std::vector<pybind11::ssize_t> shape{n, n};
            std::vector<pybind11::ssize_t> stride{n * itemsize, itemsize};
            const std::string &format = pybind11::format_descriptor<std::complex<float>>::value;
            bool readonly = true;
            return pybind11::array_t<float>(
                pybind11::buffer_info(ptr, itemsize, format, shape.size(), shape, stride, readonly));
        },
        pybind11::kw_only(),
        pybind11::arg("endian"),
        clean_doc_string(u8R"DOC(
            @signature def to_unitary_matrix(self, *, endian: str) -> np.ndarray[np.complex64]:
            Converts the tableau into a unitary matrix.

            Args:
                endian:
                    "little": The first qubit is the least significant (corresponds
                        to an offset of 1 in the state vector).
                    "big": The first qubit is the most significant (corresponds
                        to an offset of 2**(n - 1) in the state vector).

            Returns:
                A numpy array with dtype=np.complex64 and shape=(1 << len(tableau), 1 << len(tableau)).

            Example:
                >>> import stim
                >>> cnot = stim.Tableau.from_conjugated_generators(
                ...     xs=[
                ...         stim.PauliString("XX"),
                ...         stim.PauliString("_X"),
                ...     ],
                ...     zs=[
                ...         stim.PauliString("Z_"),
                ...         stim.PauliString("ZZ"),
                ...     ],
                ... )
                >>> cnot.to_unitary_matrix(endian='big')
                array([[1.+0.j, 0.+0.j, 0.+0.j, 0.+0.j],
                       [0.+0.j, 1.+0.j, 0.+0.j, 0.+0.j],
                       [0.+0.j, 0.+0.j, 0.+0.j, 1.+0.j],
                       [0.+0.j, 0.+0.j, 1.+0.j, 0.+0.j]], dtype=complex64)
        )DOC")
            .data());

    c.def(
        "to_circuit",
        [](Tableau &self, const std::string &method) {
            return tableau_to_circuit(self, method);
        },
        pybind11::kw_only(),
        pybind11::arg("method"),
        clean_doc_string(u8R"DOC(
            @signature def to_circuit(self, *, method: str) -> stim.Circuit:
            Synthesizes a circuit that implements the tableau's Clifford operation.

            The circuits returned by this method are not guaranteed to be stable
            from version to version, and may be produced using randomization.

            Args:
                method: The method to use when synthesizing the circuit. Available values are:
                    "elimination": Uses Gaussian elimination to cancel off-diagonal terms one by one.
                        Gate set: H, S, CX
                        Circuit qubit count: n
                        Circuit operation count: O(n^2)
                        Circuit depth: O(n^2)

            Returns:
                The synthesized circuit.

            Example:
                >>> import stim
                >>> tableau = stim.Tableau.from_conjugated_generators(
                ...     xs=[
                ...         stim.PauliString("-_YZ"),
                ...         stim.PauliString("-YY_"),
                ...         stim.PauliString("-XZX"),
                ...     ],
                ...     zs=[
                ...         stim.PauliString("+Y_Y"),
                ...         stim.PauliString("-_XY"),
                ...         stim.PauliString("-Y__"),
                ...     ],
                ... )
                >>> tableau.to_circuit(method="elimination")
                stim.Circuit('''
                    CX 2 0 0 2 2 0
                    S 0
                    H 0
                    S 0
                    H 1
                    CX 0 1 0 2
                    H 1 2
                    CX 1 0 2 0 2 1 1 2 2 1
                    H 1
                    S 1 2
                    H 2
                    CX 2 1
                    S 2
                    H 0 1 2
                    S 0 0 1 1 2 2
                    H 0 1 2
                    S 1 1 2 2
                ''')
        )DOC")
            .data());

    c.def_static(
        "from_named_gate",
        [](const char *name) {
            const Gate &gate = GATE_DATA.at(name);
            if (!(gate.flags & GATE_IS_UNITARY)) {
                throw std::out_of_range("Recognized name, but not unitary: " + std::string(name));
            }
            return gate.tableau();
        },
        pybind11::arg("name"),
        clean_doc_string(u8R"DOC(
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
            .data());

    c.def(
        "__len__",
        [](const Tableau &self) {
            return self.num_qubits;
        },
        "Returns the number of qubits operated on by the tableau.");

    c.def("__str__", &Tableau::str, "Returns a text description.");

    c.def(pybind11::self == pybind11::self, "Determines if two tableaus have identical contents.");
    c.def(pybind11::self != pybind11::self, "Determines if two tableaus have non-identical contents.");

    c.def(
        "__pow__",
        &Tableau::raised_to,
        pybind11::arg("exponent"),
        clean_doc_string(u8R"DOC(
            Raises the tableau to an integer power.

            Large powers are reached efficiently using repeated squaring.
            Negative powers are reached by inverting the tableau.

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
            .data());

    c.def(
        "inverse",
        &Tableau::inverse,
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(u8R"DOC(
            Computes the inverse of the tableau.

            The inverse T^-1 of a tableau T is the unique tableau with the property that T * T^-1 = T^-1 * T = I where
            I is the identity tableau.

            Args:
                unsigned: Defaults to false. When set to true, skips computing the signs of the output observables and
                    instead just set them all to be positive. This is beneficial because computing the signs takes
                    O(n^3) time and the rest of the inverse computation is O(n^2) where n is the number of qubits in the
                    tableau. So, if you only need the Pauli terms (not the signs), it is significantly cheaper.

            Returns:
                The inverse tableau.

            Examples:
                >>> import stim

                >>> # Check that the inverse agrees with hard-coded tableaus in the gate data.
                >>> s = stim.Tableau.from_named_gate("S")
                >>> s_dag = stim.Tableau.from_named_gate("S_DAG")
                >>> s.inverse() == s_dag
                True
                >>> z = stim.Tableau.from_named_gate("Z")
                >>> z.inverse() == z
                True

                >>> # Check that multiplying by the inverse produces the identity.
                >>> t = stim.Tableau.random(10)
                >>> t_inv = t.inverse()
                >>> identity = stim.Tableau(10)
                >>> t * t_inv == t_inv * t == identity
                True

                >>> # Check a manual case.
                >>> t = stim.Tableau.from_conjugated_generators(
                ...     xs=[stim.PauliString("-__Z"), stim.PauliString("+XZ_"), stim.PauliString("+_ZZ")],
                ...     zs=[stim.PauliString("-YYY"), stim.PauliString("+Z_Z"), stim.PauliString("-ZYZ")],
                ... )
                >>> print(t.inverse())
                +-xz-xz-xz-
                | -- +- --
                | XX XX YX
                | XZ Z_ X_
                | X_ YX Y_
                >>> print(t.inverse(unsigned=True))
                +-xz-xz-xz-
                | ++ ++ ++
                | XX XX YX
                | XZ Z_ X_
                | X_ YX Y_
        )DOC")
            .data());

    c.def(
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
        pybind11::arg("gate"),
        pybind11::arg("targets"),
        clean_doc_string(u8R"DOC(
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
            .data());

    c.def(
        "then",
        [](const Tableau &self, const Tableau &second) {
            if (self.num_qubits != second.num_qubits) {
                throw std::invalid_argument("len(self) != len(second)");
            }
            return self.then(second);
        },
        pybind11::arg("second"),
        clean_doc_string(u8R"DOC(
            Returns the result of composing two tableaus.

            If the tableau T1 represents the Clifford operation with unitary C1,
            and the tableau T2 represents the Clifford operation with unitary C2,
            then the tableau T1.then(T2) represents the Clifford operation with unitary C2*C1.

            Args:
                second: The result is equivalent to applying the second tableau after
                    the receiving tableau.

            Examples:
                >>> import stim
                >>> t1 = stim.Tableau.random(4)
                >>> t2 = stim.Tableau.random(4)
                >>> t3 = t1.then(t2)
                >>> p = stim.PauliString.random(4)
                >>> t3(p) == t2(t1(p))
                True
        )DOC")
            .data());

    c.def(
        "__mul__",
        [](const Tableau &self, const Tableau &rhs) {
            if (self.num_qubits != rhs.num_qubits) {
                throw std::invalid_argument("len(lhs) != len(rhs)");
            }
            return rhs.then(self);
        },
        pybind11::arg("rhs"),
        clean_doc_string(u8R"DOC(
            Returns the product of two tableaus.

            If the tableau T1 represents the Clifford operation with unitary C1,
            and the tableau T2 represents the Clifford operation with unitary C2,
            then the tableau T1*T2 represents the Clifford operation with unitary C1*C2.

            Args:
                rhs: The tableau  on the right hand side of the multiplication.

            Examples:
                >>> import stim
                >>> t1 = stim.Tableau.random(4)
                >>> t2 = stim.Tableau.random(4)
                >>> t3 = t2 * t1
                >>> p = stim.PauliString.random(4)
                >>> t3(p) == t2(t1(p))
                True
        )DOC")
            .data());

    c.def(
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
        pybind11::arg("gate"),
        pybind11::arg("targets"),
        clean_doc_string(u8R"DOC(
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
            .data());

    c.def(
        "x_output",
        [](Tableau &self, size_t target) {
            if (target >= self.num_qubits) {
                throw std::invalid_argument("target >= len(tableau)");
            }
            return PyPauliString(self.xs[target]);
        },
        pybind11::arg("target"),
        clean_doc_string(u8R"DOC(
            Returns the result of conjugating a Pauli X by the tableau's Clifford operation.

            Args:
                target: The qubit targeted by the Pauli X operation.

            Examples:
                >>> import stim
                >>> h = stim.Tableau.from_named_gate("H")
                >>> h.x_output(0)
                stim.PauliString("+Z")

                >>> cnot = stim.Tableau.from_named_gate("CNOT")
                >>> cnot.x_output(0)
                stim.PauliString("+XX")
                >>> cnot.x_output(1)
                stim.PauliString("+_X")
        )DOC")
            .data());

    c.def(
        "y_output",
        [](Tableau &self, size_t target) {
            if (target >= self.num_qubits) {
                throw std::invalid_argument("target >= len(tableau)");
            }

            // Compute Y using Y = i*X*Z.
            uint8_t log_i = 1;
            PauliString copy = self.xs[target];
            log_i += copy.ref().inplace_right_mul_returning_log_i_scalar(self.zs[target]);
            assert((log_i & 1) == 0);
            copy.sign ^= (log_i & 2) != 0;
            return PyPauliString(std::move(copy));
        },
        pybind11::arg("target"),
        clean_doc_string(u8R"DOC(
            Returns the result of conjugating a Pauli Y by the tableau's Clifford operation.

            Args:
                target: The qubit targeted by the Pauli Y operation.

            Examples:
                >>> import stim
                >>> h = stim.Tableau.from_named_gate("H")
                >>> h.y_output(0)
                stim.PauliString("-Y")

                >>> cnot = stim.Tableau.from_named_gate("CNOT")
                >>> cnot.y_output(0)
                stim.PauliString("+YX")
                >>> cnot.y_output(1)
                stim.PauliString("+ZY")
        )DOC")
            .data());

    c.def(
        "z_output",
        [](Tableau &self, size_t target) {
            if (target >= self.num_qubits) {
                throw std::invalid_argument("target >= len(tableau)");
            }
            return PyPauliString(self.zs[target]);
        },
        pybind11::arg("target"),
        clean_doc_string(u8R"DOC(
            Returns the result of conjugating a Pauli Z by the tableau's Clifford operation.

            Args:
                target: The qubit targeted by the Pauli Z operation.

            Examples:
                >>> import stim
                >>> h = stim.Tableau.from_named_gate("H")
                >>> h.z_output(0)
                stim.PauliString("+X")

                >>> cnot = stim.Tableau.from_named_gate("CNOT")
                >>> cnot.z_output(0)
                stim.PauliString("+Z_")
                >>> cnot.z_output(1)
                stim.PauliString("+ZZ")
        )DOC")
            .data());

    c.def(
        "x_output_pauli",
        &Tableau::x_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(u8R"DOC(
            Returns a Pauli term from the tableau's output pauli string for an input X generator.

            A constant-time equivalent for `tableau.x_output(input_index)[output_index]`.

            Args:
                input_index: Identifies the tableau column (the qubit of the input X generator).
                output_index: Identifies the tableau row (the output qubit).

            Returns:
                An integer identifying Pauli at the given location in the tableau:

                    0: I
                    1: X
                    2: Y
                    3: Z

            Examples:
                >>> import stim

                >>> t = stim.Tableau.from_conjugated_generators(
                ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
                ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
                ... )
                >>> t.x_output_pauli(0, 0)
                2
                >>> t.x_output_pauli(0, 1)
                0
                >>> t.x_output_pauli(1, 0)
                2
                >>> t.x_output_pauli(1, 1)
                3
        )DOC")
            .data());

    c.def(
        "y_output_pauli",
        &Tableau::y_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(u8R"DOC(
            Returns a Pauli term from the tableau's output pauli string for an input Y generator.

            A constant-time equivalent for `tableau.y_output(input_index)[output_index]`.

            Args:
                input_index: Identifies the tableau column (the qubit of the input Y generator).
                output_index: Identifies the tableau row (the output qubit).

            Returns:
                An integer identifying Pauli at the given location in the tableau:

                    0: I
                    1: X
                    2: Y
                    3: Z

            Examples:
                >>> import stim

                >>> t = stim.Tableau.from_conjugated_generators(
                ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
                ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
                ... )
                >>> t.y_output_pauli(0, 0)
                1
                >>> t.y_output_pauli(0, 1)
                2
                >>> t.y_output_pauli(1, 0)
                0
                >>> t.y_output_pauli(1, 1)
                2
        )DOC")
            .data());

    c.def(
        "z_output_pauli",
        &Tableau::z_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(u8R"DOC(
            Returns a Pauli term from the tableau's output pauli string for an input Z generator.

            A constant-time equivalent for `tableau.z_output(input_index)[output_index]`.

            Args:
                input_index: Identifies the tableau column (the qubit of the input Z generator).
                output_index: Identifies the tableau row (the output qubit).

            Returns:
                An integer identifying Pauli at the given location in the tableau:

                    0: I
                    1: X
                    2: Y
                    3: Z

            Examples:
                >>> import stim

                >>> t = stim.Tableau.from_conjugated_generators(
                ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
                ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
                ... )
                >>> t.z_output_pauli(0, 0)
                3
                >>> t.z_output_pauli(0, 1)
                2
                >>> t.z_output_pauli(1, 0)
                2
                >>> t.z_output_pauli(1, 1)
                1
        )DOC")
            .data());

    c.def(
        "inverse_x_output_pauli",
        &Tableau::inverse_x_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(u8R"DOC(
            Returns a Pauli term from the tableau's inverse's output pauli string for an input X generator.

            A constant-time equivalent for `tableau.inverse().x_output(input_index)[output_index]`.

            Args:
                input_index: Identifies the column (the qubit of the input X generator) in the inverse tableau.
                output_index: Identifies the row (the output qubit) in the inverse tableau.

            Returns:
                An integer identifying Pauli at the given location in the inverse tableau:

                    0: I
                    1: X
                    2: Y
                    3: Z

            Examples:
                >>> import stim

                >>> t_inv = stim.Tableau.from_conjugated_generators(
                ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
                ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
                ... ).inverse()
                >>> t_inv.inverse_x_output_pauli(0, 0)
                2
                >>> t_inv.inverse_x_output_pauli(0, 1)
                0
                >>> t_inv.inverse_x_output_pauli(1, 0)
                2
                >>> t_inv.inverse_x_output_pauli(1, 1)
                3
        )DOC")
            .data());

    c.def(
        "inverse_y_output_pauli",
        &Tableau::inverse_y_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(u8R"DOC(
            Returns a Pauli term from the tableau's inverse's output pauli string for an input Y generator.

            A constant-time equivalent for `tableau.inverse().y_output(input_index)[output_index]`.

            Args:
                input_index: Identifies the column (the qubit of the input Y generator) in the inverse tableau.
                output_index: Identifies the row (the output qubit) in the inverse tableau.

            Returns:
                An integer identifying Pauli at the given location in the inverse tableau:

                    0: I
                    1: X
                    2: Y
                    3: Z

            Examples:
                >>> import stim

                >>> t_inv = stim.Tableau.from_conjugated_generators(
                ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
                ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
                ... ).inverse()
                >>> t_inv.inverse_y_output_pauli(0, 0)
                1
                >>> t_inv.inverse_y_output_pauli(0, 1)
                2
                >>> t_inv.inverse_y_output_pauli(1, 0)
                0
                >>> t_inv.inverse_y_output_pauli(1, 1)
                2
        )DOC")
            .data());

    c.def(
        "inverse_z_output_pauli",
        &Tableau::inverse_z_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(u8R"DOC(
            Returns a Pauli term from the tableau's inverse's output pauli string for an input Z generator.

            A constant-time equivalent for `tableau.inverse().z_output(input_index)[output_index]`.

            Args:
                input_index: Identifies the column (the qubit of the input Z generator) in the inverse tableau.
                output_index: Identifies the row (the output qubit) in the inverse tableau.

            Returns:
                An integer identifying Pauli at the given location in the inverse tableau:

                    0: I
                    1: X
                    2: Y
                    3: Z

            Examples:
                >>> import stim

                >>> t_inv = stim.Tableau.from_conjugated_generators(
                ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
                ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
                ... ).inverse()
                >>> t_inv.inverse_z_output_pauli(0, 0)
                3
                >>> t_inv.inverse_z_output_pauli(0, 1)
                2
                >>> t_inv.inverse_z_output_pauli(1, 0)
                2
                >>> t_inv.inverse_z_output_pauli(1, 1)
                1
        )DOC")
            .data());

    c.def(
        "inverse_x_output",
        [](const Tableau &self, size_t input_index, bool skip_sign) {
            return PyPauliString(self.inverse_x_output(input_index, skip_sign));
        },
        pybind11::arg("input_index"),
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(u8R"DOC(
            Returns the result of conjugating an X Pauli generator by the inverse of the tableau.

            A faster version of `tableau.inverse(unsigned).x_output(input_index)`.

            Args:
                input_index: Identifies the column (the qubit of the X generator) to return from the inverse tableau.
                unsigned: Defaults to false. When set to true, skips computing the result's sign and instead just sets
                    it to positive. This is beneficial because computing the sign takes O(n^2) time whereas all other
                    parts of the computation take O(n) time where n is the number of qubits in the tableau.

            Returns:
                The result of conjugating an X generator by the inverse of the tableau.

            Examples:
                >>> import stim

                # Check equivalence with the inverse's x_output.
                >>> t = stim.Tableau.random(4)
                >>> expected = t.inverse().x_output(0)
                >>> t.inverse_x_output(0) == expected
                True
                >>> expected.sign = +1
                >>> t.inverse_x_output(0, unsigned=True) == expected
                True
        )DOC")
            .data());

    c.def(
        "inverse_y_output",
        [](const Tableau &self, size_t input_index, bool skip_sign) {
            return PyPauliString(self.inverse_y_output(input_index, skip_sign));
        },
        pybind11::arg("input_index"),
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(u8R"DOC(
            Returns the result of conjugating a Y Pauli generator by the inverse of the tableau.

            A faster version of `tableau.inverse(unsigned).y_output(input_index)`.

            Args:
                input_index: Identifies the column (the qubit of the Y generator) to return from the inverse tableau.
                unsigned: Defaults to false. When set to true, skips computing the result's sign and instead just sets
                    it to positive. This is beneficial because computing the sign takes O(n^2) time whereas all other
                    parts of the computation take O(n) time where n is the number of qubits in the tableau.

            Returns:
                The result of conjugating a Y generator by the inverse of the tableau.

            Examples:
                >>> import stim

                # Check equivalence with the inverse's y_output.
                >>> t = stim.Tableau.random(4)
                >>> expected = t.inverse().y_output(0)
                >>> t.inverse_y_output(0) == expected
                True
                >>> expected.sign = +1
                >>> t.inverse_y_output(0, unsigned=True) == expected
                True
        )DOC")
            .data());

    c.def(
        "inverse_z_output",
        [](const Tableau &self, size_t input_index, bool skip_sign) {
            return PyPauliString(self.inverse_z_output(input_index, skip_sign));
        },
        pybind11::arg("input_index"),
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(u8R"DOC(
            Returns the result of conjugating a Z Pauli generator by the inverse of the tableau.

            A faster version of `tableau.inverse(unsigned).z_output(input_index)`.

            Args:
                input_index: Identifies the column (the qubit of the Z generator) to return from the inverse tableau.
                unsigned: Defaults to false. When set to true, skips computing the result's sign and instead just sets
                    it to positive. This is beneficial because computing the sign takes O(n^2) time whereas all other
                    parts of the computation take O(n) time where n is the number of qubits in the tableau.

            Returns:
                The result of conjugating a Z generator by the inverse of the tableau.

            Examples:
                >>> import stim

                >>> import stim

                # Check equivalence with the inverse's z_output.
                >>> t = stim.Tableau.random(4)
                >>> expected = t.inverse().z_output(0)
                >>> t.inverse_z_output(0) == expected
                True
                >>> expected.sign = +1
                >>> t.inverse_z_output(0, unsigned=True) == expected
                True
        )DOC")
            .data());

    c.def(
        "copy",
        [](Tableau &self) {
            Tableau copy = self;
            return copy;
        },
        clean_doc_string(u8R"DOC(
            Returns a copy of the tableau. An independent tableau with the same contents.

            Examples:
                >>> import stim
                >>> t1 = stim.Tableau.random(2)
                >>> t2 = t1.copy()
                >>> t2 is t1
                False
                >>> t2 == t1
                True
        )DOC")
            .data());

    c.def_static(
        "from_conjugated_generators",
        [](const std::vector<PyPauliString> &xs, const std::vector<PyPauliString> &zs) {
            size_t n = xs.size();
            if (n != zs.size()) {
                throw std::invalid_argument("len(xs) != len(zs)");
            }
            for (const auto &p : xs) {
                if (p.imag) {
                    throw std::invalid_argument("Conjugated generator can't have imaginary sign.");
                }
                if (p.value.num_qubits != n) {
                    throw std::invalid_argument("not all(len(p) == len(xs) for p in xs)");
                }
            }
            for (const auto &p : zs) {
                if (p.imag) {
                    throw std::invalid_argument("Conjugated generator can't have imaginary sign.");
                }
                if (p.value.num_qubits != n) {
                    throw std::invalid_argument("not all(len(p) == len(zs) for p in zs)");
                }
            }
            Tableau result(n);
            for (size_t q = 0; q < n; q++) {
                result.xs[q] = xs[q].value;
                result.zs[q] = zs[q].value;
            }
            if (!result.satisfies_invariants()) {
                throw std::invalid_argument(
                    "The given generator outputs don't describe a valid Clifford operation.\n"
                    "They don't preserve commutativity.\n"
                    "Everything must commute, except for X_k anticommuting with Z_k for each k.");
            }
            return result;
        },
        pybind11::kw_only(),
        pybind11::arg("xs"),
        pybind11::arg("zs"),
        clean_doc_string(u8R"DOC(
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
            .data());

    c.def_static(
        "from_unitary_matrix",
        [](const pybind11::object &matrix, const std::string &endian) {
            bool little_endian;
            if (endian == "little") {
                little_endian = true;
            } else if (endian == "big") {
                little_endian = false;
            } else {
                throw std::invalid_argument("endian not in ['little', 'big']");
            }

            std::vector<std::vector<std::complex<float>>> converted_matrix;
            for (const auto &row : matrix) {
                converted_matrix.push_back({});
                for (const auto &cell : row) {
                    converted_matrix.back().push_back(pybind11::cast<std::complex<float>>(cell));
                }
            }
            return unitary_to_tableau(converted_matrix, little_endian);
        },
        pybind11::arg("matrix"),
        pybind11::kw_only(),
        pybind11::arg("endian"),
        clean_doc_string(u8R"DOC(
            @signature def from_unitary_matrix(matrix: Iterable[Iterable[float]], *, endian: str = 'little') -> stim.Tableau:
            Creates a tableau from the unitary matrix of a Clifford operation.

            Args:
                matrix: A unitary matrix specified as an iterable of rows, with each row is an iterable of amplitudes.
                    The unitary matrix must correspond to a Clifford operation.
                endian:
                    "little": matrix entries are in little endian order, where higher index qubits
                        correspond to larger changes in row/col indices.
                    "big": matrix entries are in little endian order, where higher index qubits correspond to
                        smaller changes in row/col indices.
            Returns:
                The tableau equivalent to the given unitary matrix (up to global phase).

            Raises:
                ValueError: The given matrix isn't the unitary matrix of a Clifford operation.

            Examples:
                >>> import stim
                >>> stim.Tableau.from_unitary_matrix([
                ...     [1, 0],
                ...     [0, 1j],
                ... ], endian='little')
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Y"),
                    ],
                    zs=[
                        stim.PauliString("+Z"),
                    ],
                )

                >>> stim.Tableau.from_unitary_matrix([
                ...     [1, 0, 0, 0],
                ...     [0, 1, 0, 0],
                ...     [0, 0, 0, -1j],
                ...     [0, 0, 1j, 0],
                ... ], endian='little')
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+XZ"),
                        stim.PauliString("+YX"),
                    ],
                    zs=[
                        stim.PauliString("+ZZ"),
                        stim.PauliString("+_Z"),
                    ],
                )
        )DOC")
            .data());

    c.def_static(
        "from_circuit",
        [](const Circuit &circuit, bool ignore_noise, bool ignore_measurement, bool ignore_reset) {
            return circuit_to_tableau(circuit, ignore_noise, ignore_measurement, ignore_reset);
        },
        pybind11::arg("circuit"),
        pybind11::kw_only(),
        pybind11::arg("ignore_noise") = false,
        pybind11::arg("ignore_measurement") = false,
        pybind11::arg("ignore_reset") = false,
        clean_doc_string(u8R"DOC(
            @signature def from_circuit(circuit: stim.Circuit, *, ignore_noise: bool = False, ignore_measurement: bool = False, ignore_reset: bool = False) -> stim.Tableau:
            Converts a circuit into an equivalent stabilizer tableau.

            Args:
                circuit: The circuit to compile into a tableau.
                ignore_noise: Defaults to False. When False, any noise operations in the circuit will cause
                    the conversion to fail with an exception. When True, noise operations are skipped over
                    as if they weren't even present in the circuit.
                ignore_measurement: Defaults to False. When False, any measurement operations in the circuit
                    will cause the conversion to fail with an exception. When True, measurement operations are
                    skipped over as if they weren't even present in the circuit.
                ignore_reset: Defaults to False. When False, any reset operations in the circuit will cause
                    the conversion to fail with an exception. When True, reset operations are skipped over
                    as if they weren't even present in the circuit.

            Returns:
                The tableau equivalent to the given circuit (up to global phase).

            Raises:
                ValueError:
                    The circuit contains noise operations but ignore_noise=False.
                    OR
                    The circuit contains measurement operations but ignore_measurement=False.
                    OR
                    The circuit contains reset operations but ignore_reset=False.

            Examples:
                >>> import stim
                >>> stim.Tableau.from_circuit(stim.Circuit("""
                ...     H 0
                ...     CNOT 0 1
                ... """))
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Z_"),
                        stim.PauliString("+_X"),
                    ],
                    zs=[
                        stim.PauliString("+XX"),
                        stim.PauliString("+ZZ"),
                    ],
                )
        )DOC")
            .data());

    c.def(
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
        },
        "Returns text that is a valid python expression evaluating to an equivalent `stim.Tableau`.");

    c.def(
        "__call__",
        [](const Tableau &self, const PyPauliString &pauli_string) {
            PyPauliString result{self(pauli_string.value)};
            if (pauli_string.imag) {
                result *= std::complex<float>(0, 1);
            }
            return result;
        },
        pybind11::arg("pauli_string"),
        clean_doc_string(u8R"DOC(
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
        )DOC")
            .data());

    c.def(
        pybind11::self + pybind11::self,
        pybind11::arg("rhs"),
        clean_doc_string(u8R"DOC(
            Returns the direct sum (diagonal concatenation) of two Tableaus.

            Args:
                rhs: A second stim.Tableau.

            Examples:
                >>> import stim

                >>> s = stim.Tableau.from_named_gate("S")
                >>> cz = stim.Tableau.from_named_gate("CZ")
                >>> print(s + cz)
                +-xz-xz-xz-
                | ++ ++ ++
                | YZ __ __
                | __ XZ Z_
                | __ Z_ XZ

            Returns:
                The direct sum.
        )DOC")
            .data());

    c.def(
        pybind11::self += pybind11::self,
        pybind11::arg("rhs"),
        clean_doc_string(u8R"DOC(
            Performs an inplace direct sum (diagonal concatenation).

            Args:
                rhs: A second stim.Tableau.

            Examples:
                >>> import stim

                >>> s = stim.Tableau.from_named_gate("S")
                >>> cz = stim.Tableau.from_named_gate("CZ")
                >>> alias = s
                >>> s += cz
                >>> alias is s
                True
                >>> print(s)
                +-xz-xz-xz-
                | ++ ++ ++
                | YZ __ __
                | __ XZ Z_
                | __ Z_ XZ

            Returns:
                The mutated tableau.
        )DOC")
            .data());

    c.def(pybind11::pickle(
        [](const Tableau &self) {
            pybind11::dict d;
            std::vector<PyPauliString> xs;
            std::vector<PyPauliString> zs;
            for (size_t q = 0; q < self.num_qubits; q++) {
                xs.push_back(PyPauliString(self.xs[q]));
            }
            for (size_t q = 0; q < self.num_qubits; q++) {
                zs.push_back(PyPauliString(self.zs[q]));
            }
            d["xs"] = xs;
            d["zs"] = zs;
            return d;
        },
        [](const pybind11::dict &d) {
            std::vector<PyPauliString> xs;
            std::vector<PyPauliString> zs;
            for (const auto &e : d["xs"]) {
                xs.push_back(pybind11::cast<PyPauliString>(e));
            }
            for (const auto &e : d["zs"]) {
                zs.push_back(pybind11::cast<PyPauliString>(e));
            }

            size_t n = xs.size();
            bool correct_shape = zs.size() == n;
            for (const auto &e : xs) {
                correct_shape &= !e.imag;
                correct_shape &= e.value.num_qubits == n;
            }
            for (const auto &e : zs) {
                correct_shape &= !e.imag;
                correct_shape &= e.value.num_qubits == n;
            }
            if (!correct_shape) {
                throw std::invalid_argument("Invalid pickle.");
            }

            Tableau result(n);
            for (size_t q = 0; q < n; q++) {
                result.xs[q] = xs[q].value;
                result.zs[q] = zs[q].value;
            }
            if (!result.satisfies_invariants()) {
                throw std::invalid_argument("Pickled tableau was invalid. It doesn't preserve commutativity.");
            }
            return result;
        }));
}
