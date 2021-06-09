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

#include "gen_surface_code.h"

#include <gtest/gtest.h>

#include "../test_util.test.h"

using namespace stim_internal;

TEST(gen_surface_code, unrotated_surface_code_hard_coded_comparison) {
    CircuitGenParameters params(5, 2, "unrotated_memory_z");
    params.after_clifford_depolarization = 0.125;
    params.after_reset_flip_probability = 0.25;
    params.before_measure_flip_probability = 0.375;
    params.before_round_data_depolarization = 0.0625;
    auto out = generate_surface_code_circuit(params);
    ASSERT_EQ(
        out.layout_str(),
        ""
        "# L0 X1 L2\n"
        "# Z3 d4 Z5\n"
        "# d6 X7 d8\n");
    ASSERT_EQ(
        out.circuit.str(),
        Circuit::from_text(R"CIRCUIT(
        R 0 2 4 6 8
        X_ERROR(0.25) 0 2 4 6 8
        R 1 3 5 7
        X_ERROR(0.25) 1 3 5 7
        TICK
        DEPOLARIZE1(0.0625) 0 2 4 6 8
        H 1 7
        DEPOLARIZE1(0.125) 1 7
        TICK
        CX 1 2 7 8 4 3
        DEPOLARIZE2(0.125) 1 2 7 8 4 3
        TICK
        CX 1 4 6 3 8 5
        DEPOLARIZE2(0.125) 1 4 6 3 8 5
        TICK
        CX 7 4 0 3 2 5
        DEPOLARIZE2(0.125) 7 4 0 3 2 5
        TICK
        CX 1 0 7 6 4 5
        DEPOLARIZE2(0.125) 1 0 7 6 4 5
        TICK
        H 1 7
        DEPOLARIZE1(0.125) 1 7
        TICK
        X_ERROR(0.375) 1 3 5 7
        MR 1 3 5 7
        X_ERROR(0.25) 1 3 5 7
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        REPEAT 4 {
            TICK
            DEPOLARIZE1(0.0625) 0 2 4 6 8
            H 1 7
            DEPOLARIZE1(0.125) 1 7
            TICK
            CX 1 2 7 8 4 3
            DEPOLARIZE2(0.125) 1 2 7 8 4 3
            TICK
            CX 1 4 6 3 8 5
            DEPOLARIZE2(0.125) 1 4 6 3 8 5
            TICK
            CX 7 4 0 3 2 5
            DEPOLARIZE2(0.125) 7 4 0 3 2 5
            TICK
            CX 1 0 7 6 4 5
            DEPOLARIZE2(0.125) 1 0 7 6 4 5
            TICK
            H 1 7
            DEPOLARIZE1(0.125) 1 7
            TICK
            X_ERROR(0.375) 1 3 5 7
            MR 1 3 5 7
            X_ERROR(0.25) 1 3 5 7
            DETECTOR rec[-1] rec[-5]
            DETECTOR rec[-2] rec[-6]
            DETECTOR rec[-3] rec[-7]
            DETECTOR rec[-4] rec[-8]
        }
        X_ERROR(0.375) 0 2 4 6 8
        M 0 2 4 6 8
        DETECTOR rec[-2] rec[-3] rec[-5] rec[-8]
        DETECTOR rec[-1] rec[-3] rec[-4] rec[-7]
        OBSERVABLE_INCLUDE(0) rec[-4] rec[-5]
    )CIRCUIT")
            .str());

    params.rounds = 1;
    params.task = "unrotated_memory_x";
    auto out2 = generate_surface_code_circuit(params);
    ASSERT_EQ(
        out2.layout_str(),
        ""
        "# L0 X1 d2\n"
        "# Z3 d4 Z5\n"
        "# L6 X7 d8\n");
    ASSERT_EQ(
        out2.circuit.str(),
        Circuit::from_text(R"CIRCUIT(
        RX 0 2 4 6 8
        Z_ERROR(0.25) 0 2 4 6 8
        R 1 3 5 7
        X_ERROR(0.25) 1 3 5 7
        TICK
        DEPOLARIZE1(0.0625) 0 2 4 6 8
        H 1 7
        DEPOLARIZE1(0.125) 1 7
        TICK
        CX 1 2 7 8 4 3
        DEPOLARIZE2(0.125) 1 2 7 8 4 3
        TICK
        CX 1 4 6 3 8 5
        DEPOLARIZE2(0.125) 1 4 6 3 8 5
        TICK
        CX 7 4 0 3 2 5
        DEPOLARIZE2(0.125) 7 4 0 3 2 5
        TICK
        CX 1 0 7 6 4 5
        DEPOLARIZE2(0.125) 1 0 7 6 4 5
        TICK
        H 1 7
        DEPOLARIZE1(0.125) 1 7
        TICK
        X_ERROR(0.375) 1 3 5 7
        MR 1 3 5 7
        X_ERROR(0.25) 1 3 5 7
        DETECTOR rec[-4]
        DETECTOR rec[-1]
        Z_ERROR(0.375) 0 2 4 6 8
        MX 0 2 4 6 8
        DETECTOR rec[-3] rec[-4] rec[-5] rec[-9]
        DETECTOR rec[-1] rec[-2] rec[-3] rec[-6]
        OBSERVABLE_INCLUDE(0) rec[-2] rec[-5]
    )CIRCUIT")
            .str());
}

