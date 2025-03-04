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

void GateDataMap::add_gate_data_period_3(bool &failed) {
    add_gate(
        failed,
        Gate{
            .name = "C_XYZ",
            .id = GateType::C_XYZ,
            .best_candidate_inverse_id = GateType::C_ZYX,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Right handed period 3 axis cycling gate, sending X -> Y -> Z -> X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f - i * 0.5f, -0.5f - 0.5f * i}, {0.5f - 0.5f * i, 0.5f + 0.5f * i}},
            .flow_data = {"Y", "X"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
S 0
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "C_NXYZ",
            .id = GateType::C_NXYZ,
            .best_candidate_inverse_id = GateType::C_ZYNX,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Performs the period-3 cycle -X -> Y -> Z -> -X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f + i * 0.5f, 0.5f - 0.5f * i}, {-0.5f - 0.5f * i, 0.5f - 0.5f * i}},
            .flow_data = {"-Y", "-X"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
S 0
H 0
S 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "C_XNYZ",
            .id = GateType::C_XNYZ,
            .best_candidate_inverse_id = GateType::C_ZNYX,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Performs the period-3 cycle X -> -Y -> Z -> X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f + i * 0.5f, -0.5f + 0.5f * i}, {0.5f + 0.5f * i, 0.5f - 0.5f * i}},
            .flow_data = {"-Y", "X"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "C_XYNZ",
            .id = GateType::C_XYNZ,
            .best_candidate_inverse_id = GateType::C_NZYX,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Performs the period-3 cycle X -> Y -> -Z -> X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f - i * 0.5f, 0.5f + 0.5f * i}, {-0.5f + 0.5f * i, 0.5f + 0.5f * i}},
            .flow_data = {"Y", "-X"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
H 0
S 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "C_ZYX",
            .id = GateType::C_ZYX,
            .best_candidate_inverse_id = GateType::C_XYZ,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Left handed period 3 axis cycling gate, sending Z -> Y -> X -> Z.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f + i * 0.5f, 0.5f + 0.5f * i}, {-0.5f + 0.5f * i, 0.5f - 0.5f * i}},
            .flow_data = {"Z", "Y"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "C_ZYNX",
            .id = GateType::C_ZYNX,
            .best_candidate_inverse_id = GateType::C_NXYZ,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Performs the period-3 cycle -X -> Z -> Y -> -X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f - i * 0.5f, -0.5f + 0.5f * i}, {0.5f + 0.5f * i, 0.5f + 0.5f * i}},
            .flow_data = {"-Z", "Y"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
H 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "C_ZNYX",
            .id = GateType::C_ZNYX,
            .best_candidate_inverse_id = GateType::C_XNYZ,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Performs the period-3 cycle X -> Z -> -Y -> X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f - i * 0.5f, 0.5f - 0.5f * i}, {-0.5f - 0.5f * i, 0.5f + 0.5f * i}},
            .flow_data = {"Z", "-Y"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
S 0
S 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "C_NZYX",
            .id = GateType::C_NZYX,
            .best_candidate_inverse_id = GateType::C_XYNZ,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
Performs the period-3 cycle X -> -Z -> Y -> X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0.5f + i * 0.5f, -0.5f + -0.5f * i}, {0.5f - 0.5f * i, 0.5f - 0.5f * i}},
            .flow_data = {"-Z", "-Y"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
H 0
S 0
S 0
S 0
)CIRCUIT",
        });
}
