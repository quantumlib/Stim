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
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}},
                    {"+IX", "+IZ", "+XI", "+ZI"},
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
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 0, i, 0}, {0, i, 0, 0}, {0, 0, 0, 1}},
                    {"+ZY", "+IZ", "+YZ", "+ZI"},
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
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, 0, -i, 0}, {0, -i, 0, 0}, {0, 0, 0, 1}},
                    {"-ZY", "+IZ", "-YZ", "+ZI"},
                };
            },
        });
}
