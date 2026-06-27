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

void GateDataMap::add_gate_data_swaps(bool &failed) {
    add_gate(
        failed,
        Gate{
            .name = "SWAP",
            .id = GateType::SWAP,
            .best_candidate_inverse_id = GateType::SWAP,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
Swaps two qubits.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}},
            .flow_data = {"+IX", "+IZ", "+XI", "+ZI"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
CNOT 0 1
CNOT 1 0
CNOT 0 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "ISWAP",
            .id = GateType::ISWAP,
            .best_candidate_inverse_id = GateType::ISWAP_DAG,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
Swaps two qubits and phases the -1 eigenspace of the ZZ observable by i.
Equivalent to `SWAP` then `CZ` then `S` on both targets.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, 0, i, 0}, {0, i, 0, 0}, {0, 0, 0, 1}},
            .flow_data = {"+ZY", "+IZ", "+YZ", "+ZI"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
CNOT 0 1
CNOT 1 0
H 1
S 1
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "ISWAP_DAG",
            .id = GateType::ISWAP_DAG,
            .best_candidate_inverse_id = GateType::ISWAP,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
Swaps two qubits and phases the -1 eigenspace of the ZZ observable by -i.
Equivalent to `SWAP` then `CZ` then `S_DAG` on both targets.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, 0, -i, 0}, {0, -i, 0, 0}, {0, 0, 0, 1}},
            .flow_data = {"-ZY", "+IZ", "-YZ", "+ZI"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
S 0
S 1
S 1
S 1
H 1
CNOT 1 0
CNOT 0 1
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "CXSWAP",
            .id = GateType::CXSWAP,
            .best_candidate_inverse_id = GateType::SWAPCX,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
A combination CX-then-SWAP gate.
This gate is kak-equivalent to the iswap gate, but preserves X/Z noise bias.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}, {0, 1, 0, 0}},
            .flow_data = {"+XX", "+IZ", "+XI", "+ZZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
CNOT 1 0
CNOT 0 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "SWAPCX",
            .id = GateType::SWAPCX,
            .best_candidate_inverse_id = GateType::CXSWAP,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
A combination SWAP-then-CX gate.
This gate is kak-equivalent to the iswap gate, but preserves X/Z noise bias.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 1, 0, 0}, {0, 0, 1, 0}},
            .flow_data = {"+IX", "+ZZ", "+XX", "+ZI"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
CNOT 0 1
CNOT 1 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "CZSWAP",
            .id = GateType::CZSWAP,
            .best_candidate_inverse_id = GateType::CZSWAP,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
A combination CZ-and-SWAP gate.
This gate is kak-equivalent to the iswap gate.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, -1}},
            .flow_data = {"+ZX", "+IZ", "+XZ", "+ZI"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
CX 0 1
CX 1 0
H 1
)CIRCUIT",
        });
    add_gate_alias(failed, "SWAPCZ", "CZSWAP");
}
