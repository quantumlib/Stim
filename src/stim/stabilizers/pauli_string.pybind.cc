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

#include "stim/stabilizers/pauli_string.h"

#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/stabilizers/pauli_string.pybind.h"
#include "stim/stabilizers/tableau.h"

using namespace stim;
using namespace stim_pybind;

PyPauliString::PyPauliString(const PauliStringRef val, bool imag) : value(val), imag(imag) {
}

PyPauliString::PyPauliString(PauliString &&val, bool imag) : value(std::move(val)), imag(imag) {
}

PyPauliString PyPauliString::operator+(const PyPauliString &rhs) const {
    PyPauliString copy = *this;
    copy += rhs;
    return copy;
}

PyPauliString &PyPauliString::operator+=(const PyPauliString &rhs) {
    if (&rhs == this) {
        *this *= 2;
        return *this;
    }

    size_t n = value.num_qubits;
    value.ensure_num_qubits(value.num_qubits + rhs.value.num_qubits);
    for (size_t k = 0; k < rhs.value.num_qubits; k++) {
        value.xs[k + n] = rhs.value.xs[k];
        value.zs[k + n] = rhs.value.zs[k];
    }
    *this *= rhs.get_phase();
    return *this;
}

PyPauliString PyPauliString::operator*(std::complex<float> scale) const {
    PyPauliString copy = *this;
    copy *= scale;
    return copy;
}

PyPauliString PyPauliString::operator/(const std::complex<float> &scale) const {
    PyPauliString copy = *this;
    copy /= scale;
    return copy;
}

PyPauliString PyPauliString::operator*(const pybind11::object &rhs) const {
    PyPauliString copy = *this;
    copy *= rhs;
    return copy;
}

PyPauliString PyPauliString::operator*(size_t power) const {
    PyPauliString copy = *this;
    copy *= power;
    return copy;
}

PyPauliString &PyPauliString::operator*=(size_t power) {
    switch (power & 3) {
        case 0:
            imag = false;
            value.sign = false;
            break;
        case 1:
            break;
        case 2:
            value.sign = imag;
            imag = false;
            break;
        case 3:
            value.sign ^= imag;
            break;
    }

    value = PauliString::from_func(value.sign, value.num_qubits * power, [&](size_t k) {
        return "_XZY"[value.xs[k % value.num_qubits] + 2 * value.zs[k % value.num_qubits]];
    });
    return *this;
}

PyPauliString &PyPauliString::operator*=(const pybind11::object &rhs) {
    if (pybind11::isinstance<PyPauliString>(rhs)) {
        return *this *= pybind11::cast<PyPauliString>(rhs);
    } else if (rhs.equal(pybind11::cast(std::complex<float>{+1, 0}))) {
        return *this;
    } else if (rhs.equal(pybind11::cast(std::complex<float>{-1, 0}))) {
        return *this *= std::complex<float>{-1, 0};
    } else if (rhs.equal(pybind11::cast(std::complex<float>{0, 1}))) {
        return *this *= std::complex<float>{0, 1};
    } else if (rhs.equal(pybind11::cast(std::complex<float>{0, -1}))) {
        return *this *= std::complex<float>{0, -1};
    } else if (pybind11::isinstance<pybind11::int_>(rhs)) {
        pybind11::ssize_t k = pybind11::int_(rhs);
        if (k >= 0) {
            return *this *= (pybind11::size_t)k;
        }
    }
    throw std::out_of_range("need isinstance(rhs, (stim.PauliString, int)) or rhs in (1, -1, 1j, -1j)");
}

PyPauliString &PyPauliString::operator/=(const std::complex<float> &rhs) {
    if (rhs == std::complex<float>{+1, 0}) {
        return *this;
    } else if (rhs == std::complex<float>{-1, 0}) {
        return *this *= std::complex<float>{-1, 0};
    } else if (rhs == std::complex<float>{0, 1}) {
        return *this *= std::complex<float>{0, -1};
    } else if (rhs == std::complex<float>{0, -1}) {
        return *this *= std::complex<float>{0, +1};
    }
    throw std::invalid_argument("divisor not in (1, -1, 1j, -1j)");
}

PyPauliString &PyPauliString::operator*=(std::complex<float> scale) {
    if (scale == std::complex<float>(-1)) {
        value.sign ^= true;
    } else if (scale == std::complex<float>(0, 1)) {
        value.sign ^= imag;
        imag ^= true;
    } else if (scale == std::complex<float>(0, -1)) {
        imag ^= true;
        value.sign ^= imag;
    } else if (scale != std::complex<float>(1)) {
        throw std::invalid_argument("phase factor not in [1, -1, 1, 1j]");
    }
    return *this;
}

