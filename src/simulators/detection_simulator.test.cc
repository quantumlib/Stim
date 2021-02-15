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

#include "detection_simulator.h"

#include <gtest/gtest.h>

#include "../test_util.test.h"

TEST(DetectionSimulator, detector_samples) {
    auto circuit = Circuit::from_text(
        "X_ERROR(1) 0\n"
        "M 0\n"
        "DETECTOR 0@-1\n");
    auto samples = detector_samples(circuit, 5, false, false, SHARED_TEST_RNG());
    ASSERT_EQ(
        samples.str(5),
        "11111\n"
        ".....\n"
        ".....\n"
        ".....\n"
        ".....");
}

TEST(DetectionSimulator, bad_detector) {
    ASSERT_THROW(
        { detector_samples(Circuit::from_text("DETECTOR 0@-5"), 5, false, false, SHARED_TEST_RNG()); },
        std::out_of_range);
}
