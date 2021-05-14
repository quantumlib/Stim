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
#include <regex>

#include "../test_util.test.h"
#include "frame_simulator.h"

using namespace stim_internal;

std::string convert(const char *text, bool use_basis_analysis = false) {
    FILE *f = tmpfile();
    ErrorFuser::convert_circuit_out(Circuit::from_text(text), f, use_basis_analysis);
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

static bool matches(std::string actual, std::string pattern) {
    // Hackily work around C++ regex not supporting multiline matching.
    std::replace(actual.begin(), actual.end(), '\n', 'X');
    std::replace(pattern.begin(), pattern.end(), '\n', 'X');
    return std::regex_match(actual, std::regex("^" + pattern + "$"));
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

    ASSERT_TRUE(matches(
        convert(R"circuit(
        DEPOLARIZE1(0.25) 3
        M 3
        DETECTOR rec[-1]
    )circuit"),
        R"graph(error\(0.1666666\d+\) D0
)graph"));

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

    ASSERT_TRUE(matches(
        convert(R"circuit(
        DEPOLARIZE2(0.25) 3 5
        M 3
        M 5
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    )circuit"),
        R"graph(error\(0.07182558\d+\) D0
error\(0.07182558\d+\) D0 D1
error\(0.07182558\d+\) D1
)graph"));

    ASSERT_TRUE(matches(
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
        R"graph(error\(0.019013\d+\) D0
error\(0.019013\d+\) D0 D1
error\(0.019013\d+\) D0 D1 D2
error\(0.019013\d+\) D0 D1 D2 D3
error\(0.019013\d+\) D0 D1 D3
error\(0.019013\d+\) D0 D2
error\(0.019013\d+\) D0 D2 D3
error\(0.019013\d+\) D0 D3
error\(0.019013\d+\) D1
error\(0.019013\d+\) D1 D2
error\(0.019013\d+\) D1 D2 D3
error\(0.019013\d+\) D1 D3
error\(0.019013\d+\) D2
error\(0.019013\d+\) D2 D3
error\(0.019013\d+\) D3
)graph"));

    ASSERT_TRUE(matches(
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
    )circuit", true),
        R"graph(edge D0
error\(0.019013\d+\) D0
error\(0.019013\d+\) D0 D1
error\(0.019013\d+\) D0 D1 D2
error\(0.019013\d+\) D0 D1 D2 D3
error\(0.019013\d+\) D0 D1 D3
error\(0.019013\d+\) D0 D2
error\(0.019013\d+\) D0 D2 D3
error\(0.019013\d+\) D0 D3
edge D1
error\(0.019013\d+\) D1
error\(0.019013\d+\) D1 D2
error\(0.019013\d+\) D1 D2 D3
error\(0.019013\d+\) D1 D3
edge D2
error\(0.019013\d+\) D2
error\(0.019013\d+\) D2 D3
edge D3
error\(0.019013\d+\) D3
)graph"));

    ASSERT_TRUE(matches(
        convert(R"circuit(
        H 0 1
        CNOT 0 2 1 3
        ZCX 0 10
        ZCX 0 11
        XCX 0 12
        XCX 0 13
        DEPOLARIZE2(0.25) 0 1
        ZCX 0 10
        ZCX 0 11
        XCX 0 12
        XCX 0 13
        M 10 11 12 13
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        DETECTOR rec[-3]
        DETECTOR rec[-4]
    )circuit", true),
        R"graph(edge D0 D1
error\(0.071825\d+\) D0 D1
error\(0.071825\d+\) D0 D1 D2 D3
edge D2 D3
error\(0.071825\d+\) D2 D3
)graph"));
}

TEST(ErrorFuser, unitary_gates_match_frame_simulator) {
    FrameSimulator f(16, 16, SIZE_MAX, SHARED_TEST_RNG());
    ErrorFuser e(16, false);
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

TEST(ErrorFuser, measure_reset_basis) {
    ASSERT_EQ(
        convert(R"circuit(
            RZ 0 1 2
            X_ERROR(0.25) 0
            Y_ERROR(0.25) 1
            Z_ERROR(0.25) 2
            MZ 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
        R"graph(error(0.25) D0
error(0.25) D1
)graph");

    ASSERT_EQ(
        convert(R"circuit(
            RX 0 1 2
            X_ERROR(0.25) 0
            Y_ERROR(0.25) 1
            Z_ERROR(0.25) 2
            MX 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
        R"graph(error(0.25) D1
error(0.25) D2
)graph");
    ASSERT_EQ(
        convert(R"circuit(
            RY 0 1 2
            X_ERROR(0.25) 0
            Y_ERROR(0.25) 1
            Z_ERROR(0.25) 2
            MY 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
        R"graph(error(0.25) D0
error(0.25) D2
)graph");

    ASSERT_EQ(
        convert(R"circuit(
            MRZ 0 1 2
            X_ERROR(0.25) 0
            Y_ERROR(0.25) 1
            Z_ERROR(0.25) 2
            MRZ 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
        R"graph(error(0.25) D0
error(0.25) D1
)graph");

    ASSERT_EQ(
        convert(R"circuit(
            MRX 0 1 2
            X_ERROR(0.25) 0
            Y_ERROR(0.25) 1
            Z_ERROR(0.25) 2
            MRX 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
        R"graph(error(0.25) D1
error(0.25) D2
)graph");
    ASSERT_EQ(
        convert(R"circuit(
            MRY 0 1 2
            X_ERROR(0.25) 0
            Y_ERROR(0.25) 1
            Z_ERROR(0.25) 2
            MRY 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
        R"graph(error(0.25) D0
error(0.25) D2
)graph");
}

TEST(ErrorFuser, repeated_measure_reset) {
    ASSERT_EQ(
        convert(R"circuit(
            MRZ 0 0
            X_ERROR(0.25) 0
            MRZ 0 0
            DETECTOR rec[-4]
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
        R"graph(error(0.25) D2
)graph");

    ASSERT_EQ(
        convert(R"circuit(
            MRY 0 0
            X_ERROR(0.25) 0
            MRY 0 0
            DETECTOR rec[-4]
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
        R"graph(error(0.25) D2
)graph");

    ASSERT_EQ(
        convert(R"circuit(
            MRX 0 0
            Z_ERROR(0.25) 0
            MRX 0 0
            DETECTOR rec[-4]
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
        R"graph(error(0.25) D2
)graph");
}

TEST(ErrorFuser, detect_bad_detectors) {
    ASSERT_ANY_THROW({
        convert(R"circuit(
            R 0
            H 0
            M 0
            DETECTOR rec[-1]
        )circuit");
    });

    ASSERT_ANY_THROW({
        convert(R"circuit(
            M 0
            H 0
            M 0
            DETECTOR rec[-1]
        )circuit");
    });

    ASSERT_ANY_THROW({
        convert(R"circuit(
            MZ 0
            MX 0
            DETECTOR rec[-1]
        )circuit");
    });

    ASSERT_ANY_THROW({
        convert(R"circuit(
            MY 0
            MX 0
            DETECTOR rec[-1]
        )circuit");
    });

    ASSERT_ANY_THROW({
        convert(R"circuit(
            MX 0
            MZ 0
            DETECTOR rec[-1]
        )circuit");
    });

    ASSERT_ANY_THROW({
        convert(R"circuit(
            RX 0
            MZ 0
            DETECTOR rec[-1]
        )circuit");
    });

    ASSERT_ANY_THROW({
        convert(R"circuit(
            RY 0
            MX 0
            DETECTOR rec[-1]
        )circuit");
    });

    ASSERT_ANY_THROW({
        convert(R"circuit(
            RZ 0
            MX 0
            DETECTOR rec[-1]
        )circuit");
    });
}
