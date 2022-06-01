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

#include "stim/gen/gen_color_code.h"

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(gen_color_code, color_code_hard_coded_comparison) {
    CircuitGenParameters params(100, 5, "memory_xyz");
    params.after_clifford_depolarization = 0.125;
    params.after_reset_flip_probability = 0.25;
    params.before_measure_flip_probability = 0.375;
    params.before_round_data_depolarization = 0.0625;
    auto out = generate_color_code_circuit(params);
    ASSERT_EQ(
        out.layout_str(),
        ""
        "#                         d27\n"
        "#                     d25     R26\n"
        "#                 B22     d23     d24\n"
        "#             d18     d19     G20     d21\n"
        "#         d13     R14     d15     d16     R17\n"
        "#     B7      d8      d9      B10     d11     d12\n"
        "# L0      L1      G2      L3      L4      G5      L6 \n");
    params.distance = 3;
    out = generate_color_code_circuit(params);
    ASSERT_EQ(
        out.layout_str(),
        ""
        "#          d9\n"
        "#       d7    R8\n"
        "#    B4    d5    d6\n"
        "# L0    L1    G2    L3\n");
    ASSERT_EQ(
        out.circuit.str(),
        Circuit(R"CIRCUIT(
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(2, 0) 2
        QUBIT_COORDS(3, 0) 3
        QUBIT_COORDS(0.5, 1) 4
        QUBIT_COORDS(1.5, 1) 5
        QUBIT_COORDS(2.5, 1) 6
        QUBIT_COORDS(1, 2) 7
        QUBIT_COORDS(2, 2) 8
        QUBIT_COORDS(1.5, 3) 9
        R 0 1 2 3 4 5 6 7 8 9
        X_ERROR(0.25) 0 1 2 3 4 5 6 7 8 9
        REPEAT 2 {
            TICK
            DEPOLARIZE1(0.0625) 0 1 3 5 6 7 9
            C_XYZ 0 1 3 5 6 7 9
            DEPOLARIZE1(0.125) 0 1 3 5 6 7 9
            TICK
            CX 5 4 3 2
            DEPOLARIZE2(0.125) 5 4 3 2
            TICK
            CX 7 4 6 2
            DEPOLARIZE2(0.125) 7 4 6 2
            TICK
            CX 1 4 6 8
            DEPOLARIZE2(0.125) 1 4 6 8
            TICK
            CX 1 2 7 8
            DEPOLARIZE2(0.125) 1 2 7 8
            TICK
            CX 5 2 9 8
            DEPOLARIZE2(0.125) 5 2 9 8
            TICK
            CX 0 4 5 8
            DEPOLARIZE2(0.125) 0 4 5 8
            TICK
            X_ERROR(0.375) 2 4 8
            MR 2 4 8
            X_ERROR(0.25) 2 4 8
        }
        DETECTOR(2, 0, 0) rec[-3] rec[-6]
        DETECTOR(0.5, 1, 0) rec[-2] rec[-5]
        DETECTOR(2, 2, 0) rec[-1] rec[-4]
        REPEAT 98 {
            TICK
            DEPOLARIZE1(0.0625) 0 1 3 5 6 7 9
            C_XYZ 0 1 3 5 6 7 9
            DEPOLARIZE1(0.125) 0 1 3 5 6 7 9
            TICK
            CX 5 4 3 2
            DEPOLARIZE2(0.125) 5 4 3 2
            TICK
            CX 7 4 6 2
            DEPOLARIZE2(0.125) 7 4 6 2
            TICK
            CX 1 4 6 8
            DEPOLARIZE2(0.125) 1 4 6 8
            TICK
            CX 1 2 7 8
            DEPOLARIZE2(0.125) 1 2 7 8
            TICK
            CX 5 2 9 8
            DEPOLARIZE2(0.125) 5 2 9 8
            TICK
            CX 0 4 5 8
            DEPOLARIZE2(0.125) 0 4 5 8
            TICK
            X_ERROR(0.375) 2 4 8
            MR 2 4 8
            X_ERROR(0.25) 2 4 8
            SHIFT_COORDS(0, 0, 1)
            DETECTOR(2, 0, 0) rec[-3] rec[-6] rec[-9]
            DETECTOR(0.5, 1, 0) rec[-2] rec[-5] rec[-8]
            DETECTOR(2, 2, 0) rec[-1] rec[-4] rec[-7]
        }
        Z_ERROR(0.375) 0 1 3 5 6 7 9
        MX 0 1 3 5 6 7 9
        DETECTOR(2, 0, 1) rec[-3] rec[-4] rec[-5] rec[-6] rec[-13]
        DETECTOR(0.5, 1, 1) rec[-2] rec[-4] rec[-6] rec[-7] rec[-12]
        DETECTOR(2, 2, 1) rec[-1] rec[-2] rec[-3] rec[-4] rec[-11]
        OBSERVABLE_INCLUDE(0) rec[-5] rec[-6] rec[-7]
    )CIRCUIT")
            .str());
}
