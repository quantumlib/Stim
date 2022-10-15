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

#include "stim/simulators/error_analyzer.h"

#include <regex>

#include "gtest/gtest.h"

#include "stim/gen/gen_rep_code.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/test_util.test.h"

using namespace stim;

TEST(ErrorAnalyzer, circuit_to_detector_error_model) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            R"circuit(
            X_ERROR(0.25) 3
            M 3
            DETECTOR rec[-1]
        )circuit",
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            R"circuit(
                X_ERROR(0.25) 3
                M 3
                DETECTOR rec[-1]
                OBSERVABLE_INCLUDE(0) rec[-1]
        )circuit",
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0 L0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            R"circuit(
                Y_ERROR(0.25) 3
                M 3
                DETECTOR rec[-1]
        )circuit",
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            R"circuit(
            Z_ERROR(0.25) 3
            M 3
            DETECTOR rec[-1]
        )circuit",
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            detector D0
        )model"));

    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
                    R"circuit(
            DEPOLARIZE1(0.25) 3
            M 3
            DETECTOR rec[-1]
        )circuit",
                    false,
                    false,
                    false,
                    0.0,
                    false,
                    true)
                    .approx_equals(
                        DetectorErrorModel(R"model(
                error(0.166666) D0
            )model"),
                        1e-4));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            R"circuit(
                X_ERROR(0.25) 0
                X_ERROR(0.125) 1
                M 0 1
                OBSERVABLE_INCLUDE(3) rec[-1]
                DETECTOR rec[-2]
        )circuit",
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0
            error(0.125) L3
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            R"circuit(
                X_ERROR(0.25) 0
                X_ERROR(0.125) 1
                M 0 1
                OBSERVABLE_INCLUDE(3) rec[-1]
                DETECTOR rec[-2]
        )circuit",
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0
            error(0.125) L3
        )model"));

    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
                    R"circuit(
            DEPOLARIZE2(0.25) 3 5
            M 3
            M 5
            DETECTOR rec[-1]
            DETECTOR rec[-2]
        )circuit",
                    false,
                    false,
                    false,
                    0.0,
                    false,
                    true)
                    .approx_equals(
                        DetectorErrorModel(R"model(
                error(0.0718255) D0
                error(0.0718255) D0 D1
                error(0.0718255) D1
            )model"),
                        1e-5));

    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
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
                    false,
                    false,
                    false,
                    0.0,
                    false,
                    true)
                    .approx_equals(
                        DetectorErrorModel(R"model(
                error(0.019013) D0
                error(0.019013) D0 D1
                error(0.019013) D0 D1 D2
                error(0.019013) D0 D1 D2 D3
                error(0.019013) D0 D1 D3
                error(0.019013) D0 D2
                error(0.019013) D0 D2 D3
                error(0.019013) D0 D3
                error(0.019013) D1
                error(0.019013) D1 D2
                error(0.019013) D1 D2 D3
                error(0.019013) D1 D3
                error(0.019013) D2
                error(0.019013) D2 D3
                error(0.019013) D3
            )model"),
                        1e-4));

    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
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
                    true,
                    false,
                    false,
                    0.0,
                    false,
                    true)
                    .approx_equals(
                        DetectorErrorModel(R"model(
                error(0.019013) D0
                error(0.019013) D1
                error(0.019013) D1 ^ D0
                error(0.019013) D1 ^ D2
                error(0.019013) D1 ^ D2 ^ D0
                error(0.019013) D2
                error(0.019013) D2 ^ D0
                error(0.019013) D3
                error(0.019013) D3 ^ D0
                error(0.019013) D3 ^ D1
                error(0.019013) D3 ^ D1 ^ D0
                error(0.019013) D3 ^ D1 ^ D2
                error(0.019013) D3 ^ D1 ^ D2 ^ D0
                error(0.019013) D3 ^ D2
                error(0.019013) D3 ^ D2 ^ D0
            )model"),
                        1e-4));

    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
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
                    true,
                    0.0,
                    false,
                    true)
                    .approx_equals(
                        DetectorErrorModel(R"model(
                error(0.071825) D0 D1
                error(0.071825) D0 D1 ^ D2 D3
                error(0.071825) D2 D3
            )model"),
                        1e-4));
}

TEST(ErrorAnalyzer, unitary_gates_match_frame_simulator) {
    FrameSimulator f(16, 16, SIZE_MAX, SHARED_TEST_RNG());
    ErrorAnalyzer e(1, 16, false, false, false, 0.0, false, true);
    for (size_t q = 0; q < 16; q++) {
        if (q & 1) {
            e.xs[q].xor_item({0});
            f.x_table[q][0] = true;
        }
        if (q & 2) {
            e.xs[q].xor_item({1});
            f.x_table[q][1] = true;
        }
        if (q & 4) {
            e.zs[q].xor_item({0});
            f.z_table[q][0] = true;
        }
        if (q & 8) {
            e.zs[q].xor_item({1});
            f.z_table[q][1] = true;
        }
    }

    std::vector<GateTarget> data;
    for (size_t k = 0; k < 16; k++) {
        data.push_back(GateTarget::qubit(k));
    }
    OperationData targets = {{}, data};
    for (const auto &gate : GATE_DATA.gates()) {
        if (gate.flags & GATE_IS_UNITARY) {
            (e.*gate.reverse_error_analyzer_function)(targets);
            (f.*gate.inverse().frame_simulator_function)(targets);
            for (size_t q = 0; q < 16; q++) {
                bool xs[2]{};
                bool zs[2]{};
                for (auto x : e.xs[q]) {
                    ASSERT_TRUE(x.data < 2) << gate.name;
                    xs[x.data] = true;
                }
                for (auto z : e.zs[q]) {
                    ASSERT_TRUE(z.data < 2) << gate.name;
                    zs[z.data] = true;
                }
                ASSERT_EQ(f.x_table[q][0], xs[0]) << gate.name;
                ASSERT_EQ(f.x_table[q][1], xs[1]) << gate.name;
                ASSERT_EQ(f.z_table[q][0], zs[0]) << gate.name;
                ASSERT_EQ(f.z_table[q][1], zs[1]) << gate.name;
            }
        }
    }
}

TEST(ErrorAnalyzer, reversed_operation_order) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            R"circuit(
            X_ERROR(0.25) 0
            CNOT 0 1
            CNOT 1 0
            M 0 1
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit",
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D1
            detector D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            R"circuit(
            X_ERROR(0.25) 0
            CNOT 0 1
            CNOT 1 0
            M 0 1
            DETECTOR rec[-1]
            DETECTOR rec[-2]
        )circuit",
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0
            detector D1
        )model"));
}

TEST(ErrorAnalyzer, classical_error_propagation) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            R"circuit(
            X_ERROR(0.125) 0
            M 0
            CNOT rec[-1] 1
            M 1
            DETECTOR rec[-1]
        )circuit",
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.125) D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            X_ERROR(0.125) 0
            M 0
            H 1
            CZ rec[-1] 1
            H 1
            M 1
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.125) D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            X_ERROR(0.125) 0
            M 0
            H 1
            CZ 1 rec[-1]
            H 1
            M 1
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.125) D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            X_ERROR(0.125) 0
            M 0
            CY rec[-1] 1
            M 1
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.125) D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            X_ERROR(0.125) 0
            M 0
            XCZ 1 rec[-1]
            M 1
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.125) D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            X_ERROR(0.125) 0
            M 0
            YCZ 1 rec[-1]
            M 1
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.125) D0
        )model"));
}

