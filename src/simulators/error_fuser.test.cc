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

#include "../gen/gen_rep_code.h"
#include "../test_util.test.h"
#include "frame_simulator.h"

using namespace stim_internal;

std::string convert(
    const Circuit &circuit,
    bool find_reducible_errors = false,
    bool fold_loops = false,
    bool validate_detectors = true) {
    return "\n" +
           ErrorFuser::circuit_to_detector_error_model(circuit, find_reducible_errors, fold_loops, validate_detectors)
               .str();
}

std::string convert(
    const char *text, bool find_reducible_errors = false, bool fold_loops = false, bool validate_detectors = true) {
    return convert(Circuit::from_text(text), find_reducible_errors, fold_loops, validate_detectors);
}

static std::string check_matches(std::string actual, std::string pattern) {
    auto old_actual = actual;
    auto old_pattern = pattern;
    // Hackily work around C++ regex not supporting multiline matching.
    std::replace(actual.begin(), actual.end(), '\n', 'X');
    std::replace(pattern.begin(), pattern.end(), '\n', 'X');
    if (!std::regex_match(actual, std::regex("^" + pattern + "$"))) {
        return "Expected:\n\n" + old_pattern + "\n\nbut got\n\n" + old_actual;
    }
    return "";
}

TEST(ErrorFuser, convert_circuit) {
    ASSERT_EQ(
        convert(R"circuit(
                X_ERROR(0.25) 3
                M 3
                DETECTOR rec[-1]
            )circuit"),
        R"graph(
error(0.25) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
                X_ERROR(0.25) 3
                M 3
                DETECTOR rec[-1]
                OBSERVABLE_INCLUDE(0) rec[-1]
            )circuit"),
        R"graph(
error(0.25) D0 L0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
                Y_ERROR(0.25) 3
                M 3
                DETECTOR rec[-1]
            )circuit"),
        R"graph(