TEST(gen_surface_code, rotated_surface_code_hard_coded_comparison) {
    CircuitGenParameters params(5, 2, "rotated_memory_z");
    params.after_clifford_depolarization = 0.125;
    params.after_reset_flip_probability = 0.25;
    params.before_measure_flip_probability = 0.375;
    params.before_round_data_depolarization = 0.0625;
    auto out = generate_surface_code_circuit(params);
    ASSERT_EQ(
        out.layout_str(),
        ""
        "#         X2 \n"
        "#     L1      L3 \n"
        "#         Z7 \n"
        "#     d6      d8 \n"
        "#         X12\n");
    ASSERT_EQ(
        out.circuit.str(),
        Circuit::from_text(R"CIRCUIT(
        R 1 3 6 8
        X_ERROR(0.25) 1 3 6 8
        R 2 7 12
        X_ERROR(0.25) 2 7 12
        TICK
        DEPOLARIZE1(0.0625) 1 3 6 8
        H 2 12
        DEPOLARIZE1(0.125) 2 12
        TICK
        CX 2 3 8 7
        DEPOLARIZE2(0.125) 2 3 8 7
        TICK
        CX 2 1 3 7
        DEPOLARIZE2(0.125) 2 1 3 7
        TICK
        CX 12 8 6 7
        DEPOLARIZE2(0.125) 12 8 6 7
        TICK
        CX 12 6 1 7
        DEPOLARIZE2(0.125) 12 6 1 7
        TICK
        H 2 12
        DEPOLARIZE1(0.125) 2 12
        TICK
        X_ERROR(0.375) 2 7 12
        MR 2 7 12
        X_ERROR(0.25) 2 7 12
        DETECTOR rec[-2]
        REPEAT 4 {
            TICK
            DEPOLARIZE1(0.0625) 1 3 6 8
            H 2 12
            DEPOLARIZE1(0.125) 2 12
            TICK
            CX 2 3 8 7
            DEPOLARIZE2(0.125) 2 3 8 7
            TICK
            CX 2 1 3 7
            DEPOLARIZE2(0.125) 2 1 3 7
            TICK
            CX 12 8 6 7
            DEPOLARIZE2(0.125) 12 8 6 7
            TICK
            CX 12 6 1 7
            DEPOLARIZE2(0.125) 12 6 1 7
            TICK
            H 2 12
            DEPOLARIZE1(0.125) 2 12
            TICK
            X_ERROR(0.375) 2 7 12
            MR 2 7 12
            X_ERROR(0.25) 2 7 12
            DETECTOR rec[-1] rec[-4]
            DETECTOR rec[-2] rec[-5]
            DETECTOR rec[-3] rec[-6]
        }
        X_ERROR(0.375) 1 3 6 8
        M 1 3 6 8
        DETECTOR rec[-1] rec[-2] rec[-3] rec[-4] rec[-6]
        OBSERVABLE_INCLUDE(0) rec[-3] rec[-4]
    )CIRCUIT")
            .str());
}
