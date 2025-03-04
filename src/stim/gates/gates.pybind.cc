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

#include "stim/gates/gates.h"

#include "stim/gates/gates.pybind.h"
#include "stim/py/base.pybind.h"
#include "stim/stabilizers/flow.h"
#include "stim/util_bot/str_util.h"

using namespace stim;
using namespace stim_pybind;

pybind11::object gate_num_parens_argument_range(const Gate &self) {
    auto r = pybind11::module::import("builtins").attr("range");
    if (self.arg_count == ARG_COUNT_SYGIL_ZERO_OR_ONE) {
        return r(2);
    }
    if (self.arg_count == ARG_COUNT_SYGIL_ANY) {
        return r(256);
    }
    return r(self.arg_count, self.arg_count + 1);
}
std::vector<std::string_view> gate_aliases(const Gate &self) {
    std::vector<std::string_view> aliases;
    for (const auto &h : GATE_DATA.hashed_name_to_gate_type_table) {
        if (h.id == self.id) {
            aliases.push_back(h.expected_name);
        }
    }
    std::sort(aliases.begin(), aliases.end());
    return aliases;
}

pybind11::object gate_tableau(const Gate &self) {
    if (self.flags & GATE_IS_UNITARY) {
        return pybind11::cast(self.tableau<MAX_BITWORD_WIDTH>());
    }
    return pybind11::none();
}
pybind11::object gate_unitary_matrix(const Gate &self) {
    if (self.has_known_unitary_matrix()) {
        auto r = self.unitary();
        auto n = r.size();
        std::complex<float> *buffer = new std::complex<float>[n * n];
        for (size_t a = 0; a < n; a++) {
            for (size_t b = 0; b < n; b++) {
                buffer[b + a * n] = r[a][b];
            }
        }

        pybind11::capsule free_when_done(buffer, [](void *f) {
            delete[] reinterpret_cast<std::complex<float> *>(f);
        });

        return pybind11::array_t<std::complex<float>>(
            {(pybind11::ssize_t)n, (pybind11::ssize_t)n},
            {(pybind11::ssize_t)(n * sizeof(std::complex<float>)), (pybind11::ssize_t)sizeof(std::complex<float>)},
            buffer,
            free_when_done);
    }
    return pybind11::none();
}

pybind11::class_<Gate> stim_pybind::pybind_gate_data(pybind11::module &m) {
    return pybind11::class_<Gate>(
        m,
        "GateData",
        clean_doc_string(R"DOC(
            Details about a gate supported by stim.

            Examples:
                >>> import stim
                >>> stim.gate_data('h').name
                'H'
                >>> stim.gate_data('h').is_unitary
                True
                >>> stim.gate_data('h').tableau
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Z"),
                    ],
                    zs=[
                        stim.PauliString("+X"),
                    ],
                )
        )DOC")
            .data());
}

