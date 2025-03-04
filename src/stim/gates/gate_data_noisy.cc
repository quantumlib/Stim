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

void GateDataMap::add_gate_data_noisy(bool &failed) {
    add_gate(
        failed,
        Gate{
            .name = "DEPOLARIZE1",
            .id = GateType::DEPOLARIZE1,
            .best_candidate_inverse_id = GateType::DEPOLARIZE1,
            .arg_count = 1,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
The single qubit depolarizing channel.

Applies a single-qubit depolarizing error with the given probability.
When a single-qubit depolarizing error is applied, a random Pauli
error (except for I) is chosen and applied. Note that this means
maximal mixing occurs when the probability parameter is set to 75%,
rather than at 100%.

Applies a randomly chosen Pauli with a given probability.

Parens Arguments:

    A single float (p) specifying the depolarization strength.

Targets:

    Qubits to apply single-qubit depolarizing noise to.

Pauli Mixture:

    1-p: I
    p/3: X
    p/3: Y
    p/3: Z

Examples:

    # Apply 1-qubit depolarization to qubit 0 using p=1%
    DEPOLARIZE1(0.01) 0

    # Apply 1-qubit depolarization to qubit 2
    # Separately apply 1-qubit depolarization to qubits 3 and 5
    DEPOLARIZE1(0.01) 2 3 5

    # Maximally mix qubits 0 through 2
    DEPOLARIZE1(0.75) 0 1 2
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });

    add_gate(
        failed,
        Gate{
            .name = "DEPOLARIZE2",
            .id = GateType::DEPOLARIZE2,
            .best_candidate_inverse_id = GateType::DEPOLARIZE2,
            .arg_count = 1,
            .flags = (GateFlags)(GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_TARGETS_PAIRS),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
The two qubit depolarizing channel.

Applies a two-qubit depolarizing error with the given probability.
When a two-qubit depolarizing error is applied, a random pair of Pauli
errors (except for II) is chosen and applied. Note that this means
maximal mixing occurs when the probability parameter is set to 93.75%,
rather than at 100%.

Parens Arguments:

    A single float (p) specifying the depolarization strength.

Targets:

    Qubit pairs to apply two-qubit depolarizing noise to.

Pauli Mixture:

     1-p: II
    p/15: IX
    p/15: IY
    p/15: IZ
    p/15: XI
    p/15: XX
    p/15: XY
    p/15: XZ
    p/15: YI
    p/15: YX
    p/15: YY
    p/15: YZ
    p/15: ZI
    p/15: ZX
    p/15: ZY
    p/15: ZZ

Examples:

    # Apply 2-qubit depolarization to qubit 0 and qubit 1 using p=1%
    DEPOLARIZE2(0.01) 0 1

    # Apply 2-qubit depolarization to qubit 2 and qubit 3
    # Separately apply 2-qubit depolarization to qubit 5 and qubit 7
    DEPOLARIZE2(0.01) 2 3 5 7

    # Maximally mix qubits 0 through 3
    DEPOLARIZE2(0.9375) 0 1 2 3
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });

    add_gate(
        failed,
        Gate{
            .name = "I_ERROR",
            .id = GateType::I_ERROR,
            .best_candidate_inverse_id = GateType::I_ERROR,
            .arg_count = 1,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
Applies an identity with a given probability.

This gate has no effect. It only exists because it can be useful as a
communication mechanism for systems built on top of stim.

Parens Arguments:

    A single float specifying the probability of applying an I operation.

Targets:

    Qubits to apply identity noise to.

Pauli Mixture:

    1-p: I
     p : I

Examples:

    I_ERROR(0.1) 0

    I_ERROR[ACTUALLY_I_AM_LEAKAGE_NOISE_IN_AN_ADVANCED_SIMULATOR](0.1) 0 2 4
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });

    add_gate(
        failed,
        Gate{
            .name = "II_ERROR",
            .id = GateType::II_ERROR,
            .best_candidate_inverse_id = GateType::II_ERROR,
            .arg_count = 1,
            .flags = (GateFlags)(GATE_TARGETS_PAIRS | GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
Applies a two-qubit identity with a given probability.

This gate has no effect. It only exists because it can be useful as a
communication mechanism for systems built on top of stim.

Parens Arguments:

    A single float specifying the probability of applying an II operation.

Targets:

    Qubits to apply identity noise to.

Pauli Mixture:

    1-p: II
     p : II

Examples:

    II_ERROR(0.1) 0 1

    II_ERROR[ACTUALLY_I_SPECIFY_LEAKAGE_TRANSPORT_FOR_AN_ADVANCED_SIMULATOR](0.1) 0 2 4 6
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });

    add_gate(
        failed,
        Gate{
            .name = "X_ERROR",
            .id = GateType::X_ERROR,
            .best_candidate_inverse_id = GateType::X_ERROR,
            .arg_count = 1,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
Applies a Pauli X with a given probability.

Parens Arguments:

    A single float specifying the probability of applying an X operation.

Targets:

    Qubits to apply bit flip noise to.

Pauli Mixture:

    1-p: I
     p : X
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });

    add_gate(
        failed,
        Gate{
            .name = "Y_ERROR",
            .id = GateType::Y_ERROR,
            .best_candidate_inverse_id = GateType::Y_ERROR,
            .arg_count = 1,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
Applies a Pauli Y with a given probability.

Parens Arguments:

    A single float specifying the probability of applying a Y operation.

Targets:

    Qubits to apply Y flip noise to.

Pauli Mixture:

    1-p: I
     p : Y
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });

    add_gate(
        failed,
        Gate{
            .name = "Z_ERROR",
            .id = GateType::Z_ERROR,
            .best_candidate_inverse_id = GateType::Z_ERROR,
            .arg_count = 1,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
Applies a Pauli Z with a given probability.

Parens Arguments:

    A single float specifying the probability of applying a Z operation.

Targets:

    Qubits to apply phase flip noise to.

Pauli Mixture:

    1-p: I
     p : Z
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });

    add_gate(
        failed,
        Gate{
            .name = "PAULI_CHANNEL_1",
            .id = GateType::PAULI_CHANNEL_1,
            .best_candidate_inverse_id = GateType::PAULI_CHANNEL_1,
            .arg_count = 3,
            .flags = (GateFlags)(GATE_IS_SINGLE_QUBIT_GATE | GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
A single qubit Pauli error channel with explicitly specified probabilities for each case.

Parens Arguments:

    Three floats specifying disjoint Pauli case probabilities.
    px: Disjoint probability of applying an X error.
    py: Disjoint probability of applying a Y error.
    pz: Disjoint probability of applying a Z error.

Targets:

    Qubits to apply the custom noise channel to.

Example:

    # Sample errors from the distribution 10% X, 15% Y, 20% Z, 55% I.
    # Apply independently to qubits 1, 2, 4.
    PAULI_CHANNEL_1(0.1, 0.15, 0.2) 1 2 4

Pauli Mixture:

    1-px-py-pz: I
    px: X
    py: Y
    pz: Z
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });

    add_gate(
        failed,
        Gate{
            .name = "PAULI_CHANNEL_2",
            .id = GateType::PAULI_CHANNEL_2,
            .best_candidate_inverse_id = GateType::PAULI_CHANNEL_2,
            .arg_count = 15,
            .flags = (GateFlags)(GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_TARGETS_PAIRS),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
A two qubit Pauli error channel with explicitly specified probabilities for each case.

Parens Arguments:

    Fifteen floats specifying the disjoint probabilities of each possible Pauli pair
    that can occur (except for the non-error double identity case).
    The disjoint probability arguments are (in order):

    1. pix: Probability of applying an IX operation.
    2. piy: Probability of applying an IY operation.
    3. piz: Probability of applying an IZ operation.
    4. pxi: Probability of applying an XI operation.
    5. pxx: Probability of applying an XX operation.
    6. pxy: Probability of applying an XY operation.
    7. pxz: Probability of applying an XZ operation.
    8. pyi: Probability of applying a YI operation.
    9. pyx: Probability of applying a YX operation.
    10. pyy: Probability of applying a YY operation.
    11. pyz: Probability of applying a YZ operation.
    12. pzi: Probability of applying a ZI operation.
    13. pzx: Probability of applying a ZX operation.
    14. pzy: Probability of applying a ZY operation.
    15. pzz: Probability of applying a ZZ operation.

Targets:

    Pairs of qubits to apply the custom noise channel to.
    There must be an even number of targets.

Example:

    # Sample errors from the distribution 10% XX, 20% YZ, 70% II.
    # Apply independently to qubit pairs (1,2), (5,6), and (8,3)
    PAULI_CHANNEL_2(0,0,0, 0.1,0,0,0, 0,0,0,0.2, 0,0,0,0) 1 2 5 6 8 3

Pauli Mixture:

    1-pix-piy-piz-pxi-pxx-pxy-pxz-pyi-pyx-pyy-pyz-pzi-pzx-pzy-pzz: II
    pix: IX
    piy: IY
    piz: IZ
    pxi: XI
    pxx: XX
    pxy: XY
    pxz: XZ
    pyi: YI
    pyx: YX
    pyy: YY
    pyz: YZ
    pzi: ZI
    pzx: ZX
    pzy: ZY
    pzz: ZZ
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });

    add_gate(
        failed,
        Gate{
            .name = "E",
            .id = GateType::E,
            .best_candidate_inverse_id = GateType::E,
            .arg_count = 1,
            .flags = (GateFlags)(GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_TARGETS_PAULI_STRING |
                                 GATE_IS_NOT_FUSABLE),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
Probabilistically applies a Pauli product error with a given probability.
Sets the "correlated error occurred flag" to true if the error occurred.
Otherwise sets the flag to false.

See also: `ELSE_CORRELATED_ERROR`.

Parens Arguments:

    A single float specifying the probability of applying the Paulis making up the error.

Targets:

    Pauli targets specifying the Paulis to apply when the error occurs.
    Note that, for backwards compatibility reasons, the targets are not combined using combiners (`*`).
    They are implicitly all combined.

Example:

    # With 60% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
    CORRELATED_ERROR(0.2) X1 Y2
    ELSE_CORRELATED_ERROR(0.25) Z2 Z3
    ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });
    add_gate_alias(failed, "CORRELATED_ERROR", "E");
    add_gate(
        failed,
        Gate{
            .name = "ELSE_CORRELATED_ERROR",
            .id = GateType::ELSE_CORRELATED_ERROR,
            .best_candidate_inverse_id = GateType::ELSE_CORRELATED_ERROR,
            .arg_count = 1,
            .flags = (GateFlags)(GATE_IS_NOISY | GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_TARGETS_PAULI_STRING |
                                 GATE_IS_NOT_FUSABLE),
            .category = "F_Noise Channels",
            .help = R"MARKDOWN(
Probabilistically applies a Pauli product error with a given probability, unless the "correlated error occurred flag" is set.
If the error occurs, sets the "correlated error occurred flag" to true.
Otherwise leaves the flag alone.

Note: when converting a circuit into a detector error model, every `ELSE_CORRELATED_ERROR` instruction must be preceded by
an ELSE_CORRELATED_ERROR instruction or an E instruction. In other words, ELSE_CORRELATED_ERROR instructions should appear
in contiguous chunks started by a CORRELATED_ERROR.

See also: `CORRELATED_ERROR`.

Parens Arguments:

    A single float specifying the probability of applying the Paulis making up the error, conditioned on the "correlated
    error occurred flag" being False.

Targets:

    Pauli targets specifying the Paulis to apply when the error occurs.
    Note that, for backwards compatibility reasons, the targets are not combined using combiners (`*`).
    They are implicitly all combined.

Example:

    # With 60% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
    CORRELATED_ERROR(0.2) X1 Y2
    ELSE_CORRELATED_ERROR(0.25) Z2 Z3
    ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3
)MARKDOWN",
            .unitary_data = {},
            .flow_data = {},
            .h_s_cx_m_r_decomposition = nullptr,
        });
}
