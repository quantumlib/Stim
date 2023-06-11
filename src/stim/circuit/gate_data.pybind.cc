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

#include "stim/circuit/gate_data.pybind.h"

#include "stim/stabilizers/tableau.pybind.h"
#include "stim/circuit/gate_data.h"
#include "stim/py/base.pybind.h"
#include "stim/str_util.h"

using namespace stim;
using namespace stim_pybind;

pybind11::class_<Gate> stim_pybind::pybind_gate_data(pybind11::module &m) {
    m.def(
        "gate_data",
        [](const std::string &name) {
            return GATE_DATA.at(name);
        },
        pybind11::arg("name"),
        clean_doc_string(R"DOC(
            Returns gate data for the given named gate.

            Examples:
                >>> import stim
                >>> stim.gate_data('cnot').aliases
                ['CNOT', 'CX', 'ZCX']
                >>> stim.gate_data('cnot').is_two_qubit_gate
                True
        )DOC")
            .data());

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
    c.def_property_readonly(
        "name",
        [](const Gate &self) -> const char * {
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
        [](const Gate &self) -> std::vector<std::string> {
            std::vector<std::string> aliases;
            for (const auto &h : GATE_DATA.hashed_name_to_gate_type_table) {
                if (h.id == self.id) {
                    aliases.push_back(h.expected_name);
                }
            }
            std::sort(aliases.begin(), aliases.end());
            return aliases;
        },
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

    c.def_property_readonly(
        "tableau",
        [](const Gate &self) -> pybind11::object {
            if (self.flags & GATE_IS_UNITARY) {
                return pybind11::cast(self.tableau());
            }
            return pybind11::none();
        },
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
        "unitary",
        [](const Gate &self) -> pybind11::object {
            if (self.flags & GATE_IS_UNITARY) {
                auto r = self.unitary();
                auto n = r.size();
                std::complex<float> *buffer = new std::complex<float>[n * n];
                for (size_t a = 0; a < n; a++) {
                    for (size_t b = 0; b < n; b++) {
                        buffer[b + a*n] = r[a][b];
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
        },
        clean_doc_string(R"DOC(
            @signature def unitary(self) -> Optional[np.ndarray]:
            Returns the gate's unitary matrix, or None if the gate isn't unitary.

            Examples:
                >>> import stim

                >>> print(stim.gate_data('M').unitary)
                None

                >>> stim.gate_data('X').unitary
                array([[0.+0.j, 1.+0.j],
                       [1.+0.j, 0.+0.j]], dtype=complex64)

                >>> stim.gate_data('ISWAP').unitary
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
                >>> stim.gate_data('CORRElATED_ERROR').is_unitary
                False
                >>> stim.gate_data('MPP').is_unitary
                False
                >>> stim.gate_data('DETECTOR').is_unitary
                False
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

            Multi-qubit gates like CORRELATED_ERROR and MPP are not
            considered two qubit gates.

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
                >>> stim.gate_data('CORRElATED_ERROR').is_two_qubit_gate
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
                >>> stim.gate_data('CORRElATED_ERROR').is_noisy_gate
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
        "is_measurement_gate",
        [](const Gate &self) -> bool {
            return self.flags & GATE_PRODUCES_RESULTS;
        },
        clean_doc_string(R"DOC(
            Returns whether or not the gate produces measurement results.

            Note that measurement+result operations are considered
            measurement operations.

            Examples:
                >>> import stim

                >>> stim.gate_data('M').is_measurement_gate
                True
                >>> stim.gate_data('MRY').is_measurement_gate
                True
                >>> stim.gate_data('MXX').is_measurement_gate
                True
                >>> stim.gate_data('MPP').is_measurement_gate
                True

                >>> stim.gate_data('H').is_measurement_gate
                False
                >>> stim.gate_data('CX').is_measurement_gate
                False
                >>> stim.gate_data('R').is_measurement_gate
                False
                >>> stim.gate_data('X_ERROR').is_measurement_gate
                False
                >>> stim.gate_data('CORRElATED_ERROR').is_measurement_gate
                False
                >>> stim.gate_data('DETECTOR').is_measurement_gate
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

                >>> stim.gate_data('CORRElATED_ERROR').takes_pauli_targets
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
                >>> stim.gate_data('CORRElATED_ERROR').takes_measurement_record_targets
                False
                >>> stim.gate_data('MPP').takes_measurement_record_targets
                False
        )DOC")
            .data());
}
