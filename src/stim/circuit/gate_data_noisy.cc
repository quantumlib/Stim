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

#include "stim/circuit/gate_data.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

void GateDataMap::add_gate_data_noisy(bool &failed) {
    add_gate(
        failed,
        Gate{
            "DEPOLARIZE1",
            1,
            &TableauSimulator::DEPOLARIZE1,
            &FrameSimulator::DEPOLARIZE1,
            &ErrorAnalyzer::DEPOLARIZE1,
            (GateFlags)(GATE_IS_NOISE | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
The single qubit depolarizing channel.

Applies a randomly chosen Pauli with a given probability.

- Parens Arguments:

    A single float specifying the depolarization strength.

- Targets:

    Qubits to apply single-qubit depolarizing noise to.

- Pauli Mixture:

    ```
    1-p: I
    p/3: X
    p/3: Y
    p/3: Z
    ```
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "DEPOLARIZE2",
            1,
            &TableauSimulator::DEPOLARIZE2,
            &FrameSimulator::DEPOLARIZE2,
            &ErrorAnalyzer::DEPOLARIZE2,
            (GateFlags)(GATE_IS_NOISE | GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
The two qubit depolarizing channel.

Applies a randomly chosen two-qubit Pauli product with a given probability.

- Parens Arguments:

    A single float specifying the depolarization strength.

- Targets:

    Qubit pairs to apply two-qubit depolarizing noise to.

- Pauli Mixture:

    ```
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
    ```
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "X_ERROR",
            1,
            &TableauSimulator::X_ERROR,
            &FrameSimulator::X_ERROR,
            &ErrorAnalyzer::X_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
Applies a Pauli X with a given probability.

- Parens Arguments:

    A single float specifying the probability of applying an X operation.

- Targets:

    Qubits to apply bit flip noise to.

- Pauli Mixture:

    ```
    1-p: I
     p : X
    ```
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "Y_ERROR",
            1,
            &TableauSimulator::Y_ERROR,
            &FrameSimulator::Y_ERROR,
            &ErrorAnalyzer::Y_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
Applies a Pauli Y with a given probability.

- Parens Arguments:

    A single float specifying the probability of applying a Y operation.

- Targets:

    Qubits to apply Y flip noise to.

- Pauli Mixture:

    ```
    1-p: I
     p : Y
    ```
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "Z_ERROR",
            1,
            &TableauSimulator::Z_ERROR,
            &FrameSimulator::Z_ERROR,
            &ErrorAnalyzer::Z_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
Applies a Pauli Z with a given probability.

- Parens Arguments:

    A single float specifying the probability of applying a Z operation.

- Targets:

    Qubits to apply phase flip noise to.

- Pauli Mixture:

    ```
    1-p: I
     p : Z
    ```
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "PAULI_CHANNEL_1",
            3,
            &TableauSimulator::PAULI_CHANNEL_1,
            &FrameSimulator::PAULI_CHANNEL_1,
            &ErrorAnalyzer::PAULI_CHANNEL_1,
            (GateFlags)(GATE_IS_NOISE | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
A single qubit Pauli error channel with explicitly specified probabilities for each case.

- Parens Arguments:

    Three floats specifying disjoint Pauli case probabilities.
    px: Probability of applying an X operation.
    py: Probability of applying a Y operation.
    pz: Probability of applying a Z operation.

- Targets:

    Qubits to apply the custom noise channel to.

- Example:

    ```
    # Sample errors from the distribution 10% X, 15% Y, 20% Z, 55% I.
    # Apply independently to qubits 1, 2, 4.
    PAULI_CHANNEL_1(0.1, 0.15, 0.2) 1 2 4
    ```

- Pauli Mixture:

    ```
    1-px-py-pz: I
    px: X
    py: Y
    pz: Z
    ```
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "PAULI_CHANNEL_2",
            15,
            &TableauSimulator::PAULI_CHANNEL_2,
            &FrameSimulator::PAULI_CHANNEL_2,
            &ErrorAnalyzer::PAULI_CHANNEL_2,
            (GateFlags)(GATE_IS_NOISE | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
A two qubit Pauli error channel with explicitly specified probabilities for each case.

- Parens Arguments:

    Fifteen floats specifying disjoint non-identity Pauli pair case probabilities.
    pix: Probability of applying an IX operation.
    piy: Probability of applying an IY operation.
    piz: Probability of applying an IZ operation.
    pxi: Probability of applying an XI operation.
    pxx: Probability of applying an XX operation.
    pxy: Probability of applying an XY operation.
    pxz: Probability of applying an XZ operation.
    pyi: Probability of applying a YI operation.
    pyx: Probability of applying a YX operation.
    pyy: Probability of applying a YY operation.
    pyz: Probability of applying a YZ operation.
    pzi: Probability of applying a ZI operation.
    pzx: Probability of applying a ZX operation.
    pzy: Probability of applying a ZY operation.
    pzz: Probability of applying a ZZ operation.

- Targets:

    Qubit pairs to apply the custom noise channel to.

- Example:

    ```
    # Sample errors from the distribution 10% XX, 20% YZ, 70% II.
    # Apply independently to qubit pairs (1,2) (5,6) (8,3)
    PAULI_CHANNEL_2(0,0,0, 0.1,0,0,0, 0,0,0,0.2, 0,0,0,0) 1 2 5 6 8 3
    ```

- Pauli Mixture:

    ```
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
    ```
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "E",
            1,
            &TableauSimulator::CORRELATED_ERROR,
            &FrameSimulator::CORRELATED_ERROR,
            &ErrorAnalyzer::CORRELATED_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_TARGETS_PAULI_STRING | GATE_IS_NOT_FUSABLE),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
Probabilistically applies a Pauli product error with a given probability.
Sets the "correlated error occurred flag" to true if the error occurred.
Otherwise sets the flag to false.

See also: `ELSE_CORRELATED_ERROR`.

- Parens Arguments:

    A single float specifying the probability of applying the Paulis making up the error.

- Targets:

    Pauli targets specifying the Paulis to apply when the error occurs.
    Note that, for backwards compatibility reasons, the targets are not combined using combiners (`*`).
    They are implicitly all combined.

- Example:

    ```
    # With 60% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
    CORRELATED_ERROR(0.2) X1 Y2
    ELSE_CORRELATED_ERROR(0.25) Z2 Z3
    ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3
    ```
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });
    add_gate_alias(failed, "CORRELATED_ERROR", "E");
    add_gate(
        failed,
        Gate{
            "ELSE_CORRELATED_ERROR",
            1,
            &TableauSimulator::ELSE_CORRELATED_ERROR,
            &FrameSimulator::ELSE_CORRELATED_ERROR,
            &ErrorAnalyzer::ELSE_CORRELATED_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_TARGETS_PAULI_STRING | GATE_IS_NOT_FUSABLE),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
Probabilistically applies a Pauli product error with a given probability, unless the "correlated error occurred flag" is set.
If the error occurs, sets the "correlated error occurred flag" to true.
Otherwise leaves the flag alone.

See also: `CORRELATED_ERROR`.

- Parens Arguments:

    A single float specifying the probability of applying the Paulis making up the error, conditioned on the "correlated
    error occurred flag" being False.

- Targets:

    Pauli targets specifying the Paulis to apply when the error occurs.
    Note that, for backwards compatibility reasons, the targets are not combined using combiners (`*`).
    They are implicitly all combined.

- Example:

    ```
    # With 60% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
    CORRELATED_ERROR(0.2) X1 Y2
    ELSE_CORRELATED_ERROR(0.25) Z2 Z3
    ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3
    ```
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });
}
