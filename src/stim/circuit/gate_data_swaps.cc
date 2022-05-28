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

void GateDataMap::add_gate_data_swaps(bool &failed) {
    add_gate(
        failed,
        Gate{
            "SWAP",
            0,
            &TableauSimulator::SWAP,
            &FrameSimulator::SWAP,
            &ErrorAnalyzer::SWAP,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Swaps two qubits.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}},
                    {"+IX", "+IZ", "+XI", "+ZI"},
                    R"CIRCUIT(
CNOT 0 1
CNOT 1 0
CNOT 0 1
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "ISWAP",
            0,
            &TableauSimulator::ISWAP,
            &FrameSimulator::ISWAP,
            &ErrorAnalyzer::ISWAP,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Swaps two qubits and phases the -1 eigenspace of the ZZ observable by i.
Equivalent to `SWAP` then `CZ` then `S` on both targets.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 0, i, 0}, {0, i, 0, 0}, {0, 0, 0, 1}},
                    {"+ZY", "+IZ", "+YZ", "+ZI"},
                    R"CIRCUIT(
CNOT 0 1
S 1
CNOT 1 0
CNOT 0 1
)CIRCUIT",
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "ISWAP_DAG",
            0,
            &TableauSimulator::ISWAP_DAG,
            &FrameSimulator::ISWAP,
            &ErrorAnalyzer::ISWAP,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Swaps two qubits and phases the -1 eigenspace of the ZZ observable by -i.
Equivalent to `SWAP` then `CZ` then `S_DAG` on both targets.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 0, -i, 0}, {0, -i, 0, 0}, {0, 0, 0, 1}},
                    {"-ZY", "+IZ", "-YZ", "+ZI"},
                    R"CIRCUIT(
CNOT 0 1
S 1
S 1
S 1
CNOT 1 0
CNOT 0 1
)CIRCUIT",
                };
            },
        });
}
