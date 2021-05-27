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

#include "gate_data.h"

#include <complex>

#include "../simulators/error_fuser.h"
#include "../simulators/frame_simulator.h"
#include "../simulators/tableau_simulator.h"

using namespace stim_internal;

static constexpr std::complex<float> i = std::complex<float>(0, 1);
static constexpr std::complex<float> s = 0.7071067811865475244f;

extern const GateDataMap stim_internal::GATE_DATA(
    {
        // Collapsing gates.
        {
            "MX",
            &TableauSimulator::measure_x,
            &FrameSimulator::measure_x,
            &ErrorFuser::MX,
            GATE_PRODUCES_RESULTS,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
X-basis measurement.
Projects each target qubit into `|+>` or `|->` and reports its value (false=`|+>`, true=`|->`).
)MARKDOWN",
                    {},
                    {"X -> mX"},
                };
            },
        },
        {
            "MY",
            &TableauSimulator::measure_y,
            &FrameSimulator::measure_y,
            &ErrorFuser::MY,
            GATE_PRODUCES_RESULTS,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Y-basis measurement.
Projects each target qubit into `|i>` or `|-i>` and reports its value (false=`|i>`, true=`|-i>`).
)MARKDOWN",
                    {},
                    {"Y -> mY"},
                };
            },
        },
        {
            "M",
            &TableauSimulator::measure_z,
            &FrameSimulator::measure_z,
            &ErrorFuser::MZ,
            GATE_PRODUCES_RESULTS,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Z-basis measurement.
Projects each target qubit into `|0>` or `|1>` and reports its value (false=`|0>`, true=`|1>`).
)MARKDOWN",
                    {},
                    {"Z -> mZ"},
                };
            },
        },
        {
            "MRX",
            &TableauSimulator::measure_reset_x,
            &FrameSimulator::measure_reset_x,
            &ErrorFuser::MRX,
            GATE_PRODUCES_RESULTS,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
X-basis demolition measurement.
Projects each target qubit into `|+>` or `|->`, reports its value (false=`|+>`, true=`|->`), then resets to `|+>`.
)MARKDOWN",
                    {},
                    {"X -> m", "1 -> +X"},
                };
            },
        },
        {
            "MRY",
            &TableauSimulator::measure_reset_y,
            &FrameSimulator::measure_reset_y,
            &ErrorFuser::MRY,
            GATE_PRODUCES_RESULTS,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Y-basis demolition measurement.
Projects each target qubit into `|i>` or `|-i>`, reports its value (false=`|i>`, true=`|-i>`), then resets to `|i>`.
)MARKDOWN",
                    {},
                    {"Y -> m", "1 -> +Y"},
                };
            },
        },
        {
            "MR",
            &TableauSimulator::measure_reset_z,
            &FrameSimulator::measure_reset_z,
            &ErrorFuser::MRZ,
            GATE_PRODUCES_RESULTS,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Z-basis demolition measurement.
Projects each target qubit into `|0>` or `|1>`, reports its value (false=`|0>`, true=`|1>`), then resets to `|0>`.
)MARKDOWN",
                    {},
                    {"Z -> m", "1 -> +Z"},
                };
            },
        },
        {
            "RX",
            &TableauSimulator::reset_x,
            &FrameSimulator::reset_x,
            &ErrorFuser::RX,
            GATE_NO_FLAGS,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
X-basis reset.
Forces each target qubit into the `|+>` state by silently measuring it in the X basis and applying a `Z` gate if it ended up in the `|->` state.
)MARKDOWN",
                    {},
                    {"1 -> +X"},
                };
            },
        },
        {
            "RY",
            &TableauSimulator::reset_y,
            &FrameSimulator::reset_y,
            &ErrorFuser::RY,
            GATE_NO_FLAGS,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Y-basis reset.
Forces each target qubit into the `|i>` state by silently measuring it in the Y basis and applying an `X` gate if it ended up in the `|-i>` state.
)MARKDOWN",
                    {},
                    {"1 -> +Y"},
                };
            },
        },
        {
            "R",
            &TableauSimulator::reset_z,
            &FrameSimulator::reset_z,
            &ErrorFuser::RZ,
            GATE_NO_FLAGS,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Z-basis reset.
Forces each target qubit into the `|0>` state by silently measuring it in the Z basis and applying an `X` gate if it ended up in the `|1>` state.
)MARKDOWN",
                    {},
                    {"1 -> +Z"},
                };
            },
        },

        // Pauli gates.
        {
            "I",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "A_Pauli Gates",
                    R"MARKDOWN(
Identity gate.
Does nothing to the target qubits.
)MARKDOWN",
                    {{1, 0}, {0, 1}},
                    {"+X", "+Z"},
                };
            },
        },
        {
            "X",
            &TableauSimulator::X,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "A_Pauli Gates",
                    R"MARKDOWN(
Pauli X gate.
The bit flip gate.
)MARKDOWN",
                    {{0, 1}, {1, 0}},
                    {"+X", "-Z"},
                };
            },
        },
        {
            "Y",
            &TableauSimulator::Y,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "A_Pauli Gates",
                    R"MARKDOWN(
Pauli Y gate.
)MARKDOWN",
                    {{0, -i}, {i, 0}},
                    {"-X", "-Z"},
                };
            },
        },
        {
            "Z",
            &TableauSimulator::Z,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "A_Pauli Gates",
                    R"MARKDOWN(
Pauli Z gate.
The phase flip gate.
)MARKDOWN",
                    {{1, 0}, {0, -1}},
                    {"-X", "+Z"},
                };
            },
        },

        // Axis exchange gates.
        {
            "H_XY",
            &TableauSimulator::H_XY,
            &FrameSimulator::H_XY,
            &ErrorFuser::H_XY,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
A variant of the Hadamard gate that swaps the X and Y axes (instead of X and Z).
A 180 degree rotation around the X+Y axis.
)MARKDOWN",
                    {{0, s - i *s}, {s + i * s, 0}},
                    {"+Y", "-Z"},
                };
            },
        },
        {
            "H",
            &TableauSimulator::H_XZ,
            &FrameSimulator::H_XZ,
            &ErrorFuser::H_XZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
The Hadamard gate.
Swaps the X and Z axes.
A 180 degree rotation around the X+Z axis.
)MARKDOWN",
                    {{s, s}, {s, -s}},
                    {"+Z", "+X"},
                };
            },
        },
        {
            "H_YZ",
            &TableauSimulator::H_YZ,
            &FrameSimulator::H_YZ,
            &ErrorFuser::H_YZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
A variant of the Hadamard gate that swaps the Y and Z axes (instead of X and Z).
A 180 degree rotation around the Y+Z axis.
)MARKDOWN",
                    {{s, -i * s}, {i * s, -s}},
                    {"-X", "+Y"},
                };
            },
        },

        // Period 3 gates.
        {
            "C_XYZ",
            &TableauSimulator::C_XYZ,
            &FrameSimulator::C_XYZ,
            &ErrorFuser::C_XYZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Right handed period 3 axis cycling gate, sending X -> Y -> Z -> X.
)MARKDOWN",
                    {{0.5f - i * 0.5f, -0.5f - 0.5f*i}, {0.5f - 0.5f * i, 0.5f + 0.5f * i}},
                    {"Y", "X"},
                };
            },
        },
        {
            "C_ZYX",
            &TableauSimulator::C_ZYX,
            &FrameSimulator::C_ZYX,
            &ErrorFuser::C_ZYX,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Left handed period 3 axis cycling gate, sending Z -> Y -> X -> Z.
)MARKDOWN",
                    {{0.5f + i * 0.5f, 0.5f + 0.5f*i}, {-0.5f + 0.5f * i, 0.5f - 0.5f * i}},
                    {"Z", "Y"},
                };
            },
        },

        // 90 degree rotation gates.
        {
            "SQRT_X",
            &TableauSimulator::SQRT_X,
            &FrameSimulator::H_YZ,
            &ErrorFuser::H_YZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principle square root of X gate.
Phases the amplitude of |-> by i.
Equivalent to `H` then `S` then `H`.
)MARKDOWN",
                    {{0.5f + 0.5f * i, 0.5f - 0.5f * i}, {0.5f - 0.5f * i, 0.5f + 0.5f * i}},
                    {"+X", "-Y"},
                };
            },
        },
        {
            "SQRT_X_DAG",
            &TableauSimulator::SQRT_X_DAG,
            &FrameSimulator::H_YZ,
            &ErrorFuser::H_YZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Adjoint square root of X gate.
Phases the amplitude of |-> by -i.
Equivalent to `H` then `S_DAG` then `H`.
)MARKDOWN",
                    {{0.5f - 0.5f * i, 0.5f + 0.5f * i}, {0.5f + 0.5f * i, 0.5f - 0.5f * i}},
                    {"+X", "+Y"},
                };
            },
        },
        {
            "SQRT_Y",
            &TableauSimulator::SQRT_Y,
            &FrameSimulator::H_XZ,
            &ErrorFuser::H_XZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principle square root of Y gate.
Phases the amplitude of |-i> by i.
Equivalent to `S` then `H` then `S` then `H` then `S_DAG`.
)MARKDOWN",
                    {{0.5f + 0.5f * i, -0.5f - 0.5f * i}, {0.5f + 0.5f * i, 0.5f + 0.5f * i}},
                    {"-Z", "+X"},
                };
            },
        },
        {
            "SQRT_Y_DAG",
            &TableauSimulator::SQRT_Y_DAG,
            &FrameSimulator::H_XZ,
            &ErrorFuser::H_XZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principle square root of Y gate.
Phases the amplitude of |-i> by -i.
Equivalent to `S` then `H` then `S_DAG` then `H` then `S_DAG`.
)MARKDOWN",
                    {{0.5f - 0.5f * i, 0.5f - 0.5f * i}, {-0.5f + 0.5f * i, 0.5f - 0.5f * i}},
                    {"+Z", "-X"},
                };
            },
        },
        {
            "S",
            &TableauSimulator::SQRT_Z,
            &FrameSimulator::H_XY,
            &ErrorFuser::H_XY,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principle square root of Z gate.
Phases the amplitude of |1> by i.
)MARKDOWN",
                    {{1, 0}, {0, i}},
                    {"+Y", "+Z"},
                };
            },
        },
        {
            "S_DAG",
            &TableauSimulator::SQRT_Z_DAG,
            &FrameSimulator::H_XY,
            &ErrorFuser::H_XY,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principle square root of Z gate.
Phases the amplitude of |1> by -i.
)MARKDOWN",
                    {{1, 0}, {0, -i}},
                    {"-Y", "+Z"},
                };
            },
        },

        // Swap gates.
        {
            "SWAP",
            &TableauSimulator::SWAP,
            &FrameSimulator::SWAP,
            &ErrorFuser::SWAP,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Swaps two qubits.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}},
                    {"+IX", "+IZ", "+XI", "+ZI"},
                };
            },
        },
        {
            "ISWAP",
            &TableauSimulator::ISWAP,
            &FrameSimulator::ISWAP,
            &ErrorFuser::ISWAP,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Swaps two qubits and phases the -1 eigenspace of the ZZ observable by i.
Equivalent to `SWAP` then `CZ` then `S` on both targets.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 0, i, 0}, {0, i, 0, 0}, {0, 0, 0, 1}},
                    {"+ZY", "+IZ", "+YZ", "+ZI"},
                };
            },
        },
        {
            "ISWAP_DAG",
            &TableauSimulator::ISWAP_DAG,
            &FrameSimulator::ISWAP,
            &ErrorFuser::ISWAP,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Swaps two qubits and phases the -1 eigenspace of the ZZ observable by -i.
Equivalent to `SWAP` then `CZ` then `S_DAG` on both targets.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 0, -i, 0}, {0, -i, 0, 0}, {0, 0, 0, 1}},
                    {"-ZY", "+IZ", "-YZ", "+ZI"},
                };
            },
        },

        // Axis interaction gates.
        {
            "XCX",
            &TableauSimulator::XCX,
            &FrameSimulator::XCX,
            &ErrorFuser::XCX,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
The X-controlled X gate.
First qubit is the control, second qubit is the target.

Applies an X gate to the target if the control is in the |-> state.

Negates the amplitude of the |->|-> state.
)MARKDOWN",
                    {{0.5f, 0.5f, 0.5f, -0.5f},
                     {0.5f, 0.5f, -0.5f, 0.5f},
                     {0.5f, -0.5f, 0.5f, 0.5f},
                     {-0.5f, 0.5f, 0.5f, 0.5f}},
                    {"+XI", "+ZX", "+IX", "+XZ"},
                };
            },
        },
        {
            "XCY",
            &TableauSimulator::XCY,
            &FrameSimulator::XCY,
            &ErrorFuser::XCY,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
The X-controlled Y gate.
First qubit is the control, second qubit is the target.

Applies a Y gate to the target if the control is in the |-> state.

Negates the amplitude of the |->|-i> state.
)MARKDOWN",
                    {{0.5f, 0.5f, -0.5f * i, 0.5f * i},
                     {0.5f, 0.5f, 0.5f * i, -0.5f * i},
                     {0.5f * i, -0.5f * i, 0.5f, 0.5f},
                     {-0.5f * i, 0.5f * i, 0.5f, 0.5f}},
                    {"+XI", "+ZY", "+XX", "+XZ"},
                };
            },
        },
        {
            "XCZ",
            &TableauSimulator::XCZ,
            &FrameSimulator::XCZ,
            &ErrorFuser::XCZ,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_MEASUREMENT_RECORD),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
