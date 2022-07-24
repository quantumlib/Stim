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

void GateDataMap::add_gate_data_annotations(bool &failed) {
    add_gate(
        failed,
        Gate{
            "DETECTOR",
            ARG_COUNT_SYGIL_ANY,
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorAnalyzer::DETECTOR,
            (GateFlags)(GATE_ONLY_TARGETS_MEASUREMENT_RECORD | GATE_IS_NOT_FUSABLE),
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Annotates that a set of measurements can be used to detect errors, because the set's parity should be deterministic.

Note that it is not necessary to say whether the measurement set's parity is even or odd; all that matters is that the
parity should be *consistent* when running the circuit and omitting all noisy operations. Note that, for example, this
means that even though `X` and `X_ERROR(1)` have equivalent effects on the measurements making up a detector, they have
differing effects on the detector (because `X` is intended, determining the expected value, and `X_ERROR` is noise,
causing deviations from the expected value).

Detectors are ignored when sampling measurements, but produce results when sampling detection events. In detector
sampling mode, each detector produces a result bit (where 0 means "measurement set had expected parity" and 1 means
"measurement set had incorrect parity"). When converting a circuit into a detector error model, errors are grouped based
on the detectors they flip (the "symptoms" of the error) and the observables they flip (the "frame changes" of the
error).

It is permitted, though not recommended, for the measurement set given to a `DETECTOR` instruction to have inconsistent
parity. When a detector's measurement set is inconsistent, the detector is called a "gauge detector" and the expected
parity of the measurement set is chosen arbitrarily (in an implementation-defined way). Some circuit analysis tools
(such as the circuit-to-detector-error-model conversion) will by default refuse to process circuits containing gauge
detectors. Gauge detectors produce random results when sampling detection events, though these results will be
appropriately correlated with other gauge detectors. For example, if `DETECTOR rec[-1]` and `DETECTOR rec[-2]` are gauge
detectors but `DETECTOR rec[-1] rec[-2]` is not, then under noiseless execution the two gauge detectors would either
always produce the same result or always produce opposite results.

Detectors can specify coordinates using their parens arguments. Coordinates have no effect on simulations, but can be
useful to tools consuming the circuit. For example, a tool drawing how the detectors in a circuit relate to each other
can use the coordinates as hints for where to place the detectors in the drawing.

Parens Arguments:

    Optional.
    Coordinate metadata, relative to the current coordinate offset accumulated from `SHIFT_COORDS` instructions.
    Can be any number of coordinates from 1 to 16.
    There is no required convention for which coordinate is which.

Targets:

    The measurement records to XOR together to get the deterministic-under-noiseless-execution parity.

Example:

    R 0
    X_ERROR(0.1) 0
    M 0  # This measurement is always False under noiseless execution.
    # Annotate that most recent measurement should be deterministic.
    DETECTOR rec[-1]

    R 0
    X 0
    X_ERROR(0.1) 0
    M 0  # This measurement is always True under noiseless execution.
    # Annotate that most recent measurement should be deterministic.
    DETECTOR rec[-1]

    R 0 1
    H 0
    CNOT 0 1
    DEPOLARIZE2(0.001) 0 1
    M 0 1  # These two measurements are always equal under noiseless execution.
    # Annotate that the parity of the previous two measurements should be consistent.
    DETECTOR rec[-1] rec[-2]

    # A series of trivial detectors with hinted coordinates along the diagonal line Y = 2X + 3.
    REPEAT 100 {
        R 0
        M 0
        SHIFT_COORDS(1, 2)
        DETECTOR(0, 3) rec[-1]
    }
)MARKDOWN",
                    {},
                    {},
                    nullptr,
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
            &ErrorAnalyzer::OBSERVABLE_INCLUDE,
            (GateFlags)(GATE_ONLY_TARGETS_MEASUREMENT_RECORD | GATE_IS_NOT_FUSABLE | GATE_ARGS_ARE_UNSIGNED_INTEGERS),
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Adds measurement records to a specified logical observable.

A potential point of confusion here is that Stim's notion of a logical observable is nothing more than a set of
measurements, potentially spanning across the entire circuit, that together produce a deterministic result. It's more
akin to the "boundary of a parity sheet" in a topological spacetime diagram than it is to the notion of a qubit
observable. For example, consider a surface code memory experiment that initializes a logical |0>, preserves the state
noise, and eventually performs a logical Z basis measurement. The circuit representing this experiment would use
`OBSERVABLE_INCLUDE` instructions to specifying which physical measurements within the logical Z basis measurement
should be XOR'd together to get the logical measurement result. This effectively identifies the logical Z observable.
But the circuit would *not* declare an X observable, because the X observable is not deterministic in a Z basis memory
experiment; it has no corresponding deterministic measurement set.

Logical observables are ignored when sampling measurements, but can produce results (if requested) when sampling
detection events. In detector sampling mode, each observable can produce a result bit (where 0 means "measurement set
had expected parity" and 1 means "measurement set had incorrect parity"). When converting a circuit into a detector
error model, errors are grouped based on the detectors they flip (the "symptoms" of the error) and the observables they
flip (the "frame changes" of the error).

Another potential point of confusion is that when sampling logical measurement results, as part of sampling detection
events in the circuit, the reported results are not measurements of the logical observable but rather whether those
measurement results *were flipped*. This has significant simulation speed benefits, and also makes it so that it is not
necessary to say whether the logical measurement result is supposed to be False or True. Note that, for example, this
means that even though `X` and `X_ERROR(1)` have equivalent effects on the measurements making up an observable, they
have differing effects on the reported value of an observable when sampling detection events (because `X` is intended,
determining the expected value, and `X_ERROR` is noise, causing deviations from the expected value).

It is not recommended for the measurement set of an observable to have inconsistent parity. For example, the
circuit-to-detector-error-model conversion will refuse to operate on circuits containing such observables.

Parens Arguments:

    A non-negative integer specifying the index of the logical observable to add the measurement records to.

Targets:

    The measurement records to add to the specified observable.

Example:

    R 0 1
    H 0
    CNOT 0 1
    M 0 1
    # Observable 0 is the parity of the previous two measurements.
    OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]

    R 0 1
    H 0
    CNOT 0 1
    M 0 1
    # Observable 1 is the parity of the previous measurement...
    OBSERVABLE_INCLUDE(1) rec[-1]
    # ...and the one before that.
    OBSERVABLE_INCLUDE(1) rec[-2]
)MARKDOWN",
                    {},
                    {},
                    nullptr,
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
            &ErrorAnalyzer::TICK,
            (GateFlags)(GATE_IS_NOT_FUSABLE | GATE_TAKES_NO_TARGETS),
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Annotates the end of a layer of gates, or that time is advancing.

This instruction is not necessary, it has no effect on simulations, but it can be used by tools that are transforming or
visualizing the circuit. For example, a tool that adds noise to a circuit may include cross-talk terms that require
knowing whether or not operations are happening in the same time step or not.

TICK instructions are added, and checked for, by `stimcirq` in order to preserve the moment structure of cirq circuits
converted between stim circuits and cirq circuits.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    This instruction takes no targets.

Example:

    # First time step.
    H 0
    CZ 1 2
    TICK

    # Second time step.
    H 1
    TICK

    # Empty time step.
    TICK
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "QUBIT_COORDS",
            ARG_COUNT_SYGIL_ANY,
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorAnalyzer::I,
            GATE_IS_NOT_FUSABLE,
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Annotates the location of a qubit.

Coordinates are not required and have no effect on simulations, but can be useful to tools consuming the circuit. For
example, a tool drawing the circuit  can use the coordinates as hints for where to place the qubits in the drawing.
`stimcirq` uses `QUBIT_COORDS` instructions to preserve `cirq.LineQubit` and `cirq.GridQubit` coordinates when
converting between stim circuits and cirq circuits

A qubit's coordinates can be specified multiple times, with the intended interpretation being that the qubit is at the
location of the most recent assignment. For example, this could be used to indicate a simulated qubit is iteratively
playing the role of many physical qubits.

Parens Arguments:

    Optional.
    The latest coordinates of the qubit, relative to accumulated offsets from `SHIFT_COORDS` instructions.
    Can be any number of coordinates from 1 to 16.
    There is no required convention for which coordinate is which.

Targets:

    The qubit or qubits the coordinates apply to.

Example:

    # Annotate that qubits 0 to 3 are at the corners of a square.
    QUBIT_COORDS(0, 0) 0
    QUBIT_COORDS(0, 1) 1
    QUBIT_COORDS(1, 0) 2
    QUBIT_COORDS(1, 1) 3
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });

    add_gate(
        failed,
        Gate{
            "SHIFT_COORDS",
            ARG_COUNT_SYGIL_ANY,
            &TableauSimulator::I,
            &FrameSimulator::I,
            &ErrorAnalyzer::SHIFT_COORDS,
            (GateFlags)(GATE_IS_NOT_FUSABLE | GATE_TAKES_NO_TARGETS),
            []() -> ExtraGateData {
                return {
                    "Z_Annotations",
                    R"MARKDOWN(
Accumulates offsets that affect qubit coordinates and detector coordinates.

Note: when qubit/detector coordinates use fewer dimensions than SHIFT_COORDS, the offsets from the additional dimensions
are ignored (i.e. not specifying a dimension is different from specifying it to be 0).

See also: `QUBIT_COORDS`, `DETECTOR`.

Parens Arguments:

    Offsets to add into the current coordinate offset.
    Can be any number of coordinate offsets from 1 to 16.
    There is no required convention for which coordinate is which.

Targets:

    This instruction takes no targets.

Example:

    SHIFT_COORDS(500.5)
    QUBIT_COORDS(1510) 0  # Actually at 2010.5
    SHIFT_COORDS(1500)
    QUBIT_COORDS(11) 1    # Actually at 2011.5
    QUBIT_COORDS(10.5) 2  # Actually at 2011.0

    # Declare some detectors with coordinates along a diagonal line.
    REPEAT 1000 {
        CNOT 0 2
        CNOT 1 2
        MR 2
        DETECTOR(10.5, 0) rec[-1] rec[-2]  # Actually at (2011.0, iteration_count).
        SHIFT_COORDS(0, 1)  # Advance 2nd coordinate to track loop iterations.
    }
)MARKDOWN",
                    {},
                    {},
                    nullptr,
                };
            },
        });
}
