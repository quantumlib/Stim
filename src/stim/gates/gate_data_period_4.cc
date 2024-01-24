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

void GateDataMap::add_gate_data_period_4(bool &failed) {
    add_gate(
        failed,
        Gate{
            .name = "SQRT_X",
            .id = GateType::SQRT_X,
            .best_candidate_inverse_id = GateType::SQRT_X_DAG,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Principal square root of X gate.
Phases the amplitude of |-> by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f + 0.5f * i, 0.5f - 0.5f * i}, {0.5f - 0.5f * i, 0.5f + 0.5f * i}},
            .flow_data = {"+X", "-Y"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
S 0
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "SQRT_X_DAG",
            .id = GateType::SQRT_X_DAG,
            .best_candidate_inverse_id = GateType::SQRT_X,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Adjoint of the principal square root of X gate.
Phases the amplitude of |-> by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f - 0.5f * i, 0.5f + 0.5f * i}, {0.5f + 0.5f * i, 0.5f - 0.5f * i}},
            .flow_data = {"+X", "+Y"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
H 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "SQRT_Y",
            .id = GateType::SQRT_Y,
            .best_candidate_inverse_id = GateType::SQRT_Y_DAG,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Principal square root of Y gate.
Phases the amplitude of |-i> by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f + 0.5f * i, -0.5f - 0.5f * i}, {0.5f + 0.5f * i, 0.5f + 0.5f * i}},
            .flow_data = {"-Z", "+X"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "SQRT_Y_DAG",
            .id = GateType::SQRT_Y_DAG,
            .best_candidate_inverse_id = GateType::SQRT_Y,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Adjoint of the principal square root of Y gate.
Phases the amplitude of |-i> by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f - 0.5f * i, 0.5f - 0.5f * i}, {-0.5f + 0.5f * i, 0.5f - 0.5f * i}},
            .flow_data = {"+Z", "-X"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
S 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "S",
            .id = GateType::S,
            .best_candidate_inverse_id = GateType::S_DAG,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Principal square root of Z gate.
Phases the amplitude of |1> by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{1, 0}, {0, i}},
            .flow_data = {"+Y", "+Z"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
)CIRCUIT",
        });

    add_gate_alias(failed, "SQRT_Z", "S");

    add_gate(
        failed,
        Gate{
            .name = "S_DAG",
            .id = GateType::S_DAG,
            .best_candidate_inverse_id = GateType::S,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Adjoint of the principal square root of Z gate.
Phases the amplitude of |1> by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{1, 0}, {0, -i}},
            .flow_data = {"-Y", "+Z"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
S 0
)CIRCUIT",
        });

    add_gate_alias(failed, "SQRT_Z_DAG", "S_DAG");
}
