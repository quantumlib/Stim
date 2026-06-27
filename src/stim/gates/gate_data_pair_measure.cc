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

void GateDataMap::add_gate_data_pair_measure(bool &failed) {
    add_gate(
        failed,
        Gate{
            .name = "MXX",
            .id = GateType::MXX,
            .best_candidate_inverse_id = GateType::MXX,
            .arg_count = ARG_COUNT_SYGIL_ZERO_OR_ONE,
            .flags = (GateFlags)(GATE_TARGETS_PAIRS | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                                 GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "L_Pair Measurement Gates",
            .help = R"MARKDOWN(
Two-qubit X basis parity measurement.

This operation measures whether pairs of qubits are in the {|++>,|-->} subspace or in the
{|+->,|-+>} subspace of the two qubit state space. |+> and |-> are the +1 and -1
eigenvectors of the X operator.

If the qubits were in the {|++>,|-->} subspace, False is appended to the measurement record.
If the qubits were in the {|+->,|-+>} subspace, True is appended to the measurement record.
Inverting one of the qubit targets inverts the result.

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The pairs of qubits to measure in the X basis.

    This operation accepts inverted qubit targets (like `!5` instead of `5`). Inverted
    targets flip the measurement result.

Examples:

    # Measure the +XX observable of qubit 1 vs qubit 2.
    MXX 1 2

    # Measure the -XX observable of qubit 1 vs qubit 2.
    MXX !1 2

    # Do a noisy measurement of the +XX observable of qubit 2 vs qubit 3.
    # The result recorded to the measurement record will be flipped 1% of the time.
    MXX(0.01) 2 3

    # Measure the +XX observable qubit 1 vs qubit 2, and also qubit 8 vs qubit 9
    MXX 1 2 8 9

    # Perform multiple noisy measurements.
    # Each measurement has an independent 2% chance of being recorded wrong.
    MXX(0.02) 2 3 5 7 11 19 17 4
)MARKDOWN",
            .unitary_data = {},
            .flow_data =
                {
                    "X_ -> +X_",
                    "_X -> +_X",
                    "ZZ -> +ZZ",
                    "XX -> rec[-1]",
                },
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
CX 0 1
H 0
M 0
H 0
CX 0 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "MYY",
            .id = GateType::MYY,
            .best_candidate_inverse_id = GateType::MYY,
            .arg_count = ARG_COUNT_SYGIL_ZERO_OR_ONE,
            .flags = (GateFlags)(GATE_TARGETS_PAIRS | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                                 GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "L_Pair Measurement Gates",
            .help = R"MARKDOWN(
Two-qubit Y basis parity measurement.

This operation measures whether pairs of qubits are in the {|ii>,|jj>} subspace or in the
{|ij>,|ji>} subspace of the two qubit state space. |i> and |j> are the +1 and -1
eigenvectors of the Y operator.

If the qubits were in the {|ii>,|jj>} subspace, False is appended to the measurement record.
If the qubits were in the {|ij>,|ji>} subspace, True is appended to the measurement record.
Inverting one of the qubit targets inverts the result.

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The pairs of qubits to measure in the Y basis.

    This operation accepts inverted qubit targets (like `!5` instead of `5`). Inverted
    targets flip the measurement result.

Examples:

    # Measure the +YY observable of qubit 1 vs qubit 2.
    MYY 1 2

    # Measure the -YY observable of qubit 1 vs qubit 2.
    MYY !1 2

    # Do a noisy measurement of the +YY observable of qubit 2 vs qubit 3.
    # The result recorded to the measurement record will be flipped 1% of the time.
    MYY(0.01) 2 3

    # Measure the +YY observable qubit 1 vs qubit 2, and also qubit 8 vs qubit 9
    MYY 1 2 8 9

    # Perform multiple noisy measurements.
    # Each measurement has an independent 2% chance of being recorded wrong.
    MYY(0.02) 2 3 5 7 11 19 17 4
)MARKDOWN",
            .unitary_data = {},
            .flow_data =
                {
                    "XX -> +XX",
                    "Y_ -> +Y_",
                    "_Y -> +_Y",
                    "YY -> rec[-1]",
                },
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
S 0 1
CX 0 1
H 0
M 0
S 1 1
H 0
CX 0 1
S 0 1
)CIRCUIT",
        });

    add_gate(
        failed,
        Gate{
            .name = "MZZ",
            .id = GateType::MZZ,
            .best_candidate_inverse_id = GateType::MZZ,
            .arg_count = ARG_COUNT_SYGIL_ZERO_OR_ONE,
            .flags = (GateFlags)(GATE_TARGETS_PAIRS | GATE_PRODUCES_RESULTS | GATE_IS_NOISY |
                                 GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            .category = "L_Pair Measurement Gates",
            .help = R"MARKDOWN(
Two-qubit Z basis parity measurement.

This operation measures whether pairs of qubits are in the {|00>,|11>} subspace or in the
{|01>,|10>} subspace of the two qubit state space. |0> and |1> are the +1 and -1
eigenvectors of the Z operator.

If the qubits were in the {|00>,|11>} subspace, False is appended to the measurement record.
If the qubits were in the {|01>,|10>} subspace, True is appended to the measurement record.
Inverting one of the qubit targets inverts the result.

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The pairs of qubits to measure in the Z basis.

    This operation accepts inverted qubit targets (like `!5` instead of `5`). Inverted
    targets flip the measurement result.

Examples:

    # Measure the +ZZ observable of qubit 1 vs qubit 2.
    MZZ 1 2

    # Measure the -ZZ observable of qubit 1 vs qubit 2.
    MZZ !1 2

    # Do a noisy measurement of the +ZZ observable of qubit 2 vs qubit 3.
    # The result recorded to the measurement record will be flipped 1% of the time.
    MZZ(0.01) 2 3

    # Measure the +ZZ observable qubit 1 vs qubit 2, and also qubit 8 vs qubit 9
    MZZ 1 2 8 9

    # Perform multiple noisy measurements.
    # Each measurement has an independent 2% chance of being recorded wrong.
    MZZ(0.02) 2 3 5 7 11 19 17 4
)MARKDOWN",
            .unitary_data = {},
            .flow_data =
                {
                    "XX -> XX",
                    "Z_ -> +Z_",
                    "_Z -> +_Z",
                    "ZZ -> rec[-1]",
                },
            .h_s_cx_m_r_decomposition = R"CIRCUIT(
CX 0 1
M 1
CX 0 1
)CIRCUIT",
        });
}
