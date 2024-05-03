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

#include <unordered_map>

#include "gtest/gtest.h"

#include "stim/main_namespaced.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

std::unordered_map<std::string_view, size_t> line_freq_with_lifetime_matching_arg(std::string_view data) {
    data = trim(data);
    std::unordered_map<std::string_view, size_t> result{};
    size_t start = 0;
    for (size_t k = 0; k <= data.size(); k++) {
        if (data[k] == '\n' || data[k] == '\0') {
            result[data.substr(start, k - start)]++;
            start = k + 1;
        }
    }
    return result;
}

std::string deviation(std::string_view sample_content, const std::unordered_map<std::string_view, float> &expected) {
    auto actual = line_freq_with_lifetime_matching_arg(sample_content);
    size_t actual_total = 0;
    for (const auto &kv : actual) {
        if (expected.find(kv.first) == expected.end()) {
            return "Sampled " + std::string(kv.first) + " which was not expected.";
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
            return "Actual rate " + std::to_string(actual_rate) + " of sample '" + std::string(kv.first) +
                   "' is more than 5 standard deviations from expected rate " + std::to_string(expected_rate);
        }
    }

    return "";
}

TEST(command_sample, sample_flag) {
    ASSERT_EQ(
        trim(run_captured_stim_main({"--sample"}, R"input(
M 0
            )input")),
        trim(R"output(
0
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--sample=1"}, R"input(
M 0
            )input")),
        trim(R"output(
0
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"sample", "--shots", "1"}, R"input(
M 0
            )input")),
        trim(R"output(
0
            )output"));
    ASSERT_EQ(
        trim(run_captured_stim_main({"sample", "--shots", "2"}, R"input(
M 0
            )input")),
        trim(R"output(
0
0
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--sample=0"}, R"input(
M 0
            )input")),
        trim(R"output(
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--sample=2"}, R"input(
M 0
            )input")),
        trim(R"output(
0
0
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--sample"}, R"input(
X 0
M 0
            )input")),
        trim(R"output(
1
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--sample"}, R"input(
M !0
            )input")),
        trim(R"output(
1
            )output"));
}

TEST(command_sample, intentional_failures) {
    ASSERT_EQ(
        "Sampled 10 which was not expected.",
        deviation(
            run_captured_stim_main({"--sample=1000"}, R"input(
X 0
M 0 1
            )input"),
            {{"00", 0.5}, {"11", 0.5}}));

    ASSERT_NE(
        "Sampled 10 which was not expected.",
        deviation(
            run_captured_stim_main({"--sample=1000"}, R"input(
H 0
M 0
            )input"),
            {{"0", 0.1}, {"1", 0.9}}));
}

TEST(command_sample, basic_distributions) {
    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=10000"}, R"input(
H 0
CNOT 0 1
M 0 1
            )input"),
            {{"00", 0.5}, {"11", 0.5}}));

    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=10000"}, R"input(
H 0
CNOT 0 1
SQRT_X 0 1
M 0 1
            )input"),
            {{"10", 0.5}, {"01", 0.5}}));

    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=10000"}, R"input(
H 0
CNOT 0 1
SQRT_Y 0 1
M 0 1
            )input"),
            {{"00", 0.5}, {"11", 0.5}}));
}

TEST(command_sample, sample_x_error) {
    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=100000"}, R"input(
X_ERROR(0.1) 0 1
M 0 1
            )input"),
            {{"00", 0.9 * 0.9}, {"01", 0.9 * 0.1}, {"10", 0.9 * 0.1}, {"11", 0.1 * 0.1}}));

    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=10"}, R"input(
H 0 1
X_ERROR(0.1) 0 1
H 0 1
M 0 1
            )input"),
            {{"00", 1}}));
}

TEST(command_sample, sample_z_error) {
    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=100000"}, R"input(
H 0 1
Z_ERROR(0.1) 0 1
H 0 1
M 0 1
            )input"),
            {{"00", 0.9 * 0.9}, {"01", 0.9 * 0.1}, {"10", 0.9 * 0.1}, {"11", 0.1 * 0.1}}));

    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=10"}, R"input(
Z_ERROR(0.1) 0 1
M 0 1
            )input"),
            {{"00", 1}}));
}

