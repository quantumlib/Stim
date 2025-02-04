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

#include "gtest/gtest.h"

#include "stim/main_namespaced.test.h"

using namespace stim;

TEST(command_analyze_errors, detector_hypergraph_deprecated) {
    ASSERT_EQ(
        trim(run_captured_stim_main({"--detector_hypergraph"}, R"input(
            )input")),
        trim(R"output(
[stderr=[DEPRECATION] Use `stim analyze_errors` instead of `--detector_hypergraph`
]
            )output"));
}

TEST(command_analyze_errors, analyze_errors) {
    ASSERT_EQ(run_captured_stim_main({"--analyze_errors"}, ""), "\n");

    ASSERT_EQ(
        trim(run_captured_stim_main({"--analyze_errors"}, R"input(
X_ERROR(0.25) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
error(0.25) D0
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"analyze_errors"}, R"input(
X_ERROR(0.25) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
error(0.25) D0
            )output"));
}

TEST(command_analyze_errors, analyze_errors_fold_loops) {
    ASSERT_EQ(
        trim(run_captured_stim_main({"--analyze_errors", "--fold_loops"}, R"input(
REPEAT 1000 {
    R 0
    X_ERROR(0.25) 0
    M 0
    DETECTOR rec[-1]
}
            )input")),
        trim(R"output(
repeat 1000 {
    error(0.25) D0
    shift_detectors 1
}
            )output"));
}

TEST(command_analyze_errors, analyze_errors_allow_gauge_detectors) {
    ASSERT_EQ(
        trim(run_captured_stim_main({"--analyze_errors", "--allow_gauge_detectors"}, R"input(
R 0
H 0
CNOT 0 1
M 0 1
DETECTOR rec[-1]
DETECTOR rec[-2]
            )input")),
        trim(R"output(
error(0.5) D0 D1
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--analyze_errors"}, R"input(
R 0
H 0
CNOT 0 1
M 0 1
DETECTOR rec[-1]
DETECTOR rec[-2]
            )input")),
        trim(
            R"OUTPUT(
[stderr=)OUTPUT"
            "\x1B"
            R"OUTPUT([31mThe circuit contains non-deterministic detectors.

To make an SVG picture of the problem, you can use the python API like this:
    your_circuit.diagram('detslice-with-ops-svg', tick=range(0, 5), filter_coords=['D0', 'D1', ])
or the command line API like this:
    stim diagram --in your_circuit_file.stim --type detslice-with-ops-svg --tick 0:5 --filter_coords D0:D1 > output_image.svg

This was discovered while analyzing a Z-basis reset (R) on:
    qubit 0

The collapse anti-commuted with these detectors/observables:
    D0
    D1

The backward-propagating error sensitivity for D0 was:
    X0
    Z1

The backward-propagating error sensitivity for D1 was:
    X0

Circuit stack trace:
    at instruction #1 [which is R 0]
)OUTPUT"
            "\x1B"
            R"OUTPUT([0m]
)OUTPUT"));
}

TEST(command_analyze_errors, analyze_errors_all_approximate_disjoint_errors) {
    ASSERT_EQ(
        trim(run_captured_stim_main({"--analyze_errors", "--approximate_disjoint_errors"}, R"input(
R 0
PAULI_CHANNEL_1(0.125, 0.25, 0.375) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
error(0.375) D0
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--analyze_errors"}, R"input(
R 0
PAULI_CHANNEL_1(0.125, 0.25, 0.375) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(
            R"OUTPUT(
[stderr=)OUTPUT"
            "\x1B"
            R"OUTPUT([31mEncountered the operation PAULI_CHANNEL_1 during error analysis, but this operation requires the `approximate_disjoint_errors` option to be enabled.
If you're calling from python, using stim.Circuit.detector_error_model, you need to add the argument approximate_disjoint_errors=True.

If you're calling from the command line, you need to specify --approximate_disjoint_errors.

Circuit stack trace:
    at instruction #2 [which is PAULI_CHANNEL_1(0.125, 0.25, 0.375) 0]
)OUTPUT"
            "\x1B"
            R"OUTPUT([0m]
)OUTPUT"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--analyze_errors", "--approximate_disjoint_errors", "0.3"}, R"input(
R 0
PAULI_CHANNEL_1(0.0, 0.25, 0.375) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(
            R"OUTPUT(
[stderr=)OUTPUT"
            "\x1B"
            R"OUTPUT([31mPAULI_CHANNEL_1 has a probability argument (0.375) larger than the `approximate_disjoint_errors` threshold (0.3).

Circuit stack trace:
    at instruction #2 [which is PAULI_CHANNEL_1(0, 0.25, 0.375) 0]
)OUTPUT"
            "\x1B"
            R"OUTPUT([0m]
)OUTPUT"));
}
