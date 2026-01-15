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

#include "stim/stabilizers/clifford_string.pybind.h"

#include "stim/gates/gates.pybind.h"
#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/stabilizers/flex_pauli_string.h"
#include "stim/stabilizers/pauli_string.h"

using namespace stim;
using namespace stim_pybind;

pybind11::class_<CliffordString<MAX_BITWORD_WIDTH>> stim_pybind::pybind_clifford_string(pybind11::module &m) {
    return pybind11::class_<CliffordString<MAX_BITWORD_WIDTH>>(
        m,
        "CliffordString",
        clean_doc_string(R"DOC(
            A tensor product of single qubit Clifford gates (e.g. "H \u2297 X \u2297 S").

            Represents a collection of Clifford operations applied pairwise to a
            collection of qubits. Ignores global phase.

            Examples:
                >>> import stim
                >>> stim.CliffordString("H,S,C_XYZ") * stim.CliffordString("H,H,H")
                stim.CliffordString("I,C_ZYX,SQRT_X_DAG")
        )DOC")
            .data());
}

static std::string_view trim(std::string_view text) {
    size_t s = 0;
    size_t e = text.size();
    while (s < e && std::isspace(text[s])) {
        s++;
    }
    while (s < e && std::isspace(text[e - 1])) {
        e--;
    }
    return text.substr(s, e - s);
}

