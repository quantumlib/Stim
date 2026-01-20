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
                auto copy = pybind11::cast<const CliffordString<MAX_BITWORD_WIDTH> &>(arg);
                return copy;
            }

            if (pybind11::isinstance<FlexPauliString>(arg)) {
                const FlexPauliString &other = pybind11::cast<const FlexPauliString &>(arg);
                CliffordString<MAX_BITWORD_WIDTH> result(other.value.num_qubits);
                result.z_signs = other.value.xs;
                result.x_signs = other.value.zs;
                return result;
            }

            if (pybind11::isinstance<Circuit>(arg)) {
                const Circuit &circuit = pybind11::cast<const Circuit &>(arg);
                size_t n = circuit.count_qubits();
                CliffordString<MAX_BITWORD_WIDTH> result(n);
                CliffordString<MAX_BITWORD_WIDTH> buffer(1);
                circuit.for_each_operation([&](const CircuitInstruction &op) {
                    const Gate &gate = GATE_DATA[op.gate_type];
                    if ((gate.flags & GATE_IS_UNITARY) && (gate.flags & GATE_IS_SINGLE_QUBIT_GATE)) {
                        for (const auto &target : op.targets) {
                            uint32_t q = target.qubit_value();
                            buffer.set_gate_at(q % MAX_BITWORD_WIDTH, op.gate_type);
                            result.set_word_at(q / MAX_BITWORD_WIDTH, buffer.word_at(0) * result.word_at(q / MAX_BITWORD_WIDTH));
                            buffer.set_gate_at(q % MAX_BITWORD_WIDTH, GateType::I);
                        }
                    } else {
                        switch (op.gate_type) {
                        case GateType::QUBIT_COORDS:
                        case GateType::SHIFT_COORDS:
                        case GateType::DETECTOR:
                        case GateType::OBSERVABLE_INCLUDE:
                        case GateType::TICK:
                            break;
                        default:
                            throw std::invalid_argument("Don't know how to convert circuit instruction into single qubit Clifford operations: " + op.str());
                        }
                    }
                });
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
            @signature def __init__(self, arg: Union[int, str, stim.CliffordString, stim.PauliString, stim.Circuit], /) -> None:
            Initializes a stim.CliffordString from the given argument.

            Args:
                arg [position-only]: This can be a variety of types, including:
                    int: initializes an identity Clifford string of the given length.
                    str: initializes by parsing a comma-separated list of gate names.
                    stim.CliffordString: initializes by copying the given Clifford string.
                    stim.PauliString: initializes by copying from the given Pauli string
                        (ignores the sign of the Pauli string).
                    stim.Circuit: initializes a CliffordString equivalent to the action
                        of the circuit (as long as the circuit only contains single qubit
                        unitary operations and annotations).
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

                >>> stim.CliffordString(stim.Circuit('''
                ...     H 0 1 2
                ...     S 2 3
                ...     TICK
                ...     S 3
                ...     I 6
                ... '''))
                stim.CliffordString("H,H,C_ZYX,Z,I,I,I")
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
            bool is_slice = normalize_index_or_slice(index_or_slice, self.num_qubits, &index, &step, &slice_length);
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
            } else if (pybind11::isinstance<Tableau<MAX_BITWORD_WIDTH>>(new_value)) {
                const Tableau<MAX_BITWORD_WIDTH> &t = pybind11::cast<const Tableau<MAX_BITWORD_WIDTH> &>(new_value);
                if (t.num_qubits == 1) {
                    GateType g = single_qubit_tableau_to_gate_type(t);
                    for (size_t k = 0; k < (size_t)slice_length; k++) {
                        size_t target_k = index + step * k;
                        self.set_gate_at(target_k, g);
                    }
                    return;
                }
            } else if (is_slice && pybind11::isinstance<CliffordString<MAX_BITWORD_WIDTH>>(new_value)) {
                const CliffordString<MAX_BITWORD_WIDTH> &v =
                    pybind11::cast<const CliffordString<MAX_BITWORD_WIDTH> &>(new_value);
                if (v.num_qubits != (size_t)slice_length) {
                    std::stringstream ss;
                    ss << "Length mismatch. The targeted slice covers " << slice_length;
                    ss << " values but the given CliffordString has " << v.num_qubits << " values.";
                    throw std::invalid_argument(ss.str());
                }
                for (size_t k = 0; k < (size_t)slice_length; k++) {
                    size_t target_k = index + step * k;
                    self.set_gate_at(target_k, v.gate_at(k));
                }
                return;
            } else if (is_slice && pybind11::isinstance<FlexPauliString>(new_value)) {
                const FlexPauliString &v = pybind11::cast<const FlexPauliString &>(new_value);
                if (v.value.num_qubits != (size_t)slice_length) {
                    std::stringstream ss;
                    ss << "Length mismatch. The targeted slice covers " << slice_length;
                    ss << " values but the given PauliString has " << v.value.num_qubits << " values.";
                    throw std::invalid_argument(ss.str());
                }
                for (size_t k = 0; k < (size_t)slice_length; k++) {
                    size_t target_k = index + step * k;
                    self.inv_x2x[target_k] = 0;
                    self.x2z[target_k] = 0;
                    self.z2x[target_k] = 0;
                    self.inv_z2z[target_k] = 0;
                    self.x_signs[target_k] = v.value.zs[k];
                    self.z_signs[target_k] = v.value.xs[k];
                }
                return;
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
            @signature def __setitem__(self, index_or_slice: Union[int, slice], new_value: Union[str, stim.GateData, stim.CliffordString, stim.PauliString, stim.Tableau]) -> None:
            Overwrites an indexed Clifford, or slice of Cliffords, with the given value.

            Args:
                index_or_slice: The index of the Clifford to overwrite, or the slice
                    of Cliffords to overwrite.
                new_value: Specifies the value to write into the Clifford string. This can
                    be set to a few different types of values:
                    - str: Name of the single qubit Clifford gate to write to the index or
                        broadcast over the slice.
                    - stim.GateData: The single qubit Clifford gate to write to the index
                        or broadcast over the slice.
                    - stim.Tableau: Must be a single qubit tableau. Specifies the single
                        qubit Clifford gate to write to the index or broadcast over the
                        slice.
                    - stim.CliffordString: String of Cliffords to write into the slice.

            Examples:
                >>> import stim
                >>> s = stim.CliffordString("I,I,I,I,I")

                >>> s[1] = 'H'
                >>> s
                stim.CliffordString("I,H,I,I,I")

                >>> s[2:] = 'SQRT_X'
                >>> s
                stim.CliffordString("I,H,SQRT_X,SQRT_X,SQRT_X")

                >>> s[0] = stim.gate_data('S_DAG').inverse
                >>> s
                stim.CliffordString("S,H,SQRT_X,SQRT_X,SQRT_X")

                >>> s[:] = 'I'
                >>> s
                stim.CliffordString("I,I,I,I,I")

                >>> s[::2] = stim.CliffordString("X,Y,Z")
                >>> s
                stim.CliffordString("X,I,Y,I,Z")

                >>> s[0] = stim.Tableau.from_named_gate("H")
                >>> s
                stim.CliffordString("H,I,Y,I,Z")

                >>> s[:] = stim.Tableau.from_named_gate("S")
                >>> s
                stim.CliffordString("S,S,S,S,S")

                >>> s[:4] = stim.PauliString("IXYZ")
                >>> s
                stim.CliffordString("I,X,Y,Z,S")
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

    c.def_static(
        "all_cliffords_string",
        []() -> CliffordString<MAX_BITWORD_WIDTH> {
            CliffordString<MAX_BITWORD_WIDTH> result(24);
            result.set_gate_at(0, GateType::I);
            result.set_gate_at(4, GateType::H_XY);
            result.set_gate_at(8, GateType::H);
            result.set_gate_at(12, GateType::H_YZ);
            result.set_gate_at(16, GateType::C_XYZ);
            result.set_gate_at(20, GateType::C_ZYX);
            for (size_t q = 0; q < 24; q++) {
                if (q % 4) {
                    result.set_gate_at(q, result.gate_at(q - 1));
                }
            }

            CliffordString<MAX_BITWORD_WIDTH> ixyz(24);
            for (size_t q = 0; q < 24; q += 4) {
                ixyz.set_gate_at(q + 0, GateType::I);
                ixyz.set_gate_at(q + 1, GateType::X);
                ixyz.set_gate_at(q + 2, GateType::Y);
                ixyz.set_gate_at(q + 3, GateType::Z);
            }
            result *= ixyz;
            return result;
        },
        clean_doc_string(R"DOC(
            Returns a stim.CliffordString containing each single qubit Clifford once.

            Useful for things like testing that a method works on every single Clifford.

            Examples:
                >>> import stim
                >>> cliffords = stim.CliffordString.all_cliffords_string()
                >>> len(cliffords)
                24

                >>> print(cliffords[:8])
                I,X,Y,Z,H_XY,S,S_DAG,H_NXY

                >>> print(cliffords[8:16])
                H,SQRT_Y_DAG,H_NXZ,SQRT_Y,H_YZ,H_NYZ,SQRT_X,SQRT_X_DAG

                >>> print(cliffords[16:])
                C_XYZ,C_XYNZ,C_NXYZ,C_XNYZ,C_ZYX,C_ZNYX,C_NZYX,C_ZYNX
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
        "__add__",
        &CliffordString<MAX_BITWORD_WIDTH>::operator+,
        pybind11::arg("rhs"),
        clean_doc_string(R"DOC(
            Concatenates two CliffordStrings.

            Args:
                rhs: The suffix of the concatenation.

            Returns:
                The concatenated Clifford string.

            Examples:
                >>> import stim
                >>> stim.CliffordString("I,X,H") + stim.CliffordString("Y,S")
                stim.CliffordString("I,X,H,Y,S")
        )DOC")
            .data());

    c.def(
        "copy",
        [](const CliffordString<MAX_BITWORD_WIDTH> &self) -> CliffordString<MAX_BITWORD_WIDTH> {
            return self;
        },
        clean_doc_string(R"DOC(
            Returns a copy of the CliffordString.

            Returns:
                The copy.

            Examples:
                >>> import stim
                >>> c = stim.CliffordString("H,X")
                >>> alias = c
                >>> copy = c.copy()
                >>> c *= 5
                >>> alias
                stim.CliffordString("H,X,H,X,H,X,H,X,H,X")
                >>> copy
                stim.CliffordString("H,X")
        )DOC")
            .data());

    c.def(
        "__iadd__",
        &CliffordString<MAX_BITWORD_WIDTH>::operator+=,
        pybind11::arg("rhs"),
        clean_doc_string(R"DOC(
            Mutates the CliffordString by concatenating onto it.

            Args:
                rhs: The suffix to concatenate onto the target CliffordString.

            Returns:
                The mutated Clifford string.

            Examples:
                >>> import stim
                >>> c = stim.CliffordString("I,X,H")
                >>> alias = c
                >>> alias += stim.CliffordString("Y,S")
                >>> c
                stim.CliffordString("I,X,H,Y,S")
        )DOC")
            .data());

    c.def(
        "__rmul__",
        [](const CliffordString<MAX_BITWORD_WIDTH> &self, size_t rhs) -> CliffordString<MAX_BITWORD_WIDTH> {
            return self * rhs;
        },
        pybind11::arg("lhs"),
        clean_doc_string(R"DOC(
            CliffordString left-multiplication.

            Args:
                lhs: The number of times to repeat the Clifford string's contents.

            Returns:
                The repeated Clifford string.

            Examples:
                >>> import stim

                >>> 2 * stim.CliffordString("I,X,H")
                stim.CliffordString("I,X,H,I,X,H")

                >>> 0 * stim.CliffordString("I,X,H")
                stim.CliffordString("")

                >>> 5 * stim.CliffordString("I")
                stim.CliffordString("I,I,I,I,I")
        )DOC")
            .data());

    c.def(
        "__mul__",
        [](const CliffordString<MAX_BITWORD_WIDTH> &self, pybind11::object rhs) -> CliffordString<MAX_BITWORD_WIDTH> {
            if (pybind11::isinstance<pybind11::int_>(rhs)) {
                return self * pybind11::cast<size_t>(rhs);
            } else if (pybind11::isinstance<CliffordString<MAX_BITWORD_WIDTH>>(rhs)) {
                return self * pybind11::cast<const CliffordString<MAX_BITWORD_WIDTH> &>(rhs);
            } else {
                std::stringstream ss;
                ss << "Don't know how to multiply by ";
                ss << pybind11::repr(rhs);
                throw std::invalid_argument(ss.str());
            }
        },
        clean_doc_string(R"DOC(
            @signature def __mul__(self, rhs: Union[stim.CliffordString, int]) -> stim.CliffordString:
            CliffordString multiplication.

            Args:
                rhs: Either a stim.CliffordString or an int. If rhs is a
                    stim.CliffordString, then the Cliffords from each string are multiplied
                    pairwise. If rhs is an int, it is the number of times to repeat the
                    Clifford string's contents.

            Examples:
                >>> import stim

                >>> stim.CliffordString("S,X,X") * stim.CliffordString("S,Z,H,Z")
                stim.CliffordString("Z,Y,SQRT_Y,Z")

                >>> stim.CliffordString("I,X,H") * 3
                stim.CliffordString("I,X,H,I,X,H,I,X,H")
        )DOC")
            .data());

    c.def(
        "__imul__",
        [](pybind11::object self_obj, pybind11::object rhs) -> pybind11::object {
            CliffordString<MAX_BITWORD_WIDTH> &self = pybind11::cast<CliffordString<MAX_BITWORD_WIDTH> &>(self_obj);
            if (pybind11::isinstance<pybind11::int_>(rhs)) {
                self *= pybind11::cast<size_t>(rhs);
                return self_obj;
            } else if (pybind11::isinstance<CliffordString<MAX_BITWORD_WIDTH>>(rhs)) {
                self *= pybind11::cast<const CliffordString<MAX_BITWORD_WIDTH> &>(rhs);
                return self_obj;
            } else {
                std::stringstream ss;
                ss << "Don't know how to multiply by ";
                ss << pybind11::repr(rhs);
                throw std::invalid_argument(ss.str());
            }
        },
        pybind11::arg("rhs"),
        clean_doc_string(R"DOC(
            @signature def __imul__(self, rhs: Union[stim.CliffordString, int]) -> stim.CliffordString:
            Inplace CliffordString multiplication.

            Mutates the CliffordString into itself multiplied by another CliffordString
            (via pairwise Clifford multipliation) or by an integer (via repeating the
            contents).

            Args:
                rhs: Either a stim.CliffordString or an int. If rhs is a
                    stim.CliffordString, then the Cliffords from each string are multiplied
                    pairwise. If rhs is an int, it is the number of times to repeat the
                    Clifford string's contents.

            Returns:
                The mutated Clifford string.

            Examples:
                >>> import stim

                >>> c = stim.CliffordString("S,X,X")
                >>> alias = c
                >>> alias *= stim.CliffordString("S,Z,H,Z")
                >>> c
                stim.CliffordString("Z,Y,SQRT_Y,Z")

                >>> c = stim.CliffordString("I,X,H")
                >>> alias = c
                >>> alias *= 2
                >>> c
                stim.CliffordString("I,X,H,I,X,H")
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
