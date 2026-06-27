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

TEST(command_detect, detect_basic) {
    ASSERT_EQ(
        trim(run_captured_stim_main({"--detect"}, R"input(
M 0
            )input")),
        trim(R"output(
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--detect"}, R"input(
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
0
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--detect"}, R"input(
X_ERROR(1) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
1
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--detect", "2"}, R"input(
X_ERROR(1) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
1
1
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"detect", "--shots", "2"}, R"input(
X_ERROR(1) 0
M 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
1
1
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--detect"}, R"input(
DETECTOR
            )input")),
        trim(R"output(
0
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--detect"}, R"input(
X_ERROR(1) 0
MR 0
DETECTOR rec[-1]
            )input")),
        trim(R"output(
1
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--detect"}, R"input(
X_ERROR(1) 0
M 0
M 0
DETECTOR rec[-1] rec[-2]
            )input")),
        trim(R"output(
0
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--detect"}, R"input(
X_ERROR(1) 0
MR 0
MR 0
DETECTOR rec[-1] rec[-2]
            )input")),
        trim(R"output(
1
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main({"--detect"}, R"input(
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
        trim(run_captured_stim_main({"--detect", "--prepend_observables"}, R"input(
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
        trim(run_captured_stim_main({"--detect", "--prepend_observables"}, R"input(
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
        trim(run_captured_stim_main({"--detect", "--append_observables"}, R"input(
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

TEST(command_detect, detection_event_simulator_counts_measurements_correctly) {
    auto s = run_captured_stim_main({"--detect=1000"}, "MPP Z8*X9\nDETECTOR rec[-1]");
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

TEST(command_detect, seeded_detecting) {
    ASSERT_EQ(
        run_captured_stim_main({"detect", "--shots=256", "--seed 5"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"),
        run_captured_stim_main({"detect", "--shots=256", "--seed 5"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"));

    ASSERT_NE(
        run_captured_stim_main({"detect", "--shots=256"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"),
        run_captured_stim_main({"detect", "--shots=256"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"));

    ASSERT_NE(
        run_captured_stim_main({"detect", "--shots=256", "--seed 5"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"),
        run_captured_stim_main({"detect", "--shots=256", "--seed 6"}, R"input(
                X_ERROR(0.5) 0
                M 0
                DETECTOR rec[-1]
            )input"));
}
