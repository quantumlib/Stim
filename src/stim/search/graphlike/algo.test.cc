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

#include "stim/search/graphlike/algo.h"

#include "gtest/gtest.h"

#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/search/graphlike/edge.h"
#include "stim/simulators/error_analyzer.h"

using namespace stim;
using namespace stim::impl_search_graphlike;

TEST(shortest_graphlike_undetectable_logical_error, no_error) {
    // No error.
    ASSERT_THROW(
        { stim::shortest_graphlike_undetectable_logical_error(DetectorErrorModel(), false); }, std::invalid_argument);

    // No undetectable error.
    ASSERT_THROW(
        {
            stim::shortest_graphlike_undetectable_logical_error(
                DetectorErrorModel(R"MODEL(
            error(0.1) D0 L0
        )MODEL"),
                false);
        },
        std::invalid_argument);

    // No logical flips.
    ASSERT_THROW(
        {
            stim::shortest_graphlike_undetectable_logical_error(
                DetectorErrorModel(R"MODEL(
            error(0.1) D0
            error(0.1) D0 D1
            error(0.1) D1
        )MODEL"),
                false);
        },
        std::invalid_argument);
}

TEST(shortest_graphlike_undetectable_logical_error, distance_1) {
    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) L0
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) L0
        )MODEL"));
}

TEST(shortest_graphlike_undetectable_logical_error, distance_2) {
    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D0
                error(0.1) D0 L0
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0
            error(1) D0 L0
        )MODEL"));

    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D0 L0
                error(0.1) D0 L1
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0 L0
            error(1) D0 L1
        )MODEL"));

    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D0 D1 L0
                error(0.1) D0 D1 L1
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0 D1 L0
            error(1) D0 D1 L1
        )MODEL"));

    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D0 D1 L1
                error(0.1) D0 D1 L0
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0 D1 L0
            error(1) D0 D1 L1
        )MODEL"));
}

TEST(shortest_graphlike_undetectable_logical_error, distance_3) {
    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D0
                error(0.1) D0 D1 L0
                error(0.1) D1
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0
            error(1) D0 D1 L0
            error(1) D1
        )MODEL"));

    ASSERT_EQ(
        stim::shortest_graphlike_undetectable_logical_error(
            DetectorErrorModel(R"MODEL(
                error(0.1) D1
                error(0.1) D1 D0 L0
                error(0.1) D0
            )MODEL"),
            false),
        DetectorErrorModel(R"MODEL(
            error(1) D0
            error(1) D0 D1 L0
            error(1) D1
        )MODEL"));
}

TEST(shortest_graphlike_undetectable_logical_error, surface_code) {
    CircuitGenParameters params(5, 5, "rotated_memory_x");
    params.after_clifford_depolarization = 0.001;
    params.before_measure_flip_probability = 0.001;
    params.after_reset_flip_probability = 0.001;
    params.before_round_data_depolarization = 0.001;
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto graphlike_model = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, 0.0, false, true);
    auto ungraphlike_model =
        ErrorAnalyzer::circuit_to_detector_error_model(circuit, false, true, false, 0.0, false, true);

    ASSERT_EQ(stim::shortest_graphlike_undetectable_logical_error(graphlike_model, false).instructions.size(), 5);

    ASSERT_EQ(stim::shortest_graphlike_undetectable_logical_error(graphlike_model, true).instructions.size(), 5);

    ASSERT_EQ(stim::shortest_graphlike_undetectable_logical_error(ungraphlike_model, true).instructions.size(), 5);

    // Throw due to ungraphlike errors.
    ASSERT_THROW(
        { stim::shortest_graphlike_undetectable_logical_error(ungraphlike_model, false); }, std::invalid_argument);
}

TEST(shortest_graphlike_undetectable_logical_error, repetition_code) {
    CircuitGenParameters params(10, 7, "memory");
    params.before_round_data_depolarization = 0.01;
    auto circuit = generate_rep_code_circuit(params).circuit;
    auto graphlike_model = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, 0.0, false, true);

    ASSERT_EQ(stim::shortest_graphlike_undetectable_logical_error(graphlike_model, false).instructions.size(), 7);
}

TEST(shortest_graphlike_undetectable_logical_error, many_observables) {
    Circuit circuit(R"CIRCUIT(
        MPP Z0*Z1 Z1*Z2 Z2*Z3 Z3*Z4
        X_ERROR(0.1) 0 1 2 3 4
        MPP Z0*Z1 Z1*Z2 Z2*Z3 Z3*Z4
        DETECTOR rec[-1] rec[-5]
        DETECTOR rec[-2] rec[-6]
        DETECTOR rec[-3] rec[-7]
        DETECTOR rec[-4] rec[-8]
        M 4
        OBSERVABLE_INCLUDE(1200) rec[-1]
    )CIRCUIT");
    auto graphlike_model = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, 0.0, false, true);
    auto err = stim::shortest_graphlike_undetectable_logical_error(graphlike_model, false);
    ASSERT_EQ(err.instructions.size(), 5);
}