The X-controlled Z gate.
First qubit is the control, second qubit is the target.
The second qubit can be replaced by a measurement record.

Applies a Z gate to the target if the control is in the |-> state.

Negates the amplitude of the |->|1> state.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}},
                    {"+XI", "+ZZ", "+XX", "+IZ"},
                };
            },
        },
        {
            "YCX",
            &TableauSimulator::YCX,
            &FrameSimulator::YCX,
            &ErrorFuser::YCX,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
The Y-controlled X gate.
First qubit is the control, second qubit is the target.

Applies an X gate to the target if the control is in the |-i> state.

Negates the amplitude of the |-i>|-> state.
)MARKDOWN",
                    {{0.5f, -i * 0.5f, 0.5f, i * 0.5f},
                     {i * 0.5f, 0.5f, -i * 0.5f, 0.5f},
                     {0.5f, i * 0.5f, 0.5f, -i * 0.5f},
                     {-i * 0.5f, 0.5f, i * 0.5f, 0.5f}},
                    {"+XX", "+ZX", "+IX", "+YZ"},
                };
            },
        },
        {
            "YCY",
            &TableauSimulator::YCY,
            &FrameSimulator::YCY,
            &ErrorFuser::YCY,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
The Y-controlled Y gate.
First qubit is the control, second qubit is the target.

Applies a Y gate to the target if the control is in the |-i> state.

Negates the amplitude of the |-i>|-i> state.
)MARKDOWN",
                    {{0.5f, -i * 0.5f, -i * 0.5f, 0.5f},
                     {i * 0.5f, 0.5f, -0.5f, -i * 0.5f},
                     {i * 0.5f, -0.5f, 0.5f, -i * 0.5f},
                     {0.5f, i * 0.5f, i * 0.5f, 0.5f}},
                    {"+XY", "+ZY", "+YX", "+YZ"},
                };
            },
        },
        {
            "YCZ",
            &TableauSimulator::YCZ,
            &FrameSimulator::YCZ,
            &ErrorFuser::YCZ,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_MEASUREMENT_RECORD),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