TEST(ErrorAnalyzer, measure_reset_basis) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
                RZ 0 1 2
                X_ERROR(0.25) 0
                Y_ERROR(0.25) 1
                Z_ERROR(0.25) 2
                MZ 0 1 2
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0
            error(0.25) D1
            detector D2
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            RX 0 1 2
            X_ERROR(0.25) 0
            Y_ERROR(0.25) 1
            Z_ERROR(0.25) 2
            MX 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D1
            error(0.25) D2
            detector D0
        )model"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            RY 0 1 2
            X_ERROR(0.25) 0
            Y_ERROR(0.25) 1
            Z_ERROR(0.25) 2
            MY 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0
            error(0.25) D2
            detector D1
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
                MRZ 0 1 2
                X_ERROR(0.25) 0
                Y_ERROR(0.25) 1
                Z_ERROR(0.25) 2
                MRZ 0 1 2
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0
            error(0.25) D1
            detector D2
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
                MRX 0 1 2
                X_ERROR(0.25) 0
                Y_ERROR(0.25) 1
                Z_ERROR(0.25) 2
                MRX 0 1 2
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D1
            error(0.25) D2
            detector D0
        )model"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
                MRY 0 1 2
                X_ERROR(0.25) 0
                Y_ERROR(0.25) 1
                Z_ERROR(0.25) 2
                MRY 0 1 2
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0
            error(0.25) D2
            detector D1
        )model"));
}

TEST(ErrorAnalyzer, repeated_measure_reset) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            MRZ 0 0
            X_ERROR(0.25) 0
            MRZ 0 0
            DETECTOR rec[-4]
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D2
            detector D0
            detector D1
            detector D3
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            RY 0 0
            MRY 0 0
            X_ERROR(0.25) 0
            MRY 0 0
            DETECTOR rec[-4]
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D2
            detector D0
            detector D1
            detector D3
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
                RX 0 0
                MRX 0 0
                Z_ERROR(0.25) 0
                MRX 0 0
                DETECTOR rec[-4]
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D2
            detector D0
            detector D1
            detector D3
        )model"));
}

TEST(ErrorAnalyzer, period_3_gates) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            RY 0 1 2
            X_ERROR(1) 0
            Y_ERROR(1) 1
            Z_ERROR(1) 2
            C_XYZ 0 1 2
            M 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(1) D0
            error(1) D2
            detector D1
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
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
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(1) D1
            error(1) D2
            detector D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
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
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(1) D0
            error(1) D2
            detector D1
        )model"));
}

TEST(ErrorAnalyzer, detect_gauge_observables) {
    ASSERT_ANY_THROW({
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                R 0
                H 0
                M 0
                OBSERVABLE_INCLUDE(0) rec[-1]
            )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true);
    });
    ASSERT_ANY_THROW({
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                R 0
                H 0
                M 0
                OBSERVABLE_INCLUDE(0) rec[-1]
            )circuit"),
            false,
            false,
            true,
            0.0,
            false,
            true);
    });
}

TEST(ErrorAnalyzer, detect_gauge_detectors) {
    ASSERT_ANY_THROW({
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                R 0
                H 0
                M 0
                DETECTOR rec[-1]
            )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                M 0
                H 0
                M 0
                DETECTOR rec[-1]
            )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                MZ 0
                MX 0
                DETECTOR rec[-1]
            )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                MY 0
                MX 0
                DETECTOR rec[-1]
            )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                MX 0
                MZ 0
                DETECTOR rec[-1]
            )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                RX 0
                MZ 0
                DETECTOR rec[-1]
            )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                RY 0
                MX 0
                DETECTOR rec[-1]
            )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                RZ 0
                MX 0
                DETECTOR rec[-1]
            )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true);
    });

    ASSERT_ANY_THROW({
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                MX 0
                DETECTOR rec[-1]
            )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true);
    });
}

TEST(ErrorAnalyzer, gauge_detectors) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                H 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit"),
            false,
            false,
            true,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
                error(0.5) D0 D1
            )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                R 0
                H 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit"),
            false,
            false,
            true,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.5) D0 D1
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                RX 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit"),
            false,
            false,
            true,
            0.0,
            false,
            true),
        R"model(
error(0.5) D0 D1
)model");

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                RY 0
                H_XY 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit"),
            false,
            false,
            true,
            0.0,
            false,
            true),
        R"model(
error(0.5) D0 D1
)model");

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                MR 0
                H 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit"),
            false,
            false,
            true,
            0.0,
            false,
            true),
        R"model(
error(0.5) D0 D1
)model");

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                MRX 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit"),
            false,
            false,
            true,
            0.0,
            false,
            true),
        R"model(
error(0.5) D0 D1
)model");

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                MRY 0
                H_XY 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit"),
            false,
            false,
            true,
            0.0,
            false,
            true),
        R"model(
error(0.5) D0 D1
)model");

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                M 0
                H 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit"),
            false,
            false,
            true,
            0.0,
            false,
            true),
        R"model(
error(0.5) D0 D1
)model");

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
                MX 0
                CNOT 0 1
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit"),
            false,
            false,
            true,
            0.0,
            false,
            true),
        R"model(
error(0.5) D0 D1
)model");

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                R"circuit(
            MY 0
            H_XY 0
            CNOT 0 1
            M 0 1
            DETECTOR rec[-1]
            DETECTOR rec[-2]
        )circuit"),
            false,
            false,
            true,
            0.0,
            false,
            true),
        R"model(
error(0.5) D0 D1
)model");
}

TEST(ErrorAnalyzer, composite_error_analysis) {
    auto measure_stabilizers = Circuit(R"circuit(
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
    auto detectors = Circuit(R"circuit(
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
    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
                    Circuit(encode + Circuit("DEPOLARIZE1(0.01) 4") + decode), true, false, false, 0.0, false, true)
                    .approx_equals(
                        DetectorErrorModel(R"model(
                error(0.0033445) D0 D4
                error(0.0033445) D0 D4 ^ D1 D3
                error(0.0033445) D1 D3
                detector D2
                detector D5
            )model"),
                        1e-6));

    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
                    Circuit(encode + Circuit("DEPOLARIZE2(0.01) 4 5") + decode), true, false, false, 0.0, false, true)
                    .approx_equals(
                        DetectorErrorModel(R"model(
                error(0.000669) D0 D2
                error(0.000669) D0 D2 ^ D1 D3
                error(0.000669) D0 D2 ^ D1 D5
                error(0.000669) D0 D2 ^ D3 D5
                error(0.000669) D0 D4
                error(0.000669) D0 D4 ^ D1 D3
                error(0.000669) D0 D4 ^ D1 D5
                error(0.000669) D0 D4 ^ D3 D5
                error(0.000669) D1 D3
                error(0.000669) D1 D3 ^ D2 D4
                error(0.000669) D1 D5
                error(0.000669) D1 D5 ^ D2 D4
                error(0.000669) D2 D4
                error(0.000669) D2 D4 ^ D3 D5
                error(0.000669) D3 D5
            )model"),
                        1e-6));

    auto expected = DetectorErrorModel(R"model(
        error(0.000669) D0 D1 D2 D3
        error(0.000669) D0 D1 D2 D5
        error(0.000669) D0 D1 D3 D4
        error(0.000669) D0 D1 D4 D5
        error(0.000669) D0 D2
        error(0.000669) D0 D2 D3 D5
        error(0.000669) D0 D3 D4 D5
        error(0.000669) D0 D4
        error(0.000669) D1 D2 D3 D4
        error(0.000669) D1 D2 D4 D5
        error(0.000669) D1 D3
        error(0.000669) D1 D5
        error(0.000669) D2 D3 D4 D5
        error(0.000669) D2 D4
        error(0.000669) D3 D5
    )model");
    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
                    Circuit(encode + Circuit("DEPOLARIZE2(0.01) 4 5") + decode), false, false, false, 0.0, false, true)
                    .approx_equals(expected, 1e-5));
    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
                    Circuit(encode + Circuit("CNOT 4 5\nDEPOLARIZE2(0.01) 4 5\nCNOT 4 5") + decode),
                    false,
                    false,
                    false,
                    0.0,
                    false,
                    true)
                    .approx_equals(expected, 1e-5));
    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
                    Circuit(encode + Circuit("H_XY 4\nCNOT 4 5\nDEPOLARIZE2(0.01) 4 5\nCNOT 4 5\nH_XY 4") + decode),
                    false,
                    false,
                    false,
                    0.0,
                    false,
                    true)
                    .approx_equals(expected, 1e-5));
}

