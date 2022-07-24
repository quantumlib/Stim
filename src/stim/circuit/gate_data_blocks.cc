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

void GateDataMap::add_gate_data_blocks(bool &failed) {
    add_gate(
        failed,
        Gate{
            "REPEAT",
            0,
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorAnalyzer::I,
            (GateFlags)(GATE_IS_BLOCK | GATE_IS_NOT_FUSABLE),
            []() -> ExtraGateData {
                return {
                    "Y_Control Flow",
                    R"MARKDOWN(
Repeats the instructions in its body N times.

Currently, repetition counts of 0 are not allowed because they create corner cases with ambiguous resolutions.
For example, if a logical observable is only given measurements inside a repeat block with a repetition count of 0, it's
ambiguous whether the output of sampling the logical observables includes a bit for that logical observable.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    A positive integer in [1, 10^18] specifying the number of repetitions.

Example:

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
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });
}
