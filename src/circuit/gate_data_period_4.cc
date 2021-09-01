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

void GateDataMap::add_gate_data_period_4(bool &failed) {
    add_gate(
        failed,
        Gate{
            "SQRT_X",
            0,
            &TableauSimulator::SQRT_X,
            &FrameSimulator::H_YZ,
            &ErrorAnalyzer::H_YZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principle square root of X gate.
Phases the amplitude of |-> by i.

- Parens Arguments:

    This instruction takes no parens arguments.

- Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{0.5f + 0.5f * i, 0.5f - 0.5f * i}, {0.5f - 0.5f * i, 0.5f + 0.5f * i}},
                    {"+X", "-Y"},
                    R"CIRCUIT(
H 0
S 0
H 0
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "SQRT_X_DAG",
            0,
            &TableauSimulator::SQRT_X_DAG,
            &FrameSimulator::H_YZ,
            &ErrorAnalyzer::H_YZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Adjoint square root of X gate.
Phases the amplitude of |-> by -i.

- Parens Arguments:

    This instruction takes no parens arguments.

- Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{0.5f - 0.5f * i, 0.5f + 0.5f * i}, {0.5f + 0.5f * i, 0.5f - 0.5f * i}},
                    {"+X", "+Y"},
                    R"CIRCUIT(
H 0
S 0
S 0
S 0
H 0
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "SQRT_Y",
            0,
            &TableauSimulator::SQRT_Y,
            &FrameSimulator::H_XZ,
            &ErrorAnalyzer::H_XZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principle square root of Y gate.
Phases the amplitude of |-i> by i.

- Parens Arguments:

    This instruction takes no parens arguments.

- Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{0.5f + 0.5f * i, -0.5f - 0.5f * i}, {0.5f + 0.5f * i, 0.5f + 0.5f * i}},
                    {"-Z", "+X"},
                    R"CIRCUIT(
S 0
S 0
H 0
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "SQRT_Y_DAG",
            0,
            &TableauSimulator::SQRT_Y_DAG,
            &FrameSimulator::H_XZ,
            &ErrorAnalyzer::H_XZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principle square root of Y gate.
Phases the amplitude of |-i> by -i.

- Parens Arguments:

    This instruction takes no parens arguments.

- Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{0.5f - 0.5f * i, 0.5f - 0.5f * i}, {-0.5f + 0.5f * i, 0.5f - 0.5f * i}},
                    {"+Z", "-X"},
                    R"CIRCUIT(
H 0
S 0
S 0
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "S",
            0,
            &TableauSimulator::SQRT_Z,
            &FrameSimulator::H_XY,
            &ErrorAnalyzer::H_XY,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principle square root of Z gate.
Phases the amplitude of |1> by i.

- Parens Arguments:

    This instruction takes no parens arguments.

- Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{1, 0}, {0, i}},
                    {"+Y", "+Z"},
                    R"CIRCUIT(
S 0
)CIRCUIT",
                };
            },
        });
    add_gate_alias(failed, "SQRT_Z", "S");

    add_gate(
        failed,
        Gate{
            "S_DAG",
            0,
            &TableauSimulator::SQRT_Z_DAG,
            &FrameSimulator::H_XY,
            &ErrorAnalyzer::H_XY,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
Principle square root of Z gate.
Phases the amplitude of |1> by -i.

- Parens Arguments:

    This instruction takes no parens arguments.

- Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{1, 0}, {0, -i}},
                    {"-Y", "+Z"},
                    R"CIRCUIT(
S 0
S 0
S 0
)CIRCUIT",
                };
            },
        });
    add_gate_alias(failed, "SQRT_Z_DAG", "S_DAG");
}
