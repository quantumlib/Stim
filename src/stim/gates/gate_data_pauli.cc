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

using namespace stim;

static constexpr std::complex<float> i = std::complex<float>(0, 1);

void GateDataMap::add_gate_data_pauli(bool &failed) {
    add_gate(
        failed,
        Gate{
            .name = "I",
            .id = GateType::I,
            .best_candidate_inverse_id = GateType::I,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "A_Pauli Gates",
            .help = R"MARKDOWN(
The identity gate.
Does nothing to the target qubits.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to do nothing to.
)MARKDOWN",
            .unitary_data = {{1, 0}, {0, 1}},
            .flow_data = {"+X", "+Z"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
# (no operations)
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "X",
            .id = GateType::X,
            .best_candidate_inverse_id = GateType::X,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "A_Pauli Gates",
            .help = R"MARKDOWN(
The Pauli X gate.
The bit flip gate.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0, 1}, {1, 0}},
            .flow_data = {"+X", "-Z"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
S 0
S 0
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "Y",
            .id = GateType::Y,
            .best_candidate_inverse_id = GateType::Y,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "A_Pauli Gates",
            .help = R"MARKDOWN(
The Pauli Y gate.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0, -i}, {i, 0}},
            .flow_data = {"-X", "-Z"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
H 0
S 0
S 0
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "Z",
            .id = GateType::Z,
            .best_candidate_inverse_id = GateType::Z,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "A_Pauli Gates",
            .help = R"MARKDOWN(
The Pauli Z gate.
The phase flip gate.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{1, 0}, {0, -1}},
            .flow_data = {"-X", "+Z"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
)CIRCUIT",
        });
}
