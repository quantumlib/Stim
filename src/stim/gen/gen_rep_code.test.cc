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

#include "stim/gen/gen_rep_code.h"

#include "gtest/gtest.h"

using namespace stim;

TEST(gen_rep_code, rep_code) {
    CircuitGenParameters params(5000, 4, "memory");
    params.after_clifford_depolarization = 0.125;
    params.after_reset_flip_probability = 0.25;
    params.before_measure_flip_probability = 0.375;
    params.before_round_data_depolarization = 0.0625;
    auto out = generate_rep_code_circuit(params);
    ASSERT_EQ(out.layout_str(), R"LAYOUT(# L0 Z1 d2 Z3 d4 Z5 d6
)LAYOUT");
    ASSERT_EQ(
        out.circuit.str(),
        Circuit(R"CIRCUIT(
        R 0 1 2 3 4 5 6
        X_ERROR(0.25) 0 1 2 3 4 5 6
        TICK
        DEPOLARIZE1(0.0625) 0 2 4 6
        CX 0 1 2 3 4 5
        DEPOLARIZE2(0.125) 0 1 2 3 4 5
        TICK
        CX 2 1 4 3 6 5
        DEPOLARIZE2(0.125) 2 1 4 3 6 5
        TICK
        X_ERROR(0.375) 1 3 5
        MR 1 3 5
        X_ERROR(0.25) 1 3 5
        DETECTOR(1, 0) rec[-3]
        DETECTOR(3, 0) rec[-2]
        DETECTOR(5, 0) rec[-1]
        REPEAT 4999 {
            TICK
            DEPOLARIZE1(0.0625) 0 2 4 6
            CX 0 1 2 3 4 5
            DEPOLARIZE2(0.125) 0 1 2 3 4 5
            TICK
            CX 2 1 4 3 6 5
            DEPOLARIZE2(0.125) 2 1 4 3 6 5
            TICK
            X_ERROR(0.375) 1 3 5
            MR 1 3 5
            X_ERROR(0.25) 1 3 5
            SHIFT_COORDS(0, 1)
            DETECTOR(1, 0) rec[-3] rec[-6]
            DETECTOR(3, 0) rec[-2] rec[-5]
            DETECTOR(5, 0) rec[-1] rec[-4]
        }
        X_ERROR(0.375) 0 2 4 6
        M 0 2 4 6
        DETECTOR(1, 1) rec[-3] rec[-4] rec[-7]
        DETECTOR(3, 1) rec[-2] rec[-3] rec[-6]
        DETECTOR(5, 1) rec[-1] rec[-2] rec[-5]
        OBSERVABLE_INCLUDE(0) rec[-1]
    )CIRCUIT")
            .str());
}
