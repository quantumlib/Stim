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
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/tableau.h"
#include "stim/stabilizers/tableau_iter.h"

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
                simd_bit_table_to_numpy(self.xs.xt, n, n, bit_packed),
                simd_bit_table_to_numpy(self.xs.zt, n, n, bit_packed),
                simd_bit_table_to_numpy(self.zs.xt, n, n, bit_packed),
                simd_bit_table_to_numpy(self.zs.zt, n, n, bit_packed),
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
}
