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

#include "flex_pauli_string.h"
#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/tableau.h"
#include "stim/stabilizers/tableau_iter.h"
#include "stim/util_top/circuit_vs_amplitudes.h"
#include "stim/util_top/stabilizers_to_tableau.h"
#include "stim/util_top/stabilizers_vs_amplitudes.h"

using namespace stim;
using namespace stim_pybind;

void check_tableau_signs_shape(const pybind11::object &numpy_array, size_t n, const char *name) {
    if (pybind11::isinstance<pybind11::array_t<uint8_t>>(numpy_array)) {
        auto arr = pybind11::cast<pybind11::array_t<uint8_t>>(numpy_array);
        if (arr.ndim() == 1) {
            size_t minor = arr.shape(0);
            if (minor != (n + 7) / 8) {
                std::stringstream ss;
                ss << name << " had dtype=uint8 (meaning it is bit packed) ";
                ss << "but its shape was " << minor << " instead of ";
                ss << (n + 7) / 8 << ".";
                throw std::invalid_argument(ss.str());
            }
            return;
        }
    } else if (pybind11::isinstance<pybind11::array_t<bool>>(numpy_array)) {
        auto arr = pybind11::cast<pybind11::array_t<bool>>(numpy_array);
        if (arr.ndim() == 1) {
            size_t minor = arr.shape(0);
            if (minor != n) {
                std::stringstream ss;
                ss << name << " had dtype=bool_ ";
                ss << "but its shape was " << minor << " instead of ";
                ss << n << ".";
                throw std::invalid_argument(ss.str());
            }
        }
        return;
    }

    std::stringstream ss;
    ss << name << " wasn't a 1d numpy array with dtype=bool_ or dtype=uint8";
    throw std::invalid_argument(ss.str());
}

void check_tableau_shape(const pybind11::object &numpy_array, size_t n, const char *name) {
    if (pybind11::isinstance<pybind11::array_t<uint8_t>>(numpy_array)) {
        auto arr = pybind11::cast<pybind11::array_t<uint8_t>>(numpy_array);
        if (arr.ndim() == 2) {
            size_t major = arr.shape(0);
            size_t minor = arr.shape(1);
            if (major != n || minor != (n + 7) / 8) {
                std::stringstream ss;
                ss << name << " had dtype=uint8 (meaning it is bit packed) ";
                ss << "but its shape was (" << major << ", " << minor << ") instead of (";
                ss << n << ", " << (n + 7) / 8 << ").";
                throw std::invalid_argument(ss.str());
            }
            return;
        }
    } else if (pybind11::isinstance<pybind11::array_t<bool>>(numpy_array)) {
        auto arr = pybind11::cast<pybind11::array_t<bool>>(numpy_array);
        if (arr.ndim() == 2) {
            size_t major = arr.shape(0);
            size_t minor = arr.shape(1);
            if (major != n || minor != n) {
                std::stringstream ss;
                ss << name << " had dtype=bool_ ";
                ss << "but its shape was (" << major << ", " << minor << ") instead of (";
                ss << n << ", " << n << ").";
                throw std::invalid_argument(ss.str());
            }
        }
        return;
    }

    std::stringstream ss;
    ss << name << " wasn't a 2d numpy array with dtype=bool_ or dtype=uint8";
    throw std::invalid_argument(ss.str());
}

size_t determine_tableau_shape(const pybind11::object &numpy_array, const char *name) {
    size_t n = 0;
    if (pybind11::isinstance<pybind11::array_t<uint8_t>>(numpy_array)) {
        auto arr = pybind11::cast<pybind11::array_t<uint8_t>>(numpy_array);
        if (arr.ndim() == 2) {
            n = arr.shape(0);
        }
    } else if (pybind11::isinstance<pybind11::array_t<bool>>(numpy_array)) {
        auto arr = pybind11::cast<pybind11::array_t<bool>>(numpy_array);
        if (arr.ndim() == 2) {
            n = arr.shape(0);
        }
    }

    check_tableau_shape(numpy_array, n, name);
    return n;
}