bool PyPauliString::operator==(const PyPauliString &other) const {
    return value == other.value && imag == other.imag;
}

bool PyPauliString::operator!=(const PyPauliString &other) const {
    return !(*this == other);
}

size_t numpy_to_size(const pybind11::object &numpy_array, size_t expected_size) {
    if (pybind11::isinstance<pybind11::array_t<uint8_t>>(numpy_array)) {
        auto arr = pybind11::cast<pybind11::array_t<uint8_t>>(numpy_array);
        if (arr.ndim() == 1) {
            size_t max_n = arr.shape(0) * 8;
            size_t min_n = max_n == 0 ? 0 : max_n - 7;
            if (expected_size == SIZE_MAX) {
                throw std::invalid_argument(
                    "Need to specify expected number of pauli terms (the `num_qubits` argument) when bit packing.\n"
                    "A numpy array is bit packed (has dtype=np.uint8) but `num_qubits=None`.");
            }
            if (expected_size < min_n || expected_size > max_n) {
                std::stringstream ss;
                ss << "Numpy array has dtype=np.uint8 (meaning it is bit packed) and shape=";
                ss << arr.shape(0) << " (meaning it has between " << min_n << " and " << max_n << " bits)";
                ss << " but len=" << expected_size << " is outside that range.";
                throw std::invalid_argument(ss.str());
            }
            return expected_size;
        }
    } else if (pybind11::isinstance<pybind11::array_t<bool>>(numpy_array)) {
        auto arr = pybind11::cast<pybind11::array_t<bool>>(numpy_array);
        if (arr.ndim() == 1) {
            size_t num_bits = arr.shape(0);
            if (expected_size != SIZE_MAX && num_bits != expected_size) {
                std::stringstream ss;
                ss << "Numpy array has dtype=bool8 and shape=" << num_bits << " which is different from the given len=" << expected_size;
                ss << ".\nEither don't specify len (as it is not needed when using bool8 arrays) or ensure the given len agrees with the given array shapes.";
                throw std::invalid_argument(ss.str());
            }
            return num_bits;
        }
    }
    throw std::invalid_argument("Bit data must be a 1-dimensional numpy array with dtype=np.uint8 or dtype=np.bool8");
}

size_t numpy_pair_to_size(const pybind11::object &numpy_array1, const pybind11::object &numpy_array2, const pybind11::object &expected_size) {
    size_t n0 = SIZE_MAX;
    if (!expected_size.is_none()) {
        n0 = pybind11::cast<size_t>(expected_size);
    }
    size_t n1 = numpy_to_size(numpy_array1, n0);
    size_t n2 = numpy_to_size(numpy_array2, n0);
    if (n1 != n2) {
        throw std::invalid_argument("Inconsistent array shapes.");
    }
    return n2;
}

std::complex<float> PyPauliString::get_phase() const {
    std::complex<float> result{value.sign ? -1.0f : +1.0f};
    if (imag) {
        result *= std::complex<float>{0, 1};
    }
    return result;
}

PyPauliString PyPauliString::operator*(const PyPauliString &rhs) const {
    PyPauliString copy = *this;
    copy *= rhs;
    return copy;
}

PyPauliString &PyPauliString::operator*=(const PyPauliString &rhs) {
    value.ensure_num_qubits(rhs.value.num_qubits);
    if (rhs.value.num_qubits < value.num_qubits) {
        PyPauliString copy = rhs;
        copy.value.ensure_num_qubits(value.num_qubits);
        *this *= copy;
        return *this;
    }

    uint8_t log_i = value.ref().inplace_right_mul_returning_log_i_scalar(rhs.value.ref());
    if (log_i & 2) {
        value.sign ^= true;
    }
    if (log_i & 1) {
        *this *= std::complex<float>{0, 1};
    }
    if (rhs.imag) {
        *this *= std::complex<float>{0, 1};
    }
    return *this;
}

std::string PyPauliString::str() const {
    auto sub = value.str();
    if (imag) {
        sub = sub.substr(0, 1) + "i" + sub.substr(1);
    }
    return sub;
}
PyPauliString PyPauliString::from_text(const char *text) {
    std::complex<float> factor{1, 0};
    int offset = 0;
    if (text[0] == 'i') {
        factor = {0, 1};
        offset = 1;
    } else if (text[0] == '-' && text[1] == 'i') {
        factor = {0, -1};
        offset = 2;
    } else if (text[0] == '+' && text[1] == 'i') {
        factor = {0, 1};
        offset = 2;
    }
    PyPauliString value{PauliString::from_str(text + offset), false};
    value *= factor;
    return value;
}

