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

#include "gtest/gtest.h"

#include "stim.h"  // The other include is in stim.test.cc; this is the second one.

TEST(stim, include2) {
    stim::Circuit c("H 0");
    ASSERT_EQ(c.count_qubits(), 1);
    ASSERT_EQ(stim::GATE_DATA.at("PAULI_CHANNEL_2").arg_count, 15);
}
