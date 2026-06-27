/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stim.h"

#include "gtest/gtest.h"

TEST(stim, include1) {
    stim::Circuit c("H 0");
    ASSERT_EQ(c.count_qubits(), 1);
    ASSERT_EQ(stim::GATE_DATA.at("PAULI_CHANNEL_2").arg_count, 15);
}

TEST(stim, include3) {
    stim::ErrorAnalyzer::circuit_to_detector_error_model({}, false, false, false, 0.0, false, true);
}