The Y-controlled Z gate.
First qubit is the control, second qubit is the target.
The second qubit can be replaced by a measurement record.

Applies a Z gate to the target if the control is in the |-i> state.

Negates the amplitude of the |-i>|1> state.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, -i}, {0, 0, i, 0}},
                    {"+XZ", "+ZZ", "+YX", "+IZ"},
                };
            },
        },
        {
            "CX",
            &TableauSimulator::ZCX,
            &FrameSimulator::ZCX,
            &ErrorFuser::ZCX,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_MEASUREMENT_RECORD),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
The Z-controlled X gate.
First qubit is the control, second qubit is the target.
The first qubit can be replaced by a measurement record.

Applies an X gate to the target if the control is in the |1> state.

Negates the amplitude of the |1>|-> state.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 1, 0, 0}},
                    {"+XX", "+ZI", "+IX", "+ZZ"},
                };
            },
        },
        {
            "CY",
            &TableauSimulator::ZCY,
            &FrameSimulator::ZCY,
            &ErrorFuser::ZCY,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_MEASUREMENT_RECORD),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
The Z-controlled Y gate.
First qubit is the control, second qubit is the target.
The first qubit can be replaced by a measurement record.

Applies a Y gate to the target if the control is in the |1> state.

Negates the amplitude of the |1>|-i> state.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 0, 0, -i}, {0, 0, 1, 0}, {0, i, 0, 0}},
                    {"+XY", "+ZI", "+ZX", "+ZZ"},
                };
            },
        },
        {
            "CZ",
            &TableauSimulator::ZCZ,
            &FrameSimulator::ZCZ,
            &ErrorFuser::ZCZ,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS | GATE_CAN_TARGET_MEASUREMENT_RECORD),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
