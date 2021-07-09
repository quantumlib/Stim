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

#include "../simulators/error_analyzer.h"
#include "../simulators/frame_simulator.h"
#include "../simulators/tableau_simulator.h"
#include "gate_data.h"

using namespace stim_internal;

static constexpr std::complex<float> i = std::complex<float>(0, 1);

void GateDataMap::add_gate_data_controlled(bool &failed) {
    add_gate(
        failed,
        Gate{
            "XCX",
            0,
            &TableauSimulator::XCX,
            &FrameSimulator::XCX,
            &ErrorAnalyzer::XCX,
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
        });

    add_gate(
        failed,
        Gate{
            "XCY",
            0,
            &TableauSimulator::XCY,
            &FrameSimulator::XCY,
            &ErrorAnalyzer::XCY,
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
        });

    add_gate(
        failed,
        Gate{
            "XCZ",
            0,
            &TableauSimulator::XCZ,
            &FrameSimulator::XCZ,
            &ErrorAnalyzer::XCZ,
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
        });

    add_gate(
        failed,
        Gate{
            "YCX",
            0,
            &TableauSimulator::YCX,
            &FrameSimulator::YCX,
            &ErrorAnalyzer::YCX,
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
        });

    add_gate(
        failed,
        Gate{
            "YCY",
            0,
            &TableauSimulator::YCY,
            &FrameSimulator::YCY,
            &ErrorAnalyzer::YCY,
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
        });

    add_gate(
        failed,
        Gate{
            "YCZ",
            0,
            &TableauSimulator::YCZ,
            &FrameSimulator::YCZ,
            &ErrorAnalyzer::YCZ,
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
        });

    add_gate(
        failed,
        Gate{
            "CX",
            0,
            &TableauSimulator::ZCX,
            &FrameSimulator::ZCX,
            &ErrorAnalyzer::ZCX,
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
        });
    add_gate_alias(failed, "ZCX", "CX");
    add_gate_alias(failed, "CNOT", "CX");

    add_gate(
        failed,
        Gate{
            "CY",
            0,
            &TableauSimulator::ZCY,
            &FrameSimulator::ZCY,
            &ErrorAnalyzer::ZCY,
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
        });
    add_gate_alias(failed, "ZCY", "CY");

    add_gate(
        failed,
        Gate{
            "CZ",
            0,
            &TableauSimulator::ZCZ,
            &FrameSimulator::ZCZ,
            &ErrorAnalyzer::ZCZ,
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
        });
    add_gate_alias(failed, "ZCZ", "CZ");
}