error(0.25) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
        Z_ERROR(0.25) 3
        M 3
        DETECTOR rec[-1]
    )circuit"),
        R"graph(
)graph");

    ASSERT_EQ(
        "",
        check_matches(
            convert(R"circuit(
                DEPOLARIZE1(0.25) 3
                M 3
                DETECTOR rec[-1]
            )circuit"),
            R"graph(
error\(0.1666666\d+\) D0
)graph"));

    ASSERT_EQ(
        convert(R"circuit(
                X_ERROR(0.25) 0
                X_ERROR(0.125) 1
                M 0 1
                OBSERVABLE_INCLUDE(3) rec[-1]
                DETECTOR rec[-2]
            )circuit"),
        R"graph(
error(0.25) D0
error(0.125) L3
)graph");

    ASSERT_EQ(
        convert(R"circuit(
                X_ERROR(0.25) 0
                X_ERROR(0.125) 1
                M 0 1
                OBSERVABLE_INCLUDE(3) rec[-1]
                DETECTOR rec[-2]
            )circuit"),
        R"graph(
error(0.25) D0
error(0.125) L3
)graph");

    ASSERT_EQ(
        "",
        check_matches(
            convert(R"circuit(
                DEPOLARIZE2(0.25) 3 5
                M 3
                M 5
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit"),
            R"graph(
error\(0.07182558\d+\) D0
error\(0.07182558\d+\) D0 D1
error\(0.07182558\d+\) D1
)graph"));

    ASSERT_EQ(
        "",
        check_matches(
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
            R"graph(
error\(0.019013\d+\) D0
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

    ASSERT_EQ(
        "",
        check_matches(
            convert(
                R"circuit(
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
                )circuit",
                true),
            R"graph(
error\(0.019013\d+\) D0
error\(0.019013\d+\) D1
reducible_error\(0.019013\d+\) D1 \^ D0
reducible_error\(0.019013\d+\) D1 \^ D2
reducible_error\(0.019013\d+\) D1 \^ D2 \^ D0
error\(0.019013\d+\) D2
reducible_error\(0.019013\d+\) D2 \^ D0
error\(0.019013\d+\) D3
reducible_error\(0.019013\d+\) D3 \^ D0
reducible_error\(0.019013\d+\) D3 \^ D1
reducible_error\(0.019013\d+\) D3 \^ D1 \^ D0
reducible_error\(0.019013\d+\) D3 \^ D1 \^ D2
reducible_error\(0.019013\d+\) D3 \^ D1 \^ D2 \^ D0
reducible_error\(0.019013\d+\) D3 \^ D2
reducible_error\(0.019013\d+\) D3 \^ D2 \^ D0
)graph"));

    ASSERT_EQ(
        "",
        check_matches(
            convert(
                R"circuit(
                    H 0 1
                    CNOT 0 2 1 3
                    # Perform depolarizing error in a different basis.
                    ZCX 0 10
                    ZCX 0 11
                    XCX 0 12
                    XCX 0 13
                    DEPOLARIZE2(0.25) 0 1
                    XCX 0 13
                    XCX 0 12
                    ZCX 0 11
                    ZCX 0 10
                    # Check where error is.
                    M 10 11 12 13
                    DETECTOR rec[-1]
                    DETECTOR rec[-2]
                    DETECTOR rec[-3]
                    DETECTOR rec[-4]
                )circuit",
                true,
                false,
                false),
            R"graph(
error\(0.071825\d+\) D0 D1
reducible_error\(0.071825\d+\) D0 D1 \^ D2 D3
error\(0.071825\d+\) D2 D3
)graph"));
}

TEST(ErrorFuser, unitary_gates_match_frame_simulator) {
    FrameSimulator f(16, 16, SIZE_MAX, SHARED_TEST_RNG());
    ErrorFuser e(16, false, false, true);
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
            (f.*gate.inverse().frame_simulator_function)(targets);
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
        R"graph(
error(0.25) D1
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
        R"graph(
error(0.125) D0
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
        R"graph(
error(0.125) D0
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
        R"graph(
error(0.125) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
            X_ERROR(0.125) 0
            M 0
            CY rec[-1] 1
            M 1
            DETECTOR rec[-1]
        )circuit"),
        R"graph(
error(0.125) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
            X_ERROR(0.125) 0
            M 0
            XCZ 1 rec[-1]
            M 1
            DETECTOR rec[-1]
        )circuit"),
        R"graph(
error(0.125) D0
)graph");

    ASSERT_EQ(
        convert(R"circuit(
            X_ERROR(0.125) 0
            M 0
            YCZ 1 rec[-1]
            M 1
            DETECTOR rec[-1]
        )circuit"),
        R"graph(
error(0.125) D0
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
        R"graph(
error(0.25) D0
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
        R"graph(
error(0.25) D1
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
        R"graph(
error(0.25) D0
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
        R"graph(
error(0.25) D0
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
        R"graph(
error(0.25) D1
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
        R"graph(
error(0.25) D0
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
        R"graph(
error(0.25) D2
)graph");

    ASSERT_EQ(
        convert(R"circuit(
                RY 0 0
                MRY 0 0
                X_ERROR(0.25) 0
                MRY 0 0
                DETECTOR rec[-4]
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )circuit"),
        R"graph(
error(0.25) D2
)graph");

    ASSERT_EQ(
        convert(R"circuit(
                RX 0 0
                MRX 0 0
                Z_ERROR(0.25) 0
                MRX 0 0
                DETECTOR rec[-4]
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )circuit"),
        R"graph(
error(0.25) D2
)graph");
}

TEST(ErrorFuser, period_3_gates) {
    ASSERT_EQ(
        convert(R"circuit(
                R 0 1 2
                C_XYZ 0 1 2
                X_ERROR(1) 0
                Y_ERROR(1) 1
                Z_ERROR(1) 2
                C_ZYX 0 1 2
                M 0 1 2
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )circuit"),
        R"graph(
error(1) D1
error(1) D2
)graph");

    ASSERT_EQ(
        convert(R"circuit(
                R 0 1 2
                C_ZYX 0 1 2
                X_ERROR(1) 0
                Y_ERROR(1) 1
                Z_ERROR(1) 2
                C_XYZ 0 1 2
                M 0 1 2
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )circuit"),
        R"graph(
error(1) D0
error(1) D2
)graph");
}

TEST(ErrorFuser, detect_bad_detectors) {
    ASSERT_ANY_THROW({
        convert(
            R"circuit(
                R 0
                H 0
                M 0
                DETECTOR rec[-1]
            )circuit",
            false,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        convert(
            R"circuit(
                M 0
                H 0
                M 0
                DETECTOR rec[-1]
            )circuit",
            false,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        convert(
            R"circuit(
                MZ 0
                MX 0
                DETECTOR rec[-1]
            )circuit",
            false,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        convert(
            R"circuit(
                MY 0
                MX 0
                DETECTOR rec[-1]
            )circuit",
            false,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        convert(
            R"circuit(
                MX 0
                MZ 0
                DETECTOR rec[-1]
            )circuit",
            false,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        convert(
            R"circuit(
                RX 0
                MZ 0
                DETECTOR rec[-1]
            )circuit",
            false,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        convert(
            R"circuit(
                RY 0
                MX 0
                DETECTOR rec[-1]
            )circuit",
            false,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        convert(
            R"circuit(
                RZ 0
                MX 0
                DETECTOR rec[-1]
            )circuit",
            false,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        convert(
            R"circuit(
                MX 0
                DETECTOR rec[-1]
            )circuit",
            false,
            false,
            true);
    });
}

