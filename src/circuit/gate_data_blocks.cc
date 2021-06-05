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

void GateDataMap::add_gate_data_blocks(bool &failed) {
    add_gate(failed, Gate{
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
    });
}
