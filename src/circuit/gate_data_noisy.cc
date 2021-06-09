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

#include <complex>

#include "../simulators/error_fuser.h"
#include "../simulators/frame_simulator.h"
#include "../simulators/tableau_simulator.h"
#include "gate_data.h"

using namespace stim_internal;

static constexpr std::complex<float> i = std::complex<float>(0, 1);
static constexpr std::complex<float> s = 0.7071067811865475244f;

void GateDataMap::add_gate_data_noisy(bool &failed) {
    add_gate(
        failed,
        Gate{
            "DEPOLARIZE1",
            &TableauSimulator::DEPOLARIZE1,
            &FrameSimulator::DEPOLARIZE1,
            &ErrorFuser::DEPOLARIZE1,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
The single qubit depolarizing channel.

Applies a randomly chosen Pauli with a given probability.

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
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "DEPOLARIZE2",
            &TableauSimulator::DEPOLARIZE2,
            &FrameSimulator::DEPOLARIZE2,
            &ErrorFuser::DEPOLARIZE2,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
The two qubit depolarizing channel.

Applies a randomly chosen two-qubit Pauli product with a given probability.

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
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "X_ERROR",
            &TableauSimulator::X_ERROR,
            &FrameSimulator::X_ERROR,
            &ErrorFuser::X_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
Applies a Pauli X with a given probability.

- Pauli Mixture:

    ```
    1-p: I
     p : X
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "Y_ERROR",
            &TableauSimulator::Y_ERROR,
            &FrameSimulator::Y_ERROR,
            &ErrorFuser::Y_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
Applies a Pauli Y with a given probability.

- Pauli Mixture:

    ```
    1-p: I
     p : Y
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "Z_ERROR",
            &TableauSimulator::Z_ERROR,
            &FrameSimulator::Z_ERROR,
            &ErrorFuser::Z_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
Applies a Pauli Z with a given probability.

- Pauli Mixture:

    ```
    1-p: I
     p : Z
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "E",
            &TableauSimulator::CORRELATED_ERROR,
            &FrameSimulator::CORRELATED_ERROR,
            &ErrorFuser::CORRELATED_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT | GATE_TARGETS_PAULI_STRING | GATE_IS_NOT_FUSABLE),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
Probabilistically applies a Pauli product error with a given probability.
Sets the "correlated error occurred flag" to true if the error occurred.
Otherwise sets the flag to false.

See also: `ELSE_CORRELATED_ERROR`.

- Example:

    ```
    # With 40% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
    CORRELATED_ERROR(0.2) X1 Y2
    ELSE_CORRELATED_ERROR(0.25) Z2 Z3
    ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        });
    add_gate_alias(failed, "CORRELATED_ERROR", "E");
    add_gate(
        failed,
        Gate{
            "ELSE_CORRELATED_ERROR",
            &TableauSimulator::ELSE_CORRELATED_ERROR,
            &FrameSimulator::ELSE_CORRELATED_ERROR,
            &ErrorFuser::ELSE_CORRELATED_ERROR,
            (GateFlags)(GATE_IS_NOISE | GATE_TAKES_PARENS_ARGUMENT | GATE_TARGETS_PAULI_STRING | GATE_IS_NOT_FUSABLE),
            []() -> ExtraGateData {
                return {
                    "F_Noise Channels",
                    R"MARKDOWN(
Probabilistically applies a Pauli product error with a given probability, unless the "correlated error occurred flag" is set.
If the error occurs, sets the "correlated error occurred flag" to true.
Otherwise leaves the flag alone.

See also: `CORRELATED_ERROR`.

- Example:

    ```
    # With 40% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
    CORRELATED_ERROR(0.2) X1 Y2
    ELSE_CORRELATED_ERROR(0.25) Z2 Z3
    ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        });
}
