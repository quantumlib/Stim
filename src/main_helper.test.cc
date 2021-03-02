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

#include "main_helper.h"

#include <gtest/gtest.h>

struct RaiiDeleteFile {
    char *c = nullptr;
    ~RaiiDeleteFile() {
        if (c != nullptr) {
            remove(c);
            c = nullptr;
        }
    }
};

std::string execute(std::vector<const char *> flags, const char *std_in_content) {
    // Setup input.
    RaiiDeleteFile raii_delete_file;
    char tmp_stdin_filename[] = "/tmp/stim_test_input_XXXXXX";
    if (std_in_content != nullptr) {
        int tmp_stdin_file_descriptor = mkstemp(tmp_stdin_filename);
        if (tmp_stdin_file_descriptor == -1) {
            throw std::runtime_error("Failed to create temporary stdin file.");
        }
        FILE *tmp_in = fdopen(tmp_stdin_file_descriptor, "w");
        if (tmp_in == nullptr) {
            throw std::runtime_error("Failed to open temporary stdin file.");
        }
        fprintf(tmp_in, "%s", std_in_content);
        fclose(tmp_in);
        raii_delete_file.c = tmp_stdin_filename;
        flags.insert(flags.begin(), tmp_stdin_filename);
        flags.insert(flags.begin(), "--in");
    }
    flags.insert(flags.begin(), "[PROGRAM_LOCATION_IGNORE]");

    // Setup output.
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    int result;
    try {
        result = main_helper(flags.size(), flags.data());
    } catch (const std::exception &e) {
        return "[exception=" + std::string(e.what()) + "]" + testing::internal::GetCapturedStdout() +
               testing::internal::GetCapturedStderr();
    }
    std::string out = testing::internal::GetCapturedStdout();
    std::string err = testing::internal::GetCapturedStderr();
    if (!err.empty()) {
        return "[stderr=" + err + "]";
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

TEST(main_helper, help_modes) {
    ASSERT_EQ(trim(execute({"--help"}, "")).substr(0, 19), "[stderr=BASIC USAGE");

    ASSERT_EQ(trim(execute({}, "")), trim(R"output(
[stderr=Need to specify exactly one of --sample or --repl or --detect or --detector_hypergraph
]
            )output"));

    ASSERT_EQ(trim(execute({"--sample", "--repl"}, "")), trim(R"output(
[stderr=Need to specify exactly one of --sample or --repl or --detect or --detector_hypergraph
]
            )output"));

    ASSERT_EQ(trim(execute({"--sample", "--repl", "--detect"}, "")), trim(R"output(
[stderr=Need to specify exactly one of --sample or --repl or --detect or --detector_hypergraph
]
            )output"));
}

TEST(main_helper, sample_flag) {
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

TEST(main_helper, intentional_failures) {
    ASSERT_EQ(
        "Sampled 10 which was not expected.", deviation(
                                                  execute({"--sample=1000"}, R"input(
X 0
M 0 1
            )input"),
                                                  {{"00", 0.5}, {"11", 0.5}}));

    ASSERT_NE(
        "Sampled 10 which was not expected.", deviation(
                                                  execute({"--sample=1000"}, R"input(
H 0
M 0
            )input"),
                                                  {{"0", 0.1}, {"1", 0.9}}));
}

TEST(main_helper, basic_distributions) {
    ASSERT_EQ(
        "", deviation(
                execute({"--sample=1000"}, R"input(
H 0
CNOT 0 1
M 0 1
            )input"),
                {{"00", 0.5}, {"11", 0.5}}));

    ASSERT_EQ(
        "", deviation(
                execute({"--sample=1000"}, R"input(
H 0
CNOT 0 1
SQRT_X 0 1
M 0 1
            )input"),
                {{"10", 0.5}, {"01", 0.5}}));

    ASSERT_EQ(
        "", deviation(
                execute({"--sample=1000"}, R"input(
H 0
CNOT 0 1
SQRT_Y 0 1
M 0 1
            )input"),
                {{"00", 0.5}, {"11", 0.5}}));
}

TEST(main_helper, sample_x_error) {
    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10000"}, R"input(
X_ERROR(0.1) 0 1
M 0 1
            )input"),
                {{"00", 0.9 * 0.9}, {"01", 0.9 * 0.1}, {"10", 0.9 * 0.1}, {"11", 0.1 * 0.1}}));

    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10"}, R"input(
H 0 1
X_ERROR(0.1) 0 1
H 0 1
M 0 1
            )input"),
                {{"00", 1}}));
}

TEST(main_helper, sample_z_error) {
    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10000"}, R"input(
H 0 1
Z_ERROR(0.1) 0 1
H 0 1
M 0 1
            )input"),
                {{"00", 0.9 * 0.9}, {"01", 0.9 * 0.1}, {"10", 0.9 * 0.1}, {"11", 0.1 * 0.1}}));

    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10"}, R"input(
Z_ERROR(0.1) 0 1
M 0 1
            )input"),
                {{"00", 1}}));
}

