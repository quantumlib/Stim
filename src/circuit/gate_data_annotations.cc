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

void GateDataMap::add_gate_data_annotations(bool &failed) {
    add_gate(
        failed,
        Gate{
            "DETECTOR",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::DETECTOR,
            (GateFlags)(GATE_ONLY_TARGETS_MEASUREMENT_RECORD | GATE_IS_NOT_FUSABLE),
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Annotates that a set of measurements have a deterministic result, which can be used to detect errors.

Detectors are ignored in measurement sampling mode.
In detector sampling mode, detectors produce results (false=expected parity, true=incorrect parity detected).

- Example:

    ```
    H 0
    CNOT 0 1
    M 0 1
    DETECTOR rec[-1] rec[-2]
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "OBSERVABLE_INCLUDE",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::OBSERVABLE_INCLUDE,
            (GateFlags)(GATE_ONLY_TARGETS_MEASUREMENT_RECORD | GATE_TAKES_PARENS_ARGUMENT | GATE_IS_NOT_FUSABLE),
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Adds measurement results to a given logical observable index.

A logical observable's measurement result is the parity of all physical measurement results added to it.

A logical observable is similar to a Detector, except the measurements making up an observable can be built up
incrementally over the entire circuit.

Logical observables are ignored in measurement sampling mode.
In detector sampling mode, observables produce results (false=expected parity, true=incorrect parity detected).
These results are optionally appended to the detector results, depending on simulator arguments / command line flags.

- Example:

    ```
    H 0
    CNOT 0 1
    M 0 1
    OBSERVABLE_INCLUDE(5) rec[-1] rec[-2]
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "TICK",
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_NOT_FUSABLE,
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Indicates the end of a layer of gates, or that time is advancing.
For example, used by `stimcirq` to preserve the moment structure of cirq circuits converted to/from stim circuits.

- Example:

    ```
    TICK
    TICK
    # Oh, and of course:
    TICK
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        });
}
