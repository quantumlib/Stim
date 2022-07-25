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

#include <regex>
#include <unordered_map>

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

std::string execute(std::vector<const char *> flags, const char *std_in_content) {
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

std::string trim(std::string text) {
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

std::unordered_map<std::string, size_t> line_freq(std::string data) {
    data = trim(data);
    std::unordered_map<std::string, size_t> result{};
    size_t start = 0;
    for (size_t k = 0; k <= data.size(); k++) {
        if (data[k] == '\n' || data[k] == '\0') {
            result[data.substr(start, k - start)]++;
            start = k + 1;
        }
    }
    return result;
}

std::string deviation(const std::string &sample_content, const std::unordered_map<std::string, float> &expected) {
    auto actual = line_freq(sample_content);
    size_t actual_total = 0;
    for (const auto &kv : actual) {
        if (expected.find(kv.first) == expected.end()) {
            return "Sampled " + kv.first + " which was not expected.";
        }
        actual_total += kv.second;
    }
    if (actual_total == 0) {
        return "No samples.";
    }
    double expected_unity = 0;
    for (const auto &kv : expected) {
        expected_unity += kv.second;
    }
    if (fabs(expected_unity - 1) > 1e-5) {
        return "Expected distribution doesn't add up to 1.";
    }
    for (const auto &kv : expected) {
        float expected_rate = kv.second;
        float allowed_variation = 5 * sqrtf(expected_rate * (1 - expected_rate) / actual_total);
        if (expected_rate - allowed_variation < 0 || expected_rate + allowed_variation > 1) {
            return "Not enough samples to bound results away from extremes.";
        }

        float actual_rate = actual[kv.first] / (float)actual_total;
        if (fabs(expected_rate - actual_rate) > allowed_variation) {
            return "Actual rate " + std::to_string(actual_rate) + " of sample '" + kv.first +
                   "' is more than 5 standard deviations from expected rate " + std::to_string(expected_rate);
        }
    }

    return "";
}

static bool matches(std::string actual, std::string pattern) {
    // Hackily work around C++ regex not supporting multiline matching.
    std::replace(actual.begin(), actual.end(), '\n', 'X');
    std::replace(pattern.begin(), pattern.end(), '\n', 'X');
    return std::regex_match(actual, std::regex("^" + pattern + "$"));
}

TEST(main, help_modes) {
    ASSERT_TRUE(matches(execute({"--help"}, ""), ".*Available stim commands.+"));
    ASSERT_TRUE(matches(execute({"help"}, ""), ".*Available stim commands.+"));
    ASSERT_TRUE(matches(execute({}, ""), ".+stderr.+No mode.+"));
    ASSERT_TRUE(matches(execute({"--sample", "--repl"}, ""), ".+stderr.+More than one mode.+"));
    ASSERT_TRUE(matches(execute({"--sample", "--repl", "--detect"}, ""), ".+stderr.+More than one mode.+"));
    ASSERT_TRUE(matches(execute({"--help", "dhnsahddjoidsa"}, ""), ".*Unrecognized.*"));
    ASSERT_TRUE(matches(execute({"--help", "H"}, ""), ".+Hadamard.+"));
    ASSERT_TRUE(matches(execute({"--help", "--sample"}, ""), ".*Samples measurements from a circuit.+"));
    ASSERT_TRUE(matches(execute({"--help", "sample"}, ""), ".*Samples measurements from a circuit.+"));
}

TEST(main, bad_flag) {
    ASSERT_EQ(
        trim(execute({"--gen", "--unknown"}, "")),
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

TEST(main, sample_flag) {
    ASSERT_EQ(
        trim(execute({"--sample"}, R"input(
M 0
            )input")),
        trim(R"output(
0
            )output"));

    ASSERT_EQ(
        trim(execute({"--sample=1"}, R"input(
M 0
            )input")),
        trim(R"output(
0
            )output"));

    ASSERT_EQ(
        trim(execute({"sample", "--shots", "1"}, R"input(
M 0
            )input")),
        trim(R"output(
0
            )output"));
    ASSERT_EQ(
        trim(execute({"sample", "--shots", "2"}, R"input(
M 0
            )input")),
        trim(R"output(
0
0
            )output"));

    ASSERT_EQ(
        trim(execute({"--sample=0"}, R"input(
M 0
            )input")),
        trim(R"output(
            )output"));

    ASSERT_EQ(
        trim(execute({"--sample=2"}, R"input(
M 0
            )input")),
        trim(R"output(
0
0
            )output"));

    ASSERT_EQ(
        trim(execute({"--sample"}, R"input(
X 0
M 0
            )input")),
        trim(R"output(
1
            )output"));

    ASSERT_EQ(
        trim(execute({"--sample"}, R"input(
M !0
            )input")),
        trim(R"output(
1
            )output"));
}

TEST(main, intentional_failures) {
    ASSERT_EQ(
        "Sampled 10 which was not expected.",
        deviation(
            execute({"--sample=1000"}, R"input(
X 0
M 0 1
            )input"),
            {{"00", 0.5}, {"11", 0.5}}));

    ASSERT_NE(
        "Sampled 10 which was not expected.",
        deviation(
            execute({"--sample=1000"}, R"input(
H 0
M 0
            )input"),
            {{"0", 0.1}, {"1", 0.9}}));
}

TEST(main, basic_distributions) {
    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=1000"}, R"input(
H 0
CNOT 0 1
M 0 1
            )input"),
            {{"00", 0.5}, {"11", 0.5}}));

    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=1000"}, R"input(
H 0
CNOT 0 1
SQRT_X 0 1
M 0 1
            )input"),
            {{"10", 0.5}, {"01", 0.5}}));

    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=1000"}, R"input(
H 0
CNOT 0 1
SQRT_Y 0 1
M 0 1
            )input"),
            {{"00", 0.5}, {"11", 0.5}}));
}

TEST(main, sample_x_error) {
    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10000"}, R"input(
X_ERROR(0.1) 0 1
M 0 1
            )input"),
            {{"00", 0.9 * 0.9}, {"01", 0.9 * 0.1}, {"10", 0.9 * 0.1}, {"11", 0.1 * 0.1}}));

    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10"}, R"input(
H 0 1
X_ERROR(0.1) 0 1
H 0 1
M 0 1
            )input"),
            {{"00", 1}}));
}

TEST(main, sample_z_error) {
    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10000"}, R"input(
H 0 1
Z_ERROR(0.1) 0 1
H 0 1
M 0 1
            )input"),
            {{"00", 0.9 * 0.9}, {"01", 0.9 * 0.1}, {"10", 0.9 * 0.1}, {"11", 0.1 * 0.1}}));

    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10"}, R"input(
Z_ERROR(0.1) 0 1
M 0 1
            )input"),
            {{"00", 1}}));
}

TEST(main, sample_y_error) {
    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10000"}, R"input(
Y_ERROR(0.1) 0 1
M 0 1
            )input"),
            {{"00", 0.9 * 0.9}, {"01", 0.9 * 0.1}, {"10", 0.9 * 0.1}, {"11", 0.1 * 0.1}}));

    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10"}, R"input(
H_YZ 0 1
Y_ERROR(0.1) 0 1
H_YZ 0 1
M 0 1
            )input"),
            {{"00", 1}}));
}

TEST(main, sample_depolarize1_error) {
    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10000"}, R"input(
DEPOLARIZE1(0.3) 0 1
M 0 1
            )input"),
            {{"00", 0.8 * 0.8}, {"01", 0.8 * 0.2}, {"10", 0.8 * 0.2}, {"11", 0.2 * 0.2}}));

    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10000"}, R"input(
H 0 1
DEPOLARIZE1(0.3) 0 1
H 0 1
M 0 1
            )input"),
            {{"00", 0.8 * 0.8}, {"01", 0.8 * 0.2}, {"10", 0.8 * 0.2}, {"11", 0.2 * 0.2}}));

    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10000"}, R"input(
H_YZ 0 1
DEPOLARIZE1(0.3) 0 1
H_YZ 0 1
M 0 1
            )input"),
            {{"00", 0.8 * 0.8}, {"01", 0.8 * 0.2}, {"10", 0.8 * 0.2}, {"11", 0.2 * 0.2}}));
}

