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

void GateDataMap::add_gate_data_collapsing(bool &failed) {
    // ===================== Measure Gates. ============================
    add_gate(
        failed,
        Gate{
            .name = "MX",
            .id = GateType::MX,
            .best_candidate_inverse_id = GateType::MX,
            .arg_count = ARG_COUNT_SYGIL_ZERO_OR_ONE,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                                 GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "L_Collapsing Gates",
            .help = R"MARKDOWN(
X-basis measurement.
Projects each target qubit into `|+>` or `|->` and reports its value (false=`|+>`, true=`|->`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the X basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the X basis, and append the result into the measurement record.
    MX 5

    # Measure qubit 5 in the X basis, and append the INVERSE of its result into the measurement record.
    MX !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MX(0.01) 5

    # Measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MX 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MX(0.02) 2 3 5
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {"X -> rec[-1]", "X -> +X"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
M 0
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "MY",
            .id = GateType::MY,
            .best_candidate_inverse_id = GateType::MY,
            .arg_count = ARG_COUNT_SYGIL_ZERO_OR_ONE,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                                 GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "L_Collapsing Gates",
            .help = R"MARKDOWN(
Y-basis measurement.
Projects each target qubit into `|i>` or `|-i>` and reports its value (false=`|i>`, true=`|-i>`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the Y basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Y basis, and append the result into the measurement record.
    MY 5

    # Measure qubit 5 in the Y basis, and append the INVERSE of its result into the measurement record.
    MY !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MY(0.01) 5

    # Measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MY 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MY(0.02) 2 3 5
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {"Y -> rec[-1]", "Y -> +Y"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
S 0
H 0
M 0
H 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "M",
            .id = GateType::M,
            .best_candidate_inverse_id = GateType::M,
            .arg_count = ARG_COUNT_SYGIL_ZERO_OR_ONE,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                                 GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "L_Collapsing Gates",
            .help = R"MARKDOWN(
Z-basis measurement.
Projects each target qubit into `|0>` or `|1>` and reports its value (false=`|0>`, true=`|1>`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the Z basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Z basis, and append the result into the measurement record.
    M 5

    # 'MZ' is the same as 'M'. This also measures qubit 5 in the Z basis.
    MZ 5

    # Measure qubit 5 in the Z basis, and append the INVERSE of its result into the measurement record.
    MZ !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MZ(0.01) 5

    # Measure multiple qubits in the Z basis, putting 3 bits into the measurement record.
    MZ 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MZ(0.02) 2 3 5
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {"Z -> rec[-1]", "Z -> +Z"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
M 0
)CIRCUIT",
        });

    add_gate_alias(failed, "MZ", "M");

    // ===================== Measure+Reset Gates. ============================
    add_gate(
        failed,
        Gate{
            .name = "MRX",
            .id = GateType::MRX,
            .best_candidate_inverse_id = GateType::MRX,
            .arg_count = ARG_COUNT_SYGIL_ZERO_OR_ONE,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                                 GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_IS_RESET),
            .category = "L_Collapsing Gates",
            .help = R"MARKDOWN(
X-basis demolition measurement (optionally noisy).
Projects each target qubit into `|+>` or `|->`, reports its value (false=`|+>`, true=`|->`), then resets to `|+>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the X basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the X basis, reset it to the |+> state, append the measurement result into the measurement record.
    MRX 5

    # Demolition measure qubit 5 in the X basis, but append the INVERSE of its result into the measurement record.
    MRX !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRX(0.01) 5

    # Demolition measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MRX 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRX(0.02) 2 3 5
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {"X -> rec[-1]", "1 -> +X"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
H 0
M 0
R 0
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "MRY",
            .id = GateType::MRY,
            .best_candidate_inverse_id = GateType::MRY,
            .arg_count = ARG_COUNT_SYGIL_ZERO_OR_ONE,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                                 GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_IS_RESET),
            .category = "L_Collapsing Gates",
            .help = R"MARKDOWN(
Y-basis demolition measurement (optionally noisy).
Projects each target qubit into `|i>` or `|-i>`, reports its value (false=`|i>`, true=`|-i>`), then resets to `|i>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the Y basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Y basis, reset it to the |i> state, append the measurement result into the measurement record.
    MRY 5

    # Demolition measure qubit 5 in the Y basis, but append the INVERSE of its result into the measurement record.
    MRY !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRY(0.01) 5

    # Demolition measure multiple qubits in the Y basis, putting 3 bits into the measurement record.
    MRY 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRY(0.02) 2 3 5
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {"Y -> rec[-1]", "1 -> +Y"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0
S 0
S 0
H 0
M 0
R 0
H 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "MR",
            .id = GateType::MR,
            .best_candidate_inverse_id = GateType::MR,
            .arg_count = ARG_COUNT_SYGIL_ZERO_OR_ONE,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                                 GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_IS_RESET),
            .category = "L_Collapsing Gates",
            .help = R"MARKDOWN(
Z-basis demolition measurement (optionally noisy).
Projects each target qubit into `|0>` or `|1>`, reports its value (false=`|0>`, true=`|1>`), then resets to `|0>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the Z basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Z basis, reset it to the |0> state, append the measurement result into the measurement record.
    MRZ 5

    # MR is also a Z-basis demolition measurement.
    MR 5

    # Demolition measure qubit 5 in the Z basis, but append the INVERSE of its result into the measurement record.
    MRZ !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRZ(0.01) 5

    # Demolition measure multiple qubits in the Z basis, putting 3 bits into the measurement record.
    MRZ 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRZ(0.02) 2 3 5
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {"Z -> rec[-1]", "1 -> +Z"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
M 0
R 0
)CIRCUIT",
        });

    add_gate_alias(failed, "MRZ", "MR");

    // ===================== Reset Gates. ============================
    add_gate(
        failed,
        Gate{
            .name = "RX",
            .id = GateType::RX,
            .best_candidate_inverse_id = GateType::MX,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_RESET),
            .category = "L_Collapsing Gates",
            .help = R"MARKDOWN(
X-basis reset.
Forces each target qubit into the `|+>` state by silently measuring it in the X basis and applying a `Z` gate if it ended up in the `|->` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the X basis.

Examples:

    # Reset qubit 5 into the |+> state.
    RX 5

    # Reset multiple qubits into the |+> state.
    RX 2 3 5
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {"1 -> +X"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
R 0
H 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "RY",
            .id = GateType::RY,
            .best_candidate_inverse_id = GateType::MY,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_RESET),
            .category = "L_Collapsing Gates",
            .help = R"MARKDOWN(
Y-basis reset.
Forces each target qubit into the `|i>` state by silently measuring it in the Y basis and applying an `X` gate if it ended up in the `|-i>` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the Y basis.

Examples:

    # Reset qubit 5 into the |i> state.
    RY 5

    # Reset multiple qubits into the |i> state.
    RY 2 3 5
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {"1 -> +Y"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
R 0
H 0
S 0
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "R",
            .id = GateType::R,
            .best_candidate_inverse_id = GateType::M,
            .arg_count = 0,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_RESET),
            .category = "L_Collapsing Gates",
            .help = R"MARKDOWN(
Z-basis reset.
Forces each target qubit into the `|0>` state by silently measuring it in the Z basis and applying an `X` gate if it ended up in the `|1>` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the Z basis.

Examples:

    # Reset qubit 5 into the |0> state.
    RZ 5

    # R means the same thing as RZ.
    R 5

    # Reset multiple qubits into the |0> state.
    RZ 2 3 5
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {"1 -> +Z"},
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
R 0
)CIRCUIT",
        });

    add_gate_alias(failed, "RZ", "R");
}
