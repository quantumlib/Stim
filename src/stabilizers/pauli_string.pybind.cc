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

#include "../stabilizers/pauli_string.h"

#include "../py/base.pybind.h"
#include "../simulators/tableau_simulator.h"
#include "../stabilizers/tableau.h"
#include "tableau.pybind.h"

void pybind_pauli_string(pybind11::module &m) {
    const char *SET_ITEM_DOC = R"DOC(
       Mutates an entry in the pauli string using the encoding 0=I, 1=X, 2=Y, 3=Z.

       Examples:
           >>> import stim
           >>> p = stim.PauliString(4)
           >>> p[2] = 1
           >>> print(p)
           +__X_
           >>> p[0] = 3
           >>> p[1] = 2
           >>> p[3] = 0
           >>> print(p)
           +ZYX_
           >>> p[0] = 'I'
           >>> p[1] = 'X'
           >>> p[2] = 'Y'
           >>> p[3] = 'Z'
           >>> print(p)
           +_XYZ
           >>> p[-1] = 'Y'
           >>> print(p)
           +_XYY

       Args:
           index: The index of the pauli to return.
    )DOC";
    const char *GET_ITEM_DOC = R"DOC(
        Returns an individual Pauli or Pauli string slice from the pauli string.

        Individual Paulis are returned as an int using the encoding 0=I, 1=X, 2=Y, 3=Z.
        Slices are returned as a stim.PauliString (always with positive sign).

        Examples:
            >>> import stim
            >>> p = stim.PauliString("_XYZ")
            >>> p[2]
            2
            >>> p[-1]
            3
            >>> p[:2]
            stim.PauliString("+_X")
            >>> p[::-1]
            stim.PauliString("+ZYX_")

        Args:
            index: The index of the pauli to return or slice of paulis to return.

        Returns:
            0: Identity.
            1: Pauli X.
            2: Pauli Y.
            3: Pauli Z.
     )DOC";
    const char *MUL_DOC = R"DOC(
        Returns the product of two commuting Pauli strings.

        The integers +1 and -1 are considered to be identity Pauli strings with the given sign.

        For anti-commuting Pauli strings, use 'stim.PauliString.extended_product' instead.

        Args:
            rhs: The right hand side Pauli string.

        Examples:
            >>> import stim
            >>> p1 = stim.PauliString("_XYZ")
            >>> p2 = stim.PauliString("_ZYX")
            >>> p3 = p1 * p2
            >>> print(p3)
            +_Y_Y
            >>> stim.PauliString("XX") * stim.PauliString("ZZ")
            stim.PauliString("-YY")

        Returns:
            The product of the two Pauli strings.

        Raises:
            ValueError: The Pauli strings don't commute, or have different lengths.
    )DOC";

    pybind11::class_<PauliString>(
        m, "PauliString",
        R"DOC(
            A signed Pauli product (e.g. "+X ⊗ X ⊗ X" or "-Y ⊗ Z".

            Represents a collection of Pauli operations (I, X, Y, Z) applied pairwise to a collection of qubits.

            Examples:
                >>> import stim
                >>> stim.PauliString("XX") * stim.PauliString("YY")
                stim.PauliString("-ZZ")
                >>> print(stim.PauliString(5))
                +_____

        )DOC")
        .def_static(
            "random",
            [](size_t num_qubits) {
                return PauliString::random(num_qubits, PYBIND_SHARED_RNG());
            },
            pybind11::arg("num_qubits"),
            R"DOC(
                Samples a uniformly random Pauli string over the given number of qubits.

                Args:
                    num_qubits: The number of qubits the Pauli string should act on.

                Examples:
                    >>> import stim
                    >>> p = stim.PauliString.random(5)

                Returns:
                    The sampled Pauli string.
            )DOC")
        .def("__str__", &PauliString::str)
        .def(
            "__repr__",
            [](const PauliString &self) {
                return "stim.PauliString(\"" + self.str() + "\")";
            })
        .def_property(
            "sign",
            [](const PauliString &self) {
                return self.sign ? -1 : +1;
            },
            [](PauliString &self, int new_sign) {
                if (new_sign != 1 && new_sign != -1) {
                    throw std::invalid_argument("new_sign != 1 && new_sign != -1");
                }
                self.sign = new_sign == -1;
            },
            R"DOC(
                The sign of the Pauli string. Can be +1 or -1. Imaginary signs are not supported.
            )DOC")
        .def(
            "__eq__",
            [](const PauliString &self, const PauliString &other) {
                return self == other;
            })
        .def(
            "__ne__",
            [](const PauliString &self, const PauliString &other) {
                return self != other;
            })
        .def(
            "__len__",
            [](const PauliString &self) {
                return self.num_qubits;
            })
        .def(
            "extended_product",
            [](const PauliString &self, const PauliString &other) {
                if (self.num_qubits != other.num_qubits) {
                    throw std::invalid_argument("len(self) != len(other)");
                }
                PauliString result = self;
                uint8_t log_i = result.ref().inplace_right_mul_returning_log_i_scalar(other);
                if (log_i & 2) {
                    result.sign ^= true;
                }
                return std::make_tuple(std::complex<float>(!(log_i & 1), log_i & 1), result);
            },
            pybind11::arg("other"),
            R"DOC(
                 Returns the product of two Pauli strings as a phase term and a non-imaginary Pauli string.

                 The phase term will be equal to 1 or to 1j. The true product is equal to the phase term
                 times the non-imaginary Pauli string.

                 Args:
                     other: The right hand side Pauli string.

                 Examples:
                     >>> import stim
                     >>> x = stim.PauliString("X_")
                     >>> z = stim.PauliString("Z_")
                     >>> x.extended_product(z)
                     (1j, stim.PauliString("-Y_"))
                     >>> z.extended_product(x)
                     (1j, stim.PauliString("+Y_"))
                     >>> x.extended_product(x)
                     ((1+0j), stim.PauliString("+__"))
                     >>> xx = stim.PauliString("XX")
                     >>> zz = stim.PauliString("ZZ")
                     >>> xx.extended_product(zz)
                     ((1+0j), stim.PauliString("-YY"))

                 Returns:
                     The product of the two Pauli strings.

                 Raises:
                     ValueError: The Pauli strings don't commute, or have different lengths.
             )DOC")
        .def(
            "__mul__",
            [](const PauliString &self, const PauliString &rhs) {
                if (self.num_qubits != rhs.num_qubits) {
                    throw std::invalid_argument("len(self) != len(other)");
                }
                PauliString result = self;
                uint8_t log_i = result.ref().inplace_right_mul_returning_log_i_scalar(rhs);
                if (log_i & 1) {
                    throw std::invalid_argument(
                        "Multiplied non-commuting Pauli strings.\n"
                        "Use stim.PauliString.extended_product instead of '*' "
                        "(i.e. stim.PauliString.__mul__) for this case.\n"
                        "stim.PauliString currently doesn't support storing imaginary signs.");
                }
                if (log_i & 2) {
                    result.sign ^= true;
                }
                return result;
            },
            pybind11::arg("rhs"), MUL_DOC)
        .def(
            "__mul__",
            [](const PauliString &self, int rhs) {
                if (rhs != -1 && rhs != +1) {
                    throw std::invalid_argument("rhs is int but rhs != -1 && rhs != +1");
                }
                PauliString result = self;
                result.sign ^= rhs == -1;
                return result;
            },
            pybind11::arg("rhs"), MUL_DOC)
        .def(
            "__neg__",
            [](const PauliString &self) {
                PauliString result = self;
                result.sign ^= 1;
                return result;
            })
        .def(
            "__pos__",
            [](const PauliString &self) {
                PauliString result = self;
                return result;
            })
        .def(
            "__rmul__",
            [](const PauliString &self, int lhs) {
                if (lhs != -1 && lhs != +1) {
                    throw std::invalid_argument("lhs is int but lhs != -1 && lhs != +1");
                }
                PauliString result = self;
                result.sign ^= lhs == -1;
                return result;
            },
            pybind11::arg("lhs"), MUL_DOC)
        .def(
            "__setitem__",
            [](PauliString &self, pybind11::ssize_t index, char new_pauli) {
                if (index < 0) {
                    index += self.num_qubits;
                }
                if (index < 0 || (size_t)index >= self.num_qubits) {
                    throw std::out_of_range("index");
                }
                size_t u = (size_t)index;
                if (new_pauli == 'X') {
                    self.xs[u] = 1;
                    self.zs[u] = 0;
                } else if (new_pauli == 'Y') {
                    self.xs[u] = 1;
                    self.zs[u] = 1;
                } else if (new_pauli == 'Z') {
                    self.xs[u] = 0;
                    self.zs[u] = 1;
                } else if (new_pauli == 'I' || new_pauli == '_') {
                    self.xs[u] = 0;
                    self.zs[u] = 0;
                } else {
                    throw std::out_of_range("Expected new_pauli in [0, 1, 2, 3, '_', 'I', 'X', 'Y', 'Z']");
                }
            },
            SET_ITEM_DOC)
        .def(
            "__setitem__",
            [](PauliString &self, pybind11::ssize_t index, int new_pauli) {
                if (index < 0) {
                    index += self.num_qubits;
                }
                if (index < 0 || (size_t)index >= self.num_qubits) {
                    throw std::out_of_range("index");
                }
                if (new_pauli < 0 || new_pauli > 3) {
                    throw std::out_of_range("Expected new_pauli in [0, 1, 2, 3, '_', 'I', 'X', 'Y', 'Z']");
                }
                size_t u = (size_t)index;
                int z = (new_pauli >> 1) & 1;
                int x = (new_pauli & 1) ^ z;
                self.xs[u] = x;
                self.zs[u] = z;
            },
            SET_ITEM_DOC)
        .def(
            "__getitem__",
            [](const PauliString &self, pybind11::ssize_t index) {
                if (index < 0) {
                    index += self.num_qubits;
                }
                if (index < 0 || (size_t)index >= self.num_qubits) {
                    throw std::out_of_range("index");
                }
                size_t u = (size_t)index;
                int x = self.xs[u];
                int z = self.zs[u];
                return (x ^ z) | (z << 1);
            },
            pybind11::arg("index"), GET_ITEM_DOC)
        .def(
            "__getitem__",
            [](const PauliString &self, pybind11::slice slice) {
                pybind11::ssize_t start, stop, step, n;
                if (!slice.compute(self.num_qubits, &start, &stop, &step, &n)) {
                    throw pybind11::error_already_set();
                }
                return PauliString::from_func(false, (size_t)n, [&](size_t i) {
                    int j = start + i * step;
                    if (j < 0) {
                        j += n;
                    }
                    return "_XZY"[self.xs[j] + self.zs[j] * 2];
                });
            },
            pybind11::arg("slice"), GET_ITEM_DOC)
        .def(
            pybind11::init(&PauliString::from_str), pybind11::arg("text"),
            R"DOC(
                Creates a stim.PauliString from a text string.

                The string can optionally start with a sign ('+' or '-').
                The rest of the string should be characters from '_IXYZ' where
                '_' and 'I' mean identity, 'X' means Pauli X, 'Y' means Pauli Y, and 'Z' means Pauli Z.

                Examples:
                    >>> import stim
                    >>> print(stim.PauliString("YZ"))
                    +YZ
                    >>> print(stim.PauliString("+IXYZ"))
                    +_XYZ
                    >>> print(stim.PauliString("-___X_"))
                    -___X_

                Args:
                    text: A text description of the Pauli string's contents, such as "+XXX" or "-_YX".
             )DOC")
        .def(
            pybind11::init<size_t>(), pybind11::arg("num_qubits"),
            R"DOC(
                Creates an identity Pauli string over the given number of qubits.

                Examples:
                    >>> import stim
                    >>> p = stim.PauliString(5)
                    >>> print(p)
                    +_____

                Args:
                    num_qubits: The number of qubits the Pauli string acts on.
             )DOC");
}