TEST(command_sample, sample_y_error) {
    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=100000"}, R"input(
Y_ERROR(0.1) 0 1
M 0 1
            )input"),
            {{"00", 0.9 * 0.9}, {"01", 0.9 * 0.1}, {"10", 0.9 * 0.1}, {"11", 0.1 * 0.1}}));

    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=10"}, R"input(
H_YZ 0 1
Y_ERROR(0.1) 0 1
H_YZ 0 1
M 0 1
            )input"),
            {{"00", 1}}));
}

TEST(command_sample, sample_depolarize1_error) {
    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=100000"}, R"input(
DEPOLARIZE1(0.3) 0 1
M 0 1
            )input"),
            {{"00", 0.8 * 0.8}, {"01", 0.8 * 0.2}, {"10", 0.8 * 0.2}, {"11", 0.2 * 0.2}}));

    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=100000"}, R"input(
H 0 1
DEPOLARIZE1(0.3) 0 1
H 0 1
M 0 1
            )input"),
            {{"00", 0.8 * 0.8}, {"01", 0.8 * 0.2}, {"10", 0.8 * 0.2}, {"11", 0.2 * 0.2}}));

    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=100000"}, R"input(
H_YZ 0 1
DEPOLARIZE1(0.3) 0 1
H_YZ 0 1
M 0 1
            )input"),
            {{"00", 0.8 * 0.8}, {"01", 0.8 * 0.2}, {"10", 0.8 * 0.2}, {"11", 0.2 * 0.2}}));
}

TEST(command_sample, sample_depolarize2_error) {
    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=100000"}, R"input(
DEPOLARIZE2(0.1) 0 1
M 0 1
            )input"),
            {{"00", 0.1 * 3 / 15 + 0.9}, {"01", 0.1 * 4 / 15}, {"10", 0.1 * 4 / 15}, {"11", 0.1 * 4 / 15}}));

    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample=100000"}, R"input(
H 0
H_YZ 1
DEPOLARIZE2(0.3) 0 1
H 0
H_YZ 1
M 0 1
            )input"),
            {{"00", 0.3 * 3 / 15 + 0.7}, {"01", 0.3 * 4 / 15}, {"10", 0.3 * 4 / 15}, {"11", 0.3 * 4 / 15}}));
}

TEST(command_sample, sample_measure_reset) {
    ASSERT_EQ(
        trim(run_captured_stim_main({"--sample"}, R"input(
X 0
R 0
M 0
            )input")),
        trim(R"output(
0
            )output"));
    ASSERT_EQ(
        trim(run_captured_stim_main({"--sample"}, R"input(
X 0
MR 0
MR 0
            )input")),
        trim(R"output(
10
            )output"));
}

TEST(command_sample, skip_reference_sample_flag) {
    ASSERT_EQ(
        "",
        deviation(
            run_captured_stim_main({"--sample", "--skip_reference_sample"}, R"input(
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
            run_captured_stim_main({"--sample=10", "--skip_reference_sample"}, R"input(
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
            run_captured_stim_main({"--sample"}, R"input(
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
            run_captured_stim_main({"--sample=10"}, R"input(
H 0
S 0
S 0
H 0
M 0
            )input"),
            {{"1", 1}}));
}

TEST(command_sample, seeded_sampling) {
    ASSERT_EQ(
        run_captured_stim_main({"sample", "--shots=256", "--seed 5"}, R"input(
                H 0
                M 0
            )input"),
        run_captured_stim_main({"sample", "--shots=256", "--seed 5"}, R"input(
                H 0
                M 0
            )input"));

    ASSERT_NE(
        run_captured_stim_main({"sample", "--shots=256"}, R"input(
                H 0
                M 0
            )input"),
        run_captured_stim_main({"sample", "--shots=256"}, R"input(
                H 0
                M 0
            )input"));

    ASSERT_NE(
        run_captured_stim_main({"sample", "--shots=256", "--seed 5"}, R"input(
                H 0
                M 0
            )input"),
        run_captured_stim_main({"sample", "--shots=256", "--seed 6"}, R"input(
                H 0
                M 0
            )input"));
}