void stim_pybind::pybind_clifford_string_methods(
    pybind11::module &m, pybind11::class_<CliffordString<MAX_BITWORD_WIDTH>> &c) {
    c.def(
        pybind11::init([](const pybind11::object &arg) -> CliffordString<MAX_BITWORD_WIDTH> {
            if (pybind11::isinstance<pybind11::int_>(arg)) {
                return CliffordString<MAX_BITWORD_WIDTH>(pybind11::cast<int64_t>(arg));
            }

            if (pybind11::isinstance<pybind11::str>(arg)) {
                std::string_view text = pybind11::cast<std::string_view>(arg);
                text = trim(text);
                if (text.empty()) {
                    return CliffordString<MAX_BITWORD_WIDTH>(0);
                }
                if (text.ends_with(',')) {
                    text = text.substr(0, text.size() - 1);
                }

                size_t n = 1;
                for (char e : text) {
                    n += e == ',';
                }
                CliffordString<MAX_BITWORD_WIDTH> result(n);
                size_t start = 0;
                size_t out_index = 0;
                for (size_t end = 0; end <= text.size(); end++) {
                    if (end == text.size() || text[end] == ',') {
                        std::string_view segment = text.substr(start, end - start);
                        segment = trim(segment);
                        auto [z_sign, x_sign, inv_x2x, x2z, z2x, inv_z2z] = gate_to_bits(GATE_DATA.at(segment).id);
                        result.z_signs[out_index] = z_sign;
                        result.x_signs[out_index] = x_sign;
                        result.inv_x2x[out_index] = inv_x2x;
                        result.x2z[out_index] = x2z;
                        result.z2x[out_index] = z2x;
                        result.inv_z2z[out_index] = inv_z2z;
                        start = end + 1;
                        out_index += 1;
                    }
                }
                return result;
            }

            if (pybind11::isinstance<CliffordString<MAX_BITWORD_WIDTH>>(arg)) {
                auto copy = pybind11::cast<CliffordString<MAX_BITWORD_WIDTH>>(arg);
                return copy;
            }

            if (pybind11::isinstance<FlexPauliString>(arg)) {
                const FlexPauliString &other = pybind11::cast<const FlexPauliString &>(arg);
                CliffordString<MAX_BITWORD_WIDTH> result(other.value.num_qubits);
                result.z_signs = other.value.xs;
                result.x_signs = other.value.zs;
                return result;
            }

            pybind11::module collections = pybind11::module::import("collections.abc");
            pybind11::object iterable_type = collections.attr("Iterable");
            if (pybind11::isinstance(arg, iterable_type)) {
                std::vector<GateType> gates;
                for (const auto &t : arg) {
                    if (pybind11::isinstance<GateTypeWrapper>(t)) {
                        gates.push_back(pybind11::cast<GateTypeWrapper>(t).type);
                        continue;
                    } else if (pybind11::isinstance<pybind11::str>(t)) {
                        gates.push_back(GATE_DATA.at(pybind11::cast<std::string_view>(t)).id);
                        continue;

                        /// Integer case is disabled until exposed encoding is decided upon.
                        // } else if (pybind11::isinstance<pybind11::int_>(t)) {
                        //     int64_t v = pybind11::cast<int64_t>(t);
                        //     if (v >= 0 && (size_t)v < INT_TO_SINGLE_QUBIT_CLIFFORD_TABLE.size() &&
                        //     INT_TO_SINGLE_QUBIT_CLIFFORD_TABLE[v] != GateType::NOT_A_GATE) {
                        //         gates.push_back(INT_TO_SINGLE_QUBIT_CLIFFORD_TABLE[v]);
                        //         continue;
                        //     }
                    }
                    throw std::invalid_argument(
                        "Don't know how to convert the following item into a Clifford: " +
                        pybind11::cast<std::string>(pybind11::repr(t)));
                }
                CliffordString<MAX_BITWORD_WIDTH> result(gates.size());
                for (size_t k = 0; k < gates.size(); k++) {
                    result.set_gate_at(k, gates[k]);
                }
                return result;
            }

            throw std::invalid_argument(
                "Don't know how to initialize a stim.CliffordString from " +
                pybind11::cast<std::string>(pybind11::repr(arg)));
        }),
        pybind11::arg("arg"),
        pybind11::pos_only(),
        clean_doc_string(R"DOC(
            @signature def __init__(self, arg: Union[int, str, stim.CliffordString, stim.PauliString], /) -> None:
            Initializes a stim.CliffordString from the given argument.

            Args:
                arg [position-only]: This can be a variety of types, including:
                    int: initializes an identity Clifford string of the given length.
                    str: initializes by parsing a comma-separated list of gate names.
                    stim.CliffordString: initializes by copying the given Clifford string.
                    stim.PauliString: initializes by copying from the given Pauli string
                        (ignores the sign of the Pauli string).
                    Iterable: initializes by interpreting each item as a Clifford.
                        Each item can be a single-qubit Clifford gate name (like "SQRT_X")
                        or stim.GateData instance.

            Examples:
                >>> import stim

                >>> stim.CliffordString(5)
                stim.CliffordString("I,I,I,I,I")

                >>> stim.CliffordString("X,Y,Z,SQRT_X")
                stim.CliffordString("X,Y,Z,SQRT_X")

                >>> stim.CliffordString(["H", stim.gate_data("S")])
                stim.CliffordString("H,S")

                >>> stim.CliffordString(stim.PauliString("XYZ"))
                stim.CliffordString("X,Y,Z")

                >>> stim.CliffordString(stim.CliffordString("X,Y,Z"))
                stim.CliffordString("X,Y,Z")
        )DOC")
            .data());

    c.def(
        "__imul__",
        &CliffordString<MAX_BITWORD_WIDTH>::operator*=,
        clean_doc_string(R"DOC(
            Returns the product of two CliffordString instances.

            Examples:
                >>> import stim
                >>> x = stim.CliffordString("S,X,X")
                >>> y = stim.CliffordString("S,Z,H,Z")
                >>> alias = x
                >>> alias *= y
                >>> x
                stim.CliffordString("Z,Y,SQRT_Y,Z")
        )DOC")
            .data());

    c.def(
        "__mul__",
        &CliffordString<MAX_BITWORD_WIDTH>::operator*,
        clean_doc_string(R"DOC(
            Returns the product of two CliffordString instances.

            Examples:
                >>> import stim
                >>> stim.CliffordString("S,X,X") * stim.CliffordString("S,Z,H,Z")
                stim.CliffordString("Z,Y,SQRT_Y,Z")
        )DOC")
            .data());

    c.def(
        "__str__",
        &CliffordString<MAX_BITWORD_WIDTH>::py_str,
        "Returns a string representation of the CliffordString's operations.");
    c.def(
        "__repr__",
        &CliffordString<MAX_BITWORD_WIDTH>::py_repr,
        "Returns text that is a valid python expression evaluating to an equivalent `stim.CliffordString`.");
    c.def(
        "__len__",
        [](const CliffordString<MAX_BITWORD_WIDTH> &self) {
            return self.num_qubits;
        },
        clean_doc_string(R"DOC(
            Returns the number of Clifford operations in the string.

            Examples:
                >>> import stim
                >>> len(stim.CliffordString("I,X,Y,Z,H"))
                5
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two Clifford strings have identical contents.");
    c.def(pybind11::self != pybind11::self, "Determines if two Clifford strings have non-identical contents.");

    c.def(
        "__getitem__",
        [](const CliffordString<MAX_BITWORD_WIDTH> &self, const pybind11::object &index_or_slice) {
            pybind11::ssize_t index, step, slice_length;
            if (normalize_index_or_slice(index_or_slice, self.num_qubits, &index, &step, &slice_length)) {
                return pybind11::cast(self.py_get_slice(index, step, slice_length));
            }
            return pybind11::cast(GateTypeWrapper{self.gate_at(index)});
        },
        pybind11::arg("index_or_slice"),
        clean_doc_string(R"DOC(
            @overload def __getitem__(self, index_or_slice: int) -> stim.GateData:
            @overload def __getitem__(self, index_or_slice: slice) -> stim.CliffordString:
            @signature def __getitem__(self, index_or_slice: Union[int, slice]) -> Union[stim.GateData, stim.CliffordString]:
            Returns a Clifford or substring from the CliffordString.

            Args:
                index_or_slice: The index of the Clifford to return, or the slice
                    corresponding to the sub CliffordString to return.

            Returns:
                The indexed Clifford (as a stim.GateData instance) or the sliced
                CliffordString.

            Examples:
                >>> import stim
                >>> s = stim.CliffordString("I,X,Y,Z,H")

                >>> s[2]
                stim.gate_data('Y')

                >>> s[-1]
                stim.gate_data('H')

                >>> s[:-1]
                stim.CliffordString("I,X,Y,Z")

                >>> s[::2]
                stim.CliffordString("I,Y,H")
        )DOC")
            .data());

    c.def(
        "__setitem__",
        [](CliffordString<MAX_BITWORD_WIDTH> &self,
           const pybind11::object &index_or_slice,
           const pybind11::object &new_value) {
            pybind11::ssize_t index, step, slice_length;
            if (normalize_index_or_slice(index_or_slice, self.num_qubits, &index, &step, &slice_length)) {
                if (pybind11::isinstance<GateTypeWrapper>(new_value)) {
                    GateType g = pybind11::cast<GateTypeWrapper>(new_value).type;
                    for (size_t k = 0; k < (size_t)slice_length; k++) {
                        size_t target_k = index + step * k;
                        self.set_gate_at(target_k, g);
                    }
                    return;
                } else if (pybind11::isinstance<pybind11::str>(new_value)) {
                    GateType g = GATE_DATA.at(pybind11::cast<std::string_view>(new_value)).id;
                    for (size_t k = 0; k < (size_t)slice_length; k++) {
                        size_t target_k = index + step * k;
                        self.set_gate_at(target_k, g);
                    }
                    return;
                } else if (pybind11::isinstance<CliffordString<MAX_BITWORD_WIDTH>>(new_value)) {
                    const CliffordString<MAX_BITWORD_WIDTH> &v =
                        pybind11::cast<CliffordString<MAX_BITWORD_WIDTH>>(new_value);
                    for (size_t k = 0; k < (size_t)slice_length; k++) {
                        size_t target_k = index + step * k;
                        self.set_gate_at(target_k, v.gate_at(k));
                    }
                    return;
                }
            } else {
                if (pybind11::isinstance<GateTypeWrapper>(new_value)) {
                    self.set_gate_at(index, pybind11::cast<GateTypeWrapper>(new_value).type);
                    return;
                } else if (pybind11::isinstance<pybind11::str>(new_value)) {
                    self.set_gate_at(index, GATE_DATA.at(pybind11::cast<std::string_view>(new_value)).id);
                    return;
                }
            }

            std::stringstream ss;
            ss << "Don't know how to write an object of type ";
            ss << pybind11::repr(pybind11::type::of(new_value));
            ss << " to index ";
            ss << pybind11::repr(index_or_slice);
            throw std::invalid_argument(ss.str());
        },
        pybind11::arg("index_or_slice"),
        pybind11::arg("new_value"),
        clean_doc_string(R"DOC(
            @signature def __setitem__(self, index_or_slice: Union[int, slice], new_value: Union[str, stim.GateData, stim.CliffordString]) -> None:
            Returns a Clifford or substring from the CliffordString.

            Args:
                index_or_slice: The index of the Clifford to overwrite, or the slice
                    of Cliffords to overwrite.

            Examples:
                >>> import stim
                >>> s = stim.CliffordString("I,X,Y,Z,H")

                >>> s[2]
                stim.gate_data('Y')

                >>> s[-1]
                stim.gate_data('H')

                >>> s[:-1]
                stim.CliffordString("I,X,Y,Z")

                >>> s[::2]
                stim.CliffordString("I,Y,H")
        )DOC")
            .data());

    c.def_static(
        "random",
        [](size_t num_qubits) {
            auto rng = make_py_seeded_rng(pybind11::none());
            return CliffordString<MAX_BITWORD_WIDTH>::random(num_qubits, rng);
        },
        pybind11::arg("num_qubits"),
        clean_doc_string(R"DOC(
            Samples a uniformly random CliffordString.

            Args:
                num_qubits: The number of qubits the CliffordString should act upon.

            Examples:
                >>> import stim
                >>> p = stim.CliffordString.random(5)
                >>> len(p)
                5

            Returns:
                The sampled Clifford string.
        )DOC")
            .data());

    c.def(
        "__ipow__",
        [](pybind11::object self, int64_t power) {
            pybind11::cast<CliffordString<MAX_BITWORD_WIDTH> &>(self).ipow(power);
            return self;
        },
        pybind11::arg("num_qubits"),
        clean_doc_string(R"DOC(
            Mutates the CliffordString into itself raised to a power.

            Args:
                power: The power to raise the CliffordString's Cliffords to.
                    This value can be negative (e.g. -1 inverts the string).

            Returns:
                The mutated Clifford string.

            Examples:
                >>> import stim

                >>> p = stim.CliffordString("I,X,H,S,C_XYZ")
                >>> p **= 3
                >>> p
                stim.CliffordString("I,X,H,S_DAG,I")

                >>> p **= 2
                >>> p
                stim.CliffordString("I,I,I,Z,I")

                >>> alias = p
                >>> alias **= 2
                >>> p
                stim.CliffordString("I,I,I,I,I")
        )DOC")
            .data());

    c.def(
        "__pow__",
        [](const CliffordString<MAX_BITWORD_WIDTH> &self, int64_t power) -> CliffordString<MAX_BITWORD_WIDTH> {
            auto copy = self;
            copy.ipow(power);
            return copy;
        },
        pybind11::arg("power"),
        clean_doc_string(R"DOC(
            Returns the CliffordString raised to a power.

            Args:
                power: The power to raise the CliffordString's Cliffords to.
                    This value can be negative (e.g. -1 returns the inverse string).

            Returns:
                The Clifford string raised to the power.

            Examples:
                >>> import stim

                >>> p = stim.CliffordString("I,X,H,S,C_XYZ")

                >>> p**0
                stim.CliffordString("I,I,I,I,I")

                >>> p**1
                stim.CliffordString("I,X,H,S,C_XYZ")

                >>> p**12000001
                stim.CliffordString("I,X,H,S,C_XYZ")

                >>> p**2
                stim.CliffordString("I,I,I,Z,C_ZYX")

                >>> p**3
                stim.CliffordString("I,X,H,S_DAG,I")

                >>> p**-1
                stim.CliffordString("I,X,H,S_DAG,C_ZYX")
        )DOC")
            .data());

    c.def(
        "x_outputs",
        [](const CliffordString<MAX_BITWORD_WIDTH> &self, bool bit_packed_signs) -> pybind11::object {
            auto ps = self.x_outputs();
            FlexPauliString result(std::move(ps));
            return pybind11::make_tuple(
                pybind11::cast(result), simd_bits_to_numpy(self.x_signs, self.num_qubits, bit_packed_signs));
        },
        pybind11::kw_only(),
        pybind11::arg("bit_packed_signs") = false,
        clean_doc_string(R"DOC(
            @signature def x_outputs(self, *, bit_packed_signs: bool = False) -> tuple[stim.PauliString, np.ndarray]:
            Returns what each Clifford in the CliffordString conjugates an X input into.

            For example, H conjugates X into +Z and S_DAG conjugates X into -Y.

            Combined with `z_outputs`, the results of this method completely specify
            the single qubit Clifford applied to each qubit.

            Args:
                bit_packed_signs: Defaults to False. When False, the sign data is returned
                    in a numpy array with dtype `np.bool_`. When True, the dtype is instead
                    `np.uint8` and 8 bits are packed into each byte (in little endian
                    order).

            Returns:
                A (paulis, signs) tuple.

                `paulis` has type stim.PauliString. Its sign is always positive.

                `signs` has type np.ndarray and an argument-dependent shape:
                    bit_packed_signs=False:
                        dtype=np.bool_
                        shape=(num_qubits,)
                    bit_packed_signs=True:
                        dtype=np.uint8
                        shape=(math.ceil(num_qubits / 8),)

            Examples:
                >>> import stim
                >>> x_paulis, x_signs = stim.CliffordString("I,Y,H,S").x_outputs()
                >>> x_paulis
                stim.PauliString("+XXZY")
                >>> x_signs
                array([False,  True, False, False])

                >>> stim.CliffordString("I,Y,H,S").x_outputs(bit_packed_signs=True)[1]
                array([2], dtype=uint8)
        )DOC")
            .data());

    c.def(
        "y_outputs",
        [](CliffordString<MAX_BITWORD_WIDTH> &self, bool bit_packed_signs) -> pybind11::object {
            simd_bits<MAX_BITWORD_WIDTH> signs(self.num_qubits);
            auto ys = self.y_outputs_and_signs(signs);
            FlexPauliString result(std::move(ys));
            return pybind11::make_tuple(
                pybind11::cast(result), simd_bits_to_numpy(signs, self.num_qubits, bit_packed_signs));
        },
        pybind11::kw_only(),
        pybind11::arg("bit_packed_signs") = false,
        clean_doc_string(R"DOC(
            @signature def y_outputs(self, *, bit_packed_signs: bool = False) -> tuple[stim.PauliString, np.ndarray]:
            Returns what each Clifford in the CliffordString conjugates a Y input into.

            For example, H conjugates Y into -Y and S_DAG conjugates Y into +X.

            Args:
                bit_packed_signs: Defaults to False. When False, the sign data is returned
                    in a numpy array with dtype `np.bool_`. When True, the dtype is instead
                    `np.uint8` and 8 bits are packed into each byte (in little endian
                    order).

            Returns:
                A (paulis, signs) tuple.

                `paulis` has type stim.PauliString. Its sign is always positive.

                `signs` has type np.ndarray and an argument-dependent shape:
                    bit_packed_signs=False:
                        dtype=np.bool_
                        shape=(num_qubits,)
                    bit_packed_signs=True:
                        dtype=np.uint8
                        shape=(math.ceil(num_qubits / 8),)

            Examples:
                >>> import stim
                >>> y_paulis, y_signs = stim.CliffordString("I,X,H,S").y_outputs()
                >>> y_paulis
                stim.PauliString("+YYYX")
                >>> y_signs
                array([False,  True,  True,  True])

                >>> stim.CliffordString("I,X,H,S").y_outputs(bit_packed_signs=True)[1]
                array([14], dtype=uint8)
        )DOC")
            .data());

    c.def(
        "z_outputs",
        [](const CliffordString<MAX_BITWORD_WIDTH> &self, bool bit_packed_signs) -> pybind11::object {
            auto ps = self.z_outputs();
            FlexPauliString result(std::move(ps));
            return pybind11::make_tuple(
                pybind11::cast(result), simd_bits_to_numpy(self.z_signs, self.num_qubits, bit_packed_signs));
        },
        pybind11::kw_only(),
        pybind11::arg("bit_packed_signs") = false,
        clean_doc_string(R"DOC(
            @signature def z_outputs(self, *, bit_packed_signs: bool = False) -> tuple[stim.PauliString, np.ndarray]:
            Returns what each Clifford in the CliffordString conjugates a Z input into.

            For example, H conjugates Z into +X and SQRT_X conjugates Z into -Y.

            Combined with `x_outputs`, the results of this method completely specify
            the single qubit Clifford applied to each qubit.

            Args:
                bit_packed_signs: Defaults to False. When False, the sign data is returned
                    in a numpy array with dtype `np.bool_`. When True, the dtype is instead
                    `np.uint8` and 8 bits are packed into each byte (in little endian
                    order).

            Returns:
                A (paulis, signs) tuple.

                `paulis` has type stim.PauliString. Its sign is always positive.

                `signs` has type np.ndarray and an argument-dependent shape:
                    bit_packed_signs=False:
                        dtype=np.bool_
                        shape=(num_qubits,)
                    bit_packed_signs=True:
                        dtype=np.uint8
                        shape=(math.ceil(num_qubits / 8),)

            Examples:
                >>> import stim
                >>> z_paulis, z_signs = stim.CliffordString("I,Y,H,S").z_outputs()
                >>> z_paulis
                stim.PauliString("+ZZXZ")
                >>> z_signs
                array([False,  True, False, False])

                >>> stim.CliffordString("I,Y,H,S").z_outputs(bit_packed_signs=True)[1]
                array([2], dtype=uint8)
        )DOC")
            .data());
}
