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
            ARG_COUNT_VARIABLE,
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

Detectors can optionally be parameterized by coordinates (e.g. `DETECTOR(1,2)` has coordinates 1,2).
These coordinates aren't really used for anything in stim, but can act as drawing hints for other tools.
Note that the coordinates are relative to accumulated coordinate shifts from SHIFT_COORDS instructions.
Also note that putting two detectors at the same coordinate does not fuse them into one detector
(beware that OBSERVABLE_INCLUDE does fuse observables at the same index, which looks very similar).
See SHIFT_COORDS for more details on using coordinates.

Note that detectors are always defined with respect to *noiseless behavior*. For example, placing an `X` gate before a
measurement cannot create detection events on detectors that include that measurement, but placing an `X_ERROR(1)`
does create detection events.

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
            1,
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::OBSERVABLE_INCLUDE,
            (GateFlags)(GATE_ONLY_TARGETS_MEASUREMENT_RECORD | GATE_IS_NOT_FUSABLE),
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

Note that observables are always defined with respect to *noiseless behavior*. For example, placing an `X` gate before a
measurement cannot flip a logical observable that include that measurement, but placing an `X_ERROR(1)` does flip the
observable. This is because observables are used for detecting errors, not for verifying noiseless functionality.

Note that observable indices are NOT shifted by SHIFT_COORDS.

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
            0,
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::I,
            (GateFlags)(GATE_IS_NOT_FUSABLE | GATE_TAKES_NO_TARGETS),
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

    add_gate(
        failed,
        Gate{
            "QUBIT_COORDS",
            ARG_COUNT_VARIABLE,
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::I,
            GATE_IS_NOT_FUSABLE,
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
An annotation used to indicate the intended location of a qubit.
The coordinates are double precision floating point numbers, relative to accumulated offsets from SHIFT_COORDS.
The coordinates are not in any particular order or number of dimensions.
As far as stim is concerned the coordinates are a list of opaque and mysterious numbers.
They have no effect on simulations of the circuit, but are potentially useful for tasks such as drawing the circuit.

See also: SHIFT_COORDS.

Note that a qubit's coordinates can be specified multiple times.
The intended interpretation is that the qubit is at the location of the most recent assignment.
For example, this could be used to indicate a simulated qubit is iteratively playing the role of many physical qubits.

- Example:

    ```
    QUBIT_COORDS(100, 101) 0
    QUBIT_COORDS(100, 101) 1
    SQRT_XX 0 1
    MR 0 1
    QUBIT_COORDS(2.5, 3.5) 2  # Floating point coordinates are allowed.
    QUBIT_COORDS(2.5, 4.5) 1  # Hint that qubit 1 is now referring to a different physical location.
    SQRT_XX 1 2
    M 1 2
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
            "SHIFT_COORDS",
            ARG_COUNT_VARIABLE,
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorFuser::I,
            (GateFlags)(GATE_IS_NOT_FUSABLE | GATE_TAKES_NO_TARGETS),
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Accumulates offsets to apply to qubit coordinates and detector coordinates.

See also: QUBIT_COORDS, DETECTOR.

Note: when qubit/detector coordinates use fewer dimensions than SHIFT_COORDS, the offsets from the additional dimensions
are ignored (i.e. not specifying a dimension is different from specifying it to be 0).

- Example:

    ```
    SHIFT_COORDS(500.5)
    QUBIT_COORDS(1510) 0  # Actually at 2010.5
    SHIFT_COORDS(1500)
    QUBIT_COORDS(11) 1    # Actually at 2011.5
    QUBIT_COORDS(10.5) 2  # Actually at 2011.0
    REPEAT 1000 {
        CNOT 0 2
        CNOT 1 2
        MR 2
        DETECTOR(10.5, 0) rec[-1] rec[-2]  # Actually at (2011.0, iteration_count).
        SHIFT_COORDS(0, 1)  # Advance 2nd coordinate to track loop iterations.
    }
    ```
)MARKDOWN",
                    {},
                    {},
                };
            },
        });
}