void stim_pybind::pybind_gate_data_methods(pybind11::module &m, pybind11::class_<Gate> &c) {
    c.def(
        pybind11::init([](const char *name) -> Gate {
            return GATE_DATA.at(name);
        }),
        pybind11::arg("name"),
        clean_doc_string(R"DOC(
            Finds gate data for the named gate.

            Examples:
                >>> import stim
                >>> stim.GateData('H').is_unitary
                True
        )DOC")
            .data());

    m.def(
        "gate_data",
        [](const pybind11::object &name) -> pybind11::object {
            if (!name.is_none()) {
                return pybind11::cast(GATE_DATA.at(pybind11::cast<std::string_view>(name)));
            }

            std::map<std::string_view, Gate> result;
            for (const auto &g : GATE_DATA.items) {
                if (g.id != GateType::NOT_A_GATE) {
                    result.insert({g.name, g});
                }
            }
            return pybind11::cast(result);
        },
        pybind11::arg("name") = pybind11::none(),
        clean_doc_string(R"DOC(
            @overload def gate_data(name: str) -> stim.GateData:
            @overload def gate_data() -> Dict[str, stim.GateData]:
            @signature def gate_data(name: Optional[str] = None) -> Union[str, Dict[str, stim.GateData]]:
            Returns gate data for the given named gate, or all gates.

            Examples:
                >>> import stim
                >>> stim.gate_data('cnot').aliases
                ['CNOT', 'CX', 'ZCX']
                >>> stim.gate_data('cnot').is_two_qubit_gate
                True
                >>> gate_dict = stim.gate_data()
                >>> len(gate_dict) > 50
                True
                >>> gate_dict['MX'].produces_measurements
                True
        )DOC")
            .data());

    c.def_property_readonly(
        "name",
        [](const Gate &self) -> std::string_view {
            return self.name;
        },
        clean_doc_string(R"DOC(
            Returns the canonical name of the gate.

            Examples:
                >>> import stim
                >>> stim.gate_data('H').name
                'H'
                >>> stim.gate_data('cnot').name
                'CX'
        )DOC")
            .data());

    c.def_property_readonly(
        "aliases",
        &gate_aliases,
        clean_doc_string(R"DOC(
            Returns all aliases that can be used to name the gate.

            Although gates can be referred to by lower case and mixed
            case named, the result only includes upper cased aliases.

            Examples:
                >>> import stim
                >>> stim.gate_data('H').aliases
                ['H', 'H_XZ']
                >>> stim.gate_data('cnot').aliases
                ['CNOT', 'CX', 'ZCX']
        )DOC")
            .data());

    c.def(
        "__repr__",
        [](const Gate &self) -> std::string {
            std::stringstream ss;
            ss << "stim.gate_data('" << self.name << "')";
            return ss.str();
        },
        "Returns text that is a valid python expression evaluating to an equivalent `stim.GateData`.");

    c.def(pybind11::self == pybind11::self, "Determines if two GateData instances are identical.");
    c.def(pybind11::self != pybind11::self, "Determines if two GateData instances are not identical.");

    c.def(
        "__str__",
        [](const Gate &self) -> std::string {
            std::stringstream ss;
            auto b = [](bool x) -> const char * {
                return x ? "True" : "False";
            };
            auto v = [](const pybind11::object &obj) {
                pybind11::object obj_repr = pybind11::repr(obj);
                std::string result;
                for (char c : pybind11::cast<std::string_view>(obj_repr)) {
                    result.push_back(c);
                    if (c == '\n') {
                        result.append("    ");
                    }
                }
                return result;
            };
            ss << "stim.GateData {\n";
            ss << "    .name = '" << self.name << "'\n";
            ss << "    .aliases = " << v(pybind11::cast(gate_aliases(self))) << "\n";
            ss << "    .is_noisy_gate = " << b(self.flags & GATE_IS_NOISY) << "\n";
            ss << "    .is_reset = " << b(self.flags & GATE_IS_RESET) << "\n";
            ss << "    .is_single_qubit_gate = " << b(self.flags & GATE_IS_SINGLE_QUBIT_GATE) << "\n";
            ss << "    .is_two_qubit_gate = " << b(self.flags & GATE_TARGETS_PAIRS) << "\n";
            ss << "    .is_unitary = " << b(self.flags & GATE_IS_UNITARY) << "\n";
            ss << "    .num_parens_arguments_range = " << v(gate_num_parens_argument_range(self)) << "\n";
            ss << "    .produces_measurements = " << b(self.flags & GATE_PRODUCES_RESULTS) << "\n";
            ss << "    .takes_measurement_record_targets = "
               << b(self.flags & (GATE_CAN_TARGET_BITS | GATE_ONLY_TARGETS_MEASUREMENT_RECORD)) << "\n";
            ss << "    .takes_pauli_targets = " << b(self.flags & GATE_TARGETS_PAULI_STRING) << "\n";
            if (self.flags & GATE_IS_UNITARY) {
                ss << "    .tableau = " << v(gate_tableau(self)) << "\n";
                ss << "    .unitary_matrix = np.array(" << v(pybind11::cast(self.unitary()))
                   << ", dtype=np.complex64)\n";
            }
            ss << "}";
            return ss.str();
        },
        "Returns text describing the gate data.");

    c.def_property_readonly(
        "tableau",
        &gate_tableau,
        clean_doc_string(R"DOC(
            @signature def tableau(self) -> Optional[stim.Tableau]:
            Returns the gate's tableau, or None if the gate has no tableau.

            Examples:
                >>> import stim
                >>> print(stim.gate_data('M').tableau)
                None
                >>> stim.gate_data('H').tableau
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+Z"),
                    ],
                    zs=[
                        stim.PauliString("+X"),
                    ],
                )
                >>> stim.gate_data('ISWAP').tableau
                stim.Tableau.from_conjugated_generators(
                    xs=[
                        stim.PauliString("+ZY"),
                        stim.PauliString("+YZ"),
                    ],
                    zs=[
                        stim.PauliString("+_Z"),
                        stim.PauliString("+Z_"),
                    ],
                )
        )DOC")
            .data());

    c.def_property_readonly(
        "unitary_matrix",
        &gate_unitary_matrix,
        clean_doc_string(R"DOC(
            @signature def unitary_matrix(self) -> Optional[np.ndarray]:
            Returns the gate's unitary matrix, or None if the gate isn't unitary.

            Examples:
                >>> import stim

                >>> print(stim.gate_data('M').unitary_matrix)
                None

                >>> stim.gate_data('X').unitary_matrix
                array([[0.+0.j, 1.+0.j],
                       [1.+0.j, 0.+0.j]], dtype=complex64)

                >>> stim.gate_data('ISWAP').unitary_matrix
                array([[1.+0.j, 0.+0.j, 0.+0.j, 0.+0.j],
                       [0.+0.j, 0.+0.j, 0.+1.j, 0.+0.j],
                       [0.+0.j, 0.+1.j, 0.+0.j, 0.+0.j],
                       [0.+0.j, 0.+0.j, 0.+0.j, 1.+0.j]], dtype=complex64)
        )DOC")
            .data());

    c.def_property_readonly(
        "is_unitary",
        [](const Gate &self) -> bool {
            return self.flags & GATE_IS_UNITARY;
        },
        clean_doc_string(R"DOC(
            Returns whether or not the gate is a unitary gate.

            Examples:
                >>> import stim

                >>> stim.gate_data('H').is_unitary
                True
                >>> stim.gate_data('CX').is_unitary
                True

                >>> stim.gate_data('R').is_unitary
                False
                >>> stim.gate_data('M').is_unitary
                False
                >>> stim.gate_data('MXX').is_unitary
                False
                >>> stim.gate_data('X_ERROR').is_unitary
                False
                >>> stim.gate_data('CORRELATED_ERROR').is_unitary
                False
                >>> stim.gate_data('MPP').is_unitary
                False
                >>> stim.gate_data('DETECTOR').is_unitary
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "num_parens_arguments_range",
        &gate_num_parens_argument_range,
        clean_doc_string(R"DOC(
            @signature def num_parens_arguments_range(self) -> range:
            Returns the min/max parens arguments taken by the gate, as a python range.

            Examples:
                >>> import stim

                >>> stim.gate_data('M').num_parens_arguments_range
                range(0, 2)
                >>> list(stim.gate_data('M').num_parens_arguments_range)
                [0, 1]
                >>> list(stim.gate_data('R').num_parens_arguments_range)
                [0]
                >>> list(stim.gate_data('H').num_parens_arguments_range)
                [0]
                >>> list(stim.gate_data('X_ERROR').num_parens_arguments_range)
                [1]
                >>> list(stim.gate_data('PAULI_CHANNEL_1').num_parens_arguments_range)
                [3]
                >>> list(stim.gate_data('PAULI_CHANNEL_2').num_parens_arguments_range)
                [15]
                >>> stim.gate_data('DETECTOR').num_parens_arguments_range
                range(0, 256)
                >>> list(stim.gate_data('OBSERVABLE_INCLUDE').num_parens_arguments_range)
                [1]
        )DOC")
            .data());

    c.def_property_readonly(
        "is_reset",
        [](const Gate &self) -> bool {
            return self.flags & GATE_IS_RESET;
        },
        clean_doc_string(R"DOC(
            Returns whether or not the gate resets qubits in any basis.

            Examples:
                >>> import stim

                >>> stim.gate_data('R').is_reset
                True
                >>> stim.gate_data('RX').is_reset
                True
                >>> stim.gate_data('MR').is_reset
                True

                >>> stim.gate_data('M').is_reset
                False
                >>> stim.gate_data('MXX').is_reset
                False
                >>> stim.gate_data('MPP').is_reset
                False
                >>> stim.gate_data('H').is_reset
                False
                >>> stim.gate_data('CX').is_reset
                False
                >>> stim.gate_data('HERALDED_ERASE').is_reset
                False
                >>> stim.gate_data('DEPOLARIZE2').is_reset
                False
                >>> stim.gate_data('X_ERROR').is_reset
                False
                >>> stim.gate_data('CORRELATED_ERROR').is_reset
                False
                >>> stim.gate_data('DETECTOR').is_reset
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "is_single_qubit_gate",
        [](const Gate &self) -> bool {
            return self.flags & GATE_IS_SINGLE_QUBIT_GATE;
        },
        clean_doc_string(R"DOC(
            Returns whether or not the gate is a single qubit gate.

            Single qubit gates apply separately to each of their targets.

            Variable-qubit gates like CORRELATED_ERROR and MPP are not
            considered single qubit gates.

            Examples:
                >>> import stim

                >>> stim.gate_data('H').is_single_qubit_gate
                True
                >>> stim.gate_data('R').is_single_qubit_gate
                True
                >>> stim.gate_data('M').is_single_qubit_gate
                True
                >>> stim.gate_data('X_ERROR').is_single_qubit_gate
                True

                >>> stim.gate_data('CX').is_single_qubit_gate
                False
                >>> stim.gate_data('MXX').is_single_qubit_gate
                False
                >>> stim.gate_data('CORRELATED_ERROR').is_single_qubit_gate
                False
                >>> stim.gate_data('MPP').is_single_qubit_gate
                False
                >>> stim.gate_data('DETECTOR').is_single_qubit_gate
                False
                >>> stim.gate_data('TICK').is_single_qubit_gate
                False
                >>> stim.gate_data('REPEAT').is_single_qubit_gate
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "flows",
        [](const Gate &self) -> pybind11::object {
            auto f = self.flows<MAX_BITWORD_WIDTH>();
            if (f.empty()) {
                return pybind11::none();
            }
            std::vector<Flow<MAX_BITWORD_WIDTH>> results;
            for (const auto &e : f) {
                results.push_back(e);
            }
            return pybind11::cast(results);
        },
        clean_doc_string(R"DOC(
            @signature def flows(self) -> Optional[List[stim.Flow]]:
            Returns stabilizer flow generators for the gate, or else None.

            A stabilizer flow describes an input-output relationship that the gate
            satisfies, where an input pauli string is transformed into an output
            pauli string mediated by certain measurement results.

            Caution: this method returns None for variable-target-count gates like MPP.
            Not because MPP has no stabilizer flows, but because its stabilizer flows
            depend on how many qubits it targets and what basis it targets them in.

            Returns:
                A list of stim.Flow instances representing the generators.

            Examples:
                >>> import stim

                >>> stim.gate_data('H').flows
                [stim.Flow("X -> Z"), stim.Flow("Z -> X")]

                >>> for e in stim.gate_data('ISWAP').flows:
                ...     print(e)
                X_ -> ZY
                Z_ -> _Z
                _X -> YZ
                _Z -> Z_

                >>> for e in stim.gate_data('MXX').flows:
                ...     print(e)
                X_ -> X_
                _X -> _X
                ZZ -> ZZ
                XX -> rec[-1]
        )DOC")
            .data());

    c.def_property_readonly(
        "is_symmetric_gate",
        [](const Gate &self) -> bool {
            return self.is_symmetric();
        },
        clean_doc_string(R"DOC(
            Returns whether or not the gate is the same when its targets are swapped.

            A two qubit gate is symmetric if it doesn't matter if you swap its targets. It
            is unaffected when conjugated by the SWAP gate.

            Single qubit gates are vacuously symmetric. A multi-qubit gate is symmetric if
            swapping any two of its targets has no effect.

            Note that this method is for symmetry *without broadcasting*. For example, SWAP
            is symmetric even though SWAP 1 2 3 4 isn't equal to SWAP 1 3 2 4.

            Returns:
                True if the gate is symmetric.
                False if the gate isn't symmetric.

            Examples:
                >>> import stim

                >>> stim.gate_data('CX').is_symmetric_gate
                False
                >>> stim.gate_data('CZ').is_symmetric_gate
                True
                >>> stim.gate_data('ISWAP').is_symmetric_gate
                True
                >>> stim.gate_data('CXSWAP').is_symmetric_gate
                False
                >>> stim.gate_data('MXX').is_symmetric_gate
                True
                >>> stim.gate_data('DEPOLARIZE2').is_symmetric_gate
                True
                >>> stim.gate_data('PAULI_CHANNEL_2').is_symmetric_gate
                False
                >>> stim.gate_data('H').is_symmetric_gate
                True
                >>> stim.gate_data('R').is_symmetric_gate
                True
                >>> stim.gate_data('X_ERROR').is_symmetric_gate
                True
                >>> stim.gate_data('CORRELATED_ERROR').is_symmetric_gate
                False
                >>> stim.gate_data('MPP').is_symmetric_gate
                False
                >>> stim.gate_data('DETECTOR').is_symmetric_gate
                False
        )DOC")
            .data());

    c.def(
        "hadamard_conjugated",
        [](const Gate &self, bool ignoring_sign) -> pybind11::object {
            GateType g = self.hadamard_conjugated(ignoring_sign);
            if (g == GateType::NOT_A_GATE) {
                return pybind11::none();
            }
            return pybind11::cast(GATE_DATA[g]);
        },
        pybind11::kw_only(),
        pybind11::arg("unsigned") = false,
        clean_doc_string(R"DOC(
            @signature def hadamard_conjugated(self, *, unsigned: bool = False) -> Optional[stim.GateData]:
            Returns a stim gate equivalent to this gate conjugated by Hadamard gates.

            The Hadamard conjugate can be thought of as the XZ dual of the gate; the gate
            you get by exchanging the X and Z bases. For example, a SQRT_X will become a
            SQRT_Z and a CX gate will switch directions into an XCZ.

            If stim doesn't define a gate equivalent to conjugating this gate by Hadamards,
            the value `None` is returned.

            Args:
                unsigned: Defaults to False. When False, the returned gate must be *exactly*
                    the Hadamard conjugation of this gate. When True, the returned gate must
                    have the same flows but the sign of the flows can be different (i.e.
                    the returned gate must be the Hadamard conjugate up to Pauli gate
                    differences).

            Returns:
                A stim.GateData instance of the Hadamard conjugate, if it exists in stim.

                None, if stim doesn't define a gate equal to the Hadamard conjugate.

            Examples:
                >>> import stim

                >>> stim.gate_data('X').hadamard_conjugated()
                stim.gate_data('Z')
                >>> stim.gate_data('CX').hadamard_conjugated()
                stim.gate_data('XCZ')
                >>> stim.gate_data('RY').hadamard_conjugated() is None
                True
                >>> stim.gate_data('RY').hadamard_conjugated(unsigned=True)
                stim.gate_data('RY')
                >>> stim.gate_data('ISWAP').hadamard_conjugated(unsigned=True) is None
                True
                >>> stim.gate_data('SWAP').hadamard_conjugated()
                stim.gate_data('SWAP')
                >>> stim.gate_data('CXSWAP').hadamard_conjugated()
                stim.gate_data('SWAPCX')
                >>> stim.gate_data('MXX').hadamard_conjugated()
                stim.gate_data('MZZ')
                >>> stim.gate_data('DEPOLARIZE1').hadamard_conjugated()
                stim.gate_data('DEPOLARIZE1')
                >>> stim.gate_data('X_ERROR').hadamard_conjugated()
                stim.gate_data('Z_ERROR')
                >>> stim.gate_data('H_XY').hadamard_conjugated()
                stim.gate_data('H_NYZ')
                >>> stim.gate_data('DETECTOR').hadamard_conjugated(unsigned=True)
                stim.gate_data('DETECTOR')
        )DOC")
            .data());

    c.def_property_readonly(
        "is_two_qubit_gate",
        [](const Gate &self) -> bool {
            return self.flags & GATE_TARGETS_PAIRS;
        },
        clean_doc_string(R"DOC(
            Returns whether or not the gate is a two qubit gate.

            Two qubit gates must be given an even number of targets.

            Variable-qubit gates like CORRELATED_ERROR and MPP are not
            considered two qubit gates.

            Returns:
                True if the gate is a two qubit gate.
                False if the gate isn't a two qubit gate.

            Examples:
                >>> import stim

                >>> stim.gate_data('CX').is_two_qubit_gate
                True
                >>> stim.gate_data('MXX').is_two_qubit_gate
                True

                >>> stim.gate_data('H').is_two_qubit_gate
                False
                >>> stim.gate_data('R').is_two_qubit_gate
                False
                >>> stim.gate_data('M').is_two_qubit_gate
                False
                >>> stim.gate_data('X_ERROR').is_two_qubit_gate
                False
                >>> stim.gate_data('CORRELATED_ERROR').is_two_qubit_gate
                False
                >>> stim.gate_data('MPP').is_two_qubit_gate
                False
                >>> stim.gate_data('DETECTOR').is_two_qubit_gate
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "is_noisy_gate",
        [](const Gate &self) -> bool {
            return self.flags & GATE_IS_NOISY;
        },
        clean_doc_string(R"DOC(
            Returns whether or not the gate can produce noise.

            Note that measurement operations are considered noisy,
            because for example `M(0.001) 2 3 5` will include
            noise that flips its result 0.1% of the time.

            Examples:
                >>> import stim

                >>> stim.gate_data('M').is_noisy_gate
                True
                >>> stim.gate_data('MXX').is_noisy_gate
                True
                >>> stim.gate_data('X_ERROR').is_noisy_gate
                True
                >>> stim.gate_data('CORRELATED_ERROR').is_noisy_gate
                True
                >>> stim.gate_data('MPP').is_noisy_gate
                True

                >>> stim.gate_data('H').is_noisy_gate
                False
                >>> stim.gate_data('CX').is_noisy_gate
                False
                >>> stim.gate_data('R').is_noisy_gate
                False
                >>> stim.gate_data('DETECTOR').is_noisy_gate
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "produces_measurements",
        [](const Gate &self) -> bool {
            return self.flags & GATE_PRODUCES_RESULTS;
        },
        clean_doc_string(R"DOC(
            Returns whether or not the gate produces measurement results.

            Examples:
                >>> import stim

                >>> stim.gate_data('M').produces_measurements
                True
                >>> stim.gate_data('MRY').produces_measurements
                True
                >>> stim.gate_data('MXX').produces_measurements
                True
                >>> stim.gate_data('MPP').produces_measurements
                True
                >>> stim.gate_data('HERALDED_ERASE').produces_measurements
                True

                >>> stim.gate_data('H').produces_measurements
                False
                >>> stim.gate_data('CX').produces_measurements
                False
                >>> stim.gate_data('R').produces_measurements
                False
                >>> stim.gate_data('X_ERROR').produces_measurements
                False
                >>> stim.gate_data('CORRELATED_ERROR').produces_measurements
                False
                >>> stim.gate_data('DETECTOR').produces_measurements
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "takes_pauli_targets",
        [](const Gate &self) -> bool {
            return self.flags & GATE_TARGETS_PAULI_STRING;
        },
        clean_doc_string(R"DOC(
            Returns whether or not the gate expects pauli targets.

            For example, `CORRELATED_ERROR` takes targets like `X0` and `Y1`
            instead of `0` or `1`.

            Examples:
                >>> import stim

                >>> stim.gate_data('CORRELATED_ERROR').takes_pauli_targets
                True
                >>> stim.gate_data('MPP').takes_pauli_targets
                True

                >>> stim.gate_data('H').takes_pauli_targets
                False
                >>> stim.gate_data('CX').takes_pauli_targets
                False
                >>> stim.gate_data('R').takes_pauli_targets
                False
                >>> stim.gate_data('M').takes_pauli_targets
                False
                >>> stim.gate_data('MRY').takes_pauli_targets
                False
                >>> stim.gate_data('MXX').takes_pauli_targets
                False
                >>> stim.gate_data('X_ERROR').takes_pauli_targets
                False
                >>> stim.gate_data('DETECTOR').takes_pauli_targets
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "inverse",
        [](const Gate &self) -> pybind11::object {
            if (self.flags & GATE_IS_UNITARY) {
                const auto &inv = GATE_DATA[self.best_candidate_inverse_id];
                return pybind11::cast(inv);
            }
            return pybind11::none();
        },
        clean_doc_string(R"DOC(
            @signature def inverse(self) -> Optional[stim.GateData]:
            The inverse of the gate, or None if it has no inverse.

            The inverse V of a gate U must have the property that V undoes the effects of U
            and that U undoes the effects of V. In particular, the circuit

                U 0 1
                V 0 1

            should be equivalent to doing nothing at all.

            Examples:
                >>> import stim

                >>> stim.gate_data('H').inverse
                stim.gate_data('H')

                >>> stim.gate_data('CX').inverse
                stim.gate_data('CX')

                >>> stim.gate_data('S').inverse
                stim.gate_data('S_DAG')

                >>> stim.gate_data('CXSWAP').inverse
                stim.gate_data('SWAPCX')

                >>> stim.gate_data('X_ERROR').inverse is None
                True
                >>> stim.gate_data('M').inverse is None
                True
                >>> stim.gate_data('R').inverse is None
                True
                >>> stim.gate_data('DETECTOR').inverse is None
                True
                >>> stim.gate_data('TICK').inverse is None
                True
        )DOC")
            .data());

    c.def_property_readonly(
        "generalized_inverse",
        [](const Gate &self) -> Gate {
            return GATE_DATA[self.best_candidate_inverse_id];
        },
        clean_doc_string(R"DOC(
            The closest-thing-to-an-inverse for the gate, if forced to pick something.

            The generalized inverse of a unitary gate U is its actual inverse U^-1.

            The generalized inverse of a reset or measurement gate U is a gate V such that,
            for every stabilizer flow that U has, V has the time reverse of that flow (up
            to Pauli feedback, with potentially more flows). For example, the time-reverse
            of R is MR because R has the single flow 1 -> Z and MR has the time reversed
            flow Z -> rec[-1].

            The generalized inverse of noise like X_ERROR is just the same noise.

            The generalized inverse of an annotation like TICK is just the same annotation.

            Examples:
                >>> import stim

                >>> stim.gate_data('H').generalized_inverse
                stim.gate_data('H')

                >>> stim.gate_data('CXSWAP').generalized_inverse
                stim.gate_data('SWAPCX')

                >>> stim.gate_data('X_ERROR').generalized_inverse
                stim.gate_data('X_ERROR')

                >>> stim.gate_data('MX').generalized_inverse
                stim.gate_data('MX')

                >>> stim.gate_data('MRY').generalized_inverse
                stim.gate_data('MRY')

                >>> stim.gate_data('R').generalized_inverse
                stim.gate_data('M')

                >>> stim.gate_data('DETECTOR').generalized_inverse
                stim.gate_data('DETECTOR')

                >>> stim.gate_data('TICK').generalized_inverse
                stim.gate_data('TICK')
        )DOC")
            .data());

    c.def_property_readonly(
        "takes_measurement_record_targets",
        [](const Gate &self) -> bool {
            return self.flags & (GATE_CAN_TARGET_BITS | GATE_ONLY_TARGETS_MEASUREMENT_RECORD);
        },
        clean_doc_string(R"DOC(
            Returns whether or not the gate can accept rec targets.

            For example, `CX` can take a measurement record target
            like `CX rec[-1] 1`.

            Examples:
                >>> import stim

                >>> stim.gate_data('CX').takes_measurement_record_targets
                True
                >>> stim.gate_data('DETECTOR').takes_measurement_record_targets
                True

                >>> stim.gate_data('H').takes_measurement_record_targets
                False
                >>> stim.gate_data('SWAP').takes_measurement_record_targets
                False
                >>> stim.gate_data('R').takes_measurement_record_targets
                False
                >>> stim.gate_data('M').takes_measurement_record_targets
                False
                >>> stim.gate_data('MRY').takes_measurement_record_targets
                False
                >>> stim.gate_data('MXX').takes_measurement_record_targets
                False
                >>> stim.gate_data('X_ERROR').takes_measurement_record_targets
                False
                >>> stim.gate_data('CORRELATED_ERROR').takes_measurement_record_targets
                False
                >>> stim.gate_data('MPP').takes_measurement_record_targets
                False
        )DOC")
            .data());
}
