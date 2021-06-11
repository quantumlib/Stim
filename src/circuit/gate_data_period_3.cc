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

void GateDataMap::add_gate_data_period_3(bool &failed) {
    add_gate(
        failed,
        Gate{
            "C_XYZ",
            0,
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
                    {{0.5f - i * 0.5f, -0.5f - 0.5f * i}, {0.5f - 0.5f * i, 0.5f + 0.5f * i}},
                    {"Y", "X"},
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "C_ZYX",
            0,
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
                    {{0.5f + i * 0.5f, 0.5f + 0.5f * i}, {-0.5f + 0.5f * i, 0.5f - 0.5f * i}},
                    {"Z", "Y"},
                };
            },
        });
}
