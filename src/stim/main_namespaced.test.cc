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

#include "stim/main_namespaced.h"
#include "stim/main_namespaced.test.h"

#include <regex>

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

std::string stim::run_captured_stim_main(std::vector<const char *> flags, const char *std_in_content) {
    // Setup input.
    RaiiTempNamedFile raii_temp_file;
    if (std_in_content != nullptr) {
        raii_temp_file.write_contents(std_in_content);
        flags.push_back("--in");
        flags.push_back(raii_temp_file.path.data());
    }
    flags.insert(flags.begin(), "[PROGRAM_LOCATION_IGNORE]");

    // Setup output.
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    int result;
    try {
        result = stim::main(flags.size(), flags.data());
    } catch (const std::exception &e) {
        return "[exception=" + std::string(e.what()) + "]" + testing::internal::GetCapturedStdout() +
               testing::internal::GetCapturedStderr();
    }
    std::string out = testing::internal::GetCapturedStdout();
    std::string err = testing::internal::GetCapturedStderr();
    if (!err.empty()) {
        return out + "[stderr=" + err + "]";
    }
    if (result != EXIT_SUCCESS) {
        return "[exit code != EXIT_SUCCESS]";
    }

    return out;
}

std::string stim::trim(std::string text) {
    size_t s = 0;
    size_t e = text.size();
    while (s < e && std::isspace(text[s])) {
        s++;
    }
    while (s < e && std::isspace(text[e - 1])) {
        e--;
    }
    return text.substr(s, e - s);
}

bool stim::matches(std::string actual, std::string pattern) {
    // Hackily work around C++ regex not supporting multiline matching.
    std::replace(actual.begin(), actual.end(), '\n', 'X');
    std::replace(pattern.begin(), pattern.end(), '\n', 'X');
    return std::regex_match(actual, std::regex("^" + pattern + "$"));
}