The Z-controlled Z gate.
First qubit is the control, second qubit is the target.
Either qubit can be replaced by a measurement record.

Applies a Z gate to the target if the control is in the |1> state.

Negates the amplitude of the |1>|1> state.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, -1}},
                    {"+XZ", "+ZI", "+ZX", "+IZ"},
                };
            },
        },

        // Noise gates.
        {
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
        },
        {
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
        },
        {
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
        },
        {
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
        },
        {
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
        },

        // Annotation gates.
        {
            "DETECTOR",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::DETECTOR,
            (GateFlags)(GATE_ONLY_TARGETS_MEASUREMENT_RECORD | GATE_IS_NOT_FUSABLE),
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Annotates that a set of measurements have a deterministic result, which can be used to detect errors.

Detectors are ignored in measurement sampling mode.
In detector sampling mode, detectors produce results (false=expected parity, true=incorrect parity detected).

- Example:

    ```
    H 0
    CNOT 0 1
    M 0 1
    DETECTOR rec[-1] rec[-2]
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        },
        {
            "OBSERVABLE_INCLUDE",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::OBSERVABLE_INCLUDE,
            (GateFlags)(GATE_ONLY_TARGETS_MEASUREMENT_RECORD | GATE_TAKES_PARENS_ARGUMENT | GATE_IS_NOT_FUSABLE),
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Adds measurement results to a given logical observable index.

A logical observable's measurement result is the parity of all physical measurement results added to it.

A logical observable is similar to a Detector, except the measurements making up an observable can be built up
incrementally over the entire circuit.

Logical observables are ignored in measurement sampling mode.
In detector sampling mode, observables produce results (false=expected parity, true=incorrect parity detected).
These results are optionally appended to the detector results, depending on simulator arguments / command line flags.

- Example:

    ```
    H 0
    CNOT 0 1
    M 0 1
    OBSERVABLE_INCLUDE(5) rec[-1] rec[-2]
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        },
        {
            "TICK",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_NOT_FUSABLE,
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Indicates the end of a layer of gates, or that time is advancing.
For example, used by `stimcirq` to preserve the moment structure of cirq circuits converted to/from stim circuits.

- Example:

    ```
    TICK
    TICK
    # Oh, and of course:
    TICK
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        },
        {
            "REPEAT",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::I,
            (GateFlags)(GATE_IS_BLOCK | GATE_IS_NOT_FUSABLE),
            []() -> ExtraGateData {
                return {
                    "Y_Control Flow",
                    R"MARKDOWN(
Repeats the instructions in its body N times.
The implementation-defined maximum value of N is 9223372036854775807.

- Example:

    ```
    REPEAT 2 {
        CNOT 0 1
        CNOT 2 1
        M 1
    }
    REPEAT 10000000 {
        CNOT 0 1
        CNOT 2 1
        M 1
        DETECTOR rec[-1] rec[-3]
    }
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        },
        {
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
        },
        {
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
        },
    },
    {
        {"H_XZ", "H"},
        {"CORRELATED_ERROR", "E"},
        {"SQRT_Z", "S"},
        {"SQRT_Z_DAG", "S_DAG"},
        {"ZCZ", "CZ"},
        {"ZCY", "CY"},
        {"ZCX", "CX"},
        {"CNOT", "CX"},
        {"MZ", "M"},
        {"RZ", "R"},
        {"MRZ", "MR"},
    });

