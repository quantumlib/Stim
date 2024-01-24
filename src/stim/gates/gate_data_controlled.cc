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

void GateDataMap::add_gate_data_controlled(bool &failed) {
    add_gate(
        failed,
        Gate{
            .name = "XCX",
            .id = GateType::XCX,
            .best_candidate_inverse_id = GateType::XCX,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
The X-controlled X gate.
First qubit is the control, second qubit is the target.

Applies an X gate to the target if the control is in the |-> state.

Negates the amplitude of the |->|-> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data =
                {{0.5f, 0.5f, 0.5f, -0.5f},
                 {0.5f, 0.5f, -0.5f, 0.5f},
                 {0.5f, -0.5f, 0.5f, 0.5f},
                 {-0.5f, 0.5f, 0.5f, 0.5f}},
            .flow_data = {"+XI", "+ZX", "+IX", "+XZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
CNOT 0 1
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "XCY",
            .id = GateType::XCY,
            .best_candidate_inverse_id = GateType::XCY,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
The X-controlled Y gate.
First qubit is the control, second qubit is the target.

Applies a Y gate to the target if the control is in the |-> state.

Negates the amplitude of the |->|-i> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data =
                {{0.5f, 0.5f, -0.5f * i, 0.5f * i},
                 {0.5f, 0.5f, 0.5f * i, -0.5f * i},
                 {0.5f * i, -0.5f * i, 0.5f, 0.5f},
                 {-0.5f * i, 0.5f * i, 0.5f, 0.5f}},
            .flow_data = {"+XI", "+ZY", "+XX", "+XZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
S 1
S 1
S 1
CNOT 0 1
H 0
S 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "XCZ",
            .id = GateType::XCZ,
            .best_candidate_inverse_id = GateType::XCZ,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_BITS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
The X-controlled Z gate.
Applies a Z gate to the target if the control is in the |-> state.
Equivalently: negates the amplitude of the |->|1> state.
Same as a CX gate, but with reversed qubit order.
The first qubit is the control, and the second qubit is the target.

To perform a classically controlled X, replace the Z target with a `rec`
target like rec[-2].

To perform an I or X gate as configured by sweep data, replace the
Z target with a `sweep` target like sweep[3].

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    # Bit flip qubit 5 controlled by qubit 2.
    XCZ 5 2

    # Perform CX 2 5 then CX 4 2.
    XCZ 5 2 2 4

    # Bit flip qubit 6 if the most recent measurement result was TRUE.
    XCZ 6 rec[-1]

    # Bit flip qubits 7 and 8 conditioned on sweep configuration data.
    XCZ 7 sweep[5] 8 sweep[5]
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}},
            .flow_data = {"+XI", "+ZZ", "+XX", "+IZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
CNOT 1 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "YCX",
            .id = GateType::YCX,
            .best_candidate_inverse_id = GateType::YCX,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
The Y-controlled X gate.
First qubit is the control, second qubit is the target.

Applies an X gate to the target if the control is in the |-i> state.

Negates the amplitude of the |-i>|-> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data =
                {{0.5f, -i * 0.5f, 0.5f, i * 0.5f},
                 {i * 0.5f, 0.5f, -i * 0.5f, 0.5f},
                 {0.5f, i * 0.5f, 0.5f, -i * 0.5f},
                 {-i * 0.5f, 0.5f, i * 0.5f, 0.5f}},
            .flow_data = {"+XX", "+ZX", "+IX", "+YZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
S 0
H 1
CNOT 1 0
S 0
H 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "YCY",
            .id = GateType::YCY,
            .best_candidate_inverse_id = GateType::YCY,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
The Y-controlled Y gate.
First qubit is the control, second qubit is the target.

Applies a Y gate to the target if the control is in the |-i> state.

Negates the amplitude of the |-i>|-i> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
            .unitary_data =
                {{0.5f, -i * 0.5f, -i * 0.5f, 0.5f},
                 {i * 0.5f, 0.5f, -0.5f, -i * 0.5f},
                 {i * 0.5f, -0.5f, 0.5f, -i * 0.5f},
                 {0.5f, i * 0.5f, i * 0.5f, 0.5f}},
            .flow_data = {"+XY", "+ZY", "+YX", "+YZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
S 0
S 1
S 1
S 1
H 0
CNOT 0 1
H 0
S 0
S 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "YCZ",
            .id = GateType::YCZ,
            .best_candidate_inverse_id = GateType::YCZ,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_BITS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
The Y-controlled Z gate.
Applies a Z gate to the target if the control is in the |-i> state.
Equivalently: negates the amplitude of the |-i>|1> state.
Same as a CY gate, but with reversed qubit order.
The first qubit is called the control, and the second qubit is the target.

To perform a classically controlled Y, replace the Z target with a `rec`
target like rec[-2].

To perform an I or Y gate as configured by sweep data, replace the
Z target with a `sweep` target like sweep[3].

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    # Apply Y to qubit 5 controlled by qubit 2.
    YCZ 5 2

    # Perform CY 2 5 then CY 4 2.
    YCZ 5 2 2 4

    # Apply Y to qubit 6 if the most recent measurement result was TRUE.
    YCZ 6 rec[-1]

    # Apply Y to qubits 7 and 8 conditioned on sweep configuration data.
    YCZ 7 sweep[5] 8 sweep[5]
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, -i}, {0, 0, i, 0}},
            .flow_data = {"+XZ", "+ZZ", "+YX", "+IZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
S 0
CNOT 1 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "CX",
            .id = GateType::CX,
            .best_candidate_inverse_id = GateType::CX,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_BITS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
The Z-controlled X gate.
Applies an X gate to the target if the control is in the |1> state.
Equivalently: negates the amplitude of the |1>|-> state.
The first qubit is called the control, and the second qubit is the target.

To perform a classically controlled X, replace the control with a `rec`
target like rec[-2].

To perform an I or X gate as configured by sweep data, replace the
control with a `sweep` target like sweep[3].

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    # Bit flip qubit 5 controlled by qubit 2.
    CX 2 5

    # Perform CX 2 5 then CX 4 2.
    CX 2 5 4 2

    # Bit flip qubit 6 if the most recent measurement result was TRUE.
    CX rec[-1] 6

    # Bit flip qubits 7 and 8 conditioned on sweep configuration data.
    CX sweep[5] 7 sweep[5] 8
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 1, 0, 0}},
            .flow_data = {"+XX", "+ZI", "+IX", "+ZZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
CNOT 0 1
)CIRCUIT",
        });

    add_gate_alias(failed, "ZCX", "CX");
    add_gate_alias(failed, "CNOT", "CX");

    add_gate(
        failed,
        Gate{
            .name = "CY",
            .id = GateType::CY,
            .best_candidate_inverse_id = GateType::CY,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_BITS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
The Z-controlled Y gate.
Applies a Y gate to the target if the control is in the |1> state.
Equivalently: negates the amplitude of the |1>|-i> state.
The first qubit is the control, and the second qubit is the target.

To perform a classically controlled Y, replace the control with a `rec`
target like rec[-2].

To perform an I or Y gate as configured by sweep data, replace the
control with a `sweep` target like sweep[3].

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    # Apply Y to qubit 5 controlled by qubit 2.
    CY 2 5

    # Perform CY 2 5 then CX 4 2.
    CY 2 5 4 2

    # Apply Y to qubit 6 if the most recent measurement result was TRUE.
    CY rec[-1] 6

    # Apply Y to qubits 7 and 8 conditioned on sweep configuration data.
    CY sweep[5] 7 sweep[5] 8
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, 0, 0, -i}, {0, 0, 1, 0}, {0, i, 0, 0}},
            .flow_data = {"+XY", "+ZI", "+ZX", "+ZZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 1
S 1
S 1
CNOT 0 1
S 1
)CIRCUIT",
        });

    add_gate_alias(failed, "ZCY", "CY");

    add_gate(
        failed,
        Gate{
            .name = "CZ",
            .id = GateType::CZ,
            .best_candidate_inverse_id = GateType::CZ,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_BITS),
            .category = "C_Two Qubit Clifford Gates",
            .help = R"MARKDOWN(
The Z-controlled Z gate.
Applies a Z gate to the target if the control is in the |1> state.
Equivalently: negates the amplitude of the |1>|1> state.
The first qubit is called the control, and the second qubit is the target.

To perform a classically controlled Z, replace either qubit with a `rec`
target like rec[-2].

To perform an I or Z gate as configured by sweep data, replace either qubit
with a `sweep` target like sweep[3].

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    # Apply Z to qubit 5 controlled by qubit 2.
    CZ 2 5

    # Perform CZ 2 5 then CZ 4 2.
    CZ 2 5 4 2

    # Apply Z to qubit 6 if the most recent measurement result was TRUE.
    CZ rec[-1] 6

    # Apply Z to qubit 7 if the 3rd most recent measurement result was TRUE.
    CZ 7 rec[-3]

    # Apply Z to qubits 7 and 8 conditioned on sweep configuration data.
    CZ sweep[5] 7 8 sweep[5]
)MARKDOWN",
            .unitary_data = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, -1}},
            .flow_data = {"+XZ", "+ZI", "+ZX", "+IZ"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 1
CNOT 0 1
H 1
)CIRCUIT",
        });

    add_gate_alias(failed, "ZCZ", "CZ");
}
