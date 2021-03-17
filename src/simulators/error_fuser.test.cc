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

#include "error_fuser.h"

#include <gtest/gtest.h>

#include "../test_util.test.h"
#include "frame_simulator.h"

#define ASSERT_APPROX_EQ(c1, c2, atol) ASSERT_TRUE(c1.approx_equals(c2, atol)) << c1

std::string convert(const char *text) {
    FILE *f = tmpfile();
    ErrorFuser::convert_circuit_out(Circuit::from_text(text), f);
    rewind(f);
    std::string s;
    while (true) {
        int c = getc(f);
        if (c == EOF) {
            break;
        }
        s.push_back(c);
    }
    return s;
}

TEST(ErrorFuser, convert_circuit) {
    ASSERT_EQ(
        convert(R"circuit(
        X_ERROR(0.25) 3
        M 3
        DETECTOR rec[-1]
    )circuit"),
        R"graph(error(0.25) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        Y_ERROR(0.25) 3
        M 3
        DETECTOR rec[-1]
    )circuit"),
        R"graph(error(0.25) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        Z_ERROR(0.25) 3
        M 3
        DETECTOR rec[-1]
    )circuit"),
        R"graph()graph");

    ASSERT_EQ(
        convert(R"circuit(
        DEPOLARIZE1(0.25) 3
        M 3
        DETECTOR rec[-1]
    )circuit"),
        R"graph(error(0.1666666666666666574) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        X_ERROR(0.25) 0
        X_ERROR(0.125) 1
        M 0 1
        OBSERVABLE_INCLUDE(3) rec[-1]
        DETECTOR rec[-2]
    )circuit"),
        R"graph(error(0.125) L3
error(0.25) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        X_ERROR(0.25) 0
        X_ERROR(0.125) 1
        M 0 1
        OBSERVABLE_INCLUDE(3) rec[-1]
        DETECTOR rec[-2]
    )circuit"),
        R"graph(error(0.125) L3
error(0.25) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        DEPOLARIZE2(0.25) 3 5
        M 3
        M 5
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    )circuit"),
        R"graph(error(0.07182558071116235121) D0
error(0.07182558071116235121) D0 D1
error(0.07182558071116235121) D1
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        H 0 1
        CNOT 0 2 1 3
        DEPOLARIZE2(0.25) 0 1
        CNOT 0 2 1 3
        H 0 1
        M 0 1 2 3
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        DETECTOR rec[-3]
        DETECTOR rec[-4]
    )circuit"),
        R"graph(error(0.01901372644820353841) D0
error(0.01901372644820353841) D0 D1
error(0.01901372644820353841) D0 D1 D2
error(0.01901372644820353841) D0 D1 D2 D3
error(0.01901372644820353841) D0 D1 D3
error(0.01901372644820353841) D0 D2
error(0.01901372644820353841) D0 D2 D3
error(0.01901372644820353841) D0 D3
error(0.01901372644820353841) D1
error(0.01901372644820353841) D1 D2
error(0.01901372644820353841) D1 D2 D3
error(0.01901372644820353841) D1 D3
error(0.01901372644820353841) D2
error(0.01901372644820353841) D2 D3
error(0.01901372644820353841) D3
)graph");
}

TEST(ErrorFuser, unitary_gates_match_frame_simulator) {
    FrameSimulator f(16, 16, SIZE_MAX, SHARED_TEST_RNG());
    ErrorFuser e(16);
    for (size_t q = 0; q < 16; q++) {
        if (q & 1) {
            e.xs[q].xor_item(0);
            f.x_table[q][0] = true;
        }
        if (q & 2) {
            e.xs[q].xor_item(1);
            f.x_table[q][1] = true;
        }
        if (q & 4) {
            e.zs[q].xor_item(0);
            f.z_table[q][0] = true;
        }
        if (q & 8) {
            e.zs[q].xor_item(1);
            f.z_table[q][1] = true;
        }
    }

    std::vector<uint32_t> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    OperationData targets = {0, data};
    for (const auto &gate : GATE_DATA.gates()) {
        if (gate.flags & GATE_IS_UNITARY) {
            (e.*gate.reverse_error_fuser_function)(targets);
            (f.*gate.frame_simulator_function)(targets);
            for (size_t q = 0; q < 16; q++) {
                bool xs[2]{};
                bool zs[2]{};
                for (auto x : e.xs[q]) {
                    ASSERT_TRUE(x < 2) << gate.name;
                    xs[x] = true;
                }
                for (auto z : e.zs[q]) {
                    ASSERT_TRUE(z < 2) << gate.name;
                    zs[z] = true;
                }
                ASSERT_EQ(f.x_table[q][0], xs[0]) << gate.name;
                ASSERT_EQ(f.x_table[q][1], xs[1]) << gate.name;
                ASSERT_EQ(f.z_table[q][0], zs[0]) << gate.name;
                ASSERT_EQ(f.z_table[q][1], zs[1]) << gate.name;
            }
        }
    }
}

TEST(ErrorFuser, reversed_operation_order) {
    ASSERT_EQ(
        convert(R"circuit(
        X_ERROR(0.25) 0
        CNOT 0 1
        CNOT 1 0
        M 0 1
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )circuit"),
        R"graph(error(0.25) D1
)graph");
}

TEST(ErrorFuser, classical_error_propagation) {
    ASSERT_EQ(
        convert(R"circuit(
        X_ERROR(0.125) 0
        M 0
        CNOT rec[-1] 1
        M 1
        DETECTOR rec[-1]
    )circuit"),
        R"graph(error(0.125) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        X_ERROR(0.125) 0
        M 0
        H 1
        CZ rec[-1] 1
        H 1
        M 1
        DETECTOR rec[-1]
    )circuit"),
        R"graph(error(0.125) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        X_ERROR(0.125) 0
        M 0
        H 1
        CZ 1 rec[-1]
        H 1
        M 1
        DETECTOR rec[-1]
    )circuit"),
        R"graph(error(0.125) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        X_ERROR(0.125) 0
        M 0
        CY rec[-1] 1
        M 1
        DETECTOR rec[-1]
    )circuit"),
        R"graph(error(0.125) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        X_ERROR(0.125) 0
        M 0
        XCZ 1 rec[-1]
        M 1
        DETECTOR rec[-1]
    )circuit"),
        R"graph(error(0.125) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        X_ERROR(0.125) 0
        M 0
        YCZ 1 rec[-1]
        M 1
        DETECTOR rec[-1]
    )circuit"),
        R"graph(error(0.125) D0
)graph");
}