Tableau Gate::tableau() const {
    const auto &tableau_data = extra_data_func().tableau_data;
    const auto &d = tableau_data;
    if (tableau_data.size() == 2) {
        return Tableau::gate1(d[0], d[1]);
    }
    if (tableau_data.size() == 4) {
        return Tableau::gate2(d[0], d[1], d[2], d[3]);
    }
    throw std::out_of_range(std::string(name) + " doesn't have 1q or 2q tableau data.");
}

std::vector<std::vector<std::complex<float>>> Gate::unitary() const {
    const auto &unitary_data = extra_data_func().unitary_data;
    if (unitary_data.size() != 2 && unitary_data.size() != 4) {
        throw std::out_of_range(std::string(name) + " doesn't have 1q or 2q unitary data.");
    }
    std::vector<std::vector<std::complex<float>>> result;
    for (size_t k = 0; k < unitary_data.size(); k++) {
        const auto &d = unitary_data[k];
        result.emplace_back();
        for (size_t j = 0; j < d.size(); j++) {
            result.back().push_back(d[j]);
        }
    }
    return result;
}

const Gate &Gate::inverse() const {
    std::string inv_name = name;
    if (!(flags & GATE_IS_UNITARY)) {
        throw std::out_of_range(inv_name + " has no inverse.");
    }

    if (GATE_DATA.has(inv_name + "_DAG")) {
        inv_name += "_DAG";
    } else if (inv_name.size() > 4 && inv_name.substr(inv_name.size() - 4) == "_DAG") {
        inv_name = inv_name.substr(0, inv_name.size() - 4);
    } else if (id == gate_name_to_id("C_XYZ")) {
        inv_name = "C_ZYX";
    } else if (id == gate_name_to_id("C_ZYX")) {
        inv_name = "C_XYZ";
    } else {
        // Self inverse.
    }
    return GATE_DATA.at(inv_name);
}