TEST(main, sample_depolarize2_error) {
    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10000"}, R"input(
DEPOLARIZE2(0.1) 0 1
M 0 1
            )input"),
            {{"00", 0.1 * 3 / 15 + 0.9}, {"01", 0.1 * 4 / 15}, {"10", 0.1 * 4 / 15}, {"11", 0.1 * 4 / 15}}));

    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10000"}, R"input(
H 0
H_YZ 1
DEPOLARIZE2(0.3) 0 1
H 0
H_YZ 1
M 0 1
            )input"),
            {{"00", 0.3 * 3 / 15 + 0.7}, {"01", 0.3 * 4 / 15}, {"10", 0.3 * 4 / 15}, {"11", 0.3 * 4 / 15}}));
}

TEST(main, sample_measure_reset) {
    ASSERT_EQ(
        trim(execute({"--sample"}, R"input(
X 0
R 0
M 0
            )input")),
        trim(R"output(
0
            )output"));
    ASSERT_EQ(
        trim(execute({"--sample"}, R"input(
X 0
MR 0
MR 0
            )input")),
        trim(R"output(
10
            )output"));
}

TEST(main, skip_reference_sample_flag) {
    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample", "--skip_reference_sample"}, R"input(
H 0
S 0
S 0
H 0
M 0
            )input"),
            {{"0", 1}}));

    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10", "--skip_reference_sample"}, R"input(
H 0
S 0
S 0
H 0
M 0
            )input"),
            {{"0", 1}}));

    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample"}, R"input(
H 0
S 0
S 0
H 0
M 0
            )input"),
            {{"1", 1}}));

    ASSERT_EQ(
        "",
        deviation(
            execute({"--sample=10"}, R"input(
H 0
S 0
S 0
H 0
M 0
            )input"),
            {{"1", 1}}));
}