TEST(ErrorFuser, handle_gauge_detectors_when_not_validating) {
    ASSERT_EQ(
        convert(
            R"circuit(
                H 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit",
            false,
            false,
            false),
        R"graph(
error(0.5) D0 D1
)graph");

    ASSERT_EQ(
        convert(
            R"circuit(
                R 0
                H 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit",
            false,
            false,
            false),
        R"graph(
error(0.5) D0 D1
)graph");

    ASSERT_EQ(
        convert(
            R"circuit(
                RX 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit",
            false,
            false,
            false),
        R"graph(
error(0.5) D0 D1
)graph");

    ASSERT_EQ(
        convert(
            R"circuit(
                RY 0
                H_XY 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit",
            false,
            false,
            false),
        R"graph(
error(0.5) D0 D1
)graph");

    ASSERT_EQ(
        convert(
            R"circuit(
                MR 0
                H 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit",
            false,
            false,
            false),
        R"graph(
error(0.5) D0 D1
)graph");

    ASSERT_EQ(
        convert(
            R"circuit(
                MRX 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit",
            false,
            false,
            false),
        R"graph(
error(0.5) D0 D1
)graph");

    ASSERT_EQ(
        convert(
            R"circuit(
                MRY 0
                H_XY 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit",
            false,
            false,
            false),
        R"graph(
error(0.5) D0 D1
)graph");

    ASSERT_EQ(
        convert(
            R"circuit(
                M 0
                H 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit",
            false,
            false,
            false),
        R"graph(
error(0.5) D0 D1
)graph");

    ASSERT_EQ(
        convert(
            R"circuit(
                MX 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit",
            false,
            false,
            false),
        R"graph(
error(0.5) D0 D1
)graph");

    ASSERT_EQ(
        convert(
            R"circuit(
            MY 0
            H_XY 0
            CNOT 0 1
            M 0 1
            DETECTOR rec[-1]
            DETECTOR rec[-2]
        )circuit",
            false,
            false,
            false),
        R"graph(
error(0.5) D0 D1
)graph");
}