std::string declare_detectors(size_t min, size_t max) {
    std::stringstream result;
    for (size_t k = min; k <= max; k++) {
        result << "detector D" << k << "\n";
    }
    return result.str();
}

TEST(ErrorAnalyzer, loop_folding) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
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
            true,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
                error(0.25) D0 L9
                REPEAT 6172839493827159 {
                    error(0.25) D1 L9
                    error(0.25) D2 L9
                    shift_detectors 2
                }
                error(0.25) D1 L9
                error(0.25) D2 L9
            )MODEL"));

    // Solve period 8 logical observable oscillation.
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
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
            true,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            detector D0
            detector D1
            detector D2
            REPEAT 1543209873456789 {
                detector D3
                detector D4
                detector D5
                detector D6
                detector D7
                detector D8
                detector D9
                detector D10
                shift_detectors 8
            }
            detector D3
            detector D4
            detector D5
            detector D6
            detector D7
            detector D8
            logical_observable L9
        )MODEL"));

    // Solve period 127 logical observable oscillation.
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
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
            true,
            0.0,
            false,
            true),
        DetectorErrorModel((declare_detectors(0, 85) + R"MODEL(
            REPEAT 97210070768930 {
                )MODEL" + declare_detectors(86, 86 + 127 - 1) +
                            R"MODEL(
                shift_detectors 127
            }
            error(1) D211
            )MODEL" + declare_detectors(86, 210) +
                            R"MODEL(
            logical_observable L9
        )MODEL")
                               .data()));
}

TEST(ErrorAnalyzer, loop_folding_nested_loop) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
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
            true,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
                REPEAT 999 {
                    REPEAT 1000 {
                        error(0.25) D0 L9
                        shift_detectors 1
                    }
                }
                REPEAT 499 {
                    error(0.25) D0 L9
                    error(0.25) D1 L9
                    shift_detectors 2
                }
                error(0.25) D0 L9
                error(0.25) D1 L9
            )MODEL"));
}

TEST(ErrorAnalyzer, loop_folding_rep_code_circuit) {
    CircuitGenParameters params(100000, 4, "memory");
    params.after_clifford_depolarization = 0.001;
    auto circuit = generate_rep_code_circuit(params).circuit;

    auto actual = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, 0.0, false, true);
    auto expected = DetectorErrorModel(R"MODEL(
        error(0.000267) D0
        error(0.000267) D0 D1
        error(0.000267) D0 D3
        error(0.000533) D0 D4
        error(0.000267) D1 D2
        error(0.000533) D1 D4
        error(0.000533) D1 D5
        error(0.000267) D2 D5
        error(0.000267) D2 L0
        error(0.000267) D3
        error(0.000267) D3 D4
        error(0.000267) D3 ^ D0
        error(0.000267) D4 D5
        error(0.000267) D5 L0
        error(0.000267) D5 L0 ^ D2 L0
        detector(1, 0) D0
        detector(3, 0) D1
        detector(5, 0) D2
        repeat 99998 {
            error(0.000267) D3
            error(0.000267) D3 D4
            error(0.000267) D3 D6
            error(0.000533) D3 D7
            error(0.000267) D4 D5
            error(0.000533) D4 D7
            error(0.000533) D4 D8
            error(0.000267) D5 D8
            error(0.000267) D5 L0
            error(0.000267) D6
            error(0.000267) D6 D7
            error(0.000267) D6 ^ D3
            error(0.000267) D7 D8
            error(0.000267) D8 L0
            error(0.000267) D8 L0 ^ D5 L0
            shift_detectors(0, 1) 0
            detector(1, 0) D3
            detector(3, 0) D4
            detector(5, 0) D5
            shift_detectors 3
        }
        error(0.000267) D3
        error(0.000267) D3 D4
        error(0.000267) D3 D6
        error(0.000533) D3 D7
        error(0.000267) D4 D5
        error(0.000533) D4 D7
        error(0.000533) D4 D8
        error(0.000267) D5 D8
        error(0.000267) D5 L0
        error(0.000267) D6
        error(0.000267) D6 D7
        error(0.000267) D6 ^ D3
        error(0.000267) D7 D8
        error(0.000267) D8 L0
        error(0.000267) D8 L0 ^ D5 L0
        shift_detectors(0, 1) 0
        detector(1, 0) D3
        detector(3, 0) D4
        detector(5, 0) D5
        detector(1, 1) D6
        detector(3, 1) D7
        detector(5, 1) D8
    )MODEL");
    ASSERT_TRUE(actual.approx_equals(expected, 0.00001)) << actual;
}

TEST(ErrorAnalyzer, multi_round_gauge_detectors_dont_grow) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
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
            true,
            0.0,
            false,
            true),
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

    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
                    Circuit(R"CIRCUIT(
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
                    true,
                    0.0,
                    false,
                    true)
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
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
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
            true,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.5) D0 D1
            error(0.5) D2 D3
            error(0.5) D6 D7
            repeat 499999999999999 {
                error(0.5) D4 D5
                error(0.5) D8 D9
                error(0.5) D10 D11
                error(0.5) D14 D15
                shift_detectors 8
            }
            error(0.5) D4 D5
            detector D0
            detector D1
            detector D2
            detector D3
            detector D6
            detector D7
        )MODEL"));
}

TEST(ErrorAnalyzer, coordinate_tracking) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                DETECTOR(1, 2)
                SHIFT_COORDS(10, 20)
                DETECTOR(100, 200)
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            detector(1, 2) D0
            shift_detectors(10, 20) 0
            detector(100, 200) D1
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                MR 1
                REPEAT 1000 {
                    REPEAT 1000 {
                        X_ERROR(0.25) 0
                        CNOT 0 1
                        MR 1
                        DETECTOR(1,2,3) rec[-2] rec[-1]
                        SHIFT_COORDS(4,5)
                    }
                    SHIFT_COORDS(6,7)
                }
                M 0
                OBSERVABLE_INCLUDE(9) rec[-1]
            )CIRCUIT"),
            false,
            true,
            true,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
                REPEAT 999 {
                    REPEAT 1000 {
                        error(0.25) D0 L9
                        detector(1, 2, 3) D0
                        shift_detectors(4, 5) 1
                    }
                    shift_detectors(6, 7) 0
                }
                REPEAT 499 {
                    error(0.25) D0 L9
                    error(0.25) D1 L9
                    detector(1, 2, 3) D0
                    shift_detectors(4, 5) 0
                    detector(1, 2, 3) D1
                    shift_detectors(4, 5) 2
                }
                error(0.25) D0 L9
                error(0.25) D1 L9
                detector(1, 2, 3) D0
                shift_detectors(4, 5) 0
                detector(1, 2, 3) D1
                shift_detectors(4, 5) 0
                shift_detectors(6, 7) 0
            )MODEL"));
}

TEST(ErrorAnalyzer, omit_vacuous_detector_observable_instructions) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            X_ERROR(0.25) 3
            M 3
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            X_ERROR(0.25) 3
            M 3
            DETECTOR(1, 0) rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) D0
            detector(1, 0) D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            M 3
            DETECTOR rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            detector D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            X_ERROR(0.25) 3
            M 3
            OBSERVABLE_INCLUDE(0) rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            error(0.25) L0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
            M 3
            OBSERVABLE_INCLUDE(0) rec[-1]
        )circuit"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"model(
            logical_observable L0
        )model"));
}