TEST(main, detect_basic) {
    ASSERT_EQ(
        trim(execute({"--detect"}, R"input(
M 0
            )input")),
        trim(R"output(
            )output"));

    ASSERT_EQ(
        trim(execute({"--detect"}, R"input(
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
0
            )output"));

    ASSERT_EQ(
        trim(execute({"--detect"}, R"input(
X_ERROR(1) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
1
            )output"));

    ASSERT_EQ(
        trim(execute({"--detect", "2"}, R"input(
X_ERROR(1) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
1
1
            )output"));

    ASSERT_EQ(
        trim(execute({"detect", "--shots", "2"}, R"input(
X_ERROR(1) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
1
1
            )output"));

    ASSERT_EQ(
        trim(execute({"--detect"}, R"input(
DETECTOR
            )input")),
        trim(R"output(
0
            )output"));

    ASSERT_EQ(
        trim(execute({"--detect"}, R"input(
X_ERROR(1) 0
MR 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
1
            )output"));

    ASSERT_EQ(
        trim(execute({"--detect"}, R"input(
X_ERROR(1) 0
M 0
M 0
DETECTOR rec[-1] rec[-2]
            )input")),
        trim(R"output(
0
            )output"));

    ASSERT_EQ(
        trim(execute({"--detect"}, R"input(
X_ERROR(1) 0
MR 0
MR 0
DETECTOR rec[-1] rec[-2]
            )input")),
        trim(R"output(
1
            )output"));

    ASSERT_EQ(
        trim(execute({"--detect"}, R"input(
M 2
M 2
REPEAT 3 {
  R 2
  CNOT 0 2 1 2
  DETECTOR rec[-1] rec[-2]
  M 2
}
M 0 1
OBSERVABLE_INCLUDE(0) rec[-2] rec[-1]
            )input")),
        trim(R"output(
000
            )output"));

    ASSERT_EQ(
        trim(execute({"--detect", "--prepend_observables"}, R"input(
M 2
M 2
REPEAT 3 {
  R 2
  CNOT 0 2 1 2
  DETECTOR rec[-1] rec[-2]
  M 2
}
M 0 1
OBSERVABLE_INCLUDE(0) rec[-2] rec[-1]
            )input")),
        trim(R"output(
0000
[stderr=[DEPRECATION] Avoid using `--prepend_observables`. Data readers assume observables are appended, not prepended.
]
            )output"));

    ASSERT_EQ(
        trim(execute({"--detect", "--prepend_observables"}, R"input(
M 2
M 2
X_ERROR(1) 0 1
REPEAT 3 {
  R 2
  CNOT 0 2 1 2
  DETECTOR rec[-1] rec[-2]
  M 2
}
M 0 1
OBSERVABLE_INCLUDE(0) rec[-2]
            )input")),
        trim(R"output(
1000
[stderr=[DEPRECATION] Avoid using `--prepend_observables`. Data readers assume observables are appended, not prepended.
]
            )output"));

    ASSERT_EQ(
        trim(execute({"--detect", "--append_observables"}, R"input(
M 2
M 2
X_ERROR(1) 0 1
REPEAT 3 {
  R 2
  CNOT 0 2 1 2
  DETECTOR rec[-1] rec[-2]
  M 2
}
M 0 1
OBSERVABLE_INCLUDE(0) rec[-2]
            )input")),
        trim(R"output(
0001
            )output"));
}

TEST(main, detector_hypergraph_deprecated) {
    ASSERT_EQ(
        trim(execute({"--detector_hypergraph"}, R"input(
            )input")),
        trim(R"output(
[stderr=[DEPRECATION] Use `stim analyze_errors` instead of `--detector_hypergraph`
]
            )output"));
}

TEST(main, analyze_errors) {
    ASSERT_EQ(execute({"--analyze_errors"}, ""), "\n");

    ASSERT_EQ(
        trim(execute({"--analyze_errors"}, R"input(
X_ERROR(0.25) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
error(0.25) D0
            )output"));

    ASSERT_EQ(
        trim(execute({"analyze_errors"}, R"input(
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
        trim(execute({"--analyze_errors", "--fold_loops"}, R"input(
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
        trim(execute({"--analyze_errors", "--allow_gauge_detectors"}, R"input(
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
        trim(execute({"--analyze_errors"}, R"input(
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
        trim(execute({"--analyze_errors", "--approximate_disjoint_errors"}, R"input(
R 0
PAULI_CHANNEL_1(0.125, 0.25, 0.375) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
error(0.3125) D0
            )output"));

    ASSERT_EQ(
        trim(execute({"--analyze_errors"}, R"input(
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
        trim(execute({"--analyze_errors", "--approximate_disjoint_errors", "0.3"}, R"input(
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

TEST(main, generate_circuits) {
    ASSERT_TRUE(matches(
        trim(execute({"--gen=repetition_code", "--rounds=3", "--distance=4", "--task=memory"}, "")),
        ".+Generated repetition_code.+"));
    ASSERT_TRUE(matches(
        trim(execute({"--gen=surface_code", "--rounds=3", "--distance=2", "--task=unrotated_memory_z"}, "")),
        ".+Generated surface_code.+"));
    ASSERT_TRUE(matches(
        trim(execute({"gen", "--code=surface_code", "--rounds=3", "--distance=2", "--task=unrotated_memory_z"}, "")),
        ".+Generated surface_code.+"));
    ASSERT_TRUE(matches(
        trim(execute({"--gen=surface_code", "--rounds=3", "--distance=2", "--task=rotated_memory_x"}, "")),
        ".+Generated surface_code.+"));
    ASSERT_TRUE(matches(
        trim(execute({"--gen=color_code", "--rounds=3", "--distance=3", "--task=memory_xyz"}, "")),
        ".+Generated color_code.+"));
}

TEST(main, detection_event_simulator_counts_measurements_correctly) {
    auto s = execute({"--detect=1000"}, "MPP Z8*X9\nDETECTOR rec[-1]");
    size_t zeroes = 0;
    size_t ones = 0;
    for (size_t k = 0; k < s.size(); k += 2) {
        zeroes += s[k] == '0';
        ones += s[k] == '1';
        ASSERT_EQ(s[k + 1], '\n');
    }
    ASSERT_EQ(zeroes + ones, 1000);
    ASSERT_TRUE(400 < zeroes && zeroes < 600);
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
        trim(execute(
            {"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.data(), "--append_observables"},
            "00\n01\n10\n11\n")),
        trim(R"output(
shot D0
shot D0 D1 L2
shot
shot D1 L2
            )output"));

    ASSERT_EQ(
        trim(execute({"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.data()}, "00\n01\n10\n11\n")),
        trim(R"output(
shot D0
shot D0 D1
shot
shot D1
            )output"));

    ASSERT_EQ(
        trim(execute(
            {"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.data(), "--skip_reference_sample"},
            "00\n01\n10\n11\n")),
        trim(R"output(
shot
shot D1
shot D0
shot D0 D1
            )output"));
}

TEST(main, seeded_sampling) {
    ASSERT_EQ(
        execute({"sample", "--shots=256", "--seed 5"}, R"input(
                H 0
                M 0
            )input"),
        execute({"sample", "--shots=256", "--seed 5"}, R"input(
                H 0
                M 0
            )input"));

    ASSERT_NE(
        execute({"sample", "--shots=256"}, R"input(
                H 0
                M 0
            )input"),
        execute({"sample", "--shots=256"}, R"input(
                H 0
                M 0
            )input"));

    ASSERT_NE(
        execute({"sample", "--shots=256", "--seed 5"}, R"input(
                H 0
                M 0
            )input"),
        execute({"sample", "--shots=256", "--seed 6"}, R"input(
                H 0
                M 0
            )input"));
}

TEST(main, seeded_detecting) {
    ASSERT_EQ(
        execute({"detect", "--shots=256", "--seed 5"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"),
        execute({"detect", "--shots=256", "--seed 5"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"));

    ASSERT_NE(
        execute({"detect", "--shots=256"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"),
        execute({"detect", "--shots=256"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"));

    ASSERT_NE(
        execute({"detect", "--shots=256", "--seed 5"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"),
        execute({"detect", "--shots=256", "--seed 6"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"));
}

TEST(main, explain_errors) {
    ASSERT_EQ(execute({"explain_errors"}, ""), "");

    RaiiTempNamedFile tmp;
    tmp.write_contents("error(1) D0\n");

    ASSERT_EQ(
        trim(execute({"explain_errors", "--dem_filter", tmp.path.data()}, R"input(
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
    ASSERT_EQ(execute({"sample_dem"}, ""), "\n");

    RaiiTempNamedFile obs_out;

    ASSERT_EQ(
        trim(execute(
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
