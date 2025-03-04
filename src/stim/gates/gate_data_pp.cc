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

void GateDataMap::add_gate_data_pp(bool &failed) {
    add_gate(
        failed,
        Gate{
            .name = "II",
            .id = GateType::II,
            .best_candidate_inverse_id = GateType::II,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
A two-qubit identity gate.

Twice as much doing-nothing as the I gate! This gate only exists because it
can be useful as a communication mechanism for systems built on top of stim.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Examples:

    II 0 1

    R 0
    II[ACTUALLY_A_LEAKAGE_ISWAP] 0 1
    R 0
    CX 1 0
)MARKDOWN",
            .unitary_data =
                {{1, 0, 0, 0},
                 {0, 1, 0, 0},
                 {0, 0, 1, 0},
                 {0, 0, 0, 1}},
            .flow_data = {"+XI", "+ZI", "+IX", "+IZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "SQRT_XX",
            .id = GateType::SQRT_XX,
            .best_candidate_inverse_id = GateType::SQRT_XX_DAG,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
Phases the -1 eigenspace of the XX observable by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data =
                {{0.5f + 0.5f * i, 0, 0, 0.5f - 0.5f * i},
                 {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                 {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                 {0.5f - 0.5f * i, 0, 0, 0.5f + 0.5f * i}},
            .flow_data = {"+XI", "-YX", "+IX", "-XY"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
CNOT 0 1
H 1
S 0
S 1
H 0
H 1
)CIRCUIT",
        });
    add_gate(
        failed,
        Gate{
            .name = "SQRT_XX_DAG",
            .id = GateType::SQRT_XX_DAG,
            .best_candidate_inverse_id = GateType::SQRT_XX,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
Phases the -1 eigenspace of the XX observable by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data =
                {{0.5f - 0.5f * i, 0, 0, 0.5f + 0.5f * i},
                 {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                 {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                 {0.5f + 0.5f * i, 0, 0, 0.5f - 0.5f * i}},
            .flow_data = {"+XI", "+YX", "+IX", "+XY"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
CNOT 0 1
H 1
S 0
S 0
S 0
S 1
S 1
S 1
H 0
H 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "SQRT_YY",
            .id = GateType::SQRT_YY,
            .best_candidate_inverse_id = GateType::SQRT_YY_DAG,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
Phases the -1 eigenspace of the YY observable by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data =
                {{0.5f + 0.5f * i, 0, 0, -0.5f + 0.5f * i},
                 {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                 {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                 {-0.5f + 0.5f * i, 0, 0, 0.5f + 0.5f * i}},
            .flow_data = {"-ZY", "+XY", "-YZ", "+YX"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
S 0
S 1
S 1
S 1
H 0
CNOT 0 1
H 1
S 0
S 1
H 0
H 1
S 0
S 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "SQRT_YY_DAG",
            .id = GateType::SQRT_YY_DAG,
            .best_candidate_inverse_id = GateType::SQRT_YY,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
Phases the -1 eigenspace of the YY observable by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data =
                {{0.5f - 0.5f * i, 0, 0, -0.5f - 0.5f * i},
                 {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                 {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                 {-0.5f - 0.5f * i, 0, 0, 0.5f - 0.5f * i}},
            .flow_data = {"+ZY", "-XY", "+YZ", "-YX"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
S 0
S 1
H 0
CNOT 0 1
H 1
S 0
S 1
H 0
H 1
S 0
S 1
S 1
S 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "SQRT_ZZ",
            .id = GateType::SQRT_ZZ,
            .best_candidate_inverse_id = GateType::SQRT_ZZ_DAG,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
Phases the -1 eigenspace of the ZZ observable by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, i, 0, 0}, {0, 0, i, 0}, {0, 0, 0, 1}},
            .flow_data = {"+YZ", "+ZI", "+ZY", "+IZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 1
CNOT 0 1
H 1
S 0
S 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "SQRT_ZZ_DAG",
            .id = GateType::SQRT_ZZ_DAG,
            .best_candidate_inverse_id = GateType::SQRT_ZZ,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
Phases the -1 eigenspace of the ZZ observable by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, -i, 0, 0}, {0, 0, -i, 0}, {0, 0, 0, 1}},
            .flow_data = {"-YZ", "+ZI", "-ZY", "+IZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 1
CNOT 0 1
H 1
S 0
S 0
S 0
S 1
S 1
S 1
)CIRCUIT",
        });
}
