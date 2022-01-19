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

#include "stim/simulators/error_candidate_finder.h"

#include <gtest/gtest.h>

using namespace stim;

TEST(ErrorCandidateFinder, X_ERROR) {
    auto actual = ErrorCandidateFinder::candidate_localized_dem_errors_from_circuit(
        Circuit(R"CIRCUIT(
            X_ERROR(0.1) 0
            M 0
            DETECTOR rec[-1]
        )CIRCUIT"),
        DetectorErrorModel());
    std::vector<MatchedDetectorCircuitError> expected{
        MatchedDetectorCircuitError{
            {DemTarget::relative_detector_id(0)},
            UINT64_MAX,
            {GateTarget::x(0)},
            0,
            0,
            1,
            "X_ERROR",
            {0},
            {0},
        },
    };
    ASSERT_EQ(actual, expected);
}