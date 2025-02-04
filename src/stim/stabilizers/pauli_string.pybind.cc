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

#include "stim/circuit/circuit_instruction.pybind.h"
#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/stabilizers/pauli_string.pybind.h"
#include "stim/stabilizers/pauli_string_iter.h"
#include "stim/stabilizers/tableau.h"

using namespace stim;
using namespace stim_pybind;

pybind11::object flex_pauli_string_to_unitary_matrix(const stim::FlexPauliString &ps, std::string_view endian) {
    bool little_endian;
    if (endian == "little") {
        little_endian = true;
    } else if (endian == "big") {
        little_endian = false;
    } else {
        throw std::invalid_argument("endian not in ['little', 'big']");
    }
    size_t q = ps.value.num_qubits;
    if (q >= 32) {
        throw std::invalid_argument("Too many qubits.");
    }
    size_t n = 1 << q;
    std::complex<float> *buffer = new std::complex<float>[n * n];
    uint64_t x = 0;
    uint64_t z = 0;
    for (size_t k = 0; k < q; k++) {
        x <<= 1;
        z <<= 1;
        if (little_endian) {
            x |= ps.value.xs[q - k - 1];
            z |= ps.value.zs[q - k - 1];
        } else {
            x |= ps.value.xs[k];
            z |= ps.value.zs[k];
        }
    }
    uint8_t start_phase = 0;
    start_phase += std::popcount(x & z);
    if (ps.imag) {
        start_phase += 1;
    }
    if (ps.value.sign) {
        start_phase += 2;
    }
    for (size_t col = 0; col < n; col++) {
        size_t row = col ^ x;
        uint8_t phase = start_phase;
        if (std::popcount(col & z) & 1) {
            phase += 2;
        }
        std::complex<float> v{1, 0};
        if (phase & 2) {
            v *= -1;
        }
        if (phase & 1) {
            v *= std::complex<float>{0, 1};
        }
        buffer[row * n + col] = v;
    }

    pybind11::capsule free_when_done(buffer, [](void *f) {
        delete[] reinterpret_cast<std::complex<float> *>(f);
    });

    pybind11::ssize_t sn = (pybind11::ssize_t)n;
    pybind11::ssize_t itemsize = sizeof(std::complex<float>);
    return pybind11::array_t<std::complex<float>>({sn, sn}, {sn * itemsize, itemsize}, buffer, free_when_done);
}

FlexPauliString &flex_pauli_string_obj_imul(FlexPauliString &lhs, const pybind11::object &rhs) {
    if (pybind11::isinstance<FlexPauliString>(rhs)) {
        return lhs *= pybind11::cast<FlexPauliString>(rhs);
    } else if (rhs.equal(pybind11::cast(std::complex<float>{+1, 0}))) {
        return lhs;
    } else if (rhs.equal(pybind11::cast(std::complex<float>{-1, 0}))) {
        return lhs *= std::complex<float>{-1, 0};
    } else if (rhs.equal(pybind11::cast(std::complex<float>{0, 1}))) {
        return lhs *= std::complex<float>{0, 1};
    } else if (rhs.equal(pybind11::cast(std::complex<float>{0, -1}))) {
        return lhs *= std::complex<float>{0, -1};
    } else if (pybind11::isinstance<pybind11::int_>(rhs)) {
        pybind11::ssize_t k = pybind11::int_(rhs);
        if (k >= 0) {
            return lhs *= (pybind11::size_t)k;
        }
    }
    throw std::out_of_range("need isinstance(rhs, (stim.PauliString, int)) or rhs in (1, -1, 1j, -1j)");
}

FlexPauliString flex_pauli_string_obj_mul(const FlexPauliString &lhs, const pybind11::object &rhs) {
    FlexPauliString copy = lhs;
    flex_pauli_string_obj_imul(copy, rhs);
    return copy;
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
                ss << "Numpy array has dtype=bool_ and shape=" << num_bits
                   << " which is different from the given len=" << expected_size;
                ss << ".\nEither don't specify len (as it is not needed when using bool_ arrays) or ensure the given "
                      "len agrees with the given array shapes.";
                throw std::invalid_argument(ss.str());
            }
            return num_bits;
        }
    }
    throw std::invalid_argument("Bit data must be a 1-dimensional numpy array with dtype=np.uint8 or dtype=np.bool_");
}

