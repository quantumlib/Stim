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

#include "stim/util_top/transform_without_feedback.h"

#include "gtest/gtest.h"

#include "stim/simulators/error_analyzer.h"

using namespace stim;

TEST(circuit_with_inlined_feedback, basic) {
    ASSERT_EQ(
        circuit_with_inlined_feedback(Circuit(R"CIRCUIT(
            MR 0
            H 0
            CX sweep[5] 0
            CY rec[-1] 0 rec[-1] 0 2 3 rec[-1] 0
            H 0
            M 0
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(2) rec[-1]
        )CIRCUIT")),
        Circuit(R"CIRCUIT(
            MR 0
            H 0
            CX sweep[5] 0
            OBSERVABLE_INCLUDE(2) rec[-1]
            CY 2 3
            H 0
            M 0
            DETECTOR rec[-2] rec[-1]
            OBSERVABLE_INCLUDE(2) rec[-1]
        )CIRCUIT"));
}

TEST(circuit_with_inlined_feedback, demolition_feedback) {
    Circuit inp = Circuit(R"CIRCUIT(
        CX 0 1
        M 1
        CX rec[-1] 1
        CX 0 1
        M 1
        DETECTOR rec[-1] rec[-2]
        OBSERVABLE_INCLUDE(0) rec[-1]
    )CIRCUIT");
    ASSERT_EQ(circuit_with_inlined_feedback(inp), Circuit(R"CIRCUIT(
        CX 0 1
        M 1
        OBSERVABLE_INCLUDE(0) rec[-1]
        CX 0 1
        M 1
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(0) rec[-1]
    )CIRCUIT"));
}

TEST(circuit_with_inlined_feedback, loop) {
    Circuit inp = Circuit(R"CIRCUIT(
        R 0 1
        X_ERROR(0.125) 0 1
        CX 0 1
        M 1
        CX rec[-1] 1
        DETECTOR rec[-1]
        REPEAT 30 {
            X_ERROR(0.125) 0 1
            CX 0 1
            M 1
            CX rec[-1] 1
            DETECTOR rec[-1] rec[-2]
        }

        M 0
        DETECTOR rec[-1] rec[-2]
    )CIRCUIT");
    auto actual = circuit_with_inlined_feedback(inp);

    auto dem1 = ErrorAnalyzer::circuit_to_detector_error_model(inp, true, true, false, 0, false, true);
    auto dem2 = ErrorAnalyzer::circuit_to_detector_error_model(actual, true, true, false, 0, false, true);
    dem1 = dem1.flattened();
    dem2 = dem2.flattened();
    ASSERT_TRUE(dem1.approx_equals(dem2, 1e-5));

    ASSERT_EQ(actual, Circuit(R"CIRCUIT(
        R 0 1
        X_ERROR(0.125) 0 1
        CX 0 1
        M 1
        DETECTOR rec[-1]

        X_ERROR(0.125) 0 1
        CX 0 1
        M 1
        DETECTOR rec[-1]

        REPEAT 29 {
            X_ERROR(0.125) 0 1
            CX 0 1
            M 1
            DETECTOR rec[-3] rec[-1]
        }

        M 0
        DETECTOR rec[-3] rec[-2] rec[-1]
    )CIRCUIT"));
}

TEST(circuit_with_inlined_feedback, mpp) {
    Circuit inp = Circuit(R"CIRCUIT(
        RX 0
        RY 1
        RZ 2
        MPP X0*Y1*Z2 Z5
        CX rec[-2] 3
        M 3
        DETECTOR rec[-1]
    )CIRCUIT");
    auto actual = circuit_with_inlined_feedback(inp);

    auto dem1 = ErrorAnalyzer::circuit_to_detector_error_model(inp, true, true, false, 0, false, true);
    auto dem2 = ErrorAnalyzer::circuit_to_detector_error_model(actual, true, true, false, 0, false, true);
    dem1 = dem1.flattened();
    dem2 = dem2.flattened();
    ASSERT_TRUE(dem1.approx_equals(dem2, 1e-5));

    ASSERT_EQ(actual, Circuit(R"CIRCUIT(
        RX 0
        RY 1
        R 2
        MPP X0*Y1*Z2 Z5
        M 3
        DETECTOR rec[-3] rec[-1]
    )CIRCUIT"));
}

TEST(circuit_with_inlined_feedback, interleaved_feedback_does_not_reorder_operations) {
    ASSERT_EQ(
        circuit_with_inlined_feedback(Circuit(R"CIRCUIT(
        H 0
        CZ
        H 1
    )CIRCUIT")),
        Circuit(R"CIRCUIT(
        H 0 1
    )CIRCUIT"));

    ASSERT_EQ(
        circuit_with_inlined_feedback(Circuit(R"CIRCUIT(
        M 0
        CX
        M 1
    )CIRCUIT")),
        Circuit(R"CIRCUIT(
        M 0 1
    )CIRCUIT"));

    ASSERT_EQ(
        circuit_with_inlined_feedback(Circuit(R"CIRCUIT(
        M 0 1
        CX
        M 2
        CX rec[-1] 3
        M 3
        DETECTOR rec[-1]
    )CIRCUIT")),
        Circuit(R"CIRCUIT(
        M 0 1 2 3
        DETECTOR rec[-2] rec[-1]
    )CIRCUIT"));
}
