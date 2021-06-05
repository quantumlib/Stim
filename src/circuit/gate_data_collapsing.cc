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

void GateDataMap::add_gate_data_collapsing(bool &failed) {
    // ===================== Measure Gates. ============================
    add_gate(failed, Gate{
        "MX",
        &TableauSimulator::measure_x,
        &FrameSimulator::measure_x,
        &ErrorFuser::MX,
        GATE_PRODUCES_RESULTS,
        []() -> ExtraGateData {
            return {
                "L_Collapsing Gates",
                R"MARKDOWN(
X-basis measurement.
Projects each target qubit into `|+>` or `|->` and reports its value (false=`|+>`, true=`|->`).
)MARKDOWN",
                {},
                {"X -> m", "X -> +X"},
            };
        },
    });
    add_gate(failed, Gate{
        "MY",
        &TableauSimulator::measure_y,
        &FrameSimulator::measure_y,
        &ErrorFuser::MY,
        GATE_PRODUCES_RESULTS,
        []() -> ExtraGateData {
            return {
                "L_Collapsing Gates",
                R"MARKDOWN(
Y-basis measurement.
Projects each target qubit into `|i>` or `|-i>` and reports its value (false=`|i>`, true=`|-i>`).
)MARKDOWN",
                {},
                {"Y -> m", "Y -> +Y"},
            };
        },
    });
    add_gate(failed, Gate{
        "M",
        &TableauSimulator::measure_z,
        &FrameSimulator::measure_z,
        &ErrorFuser::MZ,
        GATE_PRODUCES_RESULTS,
        []() -> ExtraGateData {
            return {
                "L_Collapsing Gates",
                R"MARKDOWN(
Z-basis measurement.
Projects each target qubit into `|0>` or `|1>` and reports its value (false=`|0>`, true=`|1>`).
)MARKDOWN",
                {},
                {"Z -> m", "Z -> +Z"},
            };
        },
    });
    add_gate_alias(failed, "MZ", "M");

    // ===================== Measure+Reset Gates. ============================
    add_gate(failed, Gate{
        "MRX",
        &TableauSimulator::measure_reset_x,
        &FrameSimulator::measure_reset_x,
        &ErrorFuser::MRX,
        GATE_PRODUCES_RESULTS,
        []() -> ExtraGateData {
            return {
                "L_Collapsing Gates",
                R"MARKDOWN(
X-basis demolition measurement.
Projects each target qubit into `|+>` or `|->`, reports its value (false=`|+>`, true=`|->`), then resets to `|+>`.
)MARKDOWN",
                {},
                {"X -> m", "1 -> +X"},
            };
        },
    });
    add_gate(failed, Gate{
        "MRY",
        &TableauSimulator::measure_reset_y,
        &FrameSimulator::measure_reset_y,
        &ErrorFuser::MRY,
        GATE_PRODUCES_RESULTS,
        []() -> ExtraGateData {
            return {
                "L_Collapsing Gates",
                R"MARKDOWN(
Y-basis demolition measurement.
Projects each target qubit into `|i>` or `|-i>`, reports its value (false=`|i>`, true=`|-i>`), then resets to `|i>`.
)MARKDOWN",
                {},
                {"Y -> m", "1 -> +Y"},
            };
        },
    });
    add_gate(failed, Gate{
        "MR",
        &TableauSimulator::measure_reset_z,
        &FrameSimulator::measure_reset_z,
        &ErrorFuser::MRZ,
        GATE_PRODUCES_RESULTS,
        []() -> ExtraGateData {
            return {
                "L_Collapsing Gates",
                R"MARKDOWN(
Z-basis demolition measurement.
Projects each target qubit into `|0>` or `|1>`, reports its value (false=`|0>`, true=`|1>`), then resets to `|0>`.
)MARKDOWN",
                {},
                {"Z -> m", "1 -> +Z"},
            };
        },
    });
    add_gate_alias(failed, "MRZ", "MR");


    // ===================== Reset Gates. ============================
    add_gate(failed, Gate{
        "RX",
        &TableauSimulator::reset_x,
        &FrameSimulator::reset_x,
        &ErrorFuser::RX,
        GATE_NO_FLAGS,
        []() -> ExtraGateData {
            return {
                "L_Collapsing Gates",
                R"MARKDOWN(
X-basis reset.
Forces each target qubit into the `|+>` state by silently measuring it in the X basis and applying a `Z` gate if it ended up in the `|->` state.
)MARKDOWN",
                {},
                {"1 -> +X"},
            };
        },
    });
    add_gate(failed, Gate{
        "RY",
        &TableauSimulator::reset_y,
        &FrameSimulator::reset_y,
        &ErrorFuser::RY,
        GATE_NO_FLAGS,
        []() -> ExtraGateData {
            return {
                "L_Collapsing Gates",
                R"MARKDOWN(
Y-basis reset.
Forces each target qubit into the `|i>` state by silently measuring it in the Y basis and applying an `X` gate if it ended up in the `|-i>` state.
)MARKDOWN",
                {},
                {"1 -> +Y"},
            };
        },
    });
    add_gate(failed, Gate{
        "R",
        &TableauSimulator::reset_z,
        &FrameSimulator::reset_z,
        &ErrorFuser::RZ,
        GATE_NO_FLAGS,
        []() -> ExtraGateData {
            return {
                "L_Collapsing Gates",
                R"MARKDOWN(
Z-basis reset.
Forces each target qubit into the `|0>` state by silently measuring it in the Z basis and applying an `X` gate if it ended up in the `|1>` state.
)MARKDOWN",
                {},
                {"1 -> +Z"},
            };
        },
    });
    add_gate_alias(failed, "RZ", "R");
}