TEST(ErrorAnalyzer, pauli_channel_threshold) {
    auto c1 = Circuit(R"CIRCUIT(
R 0
PAULI_CHANNEL_1(0.125, 0.25, 0.375) 0
M 0
DETECTOR rec[-1]
    )CIRCUIT");
    auto c2 = Circuit(R"CIRCUIT(
R 0
PAULI_CHANNEL_2(0.125, 0.25, 0.375, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) 1 0
M 0
DETECTOR rec[-1]
    )CIRCUIT");

    ASSERT_ANY_THROW({ ErrorAnalyzer::circuit_to_detector_error_model(c1, false, false, false, 0, false, true); });
    ASSERT_ANY_THROW({ ErrorAnalyzer::circuit_to_detector_error_model(c1, false, false, false, 0.3, false, true); });
    ASSERT_ANY_THROW({ ErrorAnalyzer::circuit_to_detector_error_model(c2, false, false, false, 0, false, true); });
    ASSERT_ANY_THROW({ ErrorAnalyzer::circuit_to_detector_error_model(c2, false, false, false, 0.3, false, true); });
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(c1, false, false, false, 0.38, false, true),
        DetectorErrorModel(R"MODEL(
            error(0.3125) D0
        )MODEL"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(c1, false, false, false, 1, false, true),
        DetectorErrorModel(R"MODEL(
            error(0.3125) D0
        )MODEL"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(c2, false, false, false, 0.38, false, true),
        DetectorErrorModel(R"MODEL(
            error(0.3125) D0
        )MODEL"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(c2, false, false, false, 1, false, true),
        DetectorErrorModel(R"MODEL(
            error(0.3125) D0
        )MODEL"));
}

TEST(ErrorAnalyzer, pauli_channel_composite_errors) {
    auto measure_stabilizers = Circuit(R"circuit(
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
    auto detectors = Circuit(R"circuit(
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
    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
                    Circuit(encode + Circuit("PAULI_CHANNEL_1(0.01, 0.02, 0.03) 4") + decode),
                    true,
                    false,
                    false,
                    0.1,
                    false,
                    true)
                    .approx_equals(
                        DetectorErrorModel(R"model(
                error(0.03) D0 D4
                error(0.02) D0 D4 ^ D1 D3
                error(0.01) D1 D3
                detector D2
                detector D5
            )model"),
                        1e-6));
    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
                    Circuit(encode + Circuit("PAULI_CHANNEL_1(0.01, 0.02, 0.03) 5") + decode),
                    true,
                    false,
                    false,
                    0.1,
                    false,
                    true)
                    .approx_equals(
                        DetectorErrorModel(R"model(
                error(0.01) D1 D5
                error(0.02) D1 D5 ^ D2 D4
                error(0.03) D2 D4
                detector D0
                detector D3
            )model"),
                        1e-6));

    ASSERT_TRUE(ErrorAnalyzer::circuit_to_detector_error_model(
                    Circuit(
                        encode +
                        Circuit("PAULI_CHANNEL_2(0.001,0.002,0.003,0.004,0.005,0.006,0.007,0.008,0.009,0.010,0."
                                "011,0."
                                "012,0.013,0.014,0.015) 4 5") +
                        decode),
                    true,
                    false,
                    false,
                    0.02,
                    false,
                    true)
                    .approx_equals(
                        DetectorErrorModel(R"model(
                error(0.015) D0 D2          # ZZ
                error(0.011) D0 D2 ^ D1 D3  # YZ
                error(0.014) D0 D2 ^ D1 D5  # ZY
                error(0.010) D0 D2 ^ D3 D5  # YY
                error(0.012) D0 D4          # Z_ basis
                error(0.008) D0 D4 ^ D1 D3  # Y_
                error(0.013) D0 D4 ^ D1 D5  # ZX
                error(0.009) D0 D4 ^ D3 D5  # YX
                error(0.004) D1 D3          # X_ basis
                error(0.007) D1 D3 ^ D2 D4  # XZ
                error(0.001) D1 D5          # _X basis
                error(0.002) D1 D5 ^ D2 D4  # _Y
                error(0.003) D2 D4          # _Z basis
                error(0.006) D2 D4 ^ D3 D5  # XY
                error(0.005) D3 D5          # XX
            )model"),
                        1e-6));
}

TEST(ErrorAnalyzer, duplicate_records_in_detectors) {
    auto m0 = ErrorAnalyzer::circuit_to_detector_error_model(
        Circuit(R"CIRCUIT(
            X_ERROR(0.25) 0
            M 0
            DETECTOR
        )CIRCUIT"),
        false,
        false,
        false,
        false,
        false,
        true);
    auto m1 = ErrorAnalyzer::circuit_to_detector_error_model(
        Circuit(R"CIRCUIT(
            X_ERROR(0.25) 0
            M 0
            DETECTOR rec[-1]
        )CIRCUIT"),
        false,
        false,
        false,
        false,
        false,
        true);
    auto m2 = ErrorAnalyzer::circuit_to_detector_error_model(
        Circuit(R"CIRCUIT(
            X_ERROR(0.25) 0
            M 0
            DETECTOR rec[-1] rec[-1]
        )CIRCUIT"),
        false,
        false,
        false,
        false,
        false,
        true);
    auto m3 = ErrorAnalyzer::circuit_to_detector_error_model(
        Circuit(R"CIRCUIT(
            X_ERROR(0.25) 0
            M 0
            DETECTOR rec[-1] rec[-1] rec[-1]
        )CIRCUIT"),
        false,
        false,
        false,
        false,
        false,
        true);
    ASSERT_EQ(m0, m2);
    ASSERT_EQ(m1, m3);
}

TEST(ErrorAnalyzer, noisy_measurement_mx) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
            RX 0
            MX(0.125) 0
            MX 0
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )CIRCUIT"),
            false,
            false,
            false,
            false,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.125) D0
            detector D1
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
            RX 0 1
            Y_ERROR(1) 0 1
            MX(0.125) 0 1
            MX 0 1
            DETECTOR rec[-4]
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )CIRCUIT"),
            false,
            false,
            false,
            false,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.125) D0
            error(1) D0 D2
            error(0.125) D1
            error(1) D1 D3
        )MODEL"));
}

TEST(ErrorAnalyzer, noisy_measurement_my) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
            RY 0
            MY(0.125) 0
            MY 0
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.125) D0
            detector D1
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
            RY 0 1
            Z_ERROR(1) 0 1
            MY(0.125) 0 1
            MY 0 1
            DETECTOR rec[-4]
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.125) D0
            error(1) D0 D2
            error(0.125) D1
            error(1) D1 D3
        )MODEL"));
}

TEST(ErrorAnalyzer, noisy_measurement_mz) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                RZ 0
                MZ(0.125) 0
                MZ 0
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.125) D0
            detector D1
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                RZ 0 1
                X_ERROR(1) 0 1
                MZ(0.125) 0 1
                MZ 0 1
                DETECTOR rec[-4]
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.125) D0
            error(1) D0 D2
            error(0.125) D1
            error(1) D1 D3
        )MODEL"));
}

TEST(ErrorAnalyzer, noisy_measurement_mrx) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                RX 0
                MRX(0.125) 0
                MRX 0
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.125) D0
            detector D1
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                RX 0 1
                Z_ERROR(1) 0 1
                MRX(0.125) 0 1
                MRX 0 1
                DETECTOR rec[-4]
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.875) D0
            error(0.875) D1
            detector D2
            detector D3
        )MODEL"));
}

TEST(ErrorAnalyzer, noisy_measurement_mry) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                RY 0
                MRY(0.125) 0
                MRY 0
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.125) D0
            detector D1
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                RY 0 1
                X_ERROR(1) 0 1
                MRY(0.125) 0 1
                MRY 0 1
                DETECTOR rec[-4]
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.875) D0
            error(0.875) D1
            detector D2
            detector D3
        )MODEL"));
}