TEST(ErrorFuser, composite_error_analysis) {
    auto measure_stabilizers = Circuit::from_text(R"circuit(
        XCX 0 1 0 3 0 4
        MR 0
        XCZ 0 1 0 2 0 4 0 5
        MR 0
        XCX 0 2 0 5 0 6
        MR 0
        XCZ 0 3 0 4 0 7
        MR 0
        XCX 0 4 0 5 0 7 0 8
        MR 0
        XCZ 0 5 0 6 0 7
        MR 0
    )circuit");
    auto detectors = Circuit::from_text(R"circuit(
        DETECTOR rec[-6] rec[-12]
        DETECTOR rec[-5] rec[-11]
        DETECTOR rec[-4] rec[-10]
        DETECTOR rec[-3] rec[-9]
        DETECTOR rec[-2] rec[-8]
        DETECTOR rec[-1] rec[-7]
    )circuit");
    // .  1  2  .
    //  X0 Z1 X2
    // 3  4  5  6
    //  Z3 X4 Z5
    // .  7  8  .

    auto encode = measure_stabilizers;
    auto decode = measure_stabilizers + detectors;
    ASSERT_EQ(
        "",
        check_matches(
            convert(encode + Circuit::from_text("DEPOLARIZE1(0.01) 4") + decode, true),
            R"graph(
error\(0.0033445\d+\) D0 D4
reducible_error\(0.0033445\d+\) D0 D4 \^ D1 D3
error\(0.0033445\d+\) D1 D3
)graph"));

    ASSERT_EQ(
        "",
        check_matches(
            convert(encode + Circuit::from_text("DEPOLARIZE2(0.01) 4 5") + decode, true),
            R"graph(
error\(0.000669\d+\) D0 D2
reducible_error\(0.000669\d+\) D0 D2 \^ D1 D3
reducible_error\(0.000669\d+\) D0 D2 \^ D1 D5
reducible_error\(0.000669\d+\) D0 D2 \^ D3 D5
error\(0.000669\d+\) D0 D4
reducible_error\(0.000669\d+\) D0 D4 \^ D1 D3
reducible_error\(0.000669\d+\) D0 D4 \^ D1 D5
reducible_error\(0.000669\d+\) D0 D4 \^ D3 D5
error\(0.000669\d+\) D1 D3
reducible_error\(0.000669\d+\) D1 D3 \^ D2 D4
error\(0.000669\d+\) D1 D5
reducible_error\(0.000669\d+\) D1 D5 \^ D2 D4
error\(0.000669\d+\) D2 D4
reducible_error\(0.000669\d+\) D2 D4 \^ D3 D5
error\(0.000669\d+\) D3 D5
)graph"));

    auto expected = R"graph(
error\(0.000669\d+\) D0 D1 D2 D3
error\(0.000669\d+\) D0 D1 D2 D5
error\(0.000669\d+\) D0 D1 D3 D4
error\(0.000669\d+\) D0 D1 D4 D5
error\(0.000669\d+\) D0 D2
error\(0.000669\d+\) D0 D2 D3 D5
error\(0.000669\d+\) D0 D3 D4 D5
error\(0.000669\d+\) D0 D4
error\(0.000669\d+\) D1 D2 D3 D4
error\(0.000669\d+\) D1 D2 D4 D5
error\(0.000669\d+\) D1 D3
error\(0.000669\d+\) D1 D5
error\(0.000669\d+\) D2 D3 D4 D5
error\(0.000669\d+\) D2 D4
error\(0.000669\d+\) D3 D5
)graph";
    ASSERT_EQ(
        "", check_matches(convert(encode + Circuit::from_text("DEPOLARIZE2(0.01) 4 5") + decode, false), expected));
    ASSERT_EQ(
        "",
        check_matches(
            convert(encode + Circuit::from_text("CNOT 4 5\nDEPOLARIZE2(0.01) 4 5\nCNOT 4 5") + decode, false),
            expected));
    ASSERT_EQ(
        "",
        check_matches(
            convert(
                encode + Circuit::from_text("H_XY 4\nCNOT 4 5\nDEPOLARIZE2(0.01) 4 5\nCNOT 4 5\nH_XY 4") + decode,
                false),
            expected));
}

TEST(ErrorFuser, loop_folding) {
    ASSERT_EQ(
        ErrorFuser::circuit_to_detector_error_model(
            Circuit::from_text(R"CIRCUIT(
                MR 1
                REPEAT 12345678987654321 {
                    X_ERROR(0.25) 0
                    CNOT 0 1
                    MR 1
                    DETECTOR rec[-2] rec[-1]
                }
                M 0
                OBSERVABLE_INCLUDE(9) rec[-1]
            )CIRCUIT"),
            false,
            true,
            true),
        DetectorErrorModel(R"MODEL(
                error(0.25) D0 L9
                REPEAT 6172839493827159 {
                    error(0.25) D1+t L9
                    error(0.25) D2+t L9
                    TICK 2
                }
                error(0.25) D12345678987654319 L9
                error(0.25) D12345678987654320 L9
            )MODEL"));

    // Solve period 8 logical observable oscillation.
    ASSERT_EQ(
        ErrorFuser::circuit_to_detector_error_model(
            Circuit::from_text(R"CIRCUIT(
            R 0 1 2 3 4
            REPEAT 12345678987654321 {
                CNOT 0 1 1 2 2 3 3 4
                DETECTOR
            }
            M 4
            OBSERVABLE_INCLUDE(9) rec[-1]
        )CIRCUIT"),
            false,
            true,
            true),
        DetectorErrorModel(R"MODEL(
            REPEAT 1543209873456789 {
                TICK 8
            }
        )MODEL"));

    // Solve period 127 logical observable oscillation.
    ASSERT_EQ(
        ErrorFuser::circuit_to_detector_error_model(
            Circuit::from_text(R"CIRCUIT(
            R 0 1 2 3 4 5 6
            REPEAT 12345678987654321 {
                CNOT 0 1 1 2 2 3 3 4 4 5 5 6 6 0
                DETECTOR
            }
            M 6
            OBSERVABLE_INCLUDE(9) rec[-1]
            R 7
            X_ERROR(1) 7
            M 7
            DETECTOR rec[-1]
        )CIRCUIT"),
            false,
            true,
            true),
        DetectorErrorModel(R"MODEL(
            REPEAT 97210070768930 {
                TICK 127
            }
            error(1) D12345678987654321
        )MODEL"));
}

TEST(ErrorFuser, loop_folding_nested_loop) {
    ASSERT_EQ(
        ErrorFuser::circuit_to_detector_error_model(
            Circuit::from_text(R"CIRCUIT(
                MR 1
                REPEAT 1000 {
                    REPEAT 1000 {
                        X_ERROR(0.25) 0
                        CNOT 0 1
                        MR 1
                        DETECTOR rec[-2] rec[-1]
                    }
                }
                M 0
                OBSERVABLE_INCLUDE(9) rec[-1]
            )CIRCUIT"),
            false,
            true,
            true),
        DetectorErrorModel(R"MODEL(
                REPEAT 999 {
                    REPEAT 1000 {
                        error(0.25) D0+t L9
                        TICK 1
                    }
                }
                REPEAT 499 {
                    error(0.25) D0+t L9
                    error(0.25) D1+t L9
                    TICK 2
                }
                error(0.25) D999998 L9
                error(0.25) D999999 L9
            )MODEL"));
}

TEST(ErrorFuser, loop_folding_rep_code_circuit) {
    CircuitGenParameters params(100000, 3, "memory");
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_rep_code_circuit(params).circuit;

    auto actual = ErrorFuser::circuit_to_detector_error_model(circuit, true, true, true);
    auto expected = DetectorErrorModel(R"MODEL(
        error(0.00026) D0 D1
        error(0.00026) D0 D3
        error(0.00026) D0 L0
        error(0.00026) D1 D2
        error(0.00053) D1 D3
        error(0.00053) D1 D4
        error(0.00026) D2
        error(0.00053) D2 D4
        error(0.00026) D2 D5
        error(0.00026) D3 D4
        error(0.00026) D3 L0
        reducible_error(0.00026) D3 L0 ^ D0 L0
        error(0.00026) D4 D5
        error(0.00026) D5
        reducible_error(0.00026) D5 ^ D2
        REPEAT 99998 {
            error(0.000266) D3+t D4+t
            error(0.000266) D3+t D6+t
            error(0.000266) D3+t L0
            error(0.000266) D4+t D5+t
            error(0.000533) D4+t D6+t
            error(0.000533) D4+t D7+t
            error(0.000266) D5+t
            error(0.000533) D5+t D7+t
            error(0.000266) D5+t D8+t
            error(0.000266) D6+t D7+t
            error(0.000266) D6+t L0
            reducible_error(0.000266) D6+t L0 ^ D3+t L0
            error(0.000266) D7+t D8+t
            error(0.000266) D8+t
            reducible_error(0.000266) D8+t ^ D5+t
            TICK 3
        }
        error(0.000266) D299997 D299998
        error(0.000266) D299997 D300000
        error(0.000266) D299997 L0
        error(0.000266) D299998 D299999
        error(0.000533) D299998 D300000
        error(0.000533) D299998 D300001
        error(0.000266) D299999
        error(0.000533) D299999 D300001
        error(0.000266) D299999 D300002
        error(0.000266) D300000 D300001
        error(0.000266) D300000 L0
        reducible_error(0.000266) D300000 L0 ^ D299997 L0
        error(0.000266) D300001 D300002
        error(0.000266) D300002
        reducible_error(0.000266) D300002 ^ D299999
    )MODEL");
    ASSERT_TRUE(actual.approx_equals(expected, 0.00001));
}

TEST(ErrorFuser, reduce_error_detector_dependence_error_message) {
    ASSERT_THROW(
        {
            try {
                convert(
                    R"CIRCUIT(
                        R 0
                        DEPOLARIZE1(0.01) 0
                        M 0
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                        DETECTOR rec[-1]
                    )CIRCUIT",
                    true,
                    false);
            } catch (const std::out_of_range &e) {
                EXPECT_EQ("", check_matches(e.what(), ".*error involves too many detectors.*"));
                throw;
            }
        },
        std::out_of_range);
}

TEST(ErrorFuser, multi_round_gauge_detectors_dont_grow) {
    ASSERT_EQ(
        ErrorFuser::circuit_to_detector_error_model(
            Circuit::from_text(R"CIRCUIT(
                # Distance 2 Bacon-Shor.
                ZCX 0 10 1 10
                ZCX 2 11 3 11
                XCX 0 12 2 12
                XCX 1 13 3 13
                MR 10 11 12 13
                REPEAT 5 {
                    ZCX 0 10 1 10
                    ZCX 2 11 3 11
                    XCX 0 12 2 12
                    XCX 1 13 3 13
                    MR 10 11 12 13
                    DETECTOR rec[-1] rec[-5]
                    DETECTOR rec[-2] rec[-6]
                    DETECTOR rec[-3] rec[-7]
                    DETECTOR rec[-4] rec[-8]
                }
            )CIRCUIT"),
            false,
            false,
            false),
        DetectorErrorModel(R"MODEL(
            error(0.5) D0 D1
            error(0.5) D2 D3
            error(0.5) D4 D5
            error(0.5) D6 D7
            error(0.5) D8 D9
            error(0.5) D10 D11
            error(0.5) D12 D13
            error(0.5) D14 D15
            error(0.5) D16 D17
            error(0.5) D18 D19
        )MODEL"));

    ASSERT_TRUE(ErrorFuser::circuit_to_detector_error_model(
                    Circuit::from_text(R"CIRCUIT(
                        # Distance 2 Bacon-Shor.
                        ZCX 0 10 1 10
                        ZCX 2 11 3 11
                        XCX 0 12 2 12
                        XCX 1 13 3 13
                        MR 10 11 12 13
                        REPEAT 5 {
                            DEPOLARIZE1(0.01) 0 1 2 3
                            ZCX 0 10 1 10
                            ZCX 2 11 3 11
                            XCX 0 12 2 12
                            XCX 1 13 3 13
                            MR 10 11 12 13
                            DETECTOR rec[-1] rec[-5]
                            DETECTOR rec[-2] rec[-6]
                            DETECTOR rec[-3] rec[-7]
                            DETECTOR rec[-4] rec[-8]
                        }
                    )CIRCUIT"),
                    false,
                    false,
                    false)
                    .approx_equals(
                        DetectorErrorModel(R"MODEL(
            error(0.00667) D0
            error(0.5) D0 D1
            error(0.00334) D0 D2
            error(0.00334) D0 D3
            error(0.00667) D1
            error(0.00334) D1 D2
            error(0.00334) D1 D3
            error(0.00667) D2
            error(0.5) D2 D3
            error(0.00667) D3
            error(0.00667) D4
            error(0.5) D4 D5
            error(0.00334) D4 D6
            error(0.00334) D4 D7
            error(0.00667) D5
            error(0.00334) D5 D6
            error(0.00334) D5 D7
            error(0.00667) D6
            error(0.5) D6 D7
            error(0.00667) D7
            error(0.00667) D8
            error(0.5) D8 D9
            error(0.00334) D8 D10
            error(0.00334) D8 D11
            error(0.00667) D9
            error(0.00334) D9 D10
            error(0.00334) D9 D11
            error(0.00667) D10
            error(0.5) D10 D11
            error(0.00667) D11
            error(0.00667) D12
            error(0.5) D12 D13
            error(0.00334) D12 D14
            error(0.00334) D12 D15
            error(0.00667) D13
            error(0.00334) D13 D14
            error(0.00334) D13 D15
            error(0.00667) D14
            error(0.5) D14 D15
            error(0.00667) D15
            error(0.00667) D16
            error(0.5) D16 D17
            error(0.00334) D16 D18
            error(0.00334) D16 D19
            error(0.00667) D17
            error(0.00334) D17 D18
            error(0.00334) D17 D19
            error(0.00667) D18
            error(0.5) D18 D19
            error(0.00667) D19
        )MODEL"),
                        0.01));

    ASSERT_EQ(
        ErrorFuser::circuit_to_detector_error_model(
            Circuit::from_text(R"CIRCUIT(
                # Distance 2 Bacon-Shor.
                ZCX 0 10 1 10
                ZCX 2 11 3 11
                XCX 0 12 2 12
                XCX 1 13 3 13
                MR 10 11 12 13
                REPEAT 1000000000000000 {
                    ZCX 0 10 1 10
                    ZCX 2 11 3 11
                    XCX 0 12 2 12
                    XCX 1 13 3 13
                    MR 10 11 12 13
                    DETECTOR rec[-1] rec[-5]
                    DETECTOR rec[-2] rec[-6]
                    DETECTOR rec[-3] rec[-7]
                    DETECTOR rec[-4] rec[-8]
                }
            )CIRCUIT"),
            false,
            true,
            false),
        DetectorErrorModel(R"MODEL(
            error(0.5) D0 D1
            error(0.5) D2 D3
            error(0.5) D6 D7
            repeat 499999999999999 {
                error(0.5) D4+t D5+t
                error(0.5) D8+t D9+t
                error(0.5) D10+t D11+t
                error(0.5) D14+t D15+t
                tick 8
            }
            error(0.5) D3999999999999996 D3999999999999997
        )MODEL"));
}
