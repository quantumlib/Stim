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
                    text = text.substr(text.size() - 1);
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

    /// Integer case is disabled until exposed encoding is decided upon.
    //     c.def_static(
    //         "gate_to_int",
    //         [](const pybind11::object &gate) {
    //             Gate raw_gate;
    //             if (pybind11::isinstance<pybind11::str>(gate)) {
    //                 raw_gate = GATE_DATA.at(pybind11::cast<std::string_view>(gate));
    //             } else if (pybind11::isinstance<GateType>(gate)) {
    //                 raw_gate = GATE_DATA.at(pybind11::cast<GateType>(gate));
    //             } else {
    //                 throw std::invalid_argument(
    //                     "Don't know how to interpret this as a gate: " +
    //                     pybind11::cast<std::string>(pybind11::repr(gate)));
    //             }
    //             if ((raw_gate.flags & GATE_IS_UNITARY) && (raw_gate.flags & GATE_IS_SINGLE_QUBIT_GATE)) {
    //                 auto vals = gate_to_bits(raw_gate.id);
    //                 auto result = 0;
    //                 for (size_t k = 0; k < vals.size(); k++) {
    //                     result ^= vals[k] << k;
    //                 }
    //                 return result;
    //             } else {
    //                 throw std::invalid_argument(
    //                     "Not a single qubit Clifford gate: " +
    //                     pybind11::cast<std::string>(pybind11::repr(gate)));
    //             }
    //         },
    //         pybind11::arg("gate"),
    //         clean_doc_string(R"DOC(
    //             Encodes a single qubit Clifford gate into a 6 bit integer.
    //             @signature def gate_to_int(self, gate: Union[str, stim.GateData]) -> int:
    //
    //             The encoding is based on copying bits from the gate's stabilizer Tableau
    //             directly into the bits of the integer. The first two bits (the least significant
    //             bits) encode the X and Z signs, the second two bits encode the X output Pauli,
    //             and the last two bits encode the Z output Pauli.
    //
    //             This binary integer:
    //
    //                 0bABCDXZ
    //
    //             Corresponds to this tableau:
    //
    //                 stim.Tableau.from_conjugated_generators(
    //                     xs=[
    //                         (-1)**X * stim.PauliString("_XZY"[A + 2*B]),
    //                     ],
    //                     zs=[
    //                         (-1)**Z * stim.PauliString("_XZY"[C + 2*D]),
    //                     ],
    //                 )
    //
    //             The explicit encoding is as follows:
    //
    //                  0 = 0b000000 = I
    //                  1 = 0b000001 = X
    //                  2 = 0b000010 = Z
    //                  3 = 0b000011 = Y
    //                  8 = 0b001000 = S
    //                  9 = 0b001001 = H_XY
    //                 10 = 0b001010 = S_DAG
    //                 11 = 0b001011 = H_NXY
    //                 <4 unused ids>
    //                 16 = 0b010000 = SQRT_X_DAG
    //                 17 = 0b010001 = SQRT_X
    //                 18 = 0b010010 = H_YZ
    //                 19 = 0b010011 = H_NYZ
    //                 <8 unused ids>
    //                 28 = 0b011100 = C_ZYX
    //                 29 = 0b011101 = C_ZNYX
    //                 30 = 0b011110 = C_ZYNX
    //                 31 = 0b011111 = C_NZYX
    //                 <24 unused ids>
    //                 56 = 0b111000 = C_XYZ
    //                 57 = 0b111001 = C_XYNZ
    //                 58 = 0b111010 = C_XNYZ
    //                 59 = 0b111011 = C_NXYZ
    //                 60 = 0b111100 = H
    //                 61 = 0b111101 = SQRT_Y_DAG
    //                 62 = 0b111110 = SQRT_Y
    //                 63 = 0b111111 = H_NXZ
    //
    //             Args:
    //
    //                 gate: The gate to encode into an integer. This can either be the name of
    //                     the gate as a string, or a stim.GateData instance. The given gate must
    //                     be a single qubit Clifford gate.
    //
    //             Examples:
    //
    //                 >>> stim.CliffordString.gate_to_int(stim.gate_data("S"))
    //                 9
    //
    //                 >>> stim.CliffordString.gate_to_int("H")
    //                 60
    //
    //                 >>> t = stim.gate_data("H").tableau
    //                 >>> manual_id = 0
    //                 >>> manual_id ^= ("IXYZ"[t.x_output_pauli(0, 0)] in "XY") << 0
    //                 >>> manual_id ^= ("IXYZ"[t.x_output_pauli(0, 0)] in "YZ") << 1
    //                 >>> manual_id ^= ("IXYZ"[t.z_output_pauli(0, 0)] in "XY") << 2
    //                 >>> manual_id ^= ("IXYZ"[t.z_output_pauli(0, 0)] in "YZ") << 3
    //                 >>> manual_id ^= (t.x_sign(0)) << 5
    //                 >>> manual_id ^= (t.z_sign(0)) << 6
    //                 >>> manual_id
    //                 60
    //         )DOC")
    //             .data());

    /// Integer case is disabled until exposed encoding is decided upon.
    //     c.def_static(
    //         "int_to_gate",
    //         [](size_t arg) -> GateType {
    //             GateType result = GateType::NOT_A_GATE;
    //             if (arg < INT_TO_SINGLE_QUBIT_CLIFFORD_TABLE.size()) {
    //                 result = INT_TO_SINGLE_QUBIT_CLIFFORD_TABLE[arg];
    //             }
    //             if (result == GateType::NOT_A_GATE) {
    //                 throw std::invalid_argument("No single qubit Clifford gate is encoded as the integer " +
    //                 std::to_string(arg) + ".");
    //             }
    //             return result;
    //         },
    //         pybind11::arg("arg"),
    //         clean_doc_string(R"DOC(
    //             Decodes a single qubit Clifford gate out of a 6 bit integer.
    //
    //             The encoding is based on the internal representation used to represent the gate
    //             (designed to make applying operations fast). The gate is specified by 6 bits,
    //             corresponding to bits (or inverted bits) from the gate's stabilizer tableau.
    //
    //             This binary integer:
    //
    //                 0bABCDXZ
    //
    //             Corresponds to this tableau:
    //
    //                 stim.Tableau.from_conjugated_generators(
    //                     xs=[
    //                         (-1)**X * stim.PauliString("_XZY"[A + 2*B]),
    //                     ],
    //                     zs=[
    //                         (-1)**Z * stim.PauliString("_XZY"[C + 2*D]),
    //                     ],
    //                 )
    //
    //             The explicit encoding is as follows:
    //
    //                  0 = 0b000000 = I
    //                  1 = 0b000001 = X
    //                  2 = 0b000010 = Z
    //                  3 = 0b000011 = Y
    //                  8 = 0b001000 = S
    //                  9 = 0b001001 = H_XY
    //                 10 = 0b001010 = S_DAG
    //                 11 = 0b001011 = H_NXY
    //                 <4 unused ids>
    //                 16 = 0b010000 = SQRT_X_DAG
    //                 17 = 0b010001 = SQRT_X
    //                 18 = 0b010010 = H_YZ
    //                 19 = 0b010011 = H_NYZ
    //                 <8 unused ids>
    //                 28 = 0b011100 = C_ZYX
    //                 29 = 0b011101 = C_ZNYX
    //                 30 = 0b011110 = C_ZYNX
    //                 31 = 0b011111 = C_NZYX
    //                 <24 unused ids>
    //                 56 = 0b111000 = C_XYZ
    //                 57 = 0b111001 = C_XYNZ
    //                 58 = 0b111010 = C_XNYZ
    //                 59 = 0b111011 = C_NXYZ
    //                 60 = 0b111100 = H
    //                 61 = 0b111101 = SQRT_Y_DAG
    //                 62 = 0b111110 = SQRT_Y
    //                 63 = 0b111111 = H_NXZ
    //
    //             Args:
    //                 arg: The encoded integer to decode into a gate.
    //
    //             Examples:
    //
    //                 >>> stim.CliffordString.int_to_gate(1)
    //                 stim.gate_data("X")
    //
    //                 >>> stim.CliffordString.int_to_gate(9)
    //                 stim.gate_data("S")
    //
    //         )DOC")
    //             .data());
}