TEST(ErrorAnalyzer, noisy_measurement_mrz) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                RZ 0
                MRZ(0.125) 0
                MRZ 0
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.125) D0
            detector D1
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                RZ 0 1
                X_ERROR(1) 0 1
                MRZ(0.125) 0 1
                MRZ 0 1
                DETECTOR rec[-4]
                DETECTOR rec[-3]
                DETECTOR rec[-2]
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.875) D0
            error(0.875) D1
            detector D2
            detector D3
        )MODEL"));
}

template <typename TEx>
std::string check_catch(std::string expected_substring, std::function<void(void)> func) {
    try {
        func();
        return "Expected an exception with message '" + expected_substring + "', but no exception was thrown.";
    } catch (const TEx &ex) {
        std::string s = ex.what();
        if (s.find(expected_substring) == std::string::npos) {
            return "Didn't find '" + expected_substring + "' in '" + std::string(ex.what()) + "'.";
        }
        return "";
    }
}

TEST(ErrorAnalyzer, context_clues_for_errors) {
    ASSERT_EQ(
        "",
        check_catch<std::invalid_argument>(
            "Can't analyze over-mixing DEPOLARIZE1 errors (probability >= 3/4).\n"
            "\n"
            "Circuit stack trace:\n"
            "    at instruction #2 [which is DEPOLARIZE1(1) 0]",
            [&] {
                ErrorAnalyzer::circuit_to_detector_error_model(
                    Circuit(R"CIRCUIT(
                X 0
                DEPOLARIZE1(1) 0
            )CIRCUIT"),
                    false,
                    false,
                    false,
                    0.0,
                    false,
                    true);
            }));

    ASSERT_EQ(
        "",
        check_catch<std::invalid_argument>(
            "Can't analyze over-mixing DEPOLARIZE1 errors (probability >= 3/4).\n"
            "\n"
            "Circuit stack trace:\n"
            "    at instruction #3 [which is a REPEAT 500 block]\n"
            "    at block's instruction #1 [which is DEPOLARIZE1(1) 0]",
            [&] {
                ErrorAnalyzer::circuit_to_detector_error_model(
                    Circuit(R"CIRCUIT(
                X 0
                Y 1
                REPEAT 500 {
                    DEPOLARIZE1(1) 0
                }
                Z 3
            )CIRCUIT"),
                    false,
                    false,
                    false,
                    0.0,
                    false,
                    true);
            }));
}

TEST(ErrorAnalyzer, too_many_symptoms) {
    auto symptoms_20 = Circuit(R"CIRCUIT(
        DEPOLARIZE1(0.001) 0
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
        DETECTOR rec[-1]
        DETECTOR rec[-1]
    )CIRCUIT");
    ASSERT_EQ("", check_catch<std::invalid_argument>("max supported number of symptoms", [&] {
                  ErrorAnalyzer::circuit_to_detector_error_model(symptoms_20, true, false, false, 0.0, false, true);
              }));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(symptoms_20, false, false, false, 0.0, false, true),
        R"model(
            error(0.0006666666666666692465) D0 D1 D2 D3 D4 D5 D6 D7 D8 D9 D10 D11 D12 D13 D14 D15 D16 D17 D18 D19
        )model");
}

TEST(ErrorAnalyzer, decompose_error_failures) {
    ASSERT_EQ("", check_catch<std::invalid_argument>("failed to decompose is 'D0, D1, D2'", [] {
                  ErrorAnalyzer::circuit_to_detector_error_model(
                      Circuit(R"CIRCUIT(
                DEPOLARIZE1(0.001) 0
                M 0
                DETECTOR rec[-1]
                DETECTOR rec[-1]
                DETECTOR rec[-1]
            )CIRCUIT"),
                      true,
                      false,
                      false,
                      0.0,
                      false,
                      true);
              }));

    ASSERT_EQ("", check_catch<std::invalid_argument>("decompose errors into graphlike components", [] {
                  ErrorAnalyzer::circuit_to_detector_error_model(
                      Circuit(R"CIRCUIT(
                X_ERROR(0.001) 0
                M 0
                DETECTOR rec[-1]
                DETECTOR rec[-1]
                DETECTOR rec[-1]
            )CIRCUIT"),
                      true,
                      false,
                      false,
                      0.0,
                      false,
                      true);
              }));

    ASSERT_EQ("", check_catch<std::invalid_argument>("failed to decompose is 'D0, D1, D2, L5'", [] {
                  ErrorAnalyzer::circuit_to_detector_error_model(
                      Circuit(R"CIRCUIT(
                X_ERROR(0.001) 0
                M 0
                DETECTOR rec[-1]
                DETECTOR rec[-1]
                DETECTOR rec[-1]
                OBSERVABLE_INCLUDE(5) rec[-1]
            )CIRCUIT"),
                      true,
                      false,
                      false,
                      0.0,
                      false,
                      true);
              }));
}

TEST(ErrorAnalyzer, other_error_decomposition_fallback) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                X_ERROR(0.125) 0
                MR 0
                X_ERROR(0.25) 0
                MR 0
                DETECTOR rec[-2]
                DETECTOR rec[-2]
                DETECTOR rec[-1] rec[-2]
                DETECTOR rec[-1] rec[-2]
                OBSERVABLE_INCLUDE(5) rec[-2]
                OBSERVABLE_INCLUDE(6) rec[-1]
            )CIRCUIT"),
            true,
            false,
            false,
            0.0,
            false,
            false),
        DetectorErrorModel(R"MODEL(
            error(0.25) D2 D3 L6
            error(0.125) D2 D3 L6 ^ D0 D1 L5 L6
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                X_ERROR(0.125) 0
                MR 0
                X_ERROR(0.25) 0
                MR 0
                DETECTOR rec[-2]
                DETECTOR rec[-2]
                DETECTOR rec[-1] rec[-2]
                DETECTOR rec[-1] rec[-2]
            )CIRCUIT"),
            true,
            false,
            false,
            0.0,
            false,
            false),
        DetectorErrorModel(R"MODEL(
            error(0.25) D2 D3
            error(0.125) D2 D3 ^ D0 D1
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                X_ERROR(0.125) 0
                MR 0
                X_ERROR(0.25) 0
                MR 0
                DETECTOR rec[-1]
                DETECTOR rec[-1]
                DETECTOR rec[-1] rec[-2]
                DETECTOR rec[-1] rec[-2]
            )CIRCUIT"),
            true,
            false,
            false,
            0.0,
            false,
            false),
        DetectorErrorModel(R"MODEL(
            error(0.125) D2 D3
            error(0.25) D2 D3 ^ D0 D1
        )MODEL"));
}

