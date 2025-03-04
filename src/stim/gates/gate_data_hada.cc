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
static constexpr std::complex<float> s = 0.7071067811865475244f;

void GateDataMap::add_gate_data_hada(bool &failed) {
    add_gate(
        failed,
        Gate{
            .name = "H",
            .id = GateType::H,
            .best_candidate_inverse_id = GateType::H,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
The Hadamard gate.
Swaps the X and Z axes.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{s, s}, {s, -s}},
            .flow_data = {"+Z", "+X"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
)CIRCUIT",
        });

    add_gate_alias(failed, "H_XZ", "H");

    add_gate(
        failed,
        Gate{
            .name = "H_XY",
            .id = GateType::H_XY,
            .best_candidate_inverse_id = GateType::H_XY,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
A variant of the Hadamard gate that swaps the X and Y axes (instead of X and Z).

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0, s - i * s}, {s + i * s, 0}},
            .flow_data = {"+Y", "-Z"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
S 0
S 0
H 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "H_YZ",
            .id = GateType::H_YZ,
            .best_candidate_inverse_id = GateType::H_YZ,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
A variant of the Hadamard gate that swaps the Y and Z axes (instead of X and Z).

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{s, -i * s}, {i * s, -s}},
            .flow_data = {"-X", "+Y"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
S 0
H 0
S 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "H_NXY",
            .id = GateType::H_NXY,
            .best_candidate_inverse_id = GateType::H_NXY,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
A variant of the Hadamard gate that swaps the -X and +Y axes.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{0, s + s*i}, {s - s*i, 0}},
            .flow_data = {"-Y", "-Z"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
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
            .name = "H_NXZ",
            .id = GateType::H_NXZ,
            .best_candidate_inverse_id = GateType::H_NXZ,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
A variant of the Hadamard gate that swaps the -X and +Z axes.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{-s, s}, {s, s}},
            .flow_data = {"-Z", "-X"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
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
            .name = "H_NYZ",
            .id = GateType::H_NYZ,
            .best_candidate_inverse_id = GateType::H_NYZ,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_UNITARY),
            .category = "B_Single Qubit Clifford Gates",
            .help = R"MARKDOWN(
A variant of the Hadamard gate that swaps the -Y and +Z axes.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
            .unitary_data = {{-s, -i*s}, {i*s, s}},
            .flow_data = {"-X", "-Y"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
H 0
S 0
H 0
)CIRCUIT",
        });
}
