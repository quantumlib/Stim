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

void GateDataMap::add_gate_data_pauli(bool &failed) {
    add_gate(
        failed,
        Gate{
            "I",
            0,
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorAnalyzer::I,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "A_Pauli Gates",
                    R"MARKDOWN(
Identity gate.
Does nothing to the target qubits.

- Parens Arguments:

    This instruction takes no parens arguments.

- Targets:

    Qubits to do nothing to.
)MARKDOWN",
                    {{1, 0}, {0, 1}},
                    {"+X", "+Z"},
                    R"CIRCUIT(
# (no operations)
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "X",
            0,
            &TableauSimulator::X,
            &FrameSimulator::I,
            &ErrorAnalyzer::I,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "A_Pauli Gates",
                    R"MARKDOWN(
Pauli X gate.
The bit flip gate.

- Parens Arguments:

    This instruction takes no parens arguments.

- Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{0, 1}, {1, 0}},
                    {"+X", "-Z"},
                    R"CIRCUIT(
H 0
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
            "Y",
            0,
            &TableauSimulator::Y,
            &FrameSimulator::I,
            &ErrorAnalyzer::I,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "A_Pauli Gates",
                    R"MARKDOWN(
Pauli Y gate.

- Parens Arguments:

    This instruction takes no parens arguments.

- Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{0, -i}, {i, 0}},
                    {"-X", "-Z"},
                    R"CIRCUIT(
S 0
S 0
H 0
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
            "Z",
            0,
            &TableauSimulator::Z,
            &FrameSimulator::I,
            &ErrorAnalyzer::I,
            GATE_IS_UNITARY,
            []() -> ExtraGateData {
                return {
                    "A_Pauli Gates",
                    R"MARKDOWN(
Pauli Z gate.
The phase flip gate.

- Parens Arguments:

    This instruction takes no parens arguments.

- Targets:

    Qubits to operate on.
)MARKDOWN",
                    {{1, 0}, {0, -1}},
                    {"-X", "+Z"},
                    R"CIRCUIT(
S 0
S 0
)CIRCUIT",
                };
            },
        });
}