TEST(ErrorAnalyzer, is_graph_like) {
    ASSERT_TRUE(is_graphlike(std::vector<DemTarget>{}));
    ASSERT_TRUE(is_graphlike(std::vector<DemTarget>{DemTarget::separator()}));
    ASSERT_TRUE(is_graphlike(std::vector<DemTarget>{
        DemTarget::observable_id(0),
        DemTarget::observable_id(1),
        DemTarget::observable_id(2),
        DemTarget::separator(),
        DemTarget::observable_id(1),
    }));
    ASSERT_TRUE(is_graphlike(std::vector<DemTarget>{
        DemTarget::observable_id(0),
        DemTarget::relative_detector_id(1),
        DemTarget::observable_id(2),
        DemTarget::separator(),
        DemTarget::observable_id(1),
    }));
    ASSERT_TRUE(is_graphlike(std::vector<DemTarget>{
        DemTarget::observable_id(0),
        DemTarget::relative_detector_id(1),
        DemTarget::relative_detector_id(2),
        DemTarget::separator(),
        DemTarget::observable_id(1),
    }));
    ASSERT_FALSE(is_graphlike(std::vector<DemTarget>{
        DemTarget::relative_detector_id(0),
        DemTarget::relative_detector_id(1),
        DemTarget::relative_detector_id(2),
        DemTarget::separator(),
        DemTarget::observable_id(1),
    }));
    ASSERT_FALSE(is_graphlike(std::vector<DemTarget>{
        DemTarget::relative_detector_id(0),
        DemTarget::relative_detector_id(1),
        DemTarget::relative_detector_id(2),
    }));
    ASSERT_FALSE(is_graphlike(std::vector<DemTarget>{
        DemTarget::separator(),
        DemTarget::separator(),
        DemTarget::relative_detector_id(0),
        DemTarget::relative_detector_id(1),
        DemTarget::relative_detector_id(2),
        DemTarget::separator(),
        DemTarget::separator(),
    }));
    ASSERT_TRUE(is_graphlike(std::vector<DemTarget>{
        DemTarget::separator(),
        DemTarget::relative_detector_id(0),
        DemTarget::separator(),
        DemTarget::relative_detector_id(1),
        DemTarget::relative_detector_id(2),
        DemTarget::separator(),
        DemTarget::separator(),
    }));
}

TEST(ErrorAnalyzer, honeycomb_code_decomposes) {
    ErrorAnalyzer::circuit_to_detector_error_model(
        Circuit(R"CIRCUIT(
            R 3 5 7 9 11 13 18 20 22 24 26 28
            X_ERROR(0.001) 3 5 7 9 11 13 18 20 22 24 26 28
            DEPOLARIZE1(0.001) 0 1 2 4 6 8 10 12 14 15 16 17 19 21 23 25 27 29
            XCX 24 1 7 6 11 12 3 15 20 21 28 27
            R 0 8 14 17 23 29
            DEPOLARIZE2(0.001) 24 1 7 6 11 12 3 15 20 21 28 27
            X_ERROR(0.001) 0 8 14 17 23 29
            YCX 20 0 7 8 3 14 11 17 24 23 28 29
            XCX 9 1 5 6 13 12 18 15 22 21 26 27
            R 2 4 10 16 19 25
            DEPOLARIZE2(0.001) 20 0 7 8 3 14 11 17 24 23 28 29 9 1 5 6 13 12 18 15 22 21 26 27
            X_ERROR(0.001) 2 4 10 16 19 25
            X_ERROR(0.001) 1 6 12 15 21 27
            CX 28 2 3 4 11 10 7 16 20 19 24 25
            YCX 5 0 9 8 13 14 26 17 22 23 18 29
            MR 1 6 12 15 21 27
            OBSERVABLE_INCLUDE(0) rec[-5] rec[-4]
            DEPOLARIZE2(0.001) 28 2 3 4 11 10 7 16 20 19 24 25 5 0 9 8 13 14 26 17 22 23 18 29
            X_ERROR(0.001) 1 6 12 15 21 27
            X_ERROR(0.001) 0 8 14 17 23 29
            XCX 24 1 7 6 11 12 3 15 20 21 28 27
            CX 13 2 5 4 9 10 22 16 18 19 26 25
            MR 0 8 14 17 23 29
            OBSERVABLE_INCLUDE(0) rec[-5] rec[-4]
            DETECTOR rec[-12] rec[-11] rec[-8] rec[-6] rec[-5] rec[-2]
            DETECTOR rec[-10] rec[-9] rec[-7] rec[-4] rec[-3] rec[-1]
            DEPOLARIZE2(0.001) 24 1 7 6 11 12 3 15 20 21 28 27 13 2 5 4 9 10 22 16 18 19 26 25
            X_ERROR(0.001) 0 8 14 17 23 29
            X_ERROR(0.001) 2 4 10 16 19 25
            YCX 20 0 7 8 3 14 11 17 24 23 28 29
            XCX 9 1 5 6 13 12 18 15 22 21 26 27
            MR 2 4 10 16 19 25
            OBSERVABLE_INCLUDE(0) rec[-5] rec[-4]
            DEPOLARIZE2(0.001) 20 0 7 8 3 14 11 17 24 23 28 29 9 1 5 6 13 12 18 15 22 21 26 27
            X_ERROR(0.001) 2 4 10 16 19 25
            X_ERROR(0.001) 1 6 12 15 21 27
            YCX 5 0 9 8 13 14 26 17 22 23 18 29
            MR 1 6 12 15 21 27
            OBSERVABLE_INCLUDE(0) rec[-5] rec[-4]
            DETECTOR rec[-24] rec[-22] rec[-19] rec[-12] rec[-10] rec[-7] rec[-6] rec[-4] rec[-1]
            DETECTOR rec[-23] rec[-21] rec[-20] rec[-11] rec[-9] rec[-8] rec[-5] rec[-3] rec[-2]
            DEPOLARIZE2(0.001) 5 0 9 8 13 14 26 17 22 23 18 29
            X_ERROR(0.001) 1 6 12 15 21 27
            X_ERROR(0.001) 0 8 14 17 23 29
            MR 0 8 14 17 23 29
            OBSERVABLE_INCLUDE(0) rec[-5] rec[-4]
            DETECTOR rec[-30] rec[-29] rec[-26] rec[-24] rec[-23] rec[-20] rec[-12] rec[-11] rec[-8] rec[-6] rec[-5] rec[-2]
            DETECTOR rec[-28] rec[-27] rec[-25] rec[-22] rec[-21] rec[-19] rec[-10] rec[-9] rec[-7] rec[-4] rec[-3] rec[-1]
            X_ERROR(0.001) 0 8 14 17 23 29
            X_ERROR(0.001) 3 5 7 9 11 13 18 20 22 24 26 28
            M 3 5 7 9 11 13 18 20 22 24 26 28
            DETECTOR rec[-36] rec[-34] rec[-31] rec[-30] rec[-29] rec[-26] rec[-18] rec[-16] rec[-13] rec[-12] rec[-11] rec[-7] rec[-6] rec[-5] rec[-1]
            DETECTOR rec[-35] rec[-33] rec[-32] rec[-28] rec[-27] rec[-25] rec[-17] rec[-15] rec[-14] rec[-10] rec[-9] rec[-8] rec[-4] rec[-3] rec[-2]
            DETECTOR rec[-24] rec[-23] rec[-20] rec[-18] rec[-17] rec[-14] rec[-11] rec[-10] rec[-9] rec[-5] rec[-4] rec[-3]
            DETECTOR rec[-22] rec[-21] rec[-19] rec[-16] rec[-15] rec[-13] rec[-12] rec[-8] rec[-7] rec[-6] rec[-2] rec[-1]
            OBSERVABLE_INCLUDE(0) rec[-12] rec[-10] rec[-9] rec[-7]
        )CIRCUIT"),
        true,
        false,
        false,
        0.0,
        false,
        false);
}

TEST(ErrorAnalyzer, measure_pauli_product_4body) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                RX 0
                Z_ERROR(0.125) 0
                MPP X0*Z1
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.125) D0
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                MPP(0.25) Z0*Z1
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.25) D0
        )MODEL"));
}

TEST(ErrorAnalyzer, ignores_sweep_controls) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                X_ERROR(0.25) 0
                CNOT sweep[0] 0
                M 0
                DETECTOR rec[-1]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            error(0.25) D0
        )MODEL"));
}