Gate::Gate() : name(nullptr) {
}

Gate::Gate(
    const char *name, void (TableauSimulator::*tableau_simulator_function)(const OperationData &),
    void (FrameSimulator::*frame_simulator_function)(const OperationData &),
    void (ErrorFuser::*hit_simulator_function)(const OperationData &), GateFlags flags,
    ExtraGateData (*extra_data_func)(void))
    : name(name),
      name_len(strlen(name)),
      tableau_simulator_function(tableau_simulator_function),
      frame_simulator_function(frame_simulator_function),
      reverse_error_fuser_function(hit_simulator_function),
      flags(flags),
      extra_data_func(extra_data_func),
      id(gate_name_to_id(name)) {
}

GateDataMap::GateDataMap(
    std::initializer_list<Gate> gates, std::initializer_list<std::pair<const char *, const char *>> alternate_names) {

    bool collision = false;
    for (const auto &gate : gates) {
        const char *c = gate.name;
        uint8_t h = gate_name_to_id(c);
        Gate &g = items[h];
        if (g.name != nullptr) {
            std::cerr << "GATE COLLISION " << gate.name << " vs " << g.name << "\n";
            collision = true;
        }
        g = gate;
    }
    for (const auto &alt : alternate_names) {
        const auto *alt_name = alt.first;
        const auto *canon_name = alt.second;
        uint8_t h_alt = gate_name_to_id(alt_name);
        Gate &g_alt = items[h_alt];
        if (g_alt.name != nullptr) {
            std::cerr << "GATE COLLISION " << alt_name << " vs " << g_alt.name << "\n";
            collision = true;
        }

        uint8_t h_canon = gate_name_to_id(canon_name);
        Gate &g_canon = items[h_canon];
        assert(g_canon.name != nullptr && g_canon.id == h_canon);
        g_alt.name = alt_name;
        g_alt.name_len = strlen(alt_name);
        g_alt.id = h_canon;
    }
    if (collision) {
        exit(EXIT_FAILURE);
    }
}

std::vector<Gate> GateDataMap::gates() const {
    std::vector<Gate> result;
    for (const auto &item : items) {
        if (item.name != nullptr) {
            result.push_back(item);
        }
    }
    return result;
}