TEST(main_helper, sample_y_error) {
    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10000"}, R"input(
Y_ERROR(0.1) 0 1
M 0 1
            )input"),
                {{"00", 0.9 * 0.9}, {"01", 0.9 * 0.1}, {"10", 0.9 * 0.1}, {"11", 0.1 * 0.1}}));

    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10"}, R"input(
H_YZ 0 1
Y_ERROR(0.1) 0 1
H_YZ 0 1
M 0 1
            )input"),
                {{"00", 1}}));
}

TEST(main_helper, sample_depolarize1_error) {
    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10000"}, R"input(
DEPOLARIZE1(0.3) 0 1
M 0 1
            )input"),
                {{"00", 0.8 * 0.8}, {"01", 0.8 * 0.2}, {"10", 0.8 * 0.2}, {"11", 0.2 * 0.2}}));

    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10000"}, R"input(
H 0 1
DEPOLARIZE1(0.3) 0 1
H 0 1
M 0 1
            )input"),
                {{"00", 0.8 * 0.8}, {"01", 0.8 * 0.2}, {"10", 0.8 * 0.2}, {"11", 0.2 * 0.2}}));

    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10000"}, R"input(
H_YZ 0 1
DEPOLARIZE1(0.3) 0 1
H_YZ 0 1
M 0 1
            )input"),
                {{"00", 0.8 * 0.8}, {"01", 0.8 * 0.2}, {"10", 0.8 * 0.2}, {"11", 0.2 * 0.2}}));
}

TEST(main_helper, sample_depolarize2_error) {
    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10000"}, R"input(
DEPOLARIZE2(0.1) 0 1
M 0 1
            )input"),
                {{"00", 0.1 * 3 / 15 + 0.9}, {"01", 0.1 * 4 / 15}, {"10", 0.1 * 4 / 15}, {"11", 0.1 * 4 / 15}}));

    ASSERT_EQ(
        "", deviation(
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

TEST(main_helper, sample_measure_reset) {
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

TEST(main_helper, frame0_flag) {
    ASSERT_EQ(
        "", deviation(
                execute({"--sample", "--frame0"}, R"input(
H 0
S 0
S 0
H 0
M 0
            )input"),
                {{"0", 1}}));

    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10", "--frame0"}, R"input(
H 0
S 0
S 0
H 0
M 0
            )input"),
                {{"0", 1}}));

    ASSERT_EQ(
        "", deviation(
                execute({"--sample"}, R"input(
H 0
S 0
S 0
H 0
M 0
            )input"),
                {{"1", 1}}));

    ASSERT_EQ(
        "", deviation(
                execute({"--sample=10"}, R"input(
H 0
S 0
S 0
H 0
M 0
            )input"),
                {{"1", 1}}));
}

TEST(main_helper, detect_basic) {
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

TEST(main_helper, detector_error_sets) {
    ASSERT_EQ(
        trim(execute({"--detector_hypergraph"}, R"input(
X_ERROR(0.25) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
error(0.25) D0
            )output"));
}