TEST(ErrorAnalyzer, mpp_ordering) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                MPP X0*X1 X0
                TICK
                MPP X0
                DETECTOR rec[-1] rec[-2]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            detector D0
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                MPP X0*X1 X0 X0
                DETECTOR rec[-1] rec[-2]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            detector D0
        )MODEL"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
                MPP X2*X1 X0
                TICK
                MPP X0
                DETECTOR rec[-1] rec[-2]
            )CIRCUIT"),
            false,
            false,
            false,
            0.0,
            false,
            true),
        DetectorErrorModel(R"MODEL(
            detector D0
        )MODEL"));

    ASSERT_THROW(
        {
            ErrorAnalyzer::circuit_to_detector_error_model(
                Circuit(R"CIRCUIT(
                MPP X0 X0*X1
                TICK
                MPP X0
                DETECTOR rec[-1] rec[-2]
            )CIRCUIT"),
                false,
                false,
                false,
                0.0,
                false,
                true);
        },
        std::invalid_argument);

    ASSERT_THROW(
        {
            ErrorAnalyzer::circuit_to_detector_error_model(
                Circuit(R"CIRCUIT(
                MPP X0 X2*X1
                TICK
                MPP X0
                DETECTOR rec[-1] rec[-2]
            )CIRCUIT"),
                false,
                false,
                false,
                0.0,
                false,
                true);
        },
        std::invalid_argument);
}

TEST(ErrorAnalyzer, anticommuting_observable_error_message_help) {
    for (size_t folding = 0; folding < 2; folding++) {
        ASSERT_EQ(
            "",
            check_catch<std::invalid_argument>(
                R"ERROR(The circuit contains non-deterministic observables.
(Error analysis requires deterministic observables.)

This was discovered while analyzing an X-basis reset (RX) on:
    qubit 2

The collapse anti-commuted with these detectors/observables:
    L0

The backward-propagating error sensitivity for L0 was:
    X0 [coords (1, 2, 3)]
    Z2

Circuit stack trace:
    during TICK layer #1 of 201
    at instruction #2 [which is RX 2])ERROR",
                [&] {
                    ErrorAnalyzer::circuit_to_detector_error_model(
                        Circuit(R"CIRCUIT(
                            QUBIT_COORDS(1, 2, 3) 0
                            RX 2
                            REPEAT 10 {
                                REPEAT 20 {
                                    C_XYZ 0
                                    R 1
                                    M 1
                                    DETECTOR rec[-1]
                                    TICK
                                }
                            }
                            M 0 2
                            OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]
                        )CIRCUIT"),
                        false,
                        folding == 1,
                        false,
                        0.0,
                        false,
                        true);
                }));

        ASSERT_EQ(
            "",
            check_catch<std::invalid_argument>(
                R"ERROR(The circuit contains non-deterministic observables.
(Error analysis requires deterministic observables.)
The circuit contains non-deterministic detectors.
(To allow non-deterministic detectors, use the `allow_gauge_detectors` option.)

This was discovered while analyzing an X-basis reset (RX) on:
    qubit 0

The collapse anti-commuted with these detectors/observables:
    D101 [coords (1004, 2105, 6)]
    L0

The backward-propagating error sensitivity for D101 was:
    Z0

The backward-propagating error sensitivity for L0 was:
    Z0
    Z1

Circuit stack trace:
    during TICK layer #101 of 1402
    at instruction #4 [which is a REPEAT 100 block]
    at block's instruction #1 [which is RX 0])ERROR",
                [&] {
                    ErrorAnalyzer::circuit_to_detector_error_model(
                        Circuit(R"CIRCUIT(
                            TICK
                            SHIFT_COORDS(1000, 2000)
                            M 0 1
                            REPEAT 100 {
                                RX 0
                                DETECTOR rec[-1]
                                TICK
                            }
                            REPEAT 200 {
                                TICK
                            }
                            REPEAT 100 {
                                M 0 1
                                SHIFT_COORDS(0, 100)
                                DETECTOR(1, 2, 3) rec[-1] rec[-3]
                                DETECTOR(4, 5, 6) rec[-2] rec[-4]
                                OBSERVABLE_INCLUDE(0) rec[-1] rec[-2] rec[-3] rec[-4]
                                TICK
                            }
                            REPEAT 1000 {
                                TICK
                            }
                        )CIRCUIT"),
                        false,
                        folding == 1,
                        false,
                        0.0,
                        false,
                        true);
                }));
    }
}

TEST(ErrorAnalyzer, brute_force_decomp_simple) {
    MonotonicBuffer<DemTarget> buf;
    std::map<FixedCapVector<DemTarget, 2>, ConstPointerRange<DemTarget>> known;
    bool actual;
    std::vector<DemTarget> problem{
        DemTarget::relative_detector_id(0),
        DemTarget::relative_detector_id(1),
        DemTarget::relative_detector_id(2),
    };
    auto add = [&](uint32_t a, uint32_t b) {
        DemTarget a2 = DemTarget::relative_detector_id(a);
        DemTarget b2 = DemTarget::relative_detector_id(b);
        FixedCapVector<DemTarget, 2> v;
        v.push_back(a2);
        if (b != UINT32_MAX) {
            v.push_back(b2);
        }
        buf.append_tail({v.begin(), v.end()});
        known[v] = buf.commit_tail();
    };

    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_FALSE(actual);
    ASSERT_TRUE(buf.tail.empty());

    add(0, 2);

    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_FALSE(actual);
    ASSERT_TRUE(buf.tail.empty());

    add(1, UINT32_MAX);

    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_TRUE(actual);
    ASSERT_EQ(buf.tail.size(), 5);
    ASSERT_EQ(buf.tail[0], DemTarget::relative_detector_id(0));
    ASSERT_EQ(buf.tail[1], DemTarget::relative_detector_id(2));
    ASSERT_EQ(buf.tail[2], DemTarget::separator());
    ASSERT_EQ(buf.tail[3], DemTarget::relative_detector_id(1));
    ASSERT_EQ(buf.tail[4], DemTarget::separator());
}

TEST(ErrorAnalyzer, brute_force_decomp_introducing_obs_pair) {
    MonotonicBuffer<DemTarget> buf;
    std::map<FixedCapVector<DemTarget, 2>, ConstPointerRange<DemTarget>> known;
    bool actual;
    std::vector<DemTarget> problem{
        DemTarget::relative_detector_id(0),
        DemTarget::relative_detector_id(1),
        DemTarget::relative_detector_id(2),
    };
    auto add = [&](uint32_t a, uint32_t b, bool obs) {
        DemTarget a2 = DemTarget::relative_detector_id(a);
        DemTarget b2 = DemTarget::relative_detector_id(b);
        FixedCapVector<DemTarget, 2> v;
        v.push_back(a2);
        if (b != UINT32_MAX) {
            v.push_back(b2);
        }
        buf.append_tail({v.begin(), v.end()});
        if (obs) {
            buf.append_tail(DemTarget::observable_id(5));
        }
        known[v] = buf.commit_tail();
    };

    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_FALSE(actual);
    ASSERT_TRUE(buf.tail.empty());

    add(0, 2, true);

    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_FALSE(actual);
    ASSERT_TRUE(buf.tail.empty());

    add(1, UINT32_MAX, false);
    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_FALSE(actual);
    ASSERT_TRUE(buf.tail.empty());

    add(1, UINT32_MAX, true);
    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_TRUE(actual);
    ASSERT_EQ(buf.tail.size(), 7);
    ASSERT_EQ(buf.tail[0], DemTarget::relative_detector_id(0));
    ASSERT_EQ(buf.tail[1], DemTarget::relative_detector_id(2));
    ASSERT_EQ(buf.tail[2], DemTarget::observable_id(5));
    ASSERT_EQ(buf.tail[3], DemTarget::separator());
    ASSERT_EQ(buf.tail[4], DemTarget::relative_detector_id(1));
    ASSERT_EQ(buf.tail[5], DemTarget::observable_id(5));
    ASSERT_EQ(buf.tail[6], DemTarget::separator());
}

