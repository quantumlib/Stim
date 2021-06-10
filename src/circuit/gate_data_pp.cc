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

void GateDataMap::add_gate_data_pp(bool &failed) {
    add_gate(
        failed,
        Gate{
            "SQRT_XX",
            &TableauSimulator::SQRT_XX,
            &FrameSimulator::SQRT_XX,
            &ErrorFuser::SQRT_XX,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the XX observable by i.
)MARKDOWN",
                    {{0.5f + 0.5f * i, 0, 0, 0.5f - 0.5f * i},
                     {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                     {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                     {0.5f - 0.5f * i, 0, 0, 0.5f + 0.5f * i}},
                    {"+XI", "-YX", "+IX", "-XY"},
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "SQRT_XX_DAG",
            &TableauSimulator::SQRT_XX_DAG,
            &FrameSimulator::SQRT_XX,
            &ErrorFuser::SQRT_XX,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the XX observable by -i.
)MARKDOWN",
                    {{0.5f - 0.5f * i, 0, 0, 0.5f + 0.5f * i},
                     {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                     {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                     {0.5f + 0.5f * i, 0, 0, 0.5f - 0.5f * i}},
                    {"+XI", "+YX", "+IX", "+XY"},
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "SQRT_YY",
            &TableauSimulator::SQRT_YY,
            &FrameSimulator::SQRT_YY,
            &ErrorFuser::SQRT_YY,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the YY observable by i.
)MARKDOWN",
                    {{0.5f + 0.5f * i, 0, 0, -0.5f + 0.5f * i},
                     {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                     {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                     {-0.5f + 0.5f * i, 0, 0, 0.5f + 0.5f * i}},
                    {"-ZY", "+XY", "-YZ", "+YX"},
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "SQRT_YY_DAG",
            &TableauSimulator::SQRT_YY_DAG,
            &FrameSimulator::SQRT_YY,
            &ErrorFuser::SQRT_YY,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the YY observable by -i.
)MARKDOWN",
                    {{0.5f - 0.5f * i, 0, 0, -0.5f - 0.5f * i},
                     {0, 0.5f - 0.5f * i, 0.5f + 0.5f * i, 0},
                     {0, 0.5f + 0.5f * i, 0.5f - 0.5f * i, 0},
                     {-0.5f - 0.5f * i, 0, 0, 0.5f - 0.5f * i}},
                    {"+ZY", "-XY", "+YZ", "-YX"},
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "SQRT_ZZ",
            &TableauSimulator::SQRT_ZZ,
            &FrameSimulator::SQRT_ZZ,
            &ErrorFuser::SQRT_ZZ,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the ZZ observable by i.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, i, 0, 0}, {0, 0, i, 0}, {0, 0, 0, 1}},
                    {"+YZ", "+ZI", "+ZY", "+IZ"},
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "SQRT_ZZ_DAG",
            &TableauSimulator::SQRT_ZZ_DAG,
            &FrameSimulator::SQRT_ZZ,
            &ErrorFuser::SQRT_ZZ,
            (GateFlags)(GATE_IS_UNITARY | GATE_TARGETS_PAIRS),
            []() -> ExtraGateData {
                return {
                    "C_Two Qubit Clifford Gates",
                    R"MARKDOWN(
Phases the -1 eigenspace of the ZZ observable by -i.
)MARKDOWN",
                    {{1, 0, 0, 0}, {0, -i, 0, 0}, {0, 0, -i, 0}, {0, 0, 0, 1}},
                    {"-YZ", "+ZI", "-ZY", "+IZ"},
                };
            },
        });
}