TEST(main, help_modes) {
    ASSERT_TRUE(matches(run_captured_stim_main({"--help"}, ""), ".*Available stim commands.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({"help"}, ""), ".*Available stim commands.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({}, ""), ".+stderr.+No mode.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({"--sample", "--repl"}, ""), ".+stderr.+More than one mode.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({"--sample", "--repl", "--detect"}, ""), ".+stderr.+More than one mode.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({"--help", "dhnsahddjoidsa"}, ""), ".*Unrecognized.*"));
    ASSERT_TRUE(matches(run_captured_stim_main({"--help", "H"}, ""), ".+Hadamard.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({"--help", "--sample"}, ""), ".*Samples measurements from a circuit.+"));
    ASSERT_TRUE(matches(run_captured_stim_main({"--help", "sample"}, ""), ".*Samples measurements from a circuit.+"));
}

TEST(main, bad_flag) {
    ASSERT_EQ(
        trim(run_captured_stim_main({"--gen", "--unknown"}, "")),
        trim("[stderr=\033[31mUnrecognized command line argument --unknown for `stim gen`.\n"
             "Recognized command line arguments for `stim gen`:\n"
             "    --after_clifford_depolarization\n"
             "    --after_reset_flip_probability\n"
             "    --before_measure_flip_probability\n"
             "    --before_round_data_depolarization\n"
             "    --code\n"
             "    --distance\n"
             "    --in\n"
             "    --out\n"
             "    --rounds\n"
             "    --task\n"
             "\033[0m]\n"));
}

TEST(main, detector_hypergraph_deprecated) {
    ASSERT_EQ(
        trim(run_captured_stim_main({"--detector_hypergraph"}, R"input(
            )input")),
        trim(R"output(
[stderr=[DEPRECATION] Use `stim analyze_errors` instead of `--detector_hypergraph`
]
            )output"));
}

TEST(main, analyze_errors) {
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

TEST(main, analyze_errors_fold_loops) {
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

TEST(main, analyze_errors_allow_gauge_detectors) {
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
        trim(R"OUTPUT(
[stderr=)OUTPUT"
             "\x1B"
             R"OUTPUT([31mThe circuit contains non-deterministic detectors.
(To allow non-deterministic detectors, use the `allow_gauge_detectors` option.)

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

TEST(main, analyze_errors_all_approximate_disjoint_errors) {
    ASSERT_EQ(
        trim(run_captured_stim_main({"--analyze_errors", "--approximate_disjoint_errors"}, R"input(
R 0
PAULI_CHANNEL_1(0.125, 0.25, 0.375) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
error(0.3125) D0
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
If you're' calling from python, using stim.Circuit.detector_error_model, you need to add the argument approximate_disjoint_errors=True.

If you're' calling from the command line, you need to specify --approximate_disjoint_errors.

Circuit stack trace:
    at instruction #2 [which is PAULI_CHANNEL_1(0.125, 0.25, 0.375) 0]
)OUTPUT"
            "\x1B"
            R"OUTPUT([0m]
)OUTPUT"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--analyze_errors", "--approximate_disjoint_errors", "0.3"}, R"input(
R 0
PAULI_CHANNEL_1(0.125, 0.25, 0.375) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(
            R"OUTPUT(
[stderr=)OUTPUT"
            "\x1B"
            R"OUTPUT([31mPAULI_CHANNEL_1 has a component probability '0.375000' larger than the `approximate_disjoint_errors` threshold of '0.300000'.

Circuit stack trace:
    at instruction #2 [which is PAULI_CHANNEL_1(0.125, 0.25, 0.375) 0]
)OUTPUT"
            "\x1B"
            R"OUTPUT([0m]
)OUTPUT"));
}

TEST(main, m2d) {
    RaiiTempNamedFile tmp;
    tmp.write_contents(R"CIRCUIT(
        X 0
        M 0 1
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(2) rec[-1]
    )CIRCUIT");

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.data(), "--append_observables"},
            "00\n01\n10\n11\n")),
        trim(R"output(
shot D0
shot D0 D1 L2
shot
shot D1 L2
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.data()}, "00\n01\n10\n11\n")),
        trim(R"output(
shot D0
shot D0 D1
shot
shot D1
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.data(), "--skip_reference_sample"},
            "00\n01\n10\n11\n")),
        trim(R"output(
shot
shot D1
shot D0
shot D0 D1
            )output"));
}

TEST(main, explain_errors) {
    ASSERT_EQ(run_captured_stim_main({"explain_errors"}, ""), "");

    RaiiTempNamedFile tmp;
    tmp.write_contents("error(1) D0\n");

    ASSERT_EQ(
        trim(run_captured_stim_main({"explain_errors", "--dem_filter", tmp.path.data()}, R"input(
X_ERROR(0.25) 0 1
M 0 1
DETECTOR rec[-1]
DETECTOR rec[-2]
            )input")),
        trim(R"output(
ExplainedError {
    dem_error_terms: D0
    CircuitErrorLocation {
        flipped_pauli_product: X1
        Circuit location stack trace:
            (after 0 TICKs)
            at instruction #1 (X_ERROR) in the circuit
            at target #2 of the instruction
            resolving to X_ERROR(0.25) 1
    }
}
            )output"));
}

TEST(main, sample_dem) {
    ASSERT_EQ(run_captured_stim_main({"sample_dem"}, ""), "\n");

    RaiiTempNamedFile obs_out;

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {
                "sample_dem",
                "--obs_out",
                obs_out.path.data(),
                "--out_format",
                "01",
                "--obs_out_format",
                "01",
                "--shots",
                "5",
                "--seed",
                "0",
            },
            R"input(
                error(0) D0
                error(1) D1 L2
            )input")),
        trim(R"output(
01
01
01
01
01
            )output"));
    ASSERT_EQ(obs_out.read_contents(), "001\n001\n001\n001\n001\n");
}