TEST(ErrorAnalyzer, brute_force_decomp_with_obs) {
    MonotonicBuffer<DemTarget> buf;
    std::map<FixedCapVector<DemTarget, 2>, ConstPointerRange<DemTarget>> known;
    bool actual;
    std::vector<DemTarget> problem{
        DemTarget::relative_detector_id(0),
        DemTarget::relative_detector_id(1),
        DemTarget::relative_detector_id(2),
        DemTarget::observable_id(5),
    };
    auto add = [&](uint32_t a, uint32_t b, bool obs) {
        DemTarget a2 = DemTarget::relative_detector_id(a);
        DemTarget b2 = DemTarget::relative_detector_id(b);
        FixedCapVector<DemTarget, 2> v;
        v.push_back(a2);
        if (b != UINT32_MAX) {
            v.push_back(b2);
        }
        buf.append_tail({v.begin(), v.end()});
        if (obs) {
            buf.append_tail(DemTarget::observable_id(5));
        }
        known[v] = buf.commit_tail();
    };

    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_FALSE(actual);
    ASSERT_TRUE(buf.tail.empty());

    add(0, 2, true);

    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_FALSE(actual);
    ASSERT_TRUE(buf.tail.empty());

    add(1, UINT32_MAX, false);
    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_TRUE(actual);
    ASSERT_EQ(buf.tail.size(), 6);
    ASSERT_EQ(buf.tail[0], DemTarget::relative_detector_id(0));
    ASSERT_EQ(buf.tail[1], DemTarget::relative_detector_id(2));
    ASSERT_EQ(buf.tail[2], DemTarget::observable_id(5));
    ASSERT_EQ(buf.tail[3], DemTarget::separator());
    ASSERT_EQ(buf.tail[4], DemTarget::relative_detector_id(1));
    ASSERT_EQ(buf.tail[5], DemTarget::separator());

    buf.discard_tail();
    add(0, 2, false);
    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_FALSE(actual);
    ASSERT_TRUE(buf.tail.empty());

    add(1, UINT32_MAX, true);
    actual = brute_force_decomposition_into_known_graphlike_errors({problem}, known, buf);
    ASSERT_TRUE(actual);
    ASSERT_EQ(buf.tail.size(), 6);
    ASSERT_EQ(buf.tail[0], DemTarget::relative_detector_id(0));
    ASSERT_EQ(buf.tail[1], DemTarget::relative_detector_id(2));
    ASSERT_EQ(buf.tail[2], DemTarget::separator());
    ASSERT_EQ(buf.tail[3], DemTarget::relative_detector_id(1));
    ASSERT_EQ(buf.tail[4], DemTarget::observable_id(5));
    ASSERT_EQ(buf.tail[5], DemTarget::separator());
}

TEST(ErrorAnalyzer, ignore_failures) {
    stim::Circuit circuit(Circuit(R"CIRCUIT(
        X_ERROR(0.25) 0
        MR 0
        DETECTOR rec[-1]
        DETECTOR rec[-1]
        DETECTOR rec[-1]

        X_ERROR(0.125) 0 1 2
        CORRELATED_ERROR(0.25) X0 X1 X2
        M 0 1 2
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        DETECTOR rec[-3]
    )CIRCUIT"));

    ASSERT_THROW(
        { ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, false, false, 0.0, false, false); },
        std::invalid_argument);

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, false, false, 0.0, true, false),
        DetectorErrorModel(R"MODEL(
            error(0.25) D0 D1 D2
            error(0.125) D3
            error(0.25) D3 ^ D4 ^ D5
            error(0.125) D4
            error(0.125) D5
        )MODEL"));
}

TEST(ErrorAnalyzer, block_remnant_edge) {
    stim::Circuit circuit(Circuit(R"CIRCUIT(
        X_ERROR(0.125) 0
        CORRELATED_ERROR(0.25) X0 X1
        M 0 1
        DETECTOR rec[-1]
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        DETECTOR rec[-2]
    )CIRCUIT"));

    ASSERT_THROW(
        { ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, false, false, 0.0, false, true); },
        std::invalid_argument);

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, false, false, 0.0, false, false),
        DetectorErrorModel(R"MODEL(
            error(0.125) D2 D3
            error(0.25) D2 D3 ^ D0 D1
        )MODEL"));
}

TEST(ErrorAnalyzer, dont_fold_when_observable_dependencies_cross_iterations) {
    Circuit c(R"CIRCUIT(
        RX 0 2
        REPEAT 100 {
            R 1
            CX 0 1 2 1
            MRZ 1
            MRX 2
        }
        MX 0
        # Doesn't include all elements from the loop.
        OBSERVABLE_INCLUDE(0) rec[-1] rec[-2] rec[-4]
    )CIRCUIT");
    ASSERT_ANY_THROW({ ErrorAnalyzer::circuit_to_detector_error_model(c, true, true, false, 1, false, false); });
}

TEST(ErrorAnalyzer, else_correlated_error_block) {
    Circuit c(R"CIRCUIT(
        CORRELATED_ERROR(0.25) X0
        ELSE_CORRELATED_ERROR(0.25) X1
        ELSE_CORRELATED_ERROR(0.25) X2
        M 0 1 2
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT");
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(c, true, true, false, 1, false, false), DetectorErrorModel(R"DEM(
            error(0.25) D0
            error(0.1875) D1
            error(0.140625) D2
        )DEM"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(c, true, true, false, 0.25, false, false),
        DetectorErrorModel(R"DEM(
            error(0.25) D0
            error(0.1875) D1
            error(0.140625) D2
        )DEM"));
    ASSERT_THROW(
        { ErrorAnalyzer::circuit_to_detector_error_model(c, true, true, false, 0, false, false); },
        std::invalid_argument);
    ASSERT_THROW(
        { ErrorAnalyzer::circuit_to_detector_error_model(c, true, true, false, 0.1, false, false); },
        std::invalid_argument);
    ASSERT_THROW(
        { ErrorAnalyzer::circuit_to_detector_error_model(c, true, true, false, 0.24, false, false); },
        std::invalid_argument);

    c = Circuit(R"CIRCUIT(
        CORRELATED_ERROR(0.25) X0
        ELSE_CORRELATED_ERROR(0.25) X1
        ELSE_CORRELATED_ERROR(0.25) X2
        CORRELATED_ERROR(0.25) X3
        ELSE_CORRELATED_ERROR(0.25) X4
        ELSE_CORRELATED_ERROR(0.25) X5
        M 0 1 2 3 4 5
        DETECTOR rec[-6]
        DETECTOR rec[-5]
        DETECTOR rec[-4]
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    )CIRCUIT");
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(c, true, true, false, 1, false, false), DetectorErrorModel(R"DEM(
            error(0.25) D0
            error(0.1875) D1
            error(0.140625) D2
            error(0.25) D3
            error(0.1875) D4
            error(0.140625) D5
        )DEM"));

    c = Circuit(R"CIRCUIT(
        CORRELATED_ERROR(0.25) X0
        ELSE_CORRELATED_ERROR(0.25) Z1
        H 1
        ELSE_CORRELATED_ERROR(0.25) X2
    )CIRCUIT");
    ASSERT_THROW(
        { ErrorAnalyzer::circuit_to_detector_error_model(c, true, true, false, 1, false, false); },
        std::invalid_argument);

    c = Circuit(R"CIRCUIT(
        CORRELATED_ERROR(0.25) X0
        REPEAT 1 {
            ELSE_CORRELATED_ERROR(0.25) Z1
        }
    )CIRCUIT");
    ASSERT_THROW(
        { ErrorAnalyzer::circuit_to_detector_error_model(c, true, true, false, 1, false, false); },
        std::invalid_argument);

    c = Circuit(R"CIRCUIT(
        ELSE_CORRELATED_ERROR(0.25) Z1
    )CIRCUIT");
    ASSERT_THROW(
        { ErrorAnalyzer::circuit_to_detector_error_model(c, true, true, false, 1, false, false); },
        std::invalid_argument);
}
