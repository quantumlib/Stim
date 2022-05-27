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

#include "stim/circuit/gate_data.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

static constexpr std::complex<float> i = std::complex<float>(0, 1);
static constexpr std::complex<float> s = 0.7071067811865475244f;

void GateDataMap::add_gate_data_hada(bool &failed) {
    add_gate(
        failed,
        Gate{
            "H",
            0,
            &TableauSimulator::H_XZ,
            &FrameSimulator::H_XZ,
            &ErrorAnalyzer::H_XZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
The Hadamard gate.
Swaps the X and Z axes.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{s, s}, {s, -s}},
                    {"+Z", "+X"},
                    R"CIRCUIT(
H 0
)CIRCUIT",
                };
            },
        });
    add_gate_alias(failed, "H_XZ", "H");

    add_gate(
        failed,
        Gate{
            "H_XY",
            0,
            &TableauSimulator::H_XY,
            &FrameSimulator::H_XY,
            &ErrorAnalyzer::H_XY,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
A variant of the Hadamard gate that swaps the X and Y axes (instead of X and Z).

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{0, s - i * s}, {s + i * s, 0}},
                    {"+Y", "-Z"},
                    R"CIRCUIT(
H 0
S 0
S 0
H 0
S 0
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "H_YZ",
            0,
            &TableauSimulator::H_YZ,
            &FrameSimulator::H_YZ,
            &ErrorAnalyzer::H_YZ,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "B_Single Qubit Clifford Gates",
                    R"MARKDOWN(
A variant of the Hadamard gate that swaps the Y and Z axes (instead of X and Z).

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{s, -i * s}, {i * s, -s}},
                    {"-X", "+Y"},
                    R"CIRCUIT(
H 0
S 0
H 0
S 0
S 0
)CIRCUIT",
                };
            },
        });
}