size_t numpy_pair_to_size(
    const pybind11::object &numpy_array1, const pybind11::object &numpy_array2, const pybind11::object &expected_size) {
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

FlexPauliString flex_pauli_string_from_unitary_matrix(
    const pybind11::object &matrix, std::string_view endian, bool ignore_sign) {
    bool little_endian;
    if (endian == "little") {
        little_endian = true;
    } else if (endian == "big") {
        little_endian = false;
    } else {
        throw std::invalid_argument("endian not in ['little', 'big']");
    }

    std::complex<float> unphase = 0;
    auto row_index_phase = [&](const pybind11::handle &row) -> std::tuple<uint64_t, uint8_t, uint64_t> {
        size_t num_cells = 0;
        size_t non_zero_index = SIZE_MAX;
        uint8_t phase = 0;
        for (const auto &cell : row) {
            auto c = pybind11::cast<std::complex<float>>(cell);
            if (abs(c) < 1e-2) {
                num_cells++;
                continue;
            }
            if (unphase == std::complex<float>{0}) {
                if (ignore_sign) {
                    unphase = conj(c);
                } else {
                    unphase = 1;
                }
            }
            c *= unphase;

            if (abs(c - std::complex<float>{1, 0}) < 1e-2) {
                phase = 0;
            } else if (abs(c - std::complex<float>{0, 1}) < 1e-2) {
                phase = 1;
            } else if (abs(c - std::complex<float>{-1, 0}) < 1e-2) {
                phase = 2;
            } else if (abs(c - std::complex<float>{0, -1}) < 1e-2) {
                phase = 3;
            } else {
                std::stringstream ss;
                ss << "The given unitary matrix isn't a Pauli string matrix. ";
                ss << "It has values besides 0, 1, -1, 1j, and -1j";
                if (ignore_sign) {
                    ss << " (up to global phase)";
                }
                ss << '.';
                throw std::invalid_argument(ss.str());
            }
            if (non_zero_index != SIZE_MAX) {
                throw std::invalid_argument(
                    "The given unitary matrix isn't a Pauli string matrix. It has two non-zero entries in the same "
                    "row.");
            }
            non_zero_index = num_cells;
            num_cells++;
        }
        if (non_zero_index == SIZE_MAX) {
            throw std::invalid_argument(
                "The given unitary matrix isn't a Pauli string matrix. It has a row with no non-zero entries.");
        }
        return {non_zero_index, phase, num_cells};
    };

    uint64_t x = 0;
    uint64_t n = 0;
    std::vector<uint8_t> phases;
    for (const auto &row : matrix) {
        auto row_data = row_index_phase(row);
        uint64_t index = std::get<0>(row_data) ^ (uint64_t)phases.size();
        phases.push_back(std::get<1>(row_data));
        size_t len = std::get<2>(row_data);
        if (phases.size() == 1) {
            x = index;
            n = len;
        } else {
            if (x != index) {
                throw std::invalid_argument(
                    "The given unitary matrix isn't a Pauli string matrix. Rows disagree about which qubits are "
                    "flipped.");
            }
            if (n != len) {
                throw std::invalid_argument(
                    "The given unitary matrix isn't a Pauli string matrix. Rows have different lengths.");
            }
        }
    }

    if (phases.size() != n) {
        throw std::invalid_argument("The given unitary matrix isn't a Pauli string matrix. It isn't square.");
    }
    if (n == 0 || (n & (n - 1))) {
        throw std::invalid_argument(
            "The given unitary matrix isn't a Pauli string matrix. Its height isn't a power of 2.");
    }

    uint64_t z = 0;
    size_t q = 0;
    for (size_t p = n >> 1; p > 0; p >>= 1) {
        z <<= 1;
        z |= ((phases[p] - phases[0]) & 2) != 0;
        q++;
    }
    for (size_t k = 0; k < n; k++) {
        uint8_t expected_phase = phases[0];
        if (std::popcount(k & z) & 1) {
            expected_phase += 2;
        }
        if ((expected_phase & 3) != phases[k]) {
            throw std::invalid_argument(
                "The given unitary matrix isn't a Pauli string matrix. It doesn't have consistent phase flips.");
        }
    }

    uint8_t leftover_phase = phases[0] + std::popcount(x & z);
    FlexPauliString result(q);
    result.imag = (leftover_phase & 1) != 0;
    result.value.sign = (leftover_phase & 2) != 0;
    auto &rx = result.value.xs.u64[0];
    auto &rz = result.value.zs.u64[0];
    if (little_endian) {
        rx = x;
        rz = z;
    } else {
        for (size_t k = 0; k < q; k++) {
            rx <<= 1;
            rz <<= 1;
            rx |= x & 1;
            rz |= z & 1;
            x >>= 1;
            z >>= 1;
        }
    }
    if (ignore_sign) {
        result.imag = false;
        result.value.sign = false;
    }
    return result;
}

pybind11::class_<FlexPauliString> stim_pybind::pybind_pauli_string(pybind11::module &m) {
    return pybind11::class_<FlexPauliString>(
        m,
        "PauliString",
        clean_doc_string(R"DOC(
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

void stim_pybind::pybind_pauli_string_methods(pybind11::module &m, pybind11::class_<FlexPauliString> &c) {
    c.def(
        pybind11::init(
            [](const pybind11::object &arg,
               const pybind11::object &num_qubits,
               const pybind11::object &text,
               const pybind11::object &other,
               const pybind11::object &pauli_indices) -> FlexPauliString {
                size_t count = 0;
                count += !arg.is_none();
                count += !num_qubits.is_none();
                count += !text.is_none();
                count += !other.is_none();
                count += !pauli_indices.is_none();
                if (count > 1) {
                    throw std::invalid_argument("Multiple arguments specified.");
                }
                if (count == 0) {
                    return FlexPauliString(0);
                }

                const auto &num_qubits_or = pybind11::isinstance<pybind11::int_>(arg) ? arg : num_qubits;
                if (!num_qubits_or.is_none()) {
                    return FlexPauliString(pybind11::cast<size_t>(num_qubits_or));
                }

                pybind11::object text_or = pybind11::isinstance<pybind11::str>(arg) ? arg : text;
                if (!text_or.is_none()) {
                    return FlexPauliString::from_text(pybind11::cast<std::string_view>(text_or));
                }

                pybind11::object other_or = pybind11::isinstance<FlexPauliString>(arg) ? arg : other;
                if (!other_or.is_none()) {
                    return pybind11::cast<FlexPauliString>(other_or);
                }

                pybind11::object pauli_indices_or = pybind11::isinstance<pybind11::iterable>(arg) ? arg : pauli_indices;
                if (!pauli_indices_or.is_none()) {
                    std::vector<uint8_t> ps;
                    for (const pybind11::handle &h : pauli_indices_or) {
                        int64_t v = -1;
                        if (pybind11::isinstance<pybind11::int_>(h)) {
                            try {
                                v = pybind11::cast<int64_t>(h);
                            } catch (const pybind11::cast_error &) {
                            }
                        } else if (pybind11::isinstance<pybind11::str>(h)) {
                            std::string_view s = pybind11::cast<std::string_view>(h);
                            if (s == "I" || s == "_") {
                                v = 0;
                            } else if (s == "X" || s == "x") {
                                v = 1;
                            } else if (s == "Y" || s == "y") {
                                v = 2;
                            } else if (s == "Z" || s == "z") {
                                v = 3;
                            }
                        }
                        if (v >= 0 && v < 4) {
                            ps.push_back((uint8_t)v);
                        } else {
                            throw std::invalid_argument(
                                "Don't know how to convert " + pybind11::cast<std::string>(pybind11::repr(h)) +
                                " into a pauli.\n"
                                "Expected something from {0, 1, 2, 3, 'I', 'X', 'Y', 'Z', '_'}.");
                        }
                    }
                    FlexPauliString result(ps.size());
                    for (size_t k = 0; k < ps.size(); k++) {
                        uint8_t p = ps[k];
                        p ^= p >> 1;
                        result.value.xs[k] = p & 1;
                        result.value.zs[k] = p & 2;
                    }
                    return result;
                }

                throw std::invalid_argument(
                    "Don't know how to initialize a stim.PauliString from " +
                    pybind11::cast<std::string>(pybind11::repr(arg)));
            }),
        pybind11::arg("arg") = pybind11::none(),
        pybind11::pos_only(),
        // These are no longer needed, and hidden from documentation, but are included to guarantee backwards
        // compatibility.
        pybind11::arg("num_qubits") = pybind11::none(),
        pybind11::arg("text") = pybind11::none(),
        pybind11::arg("other") = pybind11::none(),
        pybind11::arg("pauli_indices") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def __init__(self, arg: Union[None, int, str, stim.PauliString, Iterable[Union[int, 'Literal["_", "I", "X", "Y", "Z"]']]] = None, /) -> None:
            Initializes a stim.PauliString from the given argument.

            When given a string, the string is parsed as a pauli string. The string can
            optionally start with a sign ('+', '-', 'i', '+i', or '-i'). The rest of the
            string should be either a dense pauli string or a sparse pauli string. A dense
            pauli string is made up of characters from '_IXYZ' where '_' and 'I' mean
            identity, 'X' means Pauli X, 'Y' means Pauli Y, and 'Z' means Pauli Z. A sparse
            pauli string is a series of integers seperated by '*' and prefixed by 'I', 'X',
            'Y', or 'Z'.

            Arguments:
                arg [position-only]: This can be a variety of types, including:
                    None (default): initializes an empty Pauli string.
                    int: initializes an identity Pauli string of the given length.
                    str: initializes by parsing the given text.
                    stim.PauliString: initializes a copy of the given Pauli string.
                    Iterable: initializes by interpreting each item as a Pauli.
                        Each item can be a single-qubit Pauli string (like "X"),
                        or an integer. Integers use the convention 0=I, 1=X, 2=Y, 3=Z.

            Examples:
                >>> import stim

                >>> stim.PauliString("-XYZ")
                stim.PauliString("-XYZ")

                >>> stim.PauliString()
                stim.PauliString("+")

                >>> stim.PauliString(5)
                stim.PauliString("+_____")

                >>> stim.PauliString(stim.PauliString("XX"))
                stim.PauliString("+XX")

                >>> stim.PauliString([0, 1, 3, 2])
                stim.PauliString("+_XZY")

                >>> stim.PauliString("X" for _ in range(4))
                stim.PauliString("+XXXX")

                >>> stim.PauliString("-X2*Y6")
                stim.PauliString("-__X___Y")

                >>> stim.PauliString("X6*Y6")
                stim.PauliString("+i______Z")
        )DOC")
            .data());

    c.def_static(
        "random",
        [](size_t num_qubits, bool allow_imaginary) {
            auto rng = make_py_seeded_rng(pybind11::none());
            return FlexPauliString(
                PauliString<MAX_BITWORD_WIDTH>::random(num_qubits, rng), allow_imaginary ? (rng() & 1) : false);
        },
        pybind11::arg("num_qubits"),
        pybind11::kw_only(),
        pybind11::arg("allow_imaginary") = false,
        clean_doc_string(R"DOC(
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
        [](const FlexPauliString &self) {
            return Tableau<MAX_BITWORD_WIDTH>::from_pauli_string(self.value);
        },
        clean_doc_string(R"DOC(
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
        )DOC")
            .data());

    c.def(
        "to_unitary_matrix",
        &flex_pauli_string_to_unitary_matrix,
        pybind11::kw_only(),
        pybind11::arg("endian"),
        clean_doc_string(R"DOC(
            @signature def to_unitary_matrix(self, *, endian: str) -> np.ndarray[np.complex64]:
            Converts the pauli string into a unitary matrix.

            Args:
                endian:
                    "little": The first qubit is the least significant (corresponds
                        to an offset of 1 in the matrix).
                    "big": The first qubit is the most significant (corresponds
                        to an offset of 2**(n - 1) in the matrix).

            Returns:
                A numpy array with dtype=np.complex64 and
                shape=(1 << len(pauli_string), 1 << len(pauli_string)).

            Example:
                >>> import stim
                >>> stim.PauliString("-YZ").to_unitary_matrix(endian="little")
                array([[0.+0.j, 0.+1.j, 0.+0.j, 0.+0.j],
                       [0.-1.j, 0.+0.j, 0.+0.j, 0.+0.j],
                       [0.+0.j, 0.+0.j, 0.+0.j, 0.-1.j],
                       [0.+0.j, 0.+0.j, 0.+1.j, 0.+0.j]], dtype=complex64)
        )DOC")
            .data());

    c.def_static(
        "from_unitary_matrix",
        &flex_pauli_string_from_unitary_matrix,
        pybind11::arg("matrix"),
        pybind11::kw_only(),
        pybind11::arg("endian") = "little",
        pybind11::arg("unsigned") = false,
        clean_doc_string(R"DOC(
            @signature def from_unitary_matrix(matrix: Iterable[Iterable[Union[int, float, complex]]], *, endian: str = 'little', unsigned: bool = False) -> stim.PauliString:
            Creates a stim.PauliString from the unitary matrix of a Pauli group member.

            Args:
                matrix: A unitary matrix specified as an iterable of rows, with each row is
                    an iterable of amplitudes. The unitary matrix must correspond to a
                    Pauli string, including global phase.
                endian:
                    "little": matrix entries are in little endian order, where higher index
                        qubits correspond to larger changes in row/col indices.
                    "big": matrix entries are in big endian order, where higher index
                        qubits correspond to smaller changes in row/col indices.
                unsigned: When False, the input must only contain the values
                    [0, 1, -1, 1j, -1j] and the output will have the correct global phase.
                    When True, the input is permitted to be scaled by an arbitrary unit
                    complex value and the output will always have positive sign.
                    False is stricter but provides more information, while True is more
                    flexible but provides less information.

            Returns:
                The pauli string equal to the given unitary matrix.

            Raises:
                ValueError: The given matrix isn't the unitary matrix of a Pauli string.

            Examples:
                >>> import stim
                >>> stim.PauliString.from_unitary_matrix([
                ...     [1j, 0],
                ...     [0, -1j],
                ... ], endian='little')
                stim.PauliString("+iZ")

                >>> stim.PauliString.from_unitary_matrix([
                ...     [1j**0.1, 0],
                ...     [0, -(1j**0.1)],
                ... ], endian='little', unsigned=True)
                stim.PauliString("+Z")

                >>> stim.PauliString.from_unitary_matrix([
                ...     [0, 1, 0, 0],
                ...     [1, 0, 0, 0],
                ...     [0, 0, 0, -1],
                ...     [0, 0, -1, 0],
                ... ], endian='little')
                stim.PauliString("+XZ")
        )DOC")
            .data());

    c.def(
        "pauli_indices",
        [](const FlexPauliString &self, std::string_view include) {
            std::vector<uint64_t> result;
            size_t n64 = self.value.xs.num_u64_padded();
            bool keep_i = false;
            bool keep_x = false;
            bool keep_y = false;
            bool keep_z = false;
            for (char c : include) {
                switch (c) {
                    case '_':
                    case 'I':
                        keep_i = true;
                        break;
                    case 'x':
                    case 'X':
                        keep_x = true;
                        break;
                    case 'y':
                    case 'Y':
                        keep_y = true;
                        break;
                    case 'z':
                    case 'Z':
                        keep_z = true;
                        break;
                    default:
                        throw std::invalid_argument("Invalid character in include string: " + std::string(1, c));
                }
            }
            for (size_t k = 0; k < n64; k++) {
                uint64_t x = self.value.xs.u64[k];
                uint64_t z = self.value.zs.u64[k];
                uint64_t u = 0;
                if (keep_i) {
                    u |= ~x & ~z;
                }
                if (keep_x) {
                    u |= x & ~z;
                }
                if (keep_y) {
                    u |= x & z;
                }
                if (keep_z) {
                    u |= ~x & z;
                }
                while (u) {
                    uint8_t v = std::countr_zero(u);
                    uint64_t q = k * 64 + v;
                    if (q >= self.value.num_qubits) {
                        return result;
                    }
                    result.push_back(q);
                    u &= u - 1;
                }
            }
            return result;
        },
        pybind11::arg("included_paulis") = "XYZ",
        clean_doc_string(R"DOC(
            @signature def pauli_indices(self, included_paulis: str = "XYZ") -> List[int]:
            Returns the indices of non-identity Paulis, or of specified Paulis.

            Args:
                include: A string containing the Pauli types to include.
                    X type Pauli indices are included if "X" or "x" is in the string.
                    Y type Pauli indices are included if "Y" or "y" is in the string.
                    Z type Pauli indices are included if "Z" or "z" is in the string.
                    I type Pauli indices are included if "I" or "_" is in the string.
                    An exception is thrown if other characters are in the string.

            Returns:
                A list containing the ascending indices of matching Pauli terms.

            Examples:
                >>> import stim
                >>> stim.PauliString("_____X___Y____Z___").pauli_indices()
                [5, 9, 14]

                >>> stim.PauliString("_____X___Y____Z___").pauli_indices("XZ")
                [5, 14]

                >>> stim.PauliString("_____X___Y____Z___").pauli_indices("X")
                [5]

                >>> stim.PauliString("_____X___Y____Z___").pauli_indices("Y")
                [9]

                >>> stim.PauliString("_____X___Y____Z___").pauli_indices("IY")
                [0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17]

                >>> stim.PauliString("-X103*Y100").pauli_indices()
                [100, 103]
        )DOC")
            .data());

    c.def(
        "commutes",
        [](const FlexPauliString &self, const FlexPauliString &other) {
            return self.value.ref().commutes(other.value.ref());
        },
        pybind11::arg("other"),
        clean_doc_string(R"DOC(
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

    c.def("__str__", &FlexPauliString::str, "Returns a text description.");

    c.def(
        "__repr__",
        [](const FlexPauliString &self) {
            return "stim.PauliString(\"" + self.str() + "\")";
        },
        "Returns valid python code evaluating to an equivalent `stim.PauliString`.");

    c.def_property(
        "sign",
        &FlexPauliString::get_phase,
        [](FlexPauliString &self, std::complex<float> new_sign) {
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
                throw std::invalid_argument("new_sign not in [1, -1, 1j, -1j]");
            }
        },
        clean_doc_string(R"DOC(
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
        [](const FlexPauliString &self) {
            return self.value.num_qubits;
        },
        clean_doc_string(R"DOC(
            Returns the length the pauli string; the number of qubits it operates on.

            Examples:
                >>> import stim
                >>> len(stim.PauliString("XY_ZZ"))
                5
                >>> len(stim.PauliString("X0*Z99"))
                100
        )DOC")
            .data());

    c.def_property_readonly(
        "weight",
        [](const FlexPauliString &self) {
            return self.value.ref().weight();
        },
        clean_doc_string(R"DOC(
            Returns the number of non-identity pauli terms in the pauli string.

            Examples:
                >>> import stim
                >>> stim.PauliString("+___").weight
                0
                >>> stim.PauliString("+__X").weight
                1
                >>> stim.PauliString("+XYZ").weight
                3
                >>> stim.PauliString("-XXX___XXYZ").weight
                7
        )DOC")
            .data());

    c.def(
        "extended_product",
        [](const FlexPauliString &self, const FlexPauliString &other) {
            return std::make_tuple(std::complex<float>(1, 0), self * other);
        },
        pybind11::arg("other"),
        clean_doc_string(R"DOC(
             [DEPRECATED] Use multiplication (__mul__ or *) instead.
        )DOC")
            .data());

    c.def(
        pybind11::self + pybind11::self,
        pybind11::arg("rhs"),
        clean_doc_string(R"DOC(
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
        clean_doc_string(R"DOC(
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
        "__mul__",
        &flex_pauli_string_obj_mul,
        pybind11::arg("rhs"),
        clean_doc_string(R"DOC(
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
        [](const FlexPauliString &self, const pybind11::object &lhs) {
            if (pybind11::isinstance<FlexPauliString>(lhs)) {
                return pybind11::cast<FlexPauliString>(lhs) * self;
            }
            return flex_pauli_string_obj_mul(self, lhs);
        },
        pybind11::arg("lhs"),
        clean_doc_string(R"DOC(
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
        "__imul__",
        &flex_pauli_string_obj_imul,
        pybind11::arg("rhs"),
        clean_doc_string(R"DOC(
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
        &FlexPauliString::operator/=,
        pybind11::is_operator(),
        pybind11::arg("rhs"),
        clean_doc_string(R"DOC(
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
        &FlexPauliString::operator/,
        pybind11::is_operator(),
        pybind11::arg("rhs"),
        clean_doc_string(R"DOC(
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
        [](const FlexPauliString &self) {
            FlexPauliString result = self;
            result.value.sign ^= 1;
            return result;
        },
        clean_doc_string(R"DOC(
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
        [](const FlexPauliString &self) {
            FlexPauliString copy = self;
            return copy;
        },
        clean_doc_string(R"DOC(
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
        [](const FlexPauliString &self, bool bit_packed) {
            return pybind11::make_tuple(
                simd_bits_to_numpy(self.value.xs, self.value.num_qubits, bit_packed),
                simd_bits_to_numpy(self.value.zs, self.value.num_qubits, bit_packed));
        },
        pybind11::kw_only(),
        pybind11::arg("bit_packed") = false,
        clean_doc_string(R"DOC(
            @signature def to_numpy(self, *, bit_packed: bool = False) -> Tuple[np.ndarray, np.ndarray]:

            Decomposes the contents of the pauli string into X bit and Z bit numpy arrays.

            Args:
                bit_packed: Defaults to False. Determines whether the output numpy arrays
                    use dtype=bool_ or dtype=uint8 with 8 bools packed into each byte.

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
                    xs.dtype = zs.dtype = np.bool_
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
        [](const pybind11::object &xs,
           const pybind11::object &zs,
           const pybind11::object &sign,
           const pybind11::object &num_qubits) -> FlexPauliString {
            size_t n = numpy_pair_to_size(xs, zs, num_qubits);
            FlexPauliString result(n);
            memcpy_bits_from_numpy_to_simd(n, xs, result.value.xs);
            memcpy_bits_from_numpy_to_simd(n, zs, result.value.zs);
            flex_pauli_string_obj_imul(result, sign);
            return result;
        },
        pybind11::kw_only(),
        pybind11::arg("xs"),
        pybind11::arg("zs"),
        pybind11::arg("sign") = +1,
        pybind11::arg("num_qubits") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def from_numpy(*, xs: np.ndarray, zs: np.ndarray, sign: Union[int, float, complex] = +1, num_qubits: Optional[int] = None) -> stim.PauliString:

            Creates a pauli string from X bit and Z bit numpy arrays, using the encoding:

                x=0 and z=0 -> P=I
                x=1 and z=0 -> P=X
                x=1 and z=1 -> P=Y
                x=0 and z=1 -> P=Z

            Args:
                xs: The X bits of the pauli string. This array can either be a 1-dimensional
                    numpy array with dtype=np.bool_, or a bit packed 1-dimensional numpy
                    array with dtype=np.uint8. If the dtype is np.uint8 then the array is
                    assumed to be bit packed in little endian order and the "num_qubits"
                    argument must be specified. When bit packed, the x bit with offset k is
                    stored at (xs[k // 8] >> (k % 8)) & 1.
                zs: The Z bits of the pauli string. This array can either be a 1-dimensional
                    numpy array with dtype=np.bool_, or a bit packed 1-dimensional numpy
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

                >>> xs = np.array([1, 1, 1, 1, 1, 1, 1, 0, 0], dtype=np.bool_)
                >>> zs = np.array([0, 0, 0, 0, 1, 1, 1, 1, 1], dtype=np.bool_)
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
        [](const FlexPauliString &self) {
            FlexPauliString copy = self;
            return copy;
        },
        clean_doc_string(R"DOC(
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
        [](FlexPauliString &self, pybind11::ssize_t index, const pybind11::object &arg_new_pauli) {
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
        clean_doc_string(R"DOC(
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
        "after",
        [](const FlexPauliString &self,
           const pybind11::object &operation,
           const pybind11::object &targets) -> FlexPauliString {
            PauliString<MAX_BITWORD_WIDTH> result(0);
            if (pybind11::isinstance<Circuit>(operation)) {
                if (!targets.is_none()) {
                    throw std::invalid_argument("Don't specify 'targets' when the operation is a stim.Circuit");
                }
                result = self.value.ref().after(pybind11::cast<Circuit>(operation));
            } else if (pybind11::isinstance<PyCircuitInstruction>(operation)) {
                if (!targets.is_none()) {
                    throw std::invalid_argument(
                        "Don't specify 'targets' when the operation is a stim.CircuitInstruction");
                }
                result = self.value.ref().after(pybind11::cast<PyCircuitInstruction>(operation).as_operation_ref());
            } else if (pybind11::isinstance<Tableau<MAX_BITWORD_WIDTH>>(operation)) {
                if (targets.is_none()) {
                    throw std::invalid_argument("Must specify 'targets' when the operation is a stim.Tableau");
                }
                std::vector<size_t> raw_targets;
                for (const auto &e : targets) {
                    raw_targets.push_back(pybind11::cast<size_t>(e));
                }
                result = self.value.ref().after(pybind11::cast<Tableau<MAX_BITWORD_WIDTH>>(operation), raw_targets);
            } else {
                throw std::invalid_argument(
                    "Don't know how to apply " + pybind11::cast<std::string>(pybind11::repr(operation)));
            }
            return FlexPauliString(result, self.imag);
        },
        pybind11::arg("operation"),
        pybind11::arg("targets") = pybind11::none(),
        clean_doc_string(R"DOC(
            @overload def after(self, operation: Union[stim.Circuit, stim.CircuitInstruction]) -> stim.PauliString:
            @overload def after(self, operation: stim.Tableau, targets: Iterable[int]) -> stim.PauliString:
            @signature def after(self, operation: Union[stim.Circuit, stim.Tableau, stim.CircuitInstruction], targets: Optional[Iterable[int]] = None) -> stim.PauliString:
            Returns the result of conjugating the Pauli string by an operation.

            Args:
                operation: A circuit, tableau, or circuit instruction to
                    conjugate the Pauli string by. Must be Clifford (e.g.
                    if it's a circuit, the circuit can't have noise or
                    measurements).
                targets: Required if and only if the operation is a tableau.
                    Specifies which qubits to target.

            Examples:
                >>> import stim
                >>> p = stim.PauliString("_XYZ")

                >>> p.after(stim.CircuitInstruction("H", [1]))
                stim.PauliString("+_ZYZ")

                >>> p.after(stim.Circuit('''
                ...     C_XYZ 1 2 3
                ... '''))
                stim.PauliString("+_YZX")

                >>> p.after(stim.Tableau.from_named_gate('CZ'), targets=[0, 1])
                stim.PauliString("+ZXYZ")

            Returns:
                The conjugated Pauli string. The Pauli string after the
                operation that is exactly equivalent to the given Pauli
                string before the operation.
        )DOC")
            .data());

    c.def(
        "before",
        [](const FlexPauliString &self,
           const pybind11::object &operation,
           const pybind11::object &targets) -> FlexPauliString {
            PauliString<MAX_BITWORD_WIDTH> result(0);
            if (pybind11::isinstance<Circuit>(operation)) {
                if (!targets.is_none()) {
                    throw std::invalid_argument("Don't specify 'targets' when the operation is a stim.Circuit");
                }
                result = self.value.ref().before(pybind11::cast<Circuit>(operation));
            } else if (pybind11::isinstance<PyCircuitInstruction>(operation)) {
                if (!targets.is_none()) {
                    throw std::invalid_argument(
                        "Don't specify 'targets' when the operation is a stim.CircuitInstruction");
                }
                result = self.value.ref().before(pybind11::cast<PyCircuitInstruction>(operation).as_operation_ref());
            } else if (pybind11::isinstance<Tableau<MAX_BITWORD_WIDTH>>(operation)) {
                if (targets.is_none()) {
                    throw std::invalid_argument("Must specify 'targets' when the operation is a stim.Tableau");
                }
                std::vector<size_t> raw_targets;
                for (const auto &e : targets) {
                    raw_targets.push_back(pybind11::cast<size_t>(e));
                }
                result = self.value.ref().before(pybind11::cast<Tableau<MAX_BITWORD_WIDTH>>(operation), raw_targets);
            } else {
                throw std::invalid_argument(
                    "Don't know how to apply " + pybind11::cast<std::string>(pybind11::repr(operation)));
            }
            return FlexPauliString(result, self.imag);
        },
        pybind11::arg("operation"),
        pybind11::arg("targets") = pybind11::none(),
        clean_doc_string(R"DOC(
            @overload def before(self, operation: Union[stim.Circuit, stim.CircuitInstruction]) -> stim.PauliString:
            @overload def before(self, operation: stim.Tableau, targets: Iterable[int]) -> stim.PauliString:
            @signature def before(self, operation: Union[stim.Circuit, stim.Tableau, stim.CircuitInstruction], targets: Optional[Iterable[int]] = None) -> stim.PauliString:
            Returns the result of conjugating the Pauli string by an operation.

            Args:
                operation: A circuit, tableau, or circuit instruction to
                    anti-conjugate the Pauli string by. Must be Clifford (e.g.
                    if it's a circuit, the circuit can't have noise or
                    measurements).
                targets: Required if and only if the operation is a tableau.
                    Specifies which qubits to target.

            Examples:
                >>> import stim
                >>> p = stim.PauliString("_XYZ")

                >>> p.before(stim.CircuitInstruction("H", [1]))
                stim.PauliString("+_ZYZ")

                >>> p.before(stim.Circuit('''
                ...     C_XYZ 1 2 3
                ... '''))
                stim.PauliString("+_ZXY")

                >>> p.before(stim.Tableau.from_named_gate('CZ'), targets=[0, 1])
                stim.PauliString("+ZXYZ")

            Returns:
                The conjugated Pauli string. The Pauli string before the
                operation that is exactly equivalent to the given Pauli
                string after the operation.
        )DOC")
            .data());

    c.def(
        "__getitem__",
        [](const FlexPauliString &self, const pybind11::object &index_or_slice) -> pybind11::object {
            pybind11::ssize_t start, step, slice_length;
            if (normalize_index_or_slice(index_or_slice, self.value.num_qubits, &start, &step, &slice_length)) {
                return pybind11::cast(FlexPauliString(self.value.py_get_slice(start, step, slice_length)));
            } else {
                return pybind11::cast(self.value.py_get_item(start));
            }
        },
        pybind11::arg("index_or_slice"),
        clean_doc_string(R"DOC(
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

    c.def(
        pybind11::pickle(
            [](const FlexPauliString &self) -> pybind11::str {
                return self.str();
            },
            [](const pybind11::str &d) {
                return FlexPauliString::from_text(pybind11::cast<std::string_view>(d));
            }));

    c.def_static(
        "iter_all",
        [](size_t num_qubits,
           size_t min_weight,
           const pybind11::object &max_weight_obj,
           std::string_view allowed_paulis) -> PauliStringIterator<MAX_BITWORD_WIDTH> {
            bool allow_x = false;
            bool allow_y = false;
            bool allow_z = false;
            for (char c : allowed_paulis) {
                switch (c) {
                    case 'X':
                        allow_x = true;
                        break;
                    case 'Y':
                        allow_y = true;
                        break;
                    case 'Z':
                        allow_z = true;
                        break;
                    default:
                        throw std::invalid_argument(
                            "allowed_paulis='" + std::string(allowed_paulis) +
                            "' had characters other than 'X', 'Y', and 'Z'.");
                }
            }
            size_t max_weight = num_qubits;
            if (!max_weight_obj.is_none()) {
                int64_t v = pybind11::cast<int64_t>(max_weight_obj);
                if (v < 0) {
                    min_weight = 1;
                    max_weight = 0;
                } else {
                    max_weight = (size_t)v;
                }
            }
            return PauliStringIterator<MAX_BITWORD_WIDTH>(
                num_qubits, min_weight, max_weight, allow_x, allow_y, allow_z);
        },
        pybind11::arg("num_qubits"),
        pybind11::kw_only(),
        pybind11::arg("min_weight") = 0,
        pybind11::arg("max_weight") = pybind11::none(),
        pybind11::arg("allowed_paulis") = "XYZ",
        clean_doc_string(R"DOC(
            Returns an iterator that iterates over all matching pauli strings.

            Args:
                num_qubits: The desired number of qubits in the pauli strings.
                min_weight: Defaults to 0. The minimum number of non-identity terms that
                    must be present in each yielded pauli string.
                max_weight: Defaults to None (unused). The maximum number of non-identity
                    terms that must be present in each yielded pauli string.
                allowed_paulis: Defaults to "XYZ". Set this to a string containing the
                    non-identity paulis that are allowed to appear in each yielded pauli
                    string. This argument must be a string made up of only "X", "Y", and
                    "Z" characters. A non-identity Pauli is allowed if it appears in the
                    string, and not allowed if it doesn't. Identity Paulis are always
                    allowed.

            Returns:
                An Iterable[stim.PauliString] that yields the requested pauli strings.

            Examples:
                >>> import stim
                >>> pauli_string_iterator = stim.PauliString.iter_all(
                ...     num_qubits=3,
                ...     min_weight=1,
                ...     max_weight=2,
                ...     allowed_paulis="XZ",
                ... )
                >>> for p in pauli_string_iterator:
                ...     print(p)
                +X__
                +Z__
                +_X_
                +_Z_
                +__X
                +__Z
                +XX_
                +XZ_
                +ZX_
                +ZZ_
                +X_X
                +X_Z
                +Z_X
                +Z_Z
                +_XX
                +_XZ
                +_ZX
                +_ZZ
        )DOC")
            .data());
}