pybind11::class_<Tableau<MAX_BITWORD_WIDTH>> stim_pybind::pybind_tableau(pybind11::module &m) {
    return pybind11::class_<Tableau<MAX_BITWORD_WIDTH>>(
        m,
        "Tableau",
        clean_doc_string(R"DOC(
            A stabilizer tableau.

            Represents a Clifford operation by explicitly storing how that operation
            conjugates a list of Pauli group generators into composite Pauli products.

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
}

void stim_pybind::pybind_tableau_methods(pybind11::module &m, pybind11::class_<Tableau<MAX_BITWORD_WIDTH>> &c) {
    c.def(
        pybind11::init<size_t>(),
        pybind11::arg("num_qubits"),
        clean_doc_string(R"DOC(
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
            auto rng = make_py_seeded_rng(pybind11::none());
            return Tableau<MAX_BITWORD_WIDTH>::random(num_qubits, rng);
        },
        pybind11::arg("num_qubits"),
        clean_doc_string(R"DOC(
            Samples a uniformly random Clifford operation and returns its tableau.

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
        [](size_t num_qubits, bool unsigned_only) -> TableauIterator<MAX_BITWORD_WIDTH> {
            return TableauIterator<MAX_BITWORD_WIDTH>(num_qubits, !unsigned_only);
        },
        pybind11::arg("num_qubits"),
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(R"DOC(
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
        [](Tableau<MAX_BITWORD_WIDTH> &self, std::string_view endian) {
            bool little_endian;
            if (endian == "little") {
                little_endian = true;
            } else if (endian == "big") {
                little_endian = false;
            } else {
                throw std::invalid_argument("endian not in ['little', 'big']");
            }
            auto data = self.to_flat_unitary_matrix(little_endian);

            std::complex<float> *buffer = new std::complex<float>[data.size()];
            for (size_t k = 0; k < data.size(); k++) {
                buffer[k] = data[k];
            }

            pybind11::capsule free_when_done(buffer, [](void *f) {
                delete[] reinterpret_cast<std::complex<float> *>(f);
            });

            pybind11::ssize_t n = 1 << self.num_qubits;
            pybind11::ssize_t itemsize = sizeof(std::complex<float>);
            return pybind11::array_t<std::complex<float>>({n, n}, {n * itemsize, itemsize}, buffer, free_when_done);
        },
        pybind11::kw_only(),
        pybind11::arg("endian"),
        clean_doc_string(R"DOC(
            @signature def to_unitary_matrix(self, *, endian: str) -> np.ndarray[np.complex64]:
            Converts the tableau into a unitary matrix.

            For an n-qubit tableau, this method performs O(n 4^n) work. It uses the state
            channel duality to transform the tableau into a list of stabilizers, then
            generates a random state vector and projects it into the +1 eigenspace of each
            stabilizer.

            Note that tableaus don't have a defined global phase, so the result's global
            phase may be different from what you expect. For example, the square of
            SQRT_X's unitary might equal -X instead of +X.

            Args:
                endian:
                    "little": The first qubit is the least significant (corresponds
                        to an offset of 1 in the state vector).
                    "big": The first qubit is the most significant (corresponds
                        to an offset of 2**(n - 1) in the state vector).

            Returns:
                A numpy array with dtype=np.complex64 and
                shape=(1 << len(tableau), 1 << len(tableau)).

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
        "to_numpy",
        [](const Tableau<MAX_BITWORD_WIDTH> &self, bool bit_packed) {
            auto n = self.num_qubits;

            return pybind11::make_tuple(
                simd_bit_table_to_numpy(self.xs.xt, n, n, bit_packed, false, pybind11::none()),
                simd_bit_table_to_numpy(self.xs.zt, n, n, bit_packed, false, pybind11::none()),
                simd_bit_table_to_numpy(self.zs.xt, n, n, bit_packed, false, pybind11::none()),
                simd_bit_table_to_numpy(self.zs.zt, n, n, bit_packed, false, pybind11::none()),
                simd_bits_to_numpy(self.xs.signs, n, bit_packed),
                simd_bits_to_numpy(self.zs.signs, n, bit_packed));
        },
        pybind11::kw_only(),
        pybind11::arg("bit_packed") = false,
        clean_doc_string(R"DOC(
            @signature def to_numpy(self, *, bit_packed: bool = False) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]:

            Decomposes the contents of the tableau into six numpy arrays.

            The first four numpy arrays correspond to the four quadrants of the table
            defined in Aaronson and Gottesman's "Improved Simulation of Stabilizer Circuits"
            ( https://arxiv.org/abs/quant-ph/0406196 ).

            The last two numpy arrays are the X and Z sign bit vectors of the tableau.

            Args:
                bit_packed: Defaults to False. Determines whether the output numpy arrays
                    use dtype=bool_ or dtype=uint8 with 8 bools packed into each byte.

            Returns:
                An (x2x, x2z, z2x, z2z, x_signs, z_signs) tuple encoding the tableau.

                x2x: A 2d table of whether tableau(X_i)_j is X or Y (instead of I or Z).
                x2z: A 2d table of whether tableau(X_i)_j is Z or Y (instead of I or X).
                z2x: A 2d table of whether tableau(Z_i)_j is X or Y (instead of I or Z).
                z2z: A 2d table of whether tableau(Z_i)_j is Z or Y (instead of I or X).
                x_signs: A vector of whether tableau(X_i) is negative.
                z_signs: A vector of whether tableau(Z_i) is negative.

                If bit_packed=False then:
                    x2x.dtype = np.bool_
                    x2z.dtype = np.bool_
                    z2x.dtype = np.bool_
                    z2z.dtype = np.bool_
                    x_signs.dtype = np.bool_
                    z_signs.dtype = np.bool_
                    x2x.shape = (len(tableau), len(tableau))
                    x2z.shape = (len(tableau), len(tableau))
                    z2x.shape = (len(tableau), len(tableau))
                    z2z.shape = (len(tableau), len(tableau))
                    x_signs.shape = len(tableau)
                    z_signs.shape = len(tableau)
                    x2x[i, j] = tableau.x_output_pauli(i, j) in [1, 2]
                    x2z[i, j] = tableau.x_output_pauli(i, j) in [2, 3]
                    z2x[i, j] = tableau.z_output_pauli(i, j) in [1, 2]
                    z2z[i, j] = tableau.z_output_pauli(i, j) in [2, 3]

                If bit_packed=True then:
                    x2x.dtype = np.uint8
                    x2z.dtype = np.uint8
                    z2x.dtype = np.uint8
                    z2z.dtype = np.uint8
                    x_signs.dtype = np.uint8
                    z_signs.dtype = np.uint8
                    x2x.shape = (len(tableau), math.ceil(len(tableau) / 8))
                    x2z.shape = (len(tableau), math.ceil(len(tableau) / 8))
                    z2x.shape = (len(tableau), math.ceil(len(tableau) / 8))
                    z2z.shape = (len(tableau), math.ceil(len(tableau) / 8))
                    x_signs.shape = math.ceil(len(tableau) / 8)
                    z_signs.shape = math.ceil(len(tableau) / 8)
                    (x2x[i, j // 8] >> (j % 8)) & 1 = tableau.x_output_pauli(i, j) in [1, 2]
                    (x2z[i, j // 8] >> (j % 8)) & 1 = tableau.x_output_pauli(i, j) in [2, 3]
                    (z2x[i, j // 8] >> (j % 8)) & 1 = tableau.z_output_pauli(i, j) in [1, 2]
                    (z2z[i, j // 8] >> (j % 8)) & 1 = tableau.z_output_pauli(i, j) in [2, 3]

            Examples:
                >>> import stim
                >>> cnot = stim.Tableau.from_named_gate("CNOT")
                >>> print(repr(cnot))
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+XX"),
                        stim.PauliString("+_X"),
                    ],
                    zs=[
                        stim.PauliString("+Z_"),
                        stim.PauliString("+ZZ"),
                    ],
                )
                >>> x2x, x2z, z2x, z2z, x_signs, z_signs = cnot.to_numpy()
                >>> x2x
                array([[ True,  True],
                       [False,  True]])
                >>> x2z
                array([[False, False],
                       [False, False]])
                >>> z2x
                array([[False, False],
                       [False, False]])
                >>> z2z
                array([[ True, False],
                       [ True,  True]])
                >>> x_signs
                array([False, False])
                >>> z_signs
                array([False, False])

                >>> t = stim.Tableau.from_conjugated_generators(
                ...     xs=[
                ...         stim.PauliString("-Y_ZY"),
                ...         stim.PauliString("-Y_YZ"),
                ...         stim.PauliString("-XXX_"),
                ...         stim.PauliString("+ZYX_"),
                ...     ],
                ...     zs=[
                ...         stim.PauliString("-_ZZX"),
                ...         stim.PauliString("+YZXZ"),
                ...         stim.PauliString("+XZ_X"),
                ...         stim.PauliString("-YYXX"),
                ...     ],
                ... )

                >>> x2x, x2z, z2x, z2z, x_signs, z_signs = t.to_numpy()
                >>> x2x
                array([[ True, False, False,  True],
                       [ True, False,  True, False],
                       [ True,  True,  True, False],
                       [False,  True,  True, False]])
                >>> x2z
                array([[ True, False,  True,  True],
                       [ True, False,  True,  True],
                       [False, False, False, False],
                       [ True,  True, False, False]])
                >>> z2x
                array([[False, False, False,  True],
                       [ True, False,  True, False],
                       [ True, False, False,  True],
                       [ True,  True,  True,  True]])
                >>> z2z
                array([[False,  True,  True, False],
                       [ True,  True, False,  True],
                       [False,  True, False, False],
                       [ True,  True, False, False]])
                >>> x_signs
                array([ True,  True,  True, False])
                >>> z_signs
                array([ True, False, False,  True])

                >>> x2x, x2z, z2x, z2z, x_signs, z_signs = t.to_numpy(bit_packed=True)
                >>> x2x
                array([[9],
                       [5],
                       [7],
                       [6]], dtype=uint8)
                >>> x2z
                array([[13],
                       [13],
                       [ 0],
                       [ 3]], dtype=uint8)
                >>> z2x
                array([[ 8],
                       [ 5],
                       [ 9],
                       [15]], dtype=uint8)
                >>> z2z
                array([[ 6],
                       [11],
                       [ 2],
                       [ 3]], dtype=uint8)
                >>> x_signs
                array([7], dtype=uint8)
                >>> z_signs
                array([9], dtype=uint8)
        )DOC")
            .data());

    c.def_static(
        "from_numpy",
        [](const pybind11::object &x2x,
           const pybind11::object &x2z,
           const pybind11::object &z2x,
           const pybind11::object &z2z,
           const pybind11::object &x_signs,
           const pybind11::object &z_signs) {
            size_t n = determine_tableau_shape(x2x, "x2x");
            check_tableau_shape(x2z, n, "x2z");
            check_tableau_shape(z2x, n, "z2x");
            check_tableau_shape(z2z, n, "z2z");
            if (!x_signs.is_none()) {
                check_tableau_signs_shape(x_signs, n, "x_signs");
            }
            if (!z_signs.is_none()) {
                check_tableau_signs_shape(z_signs, n, "z_signs");
            }

            Tableau<MAX_BITWORD_WIDTH> result(n);
            memcpy_bits_from_numpy_to_simd_bit_table(n, n, x2x, result.xs.xt);
            memcpy_bits_from_numpy_to_simd_bit_table(n, n, x2z, result.xs.zt);
            memcpy_bits_from_numpy_to_simd_bit_table(n, n, z2x, result.zs.xt);
            memcpy_bits_from_numpy_to_simd_bit_table(n, n, z2z, result.zs.zt);
            if (!x_signs.is_none()) {
                memcpy_bits_from_numpy_to_simd(n, x_signs, result.xs.signs);
            }
            if (!z_signs.is_none()) {
                memcpy_bits_from_numpy_to_simd(n, z_signs, result.zs.signs);
            }
            if (!result.satisfies_invariants()) {
                throw std::invalid_argument(
                    "The given tableau data don't describe a valid Clifford operation.\n"
                    "It doesn't preserve commutativity.\n"
                    "All generator outputs must commute, except for the output of X_k anticommuting with the output of "
                    "Z_k for each k.");
            }
            return result;
        },
        pybind11::kw_only(),
        pybind11::arg("x2x"),
        pybind11::arg("x2z"),
        pybind11::arg("z2x"),
        pybind11::arg("z2z"),
        pybind11::arg("x_signs") = pybind11::none(),
        pybind11::arg("z_signs") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def from_numpy(self, *, x2x: np.ndarray, x2z: np.ndarray, z2x: np.ndarray, z2z: np.ndarray, x_signs: Optional[np.ndarray] = None, z_signs: Optional[np.ndarray] = None) -> stim.Tableau:

            Creates a tableau from numpy arrays x2x, x2z, z2x, z2z, x_signs, and z_signs.

            The x2x, x2z, z2x, z2z arrays are the four quadrants of the table defined in
            Aaronson and Gottesman's "Improved Simulation of Stabilizer Circuits"
            ( https://arxiv.org/abs/quant-ph/0406196 ).

            Args:
                x2x: A 2d numpy array containing the x-to-x coupling bits. The bits can be
                    bit packed (dtype=uint8) or not (dtype=bool_). When not bit packed, the
                    result will satisfy result.x_output_pauli(i, j) in [1, 2] == x2x[i, j].
                    Bit packing must be in little endian order and only applies to the
                    second axis.
                x2z: A 2d numpy array containing the x-to-z coupling bits. The bits can be
                    bit packed (dtype=uint8) or not (dtype=bool_). When not bit packed, the
                    result will satisfy result.x_output_pauli(i, j) in [2, 3] == x2z[i, j].
                    Bit packing must be in little endian order and only applies to the
                    second axis.
                z2x: A 2d numpy array containing the z-to-x coupling bits. The bits can be
                    bit packed (dtype=uint8) or not (dtype=bool_). When not bit packed, the
                    result will satisfy result.z_output_pauli(i, j) in [1, 2] == z2x[i, j].
                    Bit packing must be in little endian order and only applies to the
                    second axis.
                z2z: A 2d numpy array containing the z-to-z coupling bits. The bits can be
                    bit packed (dtype=uint8) or not (dtype=bool_). When not bit packed, the
                    result will satisfy result.z_output_pauli(i, j) in [2, 3] == z2z[i, j].
                    Bit packing must be in little endian order and only applies to the
                    second axis.
                x_signs: Defaults to all-positive if not specified. A 1d numpy array
                    containing the sign bits for the X generator outputs. False means
                    positive and True means negative. The bits can be bit packed
                    (dtype=uint8) or not (dtype=bool_). Bit packing must be in little endian
                    order.
                z_signs: Defaults to all-positive if not specified. A 1d numpy array
                    containing the sign bits for the Z generator outputs. False means
                    positive and True means negative. The bits can be bit packed
                    (dtype=uint8) or not (dtype=bool_). Bit packing must be in little endian
                    order.

            Returns:
                The tableau created from the numpy data.

            Examples:
                >>> import stim
                >>> import numpy as np

                >>> tableau = stim.Tableau.from_numpy(
                ...     x2x=np.array([[1, 1], [0, 1]], dtype=np.bool_),
                ...     z2x=np.array([[0, 0], [0, 0]], dtype=np.bool_),
                ...     x2z=np.array([[0, 0], [0, 0]], dtype=np.bool_),
                ...     z2z=np.array([[1, 0], [1, 1]], dtype=np.bool_),
                ... )
                >>> tableau
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+XX"),
                        stim.PauliString("+_X"),
                    ],
                    zs=[
                        stim.PauliString("+Z_"),
                        stim.PauliString("+ZZ"),
                    ],
                )
                >>> tableau == stim.Tableau.from_named_gate("CNOT")
                True

                >>> stim.Tableau.from_numpy(
                ...     x2x=np.array([[9], [5], [7], [6]], dtype=np.uint8),
                ...     x2z=np.array([[13], [13], [0], [3]], dtype=np.uint8),
                ...     z2x=np.array([[8], [5], [9], [15]], dtype=np.uint8),
                ...     z2z=np.array([[6], [11], [2], [3]], dtype=np.uint8),
                ...     x_signs=np.array([7], dtype=np.uint8),
                ...     z_signs=np.array([9], dtype=np.uint8),
                ... )
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("-Y_ZY"),
                        stim.PauliString("-Y_YZ"),
                        stim.PauliString("-XXX_"),
                        stim.PauliString("+ZYX_"),
                    ],
                    zs=[
                        stim.PauliString("-_ZZX"),
                        stim.PauliString("+YZXZ"),
                        stim.PauliString("+XZ_X"),
                        stim.PauliString("-YYXX"),
                    ],
                )
        )DOC")
            .data());

    c.def(
        "to_pauli_string",
        [](const Tableau<MAX_BITWORD_WIDTH> &self) {
            return FlexPauliString(self.to_pauli_string());
        },
        clean_doc_string(R"DOC(
            Return a Pauli string equivalent to the tableau.

            If the tableau is equivalent to a pauli product, creates
            an equivalent pauli string. If not, then an error is raised.

            Returns:
                The created pauli string

            Raises:
                ValueError: The Tableau isn't equivalent to a Pauli product.

            Example:
                >>> import stim
                >>> t = (stim.Tableau.from_named_gate("Z") +
                ...      stim.Tableau.from_named_gate("Y") +
                ...      stim.Tableau.from_named_gate("I") +
                ...      stim.Tableau.from_named_gate("X"))
                >>> print(t)
                +-xz-xz-xz-xz-
                | -+ -- ++ +-
                | XZ __ __ __
                | __ XZ __ __
                | __ __ XZ __
                | __ __ __ XZ
                >>> print(t.to_pauli_string())
                +ZY_X
        )DOC")
            .data());

    c.def(
        "to_circuit",
        [](Tableau<MAX_BITWORD_WIDTH> &self, std::string_view method) {
            return tableau_to_circuit(self, method);
        },
        pybind11::arg("method") = "elimination",
        clean_doc_string(R"DOC(
            @signature def to_circuit(self, method: 'Literal["elimination", "graph_state"]' = 'elimination') -> stim.Circuit:
            Synthesizes a circuit that implements the tableau's Clifford operation.

            The circuits returned by this method are not guaranteed to be stable
            from version to version, and may be produced using randomization.

            Args:
                method: The method to use when synthesizing the circuit. Available values:
                    "elimination": Cancels off-diagonal terms using Gaussian elimination.
                        Gate set: H, S, CX
                        Circuit qubit count: n
                        Circuit operation count: O(n^2)
                        Circuit depth: O(n^2)
                    "graph_state": Prepares the tableau's state using a graph state circuit.
                        Gate set: RX, CZ, H, S, X, Y, Z
                        Circuit qubit count: n
                        Circuit operation count: O(n^2)

                        The circuit will be made up of three layers:
                            1. An RX layer initializing all qubits.
                            2. A CZ layer coupling the qubits.
                                (Each CZ is an edge in the graph state.)
                            3. A single qubit rotation layer.

                        Note: "graph_state" treats the tableau as a state instead of as a
                        Clifford operation. It will preserve the set of stabilizers, but
                        not the exact choice of generators.
                    "mpp_state": Prepares the tableau's state using MPP and feedback.
                        Gate set: MPP, CX rec, CY rec, CZ rec
                        Circuit qubit count: n
                        Circuit operation count: O(n^2)

                        The circuit will be made up of two layers:
                            1. An MPP layer measuring each of the tableau's stabilizers.
                            2. A feedback layer using the measurement results to control
                                whether or not to apply each of the tableau's destabilizers
                                in order to get the correct sign for each stabilizer.

                        Note: "mpp_state" treats the tableau as a state instead of as a
                        Clifford operation. It will preserve the set of stabilizers, but
                        not the exact choice of generators.
                    "mpp_state_unsigned": Prepares the tableau's state up to sign using MPP.
                        Gate set: MPP
                        Circuit qubit count: n
                        Circuit operation count: O(n^2)

                        The circuit will contain a series of MPP measurements measuring each
                        of the tableau's stabilizers. The stabilizers are measured in the
                        order used by the tableau (i.e. tableau.z_output(k) is the k'th
                        stabilizer measured).

                        Note: "mpp_state_unsigned" treats the tableau as a state instead of
                        as a Clifford operation. It will preserve the set of stabilizers,
                        but not the exact choice of generators.
            Returns:
                The synthesized circuit.

            Example:
                >>> import stim
                >>> tableau = stim.Tableau.from_conjugated_generators(
                ...     xs=[
                ...         stim.PauliString("+YZ__"),
                ...         stim.PauliString("-Y_XY"),
                ...         stim.PauliString("+___Y"),
                ...         stim.PauliString("+YZX_"),
                ...     ],
                ...     zs=[
                ...         stim.PauliString("+XZYY"),
                ...         stim.PauliString("-XYX_"),
                ...         stim.PauliString("-ZXXZ"),
                ...         stim.PauliString("+XXZ_"),
                ...     ],
                ... )

                >>> tableau.to_circuit()
                stim.Circuit('''
                    S 0
                    H 0 1 3
                    CX 0 1 0 2 0 3
                    S 1 3
                    H 1 3
                    CX 1 0 3 0 3 1 1 3 3 1
                    H 1
                    S 1
                    CX 1 3
                    H 2 3
                    CX 2 1 3 1 3 2 2 3 3 2
                    H 3
                    CX 2 3
                    S 3
                    H 3 0 1 2
                    S 0 0 1 1 2 2
                    H 0 1 2
                    S 3 3
                ''')

                >>> tableau.to_circuit("graph_state")
                stim.Circuit('''
                    RX 0 1 2 3
                    TICK
                    CZ 0 3 1 2 1 3
                    TICK
                    X 0 1
                    Z 2
                    S 2 3
                    H 3
                    S 3
                ''')

                >>> tableau.to_circuit("mpp_state_unsigned")
                stim.Circuit('''
                    MPP X0*Z1*Y2*Y3 !X0*Y1*X2 !Z0*X1*X2*Z3 X0*X1*Z2
                ''')

                >>> tableau.to_circuit("mpp_state")
                stim.Circuit('''
                    MPP X0*Z1*Y2*Y3 !X0*Y1*X2 !Z0*X1*X2*Z3 X0*X1*Z2
                    CX rec[-3] 2 rec[-1] 2
                    CY rec[-4] 0 rec[-3] 0 rec[-3] 3 rec[-2] 3 rec[-1] 0
                    CZ rec[-4] 1 rec[-1] 1
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
            return gate.tableau<MAX_BITWORD_WIDTH>();
        },
        pybind11::arg("name"),
        clean_doc_string(R"DOC(
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
        [](const Tableau<MAX_BITWORD_WIDTH> &self) {
            return self.num_qubits;
        },
        clean_doc_string(R"DOC(
            Returns the number of qubits operated on by the tableau.

            Examples:
                >>> import stim
                >>> t = stim.Tableau.from_named_gate("CNOT")
                >>> len(t)
                2
        )DOC")
            .data());

    c.def("__str__", &Tableau<MAX_BITWORD_WIDTH>::str, "Returns a text description.");

    c.def(pybind11::self == pybind11::self, "Determines if two tableaus have identical contents.");
    c.def(pybind11::self != pybind11::self, "Determines if two tableaus have non-identical contents.");

    c.def(
        "__pow__",
        &Tableau<MAX_BITWORD_WIDTH>::raised_to,
        pybind11::arg("exponent"),
        clean_doc_string(R"DOC(
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
        &Tableau<MAX_BITWORD_WIDTH>::inverse,
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(R"DOC(
            Computes the inverse of the tableau.

            The inverse T^-1 of a tableau T is the unique tableau with the property that
            T * T^-1 = T^-1 * T = I where I is the identity tableau.

            Args:
                unsigned: Defaults to false. When set to true, skips computing the signs of
                    the output observables and instead just set them all to be positive.
                    This is beneficial because computing the signs takes O(n^3) time and the
                    rest of the inverse computation is O(n^2) where n is the number of
                    qubits in the tableau. So, if you only need the Pauli terms (not the
                    signs), it is significantly cheaper.

            Returns:
                The inverse tableau.

            Examples:
                >>> import stim

                >>> # Check that the inverse agrees with hard-coded tableaus.
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
                ...     xs=[
                ...         stim.PauliString("-__Z"),
                ...         stim.PauliString("+XZ_"),
                ...         stim.PauliString("+_ZZ"),
                ...     ],
                ...     zs=[
                ...         stim.PauliString("-YYY"),
                ...         stim.PauliString("+Z_Z"),
                ...         stim.PauliString("-ZYZ")
                ...     ],
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
        [](Tableau<MAX_BITWORD_WIDTH> &self,
           const Tableau<MAX_BITWORD_WIDTH> &gate,
           const std::vector<size_t> targets) {
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
        clean_doc_string(R"DOC(
            @signature def append(self, gate: stim.Tableau, targets: Sequence[int]) -> None:
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
        [](const Tableau<MAX_BITWORD_WIDTH> &self, const Tableau<MAX_BITWORD_WIDTH> &second) {
            if (self.num_qubits != second.num_qubits) {
                throw std::invalid_argument("len(self) != len(second)");
            }
            return self.then(second);
        },
        pybind11::arg("second"),
        clean_doc_string(R"DOC(
            Returns the result of composing two tableaus.

            If the tableau T1 represents the Clifford operation with unitary C1,
            and the tableau T2 represents the Clifford operation with unitary C2,
            then the tableau T1.then(T2) represents the Clifford operation with unitary
            C2*C1.

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
        [](const Tableau<MAX_BITWORD_WIDTH> &self, const Tableau<MAX_BITWORD_WIDTH> &rhs) {
            if (self.num_qubits != rhs.num_qubits) {
                throw std::invalid_argument("len(lhs) != len(rhs)");
            }
            return rhs.then(self);
        },
        pybind11::arg("rhs"),
        clean_doc_string(R"DOC(
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
        [](Tableau<MAX_BITWORD_WIDTH> &self,
           const Tableau<MAX_BITWORD_WIDTH> &gate,
           const std::vector<size_t> targets) {
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
        clean_doc_string(R"DOC(
            @signature def prepend(self, gate: stim.Tableau, targets: Sequence[int]) -> None:
            Prepends an operation's effect into this tableau, mutating this tableau.

            Time cost is O(n*m*m) where n=len(self) and m=len(gate).

            Args:
                gate: The tableau of the operation being prepended into this tableau.
                targets: The qubits being targeted by the gate.

            Examples:
                >>> import stim
                >>> t = stim.Tableau.from_named_gate("H")
                >>> t.prepend(stim.Tableau.from_named_gate("X"), [0])
                >>> t == stim.Tableau.from_named_gate("SQRT_Y_DAG")
                True
        )DOC")
            .data());

    c.def(
        "x_output",
        [](Tableau<MAX_BITWORD_WIDTH> &self, size_t target) {
            if (target >= self.num_qubits) {
                throw std::invalid_argument("target >= len(tableau)");
            }
            return FlexPauliString(self.xs[target]);
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
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
        "x_sign",
        [](Tableau<MAX_BITWORD_WIDTH> &self, pybind11::ssize_t target) -> int {
            if (target < 0 || (size_t)target >= self.num_qubits) {
                throw std::invalid_argument("not 0 <= target < len(tableau)");
            }
            return self.xs.signs[target] ? -1 : +1;
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
            Returns just the sign of the result of conjugating an X generator.

            This operation runs in constant time.

            Args:
                target: The qubit the X generator applies to.

            Examples:
                >>> import stim
                >>> stim.Tableau.from_named_gate("S_DAG").x_sign(0)
                -1
                >>> stim.Tableau.from_named_gate("S").x_sign(0)
                1
        )DOC")
            .data());

    c.def(
        "z_sign",
        [](Tableau<MAX_BITWORD_WIDTH> &self, pybind11::ssize_t target) -> int {
            if (target < 0 || (size_t)target >= self.num_qubits) {
                throw std::invalid_argument("not 0 <= target < len(tableau)");
            }
            return self.zs.signs[target] ? -1 : +1;
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
            Returns just the sign of the result of conjugating a Z generator.

            This operation runs in constant time.

            Args:
                target: The qubit the Z generator applies to.

            Examples:
                >>> import stim
                >>> stim.Tableau.from_named_gate("SQRT_X_DAG").z_sign(0)
                1
                >>> stim.Tableau.from_named_gate("SQRT_X").z_sign(0)
                -1
        )DOC")
            .data());

    c.def(
        "y_sign",
        [](Tableau<MAX_BITWORD_WIDTH> &self, pybind11::ssize_t target) -> int {
            if (target < 0 || (size_t)target >= self.num_qubits) {
                throw std::invalid_argument("not 0 <= target < len(tableau)");
            }
            return self.y_output(target).sign ? -1 : +1;
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
            Returns just the sign of the result of conjugating a Y generator.

            Unlike x_sign and z_sign, this operation runs in linear time.
            The Y generator has to be computed by multiplying the X and Z
            outputs and the sign depends on all terms.

            Args:
                target: The qubit the Y generator applies to.

            Examples:
                >>> import stim
                >>> stim.Tableau.from_named_gate("S_DAG").y_sign(0)
                1
                >>> stim.Tableau.from_named_gate("S").y_sign(0)
                -1
        )DOC")
            .data());

    c.def(
        "y_output",
        [](Tableau<MAX_BITWORD_WIDTH> &self, size_t target) {
            if (target >= self.num_qubits) {
                throw std::invalid_argument("target >= len(tableau)");
            }
            return FlexPauliString(self.y_output(target));
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
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
        [](Tableau<MAX_BITWORD_WIDTH> &self, size_t target) {
            if (target >= self.num_qubits) {
                throw std::invalid_argument("target >= len(tableau)");
            }
            return FlexPauliString(self.zs[target]);
        },
        pybind11::arg("target"),
        clean_doc_string(R"DOC(
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
        &Tableau<MAX_BITWORD_WIDTH>::x_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(R"DOC(
            Constant-time version of `tableau.x_output(input_index)[output_index]`

            Args:
                input_index: Identifies the tableau column (the qubit of the input X
                    generator).
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
        &Tableau<MAX_BITWORD_WIDTH>::y_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(R"DOC(
            Constant-time version of `tableau.y_output(input_index)[output_index]`

            Args:
                input_index: Identifies the tableau column (the qubit of the input Y
                    generator).
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
        &Tableau<MAX_BITWORD_WIDTH>::z_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(R"DOC(
            Constant-time version of `tableau.z_output(input_index)[output_index]`

            Args:
                input_index: Identifies the tableau column (the qubit of the input Z
                    generator).
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
        &Tableau<MAX_BITWORD_WIDTH>::inverse_x_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(R"DOC(
            Constant-time version of `tableau.inverse().x_output(input_index)[output_index]`

            Args:
                input_index: Identifies the column (the qubit of the input X generator) in
                    the inverse tableau.
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
        &Tableau<MAX_BITWORD_WIDTH>::inverse_y_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(R"DOC(
            Constant-time version of `tableau.inverse().y_output(input_index)[output_index]`

            Args:
                input_index: Identifies the column (the qubit of the input Y generator) in
                    the inverse tableau.
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
        &Tableau<MAX_BITWORD_WIDTH>::inverse_z_output_pauli_xyz,
        pybind11::arg("input_index"),
        pybind11::arg("output_index"),
        clean_doc_string(R"DOC(
            Constant-time version of `tableau.inverse().z_output(input_index)[output_index]`

            Args:
                input_index: Identifies the column (the qubit of the input Z generator) in
                    the inverse tableau.
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
        [](const Tableau<MAX_BITWORD_WIDTH> &self, size_t input_index, bool skip_sign) {
            return FlexPauliString(self.inverse_x_output(input_index, skip_sign));
        },
        pybind11::arg("input_index"),
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(R"DOC(
            Conjugates a single-qubit X Pauli generator by the inverse of the tableau.

            A faster version of `tableau.inverse(unsigned).x_output(input_index)`.

            Args:
                input_index: Identifies the column (the qubit of the X generator) to return
                    from the inverse tableau.
                unsigned: Defaults to false. When set to true, skips computing the result's
                    sign and instead just sets it to positive. This is beneficial because
                    computing the sign takes O(n^2) time whereas all other parts of the
                    computation take O(n) time where n is the number of qubits in the
                    tableau.

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
        [](const Tableau<MAX_BITWORD_WIDTH> &self, size_t input_index, bool skip_sign) {
            return FlexPauliString(self.inverse_y_output(input_index, skip_sign));
        },
        pybind11::arg("input_index"),
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(R"DOC(
            Conjugates a single-qubit Y Pauli generator by the inverse of the tableau.

            A faster version of `tableau.inverse(unsigned).y_output(input_index)`.

            Args:
                input_index: Identifies the column (the qubit of the Y generator) to return
                    from the inverse tableau.
                unsigned: Defaults to false. When set to true, skips computing the result's
                    sign and instead just sets it to positive. This is beneficial because
                    computing the sign takes O(n^2) time whereas all other parts of the
                    computation take O(n) time where n is the number of qubits in the
                    tableau.

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
        [](const Tableau<MAX_BITWORD_WIDTH> &self, size_t input_index, bool skip_sign) {
            return FlexPauliString(self.inverse_z_output(input_index, skip_sign));
        },
        pybind11::arg("input_index"),
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(R"DOC(
            Conjugates a single-qubit Z Pauli generator by the inverse of the tableau.

            A faster version of `tableau.inverse(unsigned).z_output(input_index)`.

            Args:
                input_index: Identifies the column (the qubit of the Z generator) to return
                    from the inverse tableau.
                unsigned: Defaults to false. When set to true, skips computing the result's
                    sign and instead just sets it to positive. This is beneficial because
                    computing the sign takes O(n^2) time whereas all other parts of the
                    computation take O(n) time where n is the number of qubits in the
                    tableau.

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
        [](Tableau<MAX_BITWORD_WIDTH> &self) {
            Tableau<MAX_BITWORD_WIDTH> copy = self;
            return copy;
        },
        clean_doc_string(R"DOC(
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
        [](const std::vector<FlexPauliString> &xs, const std::vector<FlexPauliString> &zs) {
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
            Tableau<MAX_BITWORD_WIDTH> result(n);
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
        clean_doc_string(R"DOC(
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
        [](const pybind11::object &matrix, std::string_view endian) {
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
            return unitary_to_tableau<MAX_BITWORD_WIDTH>(converted_matrix, little_endian);
        },
        pybind11::arg("matrix"),
        pybind11::kw_only(),
        pybind11::arg("endian"),
        clean_doc_string(R"DOC(
            @signature def from_unitary_matrix(matrix: Iterable[Iterable[float]], *, endian: str = 'little') -> stim.Tableau:
            Creates a tableau from the unitary matrix of a Clifford operation.

            Args:
                matrix: A unitary matrix specified as an iterable of rows, with each row is
                    an iterable of amplitudes. The unitary matrix must correspond to a
                    Clifford operation.
                endian:
                    "little": matrix entries are in little endian order, where higher index
                        qubits correspond to larger changes in row/col indices.
                    "big": matrix entries are in big endian order, where higher index
                        qubits correspond to smaller changes in row/col indices.
            Returns:
                The tableau equivalent to the given unitary matrix (up to global phase).

            Raises:
                ValueError: The given matrix isn't the unitary matrix of a Clifford
                    operation.

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
            return circuit_to_tableau<MAX_BITWORD_WIDTH>(circuit, ignore_noise, ignore_measurement, ignore_reset);
        },
        pybind11::arg("circuit"),
        pybind11::kw_only(),
        pybind11::arg("ignore_noise") = false,
        pybind11::arg("ignore_measurement") = false,
        pybind11::arg("ignore_reset") = false,
        clean_doc_string(R"DOC(
            @signature def from_circuit(circuit: stim.Circuit, *, ignore_noise: bool = False, ignore_measurement: bool = False, ignore_reset: bool = False) -> stim.Tableau:
            Converts a circuit into an equivalent stabilizer tableau.

            Args:
                circuit: The circuit to compile into a tableau.
                ignore_noise: Defaults to False. When False, any noise operations in the
                    circuit will cause the conversion to fail with an exception. When True,
                    noise operations are skipped over as if they weren't even present in the
                    circuit.
                ignore_measurement: Defaults to False. When False, any measurement
                    operations in the circuit will cause the conversion to fail with an
                    exception. When True, measurement operations are skipped over as if they
                    weren't even present in the circuit.
                ignore_reset: Defaults to False. When False, any reset operations in the
                    circuit will cause the conversion to fail with an exception. When True,
                    reset operations are skipped over as if they weren't even present in the
                    circuit.

            Returns:
                The tableau equivalent to the given circuit (up to global phase).

            Raises:
                ValueError:
                    The circuit contains noise operations but ignore_noise=False.
                    OR
                    The circuit contains measurement operations but
                    ignore_measurement=False.
                    OR
                    The circuit contains reset operations but ignore_reset=False.

            Examples:
                >>> import stim
                >>> stim.Tableau.from_circuit(stim.Circuit('''
                ...     H 0
                ...     CNOT 0 1
                ... '''))
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
        [](const Tableau<MAX_BITWORD_WIDTH> &self) {
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
        "Returns valid python code evaluating to an equal `stim.Tableau`.");

    c.def(
        "__call__",
        [](const Tableau<MAX_BITWORD_WIDTH> &self, const FlexPauliString &pauli_string) {
            FlexPauliString result{self(pauli_string.value)};
            if (pauli_string.imag) {
                result *= std::complex<float>(0, 1);
            }
            return result;
        },
        pybind11::arg("pauli_string"),
        clean_doc_string(R"DOC(
             Returns the equivalent PauliString after the Tableau's Clifford operation.

             If P is a Pauli product before a Clifford operation C, then this method returns
             Q = C * P * C**-1 (the conjugation of P by C). Q is the equivalent Pauli product
             after C. This works because:
            
                 C*P
                 = C*P * I
                 = C*P * (C**-1 * C)
                 = (C*P*C**-1) * C
                 = Q*C

            (Keep in mind that A*B means first B is applied, then A is applied.)

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
        clean_doc_string(R"DOC(
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
        clean_doc_string(R"DOC(
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

    c.def(
        pybind11::pickle(
            [](const Tableau<MAX_BITWORD_WIDTH> &self) {
                pybind11::dict d;
                std::vector<FlexPauliString> xs;
                std::vector<FlexPauliString> zs;
                for (size_t q = 0; q < self.num_qubits; q++) {
                    xs.push_back(FlexPauliString(self.xs[q]));
                }
                for (size_t q = 0; q < self.num_qubits; q++) {
                    zs.push_back(FlexPauliString(self.zs[q]));
                }
                d["xs"] = xs;
                d["zs"] = zs;
                return d;
            },
            [](const pybind11::dict &d) {
                std::vector<FlexPauliString> xs;
                std::vector<FlexPauliString> zs;
                for (const auto &e : d["xs"]) {
                    xs.push_back(pybind11::cast<FlexPauliString>(e));
                }
                for (const auto &e : d["zs"]) {
                    zs.push_back(pybind11::cast<FlexPauliString>(e));
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

                Tableau<MAX_BITWORD_WIDTH> result(n);
                for (size_t q = 0; q < n; q++) {
                    result.xs[q] = xs[q].value;
                    result.zs[q] = zs[q].value;
                }
                if (!result.satisfies_invariants()) {
                    throw std::invalid_argument("Pickled tableau was invalid. It doesn't preserve commutativity.");
                }
                return result;
            }));

    c.def_static(
        "from_stabilizers",
        [](pybind11::object &stabilizers, bool allow_redundant, bool allow_underconstrained) {
            std::vector<PauliString<MAX_BITWORD_WIDTH>> converted_stabilizers;
            for (const auto &stabilizer : stabilizers) {
                const FlexPauliString &p = pybind11::cast<FlexPauliString>(stabilizer);
                if (p.imag) {
                    throw std::invalid_argument("Stabilizers can't have imaginary sign.");
                }
                converted_stabilizers.push_back(p.value);
            }
            return stabilizers_to_tableau<MAX_BITWORD_WIDTH>(
                converted_stabilizers, allow_redundant, allow_underconstrained, false);
        },
        pybind11::arg("stabilizers"),
        pybind11::kw_only(),
        pybind11::arg("allow_redundant") = false,
        pybind11::arg("allow_underconstrained") = false,
        clean_doc_string(R"DOC(
            @signature def from_stabilizers(stabilizers: Iterable[stim.PauliString], *, allow_redundant: bool = False, allow_underconstrained: bool = False) -> stim.Tableau:
            Creates a tableau representing a state with the given stabilizers.

            Args:
                stabilizers: A list of `stim.PauliString`s specifying the stabilizers that
                    the state must have. It is permitted for stabilizers to have different
                    lengths. All stabilizers are padded up to the length of the longest
                    stabilizer by appending identity terms.
                allow_redundant: Defaults to False. If set to False, then the given
                    stabilizers must all be independent. If any one of them is a product of
                    the others (including the empty product), an exception will be raised.
                    If set to True, then redundant stabilizers are simply ignored.
                allow_underconstrained: Defaults to False. If set to False, then the given
                    stabilizers must form a complete set of generators. They must exactly
                    specify the desired stabilizer state, with no degrees of freedom left
                    over. For an n-qubit state there must be n independent stabilizers. If
                    set to True, then there can be leftover degrees of freedom which can be
                    set arbitrarily.

            Returns:
                A tableau which, when applied to the all-zeroes state, produces a state
                with the given stabilizers.

                Guarantees that result.z_output(k) will be equal to the k'th independent
                stabilizer from the `stabilizers` argument.

            Raises:
                ValueError:
                    A stabilizer is redundant but allow_redundant=True wasn't set.
                    OR
                    The given stabilizers are contradictory (e.g. "+Z" and "-Z" both
                    specified).
                    OR
                    The given stabilizers anticommute (e.g. "+Z" and "+X" both specified).
                    OR
                    The stabilizers left behind a degree of freedom but
                    allow_underconstrained=True wasn't set.
                    OR
                    A stabilizer has an imaginary sign (i or -i).

            Examples:

                >>> import stim
                >>> stim.Tableau.from_stabilizers([
                ...     stim.PauliString("XX"),
                ...     stim.PauliString("ZZ"),
                ... ])
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

                >>> stim.Tableau.from_stabilizers([
                ...     stim.PauliString("XX_"),
                ...     stim.PauliString("ZZ_"),
                ...     stim.PauliString("-YY_"),
                ...     stim.PauliString(""),
                ... ], allow_underconstrained=True, allow_redundant=True)
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Z__"),
                        stim.PauliString("+_X_"),
                        stim.PauliString("+__X"),
                    ],
                    zs=[
                        stim.PauliString("+XX_"),
                        stim.PauliString("+ZZ_"),
                        stim.PauliString("+__Z"),
                    ],
                )
        )DOC")
            .data());

    c.def_static(
        "from_state_vector",
        [](pybind11::object &state_vector, std::string_view endian) {
            bool little_endian;
            if (endian == "little") {
                little_endian = true;
            } else if (endian == "big") {
                little_endian = false;
            } else {
                throw std::invalid_argument("endian not in ['little', 'big']");
            }

            std::vector<std::complex<float>> v;
            for (const auto &obj : state_vector) {
                v.push_back(pybind11::cast<std::complex<float>>(obj));
            }

            return circuit_to_tableau<MAX_BITWORD_WIDTH>(
                stabilizer_state_vector_to_circuit(v, little_endian), false, false, false);
        },
        pybind11::arg("state_vector"),
        pybind11::kw_only(),
        pybind11::arg("endian"),
        clean_doc_string(R"DOC(
            @signature def from_state_vector(state_vector: Iterable[float], *, endian: str) -> stim.Tableau:
            Creates a tableau representing the stabilizer state of the given state vector.

            Args:
                state_vector: A list of complex amplitudes specifying a superposition. The
                    vector must correspond to a state that is reachable using Clifford
                    operations, and can be unnormalized.
                endian:
                    "little": state vector is in little endian order, where higher index
                        qubits correspond to larger changes in the state index.
                    "big": state vector is in big endian order, where higher index qubits
                        correspond to smaller changes in the state index.

            Returns:
                A tableau which, when applied to the all-zeroes state, produces a state
                with the given state vector.

            Raises:
                ValueError:
                    The given state vector isn't a list of complex values specifying a
                    stabilizer state.
                    OR
                    The given endian value isn't 'little' or 'big'.

            Examples:

                >>> import stim
                >>> stim.Tableau.from_state_vector([
                ...     0.5**0.5,
                ...     0.5**0.5 * 1j,
                ... ], endian='little')
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Z"),
                    ],
                    zs=[
                        stim.PauliString("+Y"),
                    ],
                )
                >>> stim.Tableau.from_state_vector([
                ...     0.5**0.5,
                ...     0,
                ...     0,
                ...     0.5**0.5,
                ... ], endian='little')
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
        "to_state_vector",
        [](const Tableau<MAX_BITWORD_WIDTH> &self, std::string_view endian) {
            bool little_endian;
            if (endian == "little") {
                little_endian = true;
            } else if (endian == "big") {
                little_endian = false;
            } else {
                throw std::invalid_argument("endian not in ['little', 'big']");
            }
            TableauSimulator<MAX_BITWORD_WIDTH> sim(std::mt19937_64{0}, self.num_qubits);
            sim.inv_state = self.inverse(false);
            auto complex_vec = sim.to_state_vector(little_endian);

            std::complex<float> *buffer = new std::complex<float>[complex_vec.size()];
            for (size_t k = 0; k < complex_vec.size(); k++) {
                buffer[k] = complex_vec[k];
            }

            pybind11::capsule free_when_done(buffer, [](void *f) {
                delete[] reinterpret_cast<std::complex<float> *>(f);
            });

            return pybind11::array_t<std::complex<float>>(
                {(pybind11::ssize_t)complex_vec.size()},
                {(pybind11::ssize_t)sizeof(std::complex<float>)},
                buffer,
                free_when_done);
        },
        pybind11::kw_only(),
        pybind11::arg("endian") = "little",
        clean_doc_string(R"DOC(
            @signature def to_state_vector(self, *, endian: str = 'little') -> np.ndarray[np.complex64]:
            Returns the state vector produced by applying the tableau to the |0..0> state.

            This function takes O(n * 2**n) time and O(2**n) space, where n is the number of
            qubits. The computation is done by initialization a random state vector and
            iteratively projecting it into the +1 eigenspace of each stabilizer of the
            state. The state is then canonicalized so that zero values are actually exactly
            0, and so that the first non-zero entry is positive.

            Args:
                endian:
                    "little" (default): state vector is in little endian order, where higher
                        index qubits correspond to larger changes in the state index.
                    "big": state vector is in big endian order, where higher index qubits
                        correspond to smaller changes in the state index.

            Returns:
                A `numpy.ndarray[numpy.complex64]` of computational basis amplitudes.

                If the result is in little endian order then the amplitude at offset
                b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is the amplitude for the
                computational basis state where the qubit with index 0 is storing the bit
                b_0, the qubit with index 1 is storing the bit b_1, etc.

                If the result is in big endian order then the amplitude at offset
                b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is the amplitude for the
                computational basis state where the qubit with index 0 is storing the bit
                b_{n-1}, the qubit with index 1 is storing the bit b_{n-2}, etc.

            Examples:
                >>> import stim
                >>> import numpy as np
                >>> i2 = stim.Tableau.from_named_gate('I')
                >>> x = stim.Tableau.from_named_gate('X')
                >>> h = stim.Tableau.from_named_gate('H')

                >>> (x + i2).to_state_vector(endian='little')
                array([0.+0.j, 1.+0.j, 0.+0.j, 0.+0.j], dtype=complex64)

                >>> (i2 + x).to_state_vector(endian='little')
                array([0.+0.j, 0.+0.j, 1.+0.j, 0.+0.j], dtype=complex64)

                >>> (i2 + x).to_state_vector(endian='big')
                array([0.+0.j, 1.+0.j, 0.+0.j, 0.+0.j], dtype=complex64)

                >>> (h + h).to_state_vector(endian='little')
                array([0.5+0.j, 0.5+0.j, 0.5+0.j, 0.5+0.j], dtype=complex64)
        )DOC")
            .data());

    c.def(
        "to_stabilizers",
        [](const Tableau<MAX_BITWORD_WIDTH> &self, bool canonical) {
            auto stabilizers = self.stabilizers(canonical);
            std::vector<FlexPauliString> result;
            result.reserve(stabilizers.size());
            for (auto &s : stabilizers) {
                result.emplace_back(std::move(s), false);
            }
            return result;
        },
        pybind11::kw_only(),
        pybind11::arg("canonicalize") = false,
        clean_doc_string(R"DOC(
            Returns the stabilizer generators of the tableau, optionally canonicalized.

            The stabilizer generators of the tableau are its Z outputs. Canonicalizing
            standardizes the generators, so that states that are equal will produce the
            same generators. For example, [ZI, IZ], [ZI, ZZ], amd [ZZ, ZI] describe equal
            states and all canonicalize to [ZI, IZ].

            The canonical form is computed as follows:

                1. Get a list of stabilizers using `tableau.z_output(k)` for each k.
                2. Perform Gaussian elimination. pivoting on standard generators.
                    2a) Pivot on g=X0 first, then Z0, X1, Z1, X2, Z2, etc.
                    2b) Find a stabilizer that uses the generator g. If there are none,
                        go to the next g.
                    2c) Multiply that stabilizer into all other stabilizers that use the
                        generator g.
                    2d) Swap that stabilizer with the stabilizer at position `r` then
                        increment `r`. `r` starts at 0.

            Args:
                canonicalize: Defaults to False. When False, the tableau's Z outputs
                    are returned unchanged. When True, the Z outputs are rewritten
                    into a standard form. Two stabilizer states have the same standard
                    form if and only if they describe equivalent quantum states.

            Returns:
                A List[stim.PauliString] of the tableau's stabilizer generators.

            Examples:
                >>> import stim
                >>> t = stim.Tableau.from_named_gate("CNOT")

                >>> raw_stabilizers = t.to_stabilizers()
                >>> for e in raw_stabilizers:
                ...     print(repr(e))
                stim.PauliString("+Z_")
                stim.PauliString("+ZZ")

                >>> canonical_stabilizers = t.to_stabilizers(canonicalize=True)
                >>> for e in canonical_stabilizers:
                ...     print(repr(e))
                stim.PauliString("+Z_")
                stim.PauliString("+_Z")
        )DOC")
            .data());
}