pybind11::class_<PyPauliString> stim_pybind::pybind_pauli_string(pybind11::module &m) {
    return pybind11::class_<PyPauliString>(
        m,
        "PauliString",
        clean_doc_string(u8R"DOC(
            A signed Pauli tensor product (e.g. "+X \u2297 X \u2297 X" or "-Y \u2297 Z".

            Represents a collection of Pauli operations (I, X, Y, Z) applied pairwise to a
            collection of qubits.

            Examples:
                >>> import stim
                >>> stim.PauliString("XX") * stim.PauliString("YY")
                stim.PauliString("-ZZ")
                >>> print(stim.PauliString(5))
                +_____
        )DOC")
            .data());
}

void stim_pybind::pybind_pauli_string_methods(pybind11::module &m, pybind11::class_<PyPauliString> &c) {
    c.def(
        pybind11::init([](size_t num_qubits) {
            PyPauliString result{PauliString(num_qubits), false};
            return result;
        }),
        pybind11::arg("num_qubits"),
        clean_doc_string(u8R"DOC(
            Creates an identity Pauli string over the given number of qubits.

            Examples:
                >>> import stim
                >>> p = stim.PauliString(5)
                >>> print(p)
                +_____

            Args:
                num_qubits: The number of qubits the Pauli string acts on.
        )DOC")
            .data());

    c.def(
        pybind11::init(&PyPauliString::from_text),
        pybind11::arg("text"),
        clean_doc_string(u8R"DOC(
            Creates a stim.PauliString from a text string.

            The string can optionally start with a sign ('+', '-', 'i', '+i', or '-i').
            The rest of the string should be characters from '_IXYZ' where
            '_' and 'I' mean identity, 'X' means Pauli X, 'Y' means Pauli Y, and 'Z' means
            Pauli Z.

            Examples:
                >>> import stim
                >>> print(stim.PauliString("YZ"))
                +YZ
                >>> print(stim.PauliString("+IXYZ"))
                +_XYZ
                >>> print(stim.PauliString("-___X_"))
                -___X_
                >>> print(stim.PauliString("iX"))
                +iX

            Args:
                text: A text description of the Pauli string's contents, such as "+XXX" or
                    "-_YX" or "-iZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZY".

            Returns:
                The created stim.PauliString.
        )DOC")
            .data());

    c.def(
        pybind11::init([](const PyPauliString &other) {
            PyPauliString copy = other;
            return copy;
        }),
        pybind11::arg("copy"),
        clean_doc_string(u8R"DOC(
            Creates a copy of a stim.PauliString.

            Examples:
                >>> import stim
                >>> a = stim.PauliString("YZ")
                >>> b = stim.PauliString(a)
                >>> b is a
                False
                >>> b == a
                True

            Args:
                copy: The pauli string to make a copy of.
        )DOC")
            .data());

    c.def(
        pybind11::init([](const std::vector<pybind11::ssize_t> &pauli_indices) {
            return PyPauliString(
                PauliString::from_func(
                    false,
                    pauli_indices.size(),
                    [&](size_t i) {
                        pybind11::ssize_t p = pauli_indices[i];
                        if (p < 0 || p > 3) {
                            throw std::invalid_argument(
                                "Expected a pauli index (0->I, 1->X, 2->Y, 3->Z) but got " + std::to_string(p));
                        }
                        return "_XYZ"[p];
                    }),
                false);
        }),
        pybind11::arg("pauli_indices"),
        clean_doc_string(u8R"DOC(
            Creates a stim.PauliString from a list of integer pauli indices.

            The indexing scheme that is used is:
                0 -> I
                1 -> X
                2 -> Y
                3 -> Z

            Examples:
                >>> import stim
                >>> stim.PauliString([0, 1, 2, 3, 0, 3])
                stim.PauliString("+_XYZ_Z")

            Args:
                pauli_indices: A sequence of integers from 0 to 3 (inclusive) indicating
                    paulis.
        )DOC")
            .data());

    c.def_static(
        "random",
        [](size_t num_qubits, bool allow_imaginary) {
            auto rng = make_py_seeded_rng(pybind11::none());
            return PyPauliString(PauliString::random(num_qubits, *rng), allow_imaginary ? ((*rng)() & 1) : false);
        },
        pybind11::arg("num_qubits"),
        pybind11::kw_only(),
        pybind11::arg("allow_imaginary") = false,
        clean_doc_string(u8R"DOC(
            Samples a uniformly random Hermitian Pauli string.

            Args:
                num_qubits: The number of qubits the Pauli string should act on.
                allow_imaginary: Defaults to False. If True, the sign of the result may be
                    1j or -1j in addition to +1 or -1. In other words, setting this to True
                    allows the result to be non-Hermitian.

            Examples:
                >>> import stim
                >>> p = stim.PauliString.random(5)
                >>> len(p)
                5
                >>> p.sign in [-1, +1]
                True

                >>> p2 = stim.PauliString.random(3, allow_imaginary=True)
                >>> len(p2)
                3
                >>> p2.sign in [-1, +1, 1j, -1j]
                True

            Returns:
                The sampled Pauli string.
        )DOC")
            .data());

    c.def(
        "to_tableau",
        [](const PyPauliString &self) {
            return Tableau::from_pauli_string(self.value);
        },
        clean_doc_string(u8R"DOC(
            Creates a Tableau equivalent to this Pauli string.

            The tableau represents a Clifford operation that multiplies qubits
            by the corresponding Pauli operations from this Pauli string.
            The global phase of the pauli operation is lost in the conversion.

            Returns:
                The created tableau.

            Examples:
                >>> import stim
                >>> p = stim.PauliString("ZZ")
                >>> p.to_tableau()
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("-X_"),
                        stim.PauliString("-_X"),
                    ],
                    zs=[
                        stim.PauliString("+Z_"),
                        stim.PauliString("+_Z"),
                    ],
                )
                >>> q = stim.PauliString("YX_Z")
                >>> q.to_tableau()
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("-X___"),
                        stim.PauliString("+_X__"),
                        stim.PauliString("+__X_"),
                        stim.PauliString("-___X"),
                    ],
                    zs=[
                        stim.PauliString("-Z___"),
                        stim.PauliString("-_Z__"),
                        stim.PauliString("+__Z_"),
                        stim.PauliString("+___Z"),
                    ],
                )
        )DOC").data());

    c.def(
        "commutes",
        [](const PyPauliString &self, const PyPauliString &other) {
            return self.value.ref().commutes(other.value.ref());
        },
        pybind11::arg("other"),
        clean_doc_string(u8R"DOC(
            Determines if two Pauli strings commute or not.

            Two Pauli strings commute if they have an even number of matched
            non-equal non-identity Pauli terms. Otherwise they anticommute.

            Args:
                other: The other Pauli string.

            Examples:
                >>> import stim
                >>> xx = stim.PauliString("XX")
                >>> xx.commutes(stim.PauliString("X_"))
                True
                >>> xx.commutes(stim.PauliString("XX"))
                True
                >>> xx.commutes(stim.PauliString("XY"))
                False
                >>> xx.commutes(stim.PauliString("XZ"))
                False
                >>> xx.commutes(stim.PauliString("ZZ"))
                True
                >>> xx.commutes(stim.PauliString("X_Y__"))
                True
                >>> xx.commutes(stim.PauliString(""))
                True

            Returns:
                True if the Pauli strings commute, False if they anti-commute.
        )DOC")
            .data());

    c.def("__str__", &PyPauliString::str, "Returns a text description.");

    c.def(
        "__repr__",
        [](const PyPauliString &self) {
            return "stim.PauliString(\"" + self.str() + "\")";
        },
        "Returns valid python code evaluating to an equivalent `stim.PauliString`.");

    c.def_property(
        "sign",
        &PyPauliString::get_phase,
        [](PyPauliString &self, std::complex<float> new_sign) {
            if (new_sign == std::complex<float>(1)) {
                self.value.sign = false;
                self.imag = false;
            } else if (new_sign == std::complex<float>(-1)) {
                self.value.sign = true;
                self.imag = false;
            } else if (new_sign == std::complex<float>(0, 1)) {
                self.value.sign = false;
                self.imag = true;
            } else if (new_sign == std::complex<float>(0, -1)) {
                self.value.sign = true;
                self.imag = true;
            } else {
                throw std::invalid_argument("new_sign not in [1, -1, 1, 1j]");
            }
        },
        clean_doc_string(u8R"DOC(
            The sign of the Pauli string. Can be +1, -1, 1j, or -1j.

            Examples:
                >>> import stim
                >>> stim.PauliString("X").sign
                (1+0j)
                >>> stim.PauliString("-X").sign
                (-1+0j)
                >>> stim.PauliString("iX").sign
                1j
                >>> stim.PauliString("-iX").sign
                (-0-1j)
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two Pauli strings have identical contents.");
    c.def(pybind11::self != pybind11::self, "Determines if two Pauli strings have non-identical contents.");

    c.def(
        "__len__",
        [](const PyPauliString &self) {
            return self.value.num_qubits;
        },
        clean_doc_string(u8R"DOC(
            Returns the length the pauli string; the number of qubits it operates on.
        )DOC")
            .data());

    c.def(
        "extended_product",
        [](const PyPauliString &self, const PyPauliString &other) {
            return std::make_tuple(std::complex<float>(1, 0), self * other);
        },
        pybind11::arg("other"),
        clean_doc_string(u8R"DOC(
             [DEPRECATED] Use multiplication (__mul__ or *) instead.
        )DOC")
            .data());

    c.def(
        pybind11::self + pybind11::self,
        pybind11::arg("rhs"),
        clean_doc_string(u8R"DOC(
            Returns the tensor product of two Pauli strings.

            Concatenates the Pauli strings and multiplies their signs.

            Args:
                rhs: A second stim.PauliString.

            Examples:
                >>> import stim

                >>> stim.PauliString("X") + stim.PauliString("YZ")
                stim.PauliString("+XYZ")

                >>> stim.PauliString("iX") + stim.PauliString("-X")
                stim.PauliString("-iXX")

            Returns:
                The tensor product.
        )DOC")
            .data());

    c.def(
        pybind11::self += pybind11::self,
        pybind11::arg("rhs"),
        clean_doc_string(u8R"DOC(
            Performs an inplace tensor product.

            Concatenates the given Pauli string onto the receiving string and multiplies
            their signs.

            Args:
                rhs: A second stim.PauliString.

            Examples:
                >>> import stim

                >>> p = stim.PauliString("iX")
                >>> alias = p
                >>> p += stim.PauliString("-YY")
                >>> p
                stim.PauliString("-iXYY")
                >>> alias is p
                True

            Returns:
                The mutated pauli string.
        )DOC")
            .data());

    c.def(
        pybind11::self * pybind11::object(),
        pybind11::arg("rhs"),
        clean_doc_string(u8R"DOC(
            Right-multiplies the Pauli string.

            Can multiply by another Pauli string, a complex unit, or a tensor power.

            Args:
                rhs: The right hand side of the multiplication. This can be:
                    - A stim.PauliString to right-multiply term-by-term with the paulis of
                        the pauli string.
                    - A complex unit (1, -1, 1j, -1j) to multiply with the sign of the pauli
                        string.
                    - A non-negative integer indicating the tensor power to raise the pauli
                        string to (how many times to repeat it).

            Examples:
                >>> import stim

                >>> stim.PauliString("X") * 1
                stim.PauliString("+X")
                >>> stim.PauliString("X") * -1
                stim.PauliString("-X")
                >>> stim.PauliString("X") * 1j
                stim.PauliString("+iX")

                >>> stim.PauliString("X") * 2
                stim.PauliString("+XX")
                >>> stim.PauliString("-X") * 2
                stim.PauliString("+XX")
                >>> stim.PauliString("iX") * 2
                stim.PauliString("-XX")
                >>> stim.PauliString("X") * 3
                stim.PauliString("+XXX")
                >>> stim.PauliString("iX") * 3
                stim.PauliString("-iXXX")

                >>> stim.PauliString("X") * stim.PauliString("Y")
                stim.PauliString("+iZ")
                >>> stim.PauliString("X") * stim.PauliString("XX_")
                stim.PauliString("+_X_")
                >>> stim.PauliString("XXXX") * stim.PauliString("_XYZ")
                stim.PauliString("+X_ZY")

            Returns:
                The product or tensor power.

            Raises:
                TypeError: The right hand side isn't a stim.PauliString, a non-negative
                    integer, or a complex unit (1, -1, 1j, or -1j).
        )DOC")
            .data());

    c.def(
        "__rmul__",
        [](const PyPauliString &self, const pybind11::object &lhs) {
            if (pybind11::isinstance<PyPauliString>(lhs)) {
                return pybind11::cast<PyPauliString>(lhs) * self;
            }
            return self * lhs;
        },
        pybind11::arg("lhs"),
        clean_doc_string(u8R"DOC(
            Left-multiplies the Pauli string.

            Can multiply by another Pauli string, a complex unit, or a tensor power.

            Args:
                lhs: The left hand side of the multiplication. This can be:
                    - A stim.PauliString to right-multiply term-by-term with the paulis of
                        the pauli string.
                    - A complex unit (1, -1, 1j, -1j) to multiply with the sign of the pauli
                        string.
                    - A non-negative integer indicating the tensor power to raise the pauli
                        string to (how many times to repeat it).

            Examples:
                >>> import stim

                >>> 1 * stim.PauliString("X")
                stim.PauliString("+X")
                >>> -1 * stim.PauliString("X")
                stim.PauliString("-X")
                >>> 1j * stim.PauliString("X")
                stim.PauliString("+iX")

                >>> 2 * stim.PauliString("X")
                stim.PauliString("+XX")
                >>> 2 * stim.PauliString("-X")
                stim.PauliString("+XX")
                >>> 2 * stim.PauliString("iX")
                stim.PauliString("-XX")
                >>> 3 * stim.PauliString("X")
                stim.PauliString("+XXX")
                >>> 3 * stim.PauliString("iX")
                stim.PauliString("-iXXX")

                >>> stim.PauliString("X") * stim.PauliString("Y")
                stim.PauliString("+iZ")
                >>> stim.PauliString("X") * stim.PauliString("XX_")
                stim.PauliString("+_X_")
                >>> stim.PauliString("XXXX") * stim.PauliString("_XYZ")
                stim.PauliString("+X_ZY")

            Returns:
                The product.

            Raises:
                ValueError: The scalar phase factor isn't 1, -1, 1j, or -1j.
        )DOC")
            .data());

    c.def(
        pybind11::self *= pybind11::object(),
        pybind11::arg("rhs"),
        clean_doc_string(u8R"DOC(
            Inplace right-multiplies the Pauli string.

            Can multiply by another Pauli string, a complex unit, or a tensor power.

            Args:
                rhs: The right hand side of the multiplication. This can be:
                    - A stim.PauliString to right-multiply term-by-term into the paulis of
                        the pauli string.
                    - A complex unit (1, -1, 1j, -1j) to multiply into the sign of the pauli
                        string.
                    - A non-negative integer indicating the tensor power to raise the pauli
                        string to (how many times to repeat it).

            Examples:
                >>> import stim

                >>> p = stim.PauliString("X")
                >>> p *= 1j
                >>> p
                stim.PauliString("+iX")

                >>> p = stim.PauliString("iXY_")
                >>> p *= 3
                >>> p
                stim.PauliString("-iXY_XY_XY_")

                >>> p = stim.PauliString("X")
                >>> alias = p
                >>> p *= stim.PauliString("Y")
                >>> alias
                stim.PauliString("+iZ")

                >>> p = stim.PauliString("X")
                >>> p *= stim.PauliString("_YY")
                >>> p
                stim.PauliString("+XYY")

            Returns:
                The mutated Pauli string.
        )DOC")
            .data());

    c.def(
        "__itruediv__",
        &PyPauliString::operator/=,
        pybind11::is_operator(),
        pybind11::arg("rhs"),
        clean_doc_string(u8R"DOC(
            Inplace divides the Pauli string by a complex unit.

            Args:
                rhs: The divisor. Can be 1, -1, 1j, or -1j.

            Examples:
                >>> import stim

                >>> p = stim.PauliString("X")
                >>> p /= 1j
                >>> p
                stim.PauliString("-iX")

            Returns:
                The mutated Pauli string.

            Raises:
                ValueError: The divisor isn't 1, -1, 1j, or -1j.
        )DOC")
            .data());

    c.def(
        "__truediv__",
        &PyPauliString::operator/,
        pybind11::is_operator(),
        pybind11::arg("rhs"),
        clean_doc_string(u8R"DOC(
            Divides the Pauli string by a complex unit.

            Args:
                rhs: The divisor. Can be 1, -1, 1j, or -1j.

            Examples:
                >>> import stim

                >>> stim.PauliString("X") / 1j
                stim.PauliString("-iX")

            Returns:
                The quotient.

            Raises:
                ValueError: The divisor isn't 1, -1, 1j, or -1j.
        )DOC")
            .data());

    c.def(
        "__neg__",
        [](const PyPauliString &self) {
            PyPauliString result = self;
            result.value.sign ^= 1;
            return result;
        },
        clean_doc_string(u8R"DOC(
            Returns the negation of the pauli string.

            Examples:
                >>> import stim
                >>> -stim.PauliString("X")
                stim.PauliString("-X")
                >>> -stim.PauliString("-Y")
                stim.PauliString("+Y")
                >>> -stim.PauliString("iZZZ")
                stim.PauliString("-iZZZ")
        )DOC")
            .data());

    c.def(
        "copy",
        [](const PyPauliString &self) {
            PyPauliString copy = self;
            return copy;
        },
        clean_doc_string(u8R"DOC(
            Returns a copy of the pauli string.

            The copy is an independent pauli string with the same contents.

            Examples:
                >>> import stim
                >>> p1 = stim.PauliString.random(2)
                >>> p2 = p1.copy()
                >>> p2 is p1
                False
                >>> p2 == p1
                True
        )DOC")
            .data());

    c.def(
        "to_numpy",
        [](const PyPauliString &self, bool bit_packed) {
            return pybind11::make_tuple(
                simd_bits_to_numpy(self.value.xs, self.value.num_qubits, bit_packed),
                simd_bits_to_numpy(self.value.zs, self.value.num_qubits, bit_packed));
        },
        pybind11::kw_only(),
        pybind11::arg("bit_packed") = false,
        clean_doc_string(u8R"DOC(
            @signature def to_numpy(self, *, bit_packed: bool = False) -> Tuple[np.ndarray, np.ndarray]:

            Decomposes the contents of the pauli string into X bit and Z bit numpy arrays.

            Args:
                bit_packed: Defaults to False. Determines whether the output numpy arrays
                    use dtype=bool8 or dtype=uint8 with 8 bools packed into each byte.

            Returns:
                An (xs, zs) tuple encoding the paulis from the string. The k'th Pauli from
                the string is encoded into k'th bit of xs and the k'th bit of zs using
                the "xz" encoding:

                    P=I -> x=0 and z=0
                    P=X -> x=1 and z=0
                    P=Y -> x=1 and z=1
                    P=Z -> x=0 and z=1

                The dtype and shape of the result depends on the bit_packed argument.

                If bit_packed=False:
                    Each bit gets its own byte.
                    xs.dtype = zs.dtype = np.bool8
                    xs.shape = zs.shape = len(self)
                    xs_k = xs[k]
                    zs_k = zs[k]

                If bit_packed=True:
                    Equivalent to applying np.packbits(bitorder='little') to the result.
                    xs.dtype = zs.dtype = np.uint8
                    xs.shape = zs.shape = math.ceil(len(self) / 8)
                    xs_k = (xs[k // 8] >> (k % 8)) & 1
                    zs_k = (zs[k // 8] >> (k % 8)) & 1

            Examples:
                >>> import stim

                >>> xs, zs = stim.PauliString("XXXXYYYZZ").to_numpy()
                >>> xs
                array([ True,  True,  True,  True,  True,  True,  True, False, False])
                >>> zs
                array([False, False, False, False,  True,  True,  True,  True,  True])

                >>> xs, zs = stim.PauliString("XXXXYYYZZ").to_numpy(bit_packed=True)
                >>> xs
                array([127,   0], dtype=uint8)
                >>> zs
                array([240,   1], dtype=uint8)
        )DOC")
            .data());

    c.def_static(
        "from_numpy",
        [](const pybind11::object &xs, const pybind11::object &zs, const pybind11::object &sign, const pybind11::object &num_qubits) -> PyPauliString {
            size_t n = numpy_pair_to_size(xs, zs, num_qubits);
            PyPauliString result{PauliString(n)};
            memcpy_bits_from_numpy_to_simd(n, xs, result.value.xs);
            memcpy_bits_from_numpy_to_simd(n, zs, result.value.zs);
            result *= sign;
            return result;
        },
        pybind11::kw_only(),
        pybind11::arg("xs"),
        pybind11::arg("zs"),
        pybind11::arg("sign") = +1,
        pybind11::arg("num_qubits") = pybind11::none(),
        clean_doc_string(u8R"DOC(
            @signature def from_numpy(*, xs: np.ndarray, zs: np.ndarray, sign: Union[int, float, complex] = +1, num_qubits: Optional[int] = None) -> stim.PauliString:

            Creates a pauli string from X bit and Z bit numpy arrays, using the encoding:

                x=0 and z=0 -> P=I
                x=1 and z=0 -> P=X
                x=1 and z=1 -> P=Y
                x=0 and z=1 -> P=Z

            Args:
                xs: The X bits of the pauli string. This array can either be a 1-dimensional
                    numpy array with dtype=np.bool8, or a bit packed 1-dimensional numpy
                    array with dtype=np.uint8. If the dtype is np.uint8 then the array is
                    assumed to be bit packed in little endian order and the "num_qubits"
                    argument must be specified. When bit packed, the x bit with offset k is
                    stored at (xs[k // 8] >> (k % 8)) & 1.
                zs: The Z bits of the pauli string. This array can either be a 1-dimensional
                    numpy array with dtype=np.bool8, or a bit packed 1-dimensional numpy
                    array with dtype=np.uint8. If the dtype is np.uint8 then the array is
                    assumed to be bit packed in little endian order and the "num_qubits"
                    argument must be specified. When bit packed, the x bit with offset k is
                    stored at (xs[k // 8] >> (k % 8)) & 1.
                sign: Defaults to +1. Set to +1, -1, 1j, or -1j to control the sign of the
                    returned Pauli string.
                num_qubits: Must be specified if xs or zs is a bit packed array. Specifies
                    the expected length of the Pauli string.

            Returns:
                The created pauli string.

            Examples:
                >>> import stim
                >>> import numpy as np

                >>> xs = np.array([1, 1, 1, 1, 1, 1, 1, 0, 0], dtype=np.bool8)
                >>> zs = np.array([0, 0, 0, 0, 1, 1, 1, 1, 1], dtype=np.bool8)
                >>> stim.PauliString.from_numpy(xs=xs, zs=zs, sign=-1)
                stim.PauliString("-XXXXYYYZZ")

                >>> xs = np.array([127, 0], dtype=np.uint8)
                >>> zs = np.array([240, 1], dtype=np.uint8)
                >>> stim.PauliString.from_numpy(xs=xs, zs=zs, num_qubits=9)
                stim.PauliString("+XXXXYYYZZ")
        )DOC")
            .data());

    c.def(
        "__pos__",
        [](const PyPauliString &self) {
            PyPauliString copy = self;
            return copy;
        },
        clean_doc_string(u8R"DOC(
            Returns a pauli string with the same contents.

            Examples:
                >>> import stim
                >>> +stim.PauliString("+X")
                stim.PauliString("+X")
                >>> +stim.PauliString("-YY")
                stim.PauliString("-YY")
                >>> +stim.PauliString("iZZZ")
                stim.PauliString("+iZZZ")
        )DOC")
            .data());

    c.def(
        "__setitem__",
        [](PyPauliString &self, pybind11::ssize_t index, const pybind11::object &arg_new_pauli) {
            if (index < 0) {
                index += self.value.num_qubits;
            }
            if (index < 0 || (size_t)index >= self.value.num_qubits) {
                throw std::out_of_range("index");
            }
            size_t u = (size_t)index;
            try {
                pybind11::ssize_t new_pauli = pybind11::cast<pybind11::ssize_t>(arg_new_pauli);
                if (new_pauli < 0 || new_pauli > 3) {
                    throw std::out_of_range("Expected new_pauli in [0, 1, 2, 3, '_', 'I', 'X', 'Y', 'Z']");
                }
                int z = (new_pauli >> 1) & 1;
                int x = (new_pauli & 1) ^ z;
                self.value.xs[u] = x;
                self.value.zs[u] = z;
            } catch (const pybind11::cast_error &) {
                char new_pauli = pybind11::cast<char>(arg_new_pauli);
                if (new_pauli == 'X') {
                    self.value.xs[u] = 1;
                    self.value.zs[u] = 0;
                } else if (new_pauli == 'Y') {
                    self.value.xs[u] = 1;
                    self.value.zs[u] = 1;
                } else if (new_pauli == 'Z') {
                    self.value.xs[u] = 0;
                    self.value.zs[u] = 1;
                } else if (new_pauli == 'I' || new_pauli == '_') {
                    self.value.xs[u] = 0;
                    self.value.zs[u] = 0;
                } else {
                    throw std::out_of_range("Expected new_pauli in [0, 1, 2, 3, '_', 'I', 'X', 'Y', 'Z']");
                }
            }
        },
        pybind11::arg("index"),
        pybind11::arg("new_pauli"),
        clean_doc_string(u8R"DOC(
           Mutates an entry in the pauli string using the encoding 0=I, 1=X, 2=Y, 3=Z.

           Args:
               index: The index of the pauli to overwrite.
               new_pauli: Either a character from '_IXYZ' or an integer from range(4).

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
        )DOC")
            .data());

    c.def(
        "__getitem__",
        [](const PyPauliString &self, const pybind11::object &index_or_slice) -> pybind11::object {
            pybind11::ssize_t start, step, slice_length;
            if (normalize_index_or_slice(index_or_slice, self.value.num_qubits, &start, &step, &slice_length)) {
                return pybind11::cast(PyPauliString(self.value.py_get_slice(start, step, slice_length)));
            } else {
                return pybind11::cast(self.value.py_get_item(start));
            }
        },
        pybind11::arg("index_or_slice"),
        clean_doc_string(u8R"DOC(
            Returns an individual Pauli or Pauli string slice from the pauli string.
            @overload def __getitem__(self, index_or_slice: int) -> int:
            @overload def __getitem__(self, index_or_slice: slice) -> stim.PauliString:

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
                index_or_slice: The index of the pauli to return, or the slice of paulis to
                    return.

            Returns:
                0: Identity.
                1: Pauli X.
                2: Pauli Y.
                3: Pauli Z.
        )DOC")
            .data());

    c.def(pybind11::pickle(
        [](const PyPauliString &self) -> pybind11::str {
            return self.str();
        },
        [](const pybind11::str &d) {
            return PyPauliString::from_text(pybind11::cast<std::string>(d).data());
        }));
}
