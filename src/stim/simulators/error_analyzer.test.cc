#include "stim/simulators/error_analyzer.h"

#include <regex>

#include "gtest/gtest.h"

#include "stim/circuit/circuit.test.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/mem/simd_word.test.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/util_bot/test_util.test.h"
#include "stim/util_top/circuit_to_dem.h"

using namespace stim;

TEST(ErrorAnalyzer, circuit_to_detector_error_model) {
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
                DETECTOR rec[-1]
                OBSERVABLE_INCLUDE(0) rec[-1]
            )circuit"),
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
            Circuit(R"circuit(
                Y_ERROR(0.25) 3
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
                Z_ERROR(0.25) 3
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

    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
                DEPOLARIZE1(0.25) 3
                M 3
                DETECTOR rec[-1]
            )circuit"),
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
            Circuit(R"circuit(
                X_ERROR(0.25) 0
                X_ERROR(0.125) 1
                M 0 1
                OBSERVABLE_INCLUDE(3) rec[-1]
                DETECTOR rec[-2]
        )circuit"),
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
            Circuit(R"circuit(
                X_ERROR(0.25) 0
                X_ERROR(0.125) 1
                M 0 1
                OBSERVABLE_INCLUDE(3) rec[-1]
                DETECTOR rec[-2]
            )circuit"),
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

    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
                        DEPOLARIZE2(0.25) 3 5
                        M 3
                        M 5
                        DETECTOR rec[-1]
                        DETECTOR rec[-2]
                    )circuit"),
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

    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
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

    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
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

    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
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
                    )circuit"),
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

TEST_EACH_WORD_SIZE_W(ErrorAnalyzer, unitary_gates_match_frame_simulator, {
    CircuitStats stats;
    stats.num_qubits = 16;
    stats.num_measurements = 100;
    FrameSimulator<W> f(stats, FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, 16, INDEPENDENT_TEST_RNG());
    ErrorAnalyzer e(100, 1, 16, 100, false, false, false, 0.0, false, true);
    for (size_t q = 0; q < 16; q++) {
        if (q & 1) {
            e.tracker.xs[q].xor_item({0});
            f.x_table[q][0] = true;
        }
        if (q & 2) {
            e.tracker.xs[q].xor_item({1});
            f.x_table[q][1] = true;
        }
        if (q & 4) {
            e.tracker.zs[q].xor_item({0});
            f.z_table[q][0] = true;
        }
        if (q & 8) {
            e.tracker.zs[q].xor_item({1});
            f.z_table[q][1] = true;
        }
    }

    std::vector<GateTarget> data;
    for (size_t k = 0; k < 16; k++) {
        data.push_back(GateTarget::qubit(k));
    }
    for (const auto &gate : GATE_DATA.items) {
        if (gate.has_known_unitary_matrix()) {
            e.undo_gate(CircuitInstruction{gate.id, {}, data, ""});
            f.do_gate(CircuitInstruction{gate.inverse().id, {}, data, ""});
            for (size_t q = 0; q < 16; q++) {
                bool xs[2]{};
                bool zs[2]{};
                for (auto x : e.tracker.xs[q]) {
                    ASSERT_TRUE(x.data < 2) << gate.name;
                    xs[x.data] = true;
                }
                for (auto z : e.tracker.zs[q]) {
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
})

TEST(ErrorAnalyzer, reversed_operation_order) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
                X_ERROR(0.25) 0
                CNOT 0 1
                CNOT 1 0
                M 0 1
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
            detector D0
        )model"));

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"circuit(
                X_ERROR(0.25) 0
                CNOT 0 1
                CNOT 1 0
                M 0 1
                DETECTOR rec[-1]
                DETECTOR rec[-2]
            )circuit"),
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
            Circuit(R"circuit(
                X_ERROR(0.125) 0
                M 0
                CNOT rec[-1] 1
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
            Circuit(R"circuit(
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
        DetectorErrorModel(R"model(
            error(0.5) D0 D1
        )model"));

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
        DetectorErrorModel(R"model(
            error(0.5) D0 D1
        )model"));

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
        DetectorErrorModel(R"model(
            error(0.5) D0 D1
        )model"));

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
        DetectorErrorModel(R"model(
            error(0.5) D0 D1
        )model"));

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
        DetectorErrorModel(R"model(
            error(0.5) D0 D1
        )model"));

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
        DetectorErrorModel(R"model(
            error(0.5) D0 D1
        )model"));

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
        DetectorErrorModel(R"model(
            error(0.5) D0 D1
        )model"));

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
        DetectorErrorModel(R"model(
            error(0.5) D0 D1
        )model"));
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
    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
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

    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
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
    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(encode + Circuit("DEPOLARIZE2(0.01) 4 5") + decode), false, false, false, 0.0, false, true)
            .approx_equals(expected, 1e-5));
    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(encode + Circuit("CNOT 4 5\nDEPOLARIZE2(0.01) 4 5\nCNOT 4 5") + decode),
            false,
            false,
            false,
            0.0,
            false,
            true)
            .approx_equals(expected, 1e-5));
    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
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

    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
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

TEST(ErrorAnalyzer, exact_solved_pauli_channel_1_is_let_through) {
    auto c = Circuit(R"CIRCUIT(
        R 0
        PAULI_CHANNEL_1(0.1, 0.2, 0.15) 0
        M 0
        DETECTOR rec[-1]
    )CIRCUIT");
    auto actual_dem = ErrorAnalyzer::circuit_to_detector_error_model(c, false, false, false, 0, false, true);
    ASSERT_TRUE(actual_dem.approx_equals(
        DetectorErrorModel(R"MODEL(
        error(0.3) D0
    )MODEL"),
        1e-6));
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
            error(0.375) D0
        )MODEL"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(c1, false, false, false, 1, false, true),
        DetectorErrorModel(R"MODEL(
            error(0.375) D0
        )MODEL"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(c2, false, false, false, 0.38, false, true),
        DetectorErrorModel(R"MODEL(
            error(0.375) D0
        )MODEL"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(c2, false, false, false, 1, false, true),
        DetectorErrorModel(R"MODEL(
            error(0.375) D0
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
    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(encode + Circuit("PAULI_CHANNEL_1(0.00001, 0.02, 0.03) 4") + decode),
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
                error(0.00001) D1 D3
                detector D2
                detector D5
            )model"),
                1e-6));
    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(encode + Circuit("PAULI_CHANNEL_1(0.00001, 0.02, 0.03) 5") + decode),
            true,
            false,
            false,
            0.1,
            false,
            true)
            .approx_equals(
                DetectorErrorModel(R"model(
                error(0.00001) D1 D5
                error(0.02) D1 D5 ^ D2 D4
                error(0.03) D2 D4
                detector D0
                detector D3
            )model"),
                1e-6));

    ASSERT_TRUE(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(
                encode +
                Circuit(
                    "PAULI_CHANNEL_2(0.001,0.002,0.003,0.004,0.005,0.006,0.007,0.008,0.009,0.010,0."
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
std::string expect_catch_message(std::function<void(void)> func) {
    try {
        func();
        EXPECT_TRUE(false) << "Function didn't throw an exception.";
        return "";
    } catch (const TEx &ex) {
        return ex.what();
    }
}

TEST(ErrorAnalyzer, context_clues_for_errors) {
    ASSERT_EQ(
        expect_catch_message<std::invalid_argument>([&]() {
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
        }),
        "Can't analyze over-mixing DEPOLARIZE1 errors (probability > 3/4).\n"
        "\n"
        "Circuit stack trace:\n"
        "    at instruction #2 [which is DEPOLARIZE1(1) 0]");

    ASSERT_EQ(
        expect_catch_message<std::invalid_argument>([&]() {
            circuit_to_dem(
                Circuit(R"CIRCUIT(
                X 0
                Y 1
                REPEAT 500 {
                    DEPOLARIZE1(1) 0
                }
                Z 3
            )CIRCUIT"),
                {.block_decomposition_from_introducing_remnant_edges = true});
        }),
        "Can't analyze over-mixing DEPOLARIZE1 errors (probability > 3/4).\n"
        "\n"
        "Circuit stack trace:\n"
        "    at instruction #3 [which is a REPEAT 500 block]\n"
        "    at block's instruction #1 [which is DEPOLARIZE1(1) 0]");
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

    ASSERT_EQ(
        expect_catch_message<std::invalid_argument>([&]() {
            circuit_to_dem(
                symptoms_20, {.decompose_errors = true, .block_decomposition_from_introducing_remnant_edges = true});
        }),
        R"MSG(An error case in a composite error exceeded the max supported number of symptoms (<=15).
The 2 basis error cases (e.g. X, Z) used to form the combined error cases (e.g. Y = X*Z) are:
0:
1: D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15, D16, D17, D18, D19


Circuit stack trace:
    at instruction #1 [which is DEPOLARIZE1(0.001) 0])MSG");

    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(symptoms_20, false, false, false, 0.0, false, true),
        DetectorErrorModel(R"model(
            error(0.0006666666666666692465) D0 D1 D2 D3 D4 D5 D6 D7 D8 D9 D10 D11 D12 D13 D14 D15 D16 D17 D18 D19
        )model"));
}

TEST(ErrorAnalyzer, decompose_error_failures) {
    ASSERT_EQ(
        expect_catch_message<std::invalid_argument>([&]() {
            circuit_to_dem(
                Circuit(R"CIRCUIT(
                DEPOLARIZE1(0.001) 0
                M 0
                DETECTOR rec[-1]
                DETECTOR rec[-1]
                DETECTOR rec[-1]
            )CIRCUIT"),
                {.decompose_errors = true, .block_decomposition_from_introducing_remnant_edges = true});
        }),
        R"MSG(Failed to decompose errors into graphlike components with at most two symptoms.
The error component that failed to decompose is 'D0, D1, D2'.

In Python, you can ignore this error by passing `ignore_decomposition_failures=True` to `stim.Circuit.detector_error_model(...)`.
From the command line, you can ignore this error by passing the flag `--ignore_decomposition_failures` to `stim analyze_errors`.

Note: `block_decomposition_from_introducing_remnant_edges` is ON.
Turning it off may prevent this error.)MSG");

    ASSERT_EQ(
        expect_catch_message<std::invalid_argument>([&]() {
            circuit_to_dem(
                Circuit(R"CIRCUIT(
                X_ERROR(0.001) 0
                M 0
                DETECTOR rec[-1]
                DETECTOR rec[-1]
                DETECTOR rec[-1]
            )CIRCUIT"),
                {.decompose_errors = true, .block_decomposition_from_introducing_remnant_edges = true});
        }),
        R"MSG(Failed to decompose errors into graphlike components with at most two symptoms.
The error component that failed to decompose is 'D0, D1, D2'.

In Python, you can ignore this error by passing `ignore_decomposition_failures=True` to `stim.Circuit.detector_error_model(...)`.
From the command line, you can ignore this error by passing the flag `--ignore_decomposition_failures` to `stim analyze_errors`.

Note: `block_decomposition_from_introducing_remnant_edges` is ON.
Turning it off may prevent this error.)MSG");

    ASSERT_EQ(
        expect_catch_message<std::invalid_argument>([&]() {
            circuit_to_dem(
                Circuit(R"CIRCUIT(
                X_ERROR(0.001) 0
                M 0
                DETECTOR rec[-1]
                DETECTOR rec[-1]
                DETECTOR rec[-1]
                OBSERVABLE_INCLUDE(5) rec[-1]
            )CIRCUIT"),
                {.decompose_errors = true, .block_decomposition_from_introducing_remnant_edges = true});
        }),
        R"MSG(Failed to decompose errors into graphlike components with at most two symptoms.
The error component that failed to decompose is 'D0, D1, D2, L5'.

In Python, you can ignore this error by passing `ignore_decomposition_failures=True` to `stim.Circuit.detector_error_model(...)`.
From the command line, you can ignore this error by passing the flag `--ignore_decomposition_failures` to `stim analyze_errors`.

Note: `block_decomposition_from_introducing_remnant_edges` is ON.
Turning it off may prevent this error.)MSG");
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
    ASSERT_TRUE(is_graphlike(
        std::vector<DemTarget>{
            DemTarget::observable_id(0),
            DemTarget::observable_id(1),
            DemTarget::observable_id(2),
            DemTarget::separator(),
            DemTarget::observable_id(1),
        }));
    ASSERT_TRUE(is_graphlike(
        std::vector<DemTarget>{
            DemTarget::observable_id(0),
            DemTarget::relative_detector_id(1),
            DemTarget::observable_id(2),
            DemTarget::separator(),
            DemTarget::observable_id(1),
        }));
    ASSERT_TRUE(is_graphlike(
        std::vector<DemTarget>{
            DemTarget::observable_id(0),
            DemTarget::relative_detector_id(1),
            DemTarget::relative_detector_id(2),
            DemTarget::separator(),
            DemTarget::observable_id(1),
        }));
    ASSERT_FALSE(is_graphlike(
        std::vector<DemTarget>{
            DemTarget::relative_detector_id(0),
            DemTarget::relative_detector_id(1),
            DemTarget::relative_detector_id(2),
            DemTarget::separator(),
            DemTarget::observable_id(1),
        }));
    ASSERT_FALSE(is_graphlike(
        std::vector<DemTarget>{
            DemTarget::relative_detector_id(0),
            DemTarget::relative_detector_id(1),
            DemTarget::relative_detector_id(2),
        }));
    ASSERT_FALSE(is_graphlike(
        std::vector<DemTarget>{
            DemTarget::separator(),
            DemTarget::separator(),
            DemTarget::relative_detector_id(0),
            DemTarget::relative_detector_id(1),
            DemTarget::relative_detector_id(2),
            DemTarget::separator(),
            DemTarget::separator(),
        }));
    ASSERT_TRUE(is_graphlike(
        std::vector<DemTarget>{
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
            expect_catch_message<std::invalid_argument>([&]() {
                circuit_to_dem(
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
                    {.flatten_loops = folding != 1});
            }),
            R"ERROR(The circuit contains non-deterministic observables.

To make an SVG picture of the problem, you can use the python API like this:
    your_circuit.diagram('detslice-with-ops-svg', tick=range(0, 5), filter_coords=['L0', ])
or the command line API like this:
    stim diagram --in your_circuit_file.stim --type detslice-with-ops-svg --tick 0:5 --filter_coords L0 > output_image.svg

This was discovered while analyzing an X-basis reset (RX) on:
    qubit 2

The collapse anti-commuted with these detectors/observables:
    L0

The backward-propagating error sensitivity for L0 was:
    X0 [coords (1, 2, 3)]
    Z2

Circuit stack trace:
    during TICK layer #1 of 201
    at instruction #2 [which is RX 2])ERROR");

        ASSERT_EQ(
            expect_catch_message<std::invalid_argument>([&]() {
                circuit_to_dem(
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
                    {.flatten_loops = folding != 1});
            }),
            R"ERROR(The circuit contains non-deterministic observables.
The circuit contains non-deterministic detectors.

To make an SVG picture of the problem, you can use the python API like this:
    your_circuit.diagram('detslice-with-ops-svg', tick=range(95, 105), filter_coords=['D101', 'L0', ])
or the command line API like this:
    stim diagram --in your_circuit_file.stim --type detslice-with-ops-svg --tick 95:105 --filter_coords D101:L0 > output_image.svg

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
    at block's instruction #1 [which is RX 0])ERROR");
    }
}

TEST(ErrorAnalyzer, brute_force_decomp_simple) {
    MonotonicBuffer<DemTarget> buf;
    std::map<FixedCapVector<DemTarget, 2>, SpanRef<const DemTarget>> known;
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
    std::map<FixedCapVector<DemTarget, 2>, SpanRef<const DemTarget>> known;
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
    std::map<FixedCapVector<DemTarget, 2>, SpanRef<const DemTarget>> known;
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

TEST(ErrorAnalyzer, measurement_before_beginning) {
    Circuit c(R"CIRCUIT(
        DETECTOR rec[-1]
    )CIRCUIT");
    ASSERT_THROW(
        { ErrorAnalyzer::circuit_to_detector_error_model(c, false, false, false, false, false, false); },
        std::invalid_argument);

    c = Circuit(R"CIRCUIT(
        OBSERVABLE_INCLUDE(0) rec[-1]
    )CIRCUIT");
    ASSERT_THROW(
        { ErrorAnalyzer::circuit_to_detector_error_model(c, false, false, false, false, false, false); },
        std::invalid_argument);
}

TEST(ErrorAnalyzer, mpad) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
            M(0.125) 5
            MPAD 0 1
            DETECTOR rec[-1] rec[-2]
            DETECTOR rec[-3]
        )CIRCUIT"),
            false,
            false,
            false,
            false,
            false,
            false),
        DetectorErrorModel(R"DEM(
            error(0.125) D1
            detector D0
        )DEM"));
}

TEST(ErrorAnalyzer, mxx) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
            RX 0 1
            MXX(0.125) 0 1
            DETECTOR rec[-1]
        )CIRCUIT"),
            false,
            false,
            false,
            false,
            false,
            false),
        DetectorErrorModel(R"DEM(
            error(0.125) D0
        )DEM"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
            RX 0 1 2 3
            X_ERROR(0.125) 0
            Y_ERROR(0.25) 1
            Z_ERROR(0.375) 2
            MXX 0 1 !2 !3
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )CIRCUIT"),
            false,
            false,
            false,
            false,
            false,
            false),
        DetectorErrorModel(R"DEM(
            error(0.25) D0
            error(0.375) D1
        )DEM"));
}

TEST(ErrorAnalyzer, myy) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
            RY 0 1
            MYY(0.125) 0 1
            DETECTOR rec[-1]
        )CIRCUIT"),
            false,
            false,
            false,
            false,
            false,
            false),
        DetectorErrorModel(R"DEM(
            error(0.125) D0
        )DEM"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
            RY 0 1 2 3
            Y_ERROR(0.125) 0
            X_ERROR(0.25) 1
            Z_ERROR(0.375) 2
            MYY 0 1 !2 !3
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )CIRCUIT"),
            false,
            false,
            false,
            false,
            false,
            false),
        DetectorErrorModel(R"DEM(
            error(0.25) D0
            error(0.375) D1
        )DEM"));
}

TEST(ErrorAnalyzer, mzz) {
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
            RZ 0 1
            MZZ(0.125) 0 1
            DETECTOR rec[-1]
        )CIRCUIT"),
            false,
            false,
            false,
            false,
            false,
            false),
        DetectorErrorModel(R"DEM(
            error(0.125) D0
        )DEM"));
    ASSERT_EQ(
        ErrorAnalyzer::circuit_to_detector_error_model(
            Circuit(R"CIRCUIT(
            RZ 0 1 2 3
            Z_ERROR(0.125) 0
            Y_ERROR(0.25) 1
            X_ERROR(0.375) 2
            MZZ 0 1 !2 !3
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        )CIRCUIT"),
            false,
            false,
            false,
            false,
            false,
            false),
        DetectorErrorModel(R"DEM(
            error(0.25) D0
            error(0.375) D1
        )DEM"));
}

TEST(ErrorAnalyzer, heralded_erase_conditional_division) {
    auto dem = [](bool h, bool x, bool z) {
        Circuit c(R"CIRCUIT(
            MPP X0*X1 Z0*Z1
            HERALDED_ERASE(1.0) 0
            MPP X0*X1 Z0*Z1
        )CIRCUIT");
        if (h) {
            c.append_from_text("DETECTOR rec[-3]");
        } else {
            c.append_from_text("DETECTOR");
        }
        if (x) {
            c.append_from_text("DETECTOR rec[-2] rec[-5]");
        } else {
            c.append_from_text("DETECTOR");
        }
        if (z) {
            c.append_from_text("DETECTOR rec[-1] rec[-4]");
        } else {
            c.append_from_text("DETECTOR");
        }
        return ErrorAnalyzer::circuit_to_detector_error_model(c, false, false, false, 1.0, false, false);
    };
    EXPECT_EQ(dem(0, 0, 0), DetectorErrorModel(R"DEM(
        detector D0
        detector D1
        detector D2
    )DEM"));
    EXPECT_EQ(dem(0, 0, 1), DetectorErrorModel(R"DEM(
        error(0.5) D2
        detector D0
        detector D1
    )DEM"));
    EXPECT_EQ(dem(0, 1, 0), DetectorErrorModel(R"DEM(
        error(0.5) D1
        detector D0
        detector D2
    )DEM"));
    EXPECT_EQ(dem(0, 1, 1), DetectorErrorModel(R"DEM(
        error(0.25) D1
        error(0.25) D1 D2
        error(0.25) D2
        detector D0
    )DEM"));
    EXPECT_EQ(dem(1, 0, 0), DetectorErrorModel(R"DEM(
        error(1.0) D0
        detector D1
        detector D2
    )DEM"));
    EXPECT_EQ(dem(1, 0, 1), DetectorErrorModel(R"DEM(
        error(0.5) D0
        error(0.5) D0 D2
        detector D1
    )DEM"));
    EXPECT_EQ(dem(1, 1, 0), DetectorErrorModel(R"DEM(
        error(0.5) D0
        error(0.5) D0 D1
        detector D2
    )DEM"));
    EXPECT_EQ(dem(1, 1, 1), DetectorErrorModel(R"DEM(
        error(0.25) D0
        error(0.25) D0 D1
        error(0.25) D0 D1 D2
        error(0.25) D0 D2
    )DEM"));
}

TEST(ErrorAnalyzer, heralded_erase) {
    circuit_to_dem(Circuit("HERALDED_ERASE(0.25) 0"), {.approximate_disjoint_errors_threshold = 0.3});
    ASSERT_THROW(
        { circuit_to_dem(Circuit("HERALDED_ERASE(0.25) 0"), {.approximate_disjoint_errors_threshold = 0.2}); },
        std::invalid_argument);

    ASSERT_EQ(
        circuit_to_dem(
            Circuit(R"CIRCUIT(
                MZZ 0 1
                MXX 0 1
                HERALDED_ERASE(0.25) 0
                MZZ 0 1
                MXX 0 1
                DETECTOR rec[-1] rec[-4]
                DETECTOR rec[-2] rec[-5]
                DETECTOR rec[-3]
            )CIRCUIT"),
            {.approximate_disjoint_errors_threshold = 1}),
        DetectorErrorModel(R"DEM(
            error(0.0625) D0 D1 D2
            error(0.0625) D0 D2
            error(0.0625) D1 D2
            error(0.0625) D2
        )DEM"));

    ASSERT_EQ(
        circuit_to_dem(
            Circuit(R"CIRCUIT(
                MPP X10*X11*X20*X21
                MPP Z11*Z12*Z21*Z22
                MPP Z20*Z21*Z30*Z31
                MPP X21*X22*X31*X32
                HERALDED_ERASE(0.25) 21
                MPP X10*X11*X20*X21
                MPP Z11*Z12*Z21*Z22
                MPP Z20*Z21*Z30*Z31
                MPP X21*X22*X31*X32
                DETECTOR rec[-1] rec[-6]
                DETECTOR rec[-2] rec[-7]
                DETECTOR rec[-3] rec[-8]
                DETECTOR rec[-4] rec[-9]
                DETECTOR rec[-5]
            )CIRCUIT"),
            {.decompose_errors = true, .approximate_disjoint_errors_threshold = 1}),
        DetectorErrorModel(R"DEM(
            error(0.0625) D0 D3 ^ D1 D2 ^ D4
            error(0.0625) D0 D3 ^ D4
            error(0.0625) D1 D2 ^ D4
            error(0.0625) D4
        )DEM"));

    ASSERT_EQ(
        circuit_to_dem(
            Circuit(R"CIRCUIT(
                M 0
                HERALDED_ERASE(0.25) 9 0 9 9 9
                M 0
                DETECTOR rec[-1] rec[-7]
                DETECTOR rec[-5]
            )CIRCUIT"),
            {.approximate_disjoint_errors_threshold = 1}),
        DetectorErrorModel(R"DEM(
            error(0.125) D0 D1
            error(0.125) D1
        )DEM"));

    ASSERT_EQ(
        circuit_to_dem(
            Circuit(R"CIRCUIT(
                MPAD 0
                MPAD 0
                MPP Z20*Z21*Z30*Z31
                MPP X21*X22*X31*X32
                HERALDED_ERASE(0.25) 21
                MPAD 0
                MPAD 0
                MPP Z20*Z21*Z30*Z31
                MPP X21*X22*X31*X32
                DETECTOR rec[-1] rec[-6]
                DETECTOR rec[-2] rec[-7]
                DETECTOR rec[-3] rec[-8]
                DETECTOR rec[-4] rec[-9]
                DETECTOR rec[-5]
            )CIRCUIT"),
            {.decompose_errors = true, .approximate_disjoint_errors_threshold = 1}),
        DetectorErrorModel(R"DEM(
            error(0.0625) D0 ^ D1 ^ D4
            error(0.0625) D0 ^ D4
            error(0.0625) D1 ^ D4
            error(0.0625) D4
            detector D2
            detector D3
        )DEM"));
}

TEST(ErrorAnalyzer, runs_on_general_circuit) {
    auto circuit = generate_test_circuit_with_all_operations();
    auto dem = ErrorAnalyzer::circuit_to_detector_error_model(circuit, false, false, false, true, false, false);
    ASSERT_GT(dem.instructions.size(), 0);
}

TEST(ErrorAnalyzer, heralded_pauli_channel_1) {
    ErrorAnalyzer::circuit_to_detector_error_model(
        Circuit("HERALDED_PAULI_CHANNEL_1(0.01, 0.02, 0.25, 0.03) 0"), false, false, false, 0.3, false, false);
    ASSERT_THROW(
        {
            ErrorAnalyzer::circuit_to_detector_error_model(
                Circuit("HERALDED_PAULI_CHANNEL_1(0.01, 0.02, 0.25, 0.03) 0"), false, false, false, 0.2, false, false);
        },
        std::invalid_argument);

    ASSERT_TRUE(circuit_to_dem(
                    Circuit(R"CIRCUIT(
                        MZZ 0 1
                        MXX 0 1
                        HERALDED_PAULI_CHANNEL_1(0.01, 0.02, 0.03, 0.04) 0
                        MZZ 0 1
                        MXX 0 1
                        DETECTOR rec[-1] rec[-4]
                        DETECTOR rec[-2] rec[-5]
                        DETECTOR rec[-3]
                    )CIRCUIT"),
                    {.approximate_disjoint_errors_threshold = 1})
                    .approx_equals(
                        DetectorErrorModel(R"DEM(
                            error(0.03) D0 D1 D2
                            error(0.04) D0 D2
                            error(0.02) D1 D2
                            error(0.01) D2
                        )DEM"),
                        1e-6));

    ASSERT_TRUE(circuit_to_dem(
                    Circuit(R"CIRCUIT(
                        MZZ 0 1
                        MXX 0 1
                        HERALDED_PAULI_CHANNEL_1(0.01, 0.02, 0.03, 0.1) 0
                        MZZ 0 1
                        MXX 0 1
                        DETECTOR
                        DETECTOR rec[-2] rec[-5]
                        DETECTOR rec[-3]
                    )CIRCUIT"),
                    {.approximate_disjoint_errors_threshold = 1})
                    .approx_equals(
                        DetectorErrorModel(R"DEM(
                            error(0.05) D1 D2
                            error(0.11) D2
                            detector D0
                        )DEM"),
                        1e-6));
}


TEST(ErrorAnalyzer, OBS_INCLUDE_PAULIS) {
    auto circuit = Circuit(R"CIRCUIT(
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
        X_ERROR(0.125) 0
        Y_ERROR(0.25) 0
        Z_ERROR(0.375) 0
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
    )CIRCUIT");
    ASSERT_EQ(circuit_to_dem(circuit), DetectorErrorModel(R"DEM(
        error(0.375) L0 L1
        error(0.25) L0 L2
        error(0.125) L1 L2
    )DEM"));

    circuit = Circuit(R"CIRCUIT(
        DEPOLARIZE1(0.125) 0
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
        X_ERROR(0.25) 0
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
        DEPOLARIZE1(0.125) 0
    )CIRCUIT");
    ASSERT_EQ(circuit_to_dem(circuit), DetectorErrorModel(R"DEM(
        error(0.25) L1 L2
        logical_observable L0
        logical_observable L0
    )DEM"));

    circuit = Circuit(R"CIRCUIT(
        DEPOLARIZE1(0.125) 0
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
        Y_ERROR(0.25) 0
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
        DEPOLARIZE1(0.125) 0
    )CIRCUIT");
    ASSERT_EQ(circuit_to_dem(circuit), DetectorErrorModel(R"DEM(
        error(0.25) L0 L2
        logical_observable L1
        logical_observable L1
    )DEM"));

    circuit = Circuit(R"CIRCUIT(
        DEPOLARIZE1(0.125) 0
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
        Z_ERROR(0.25) 0
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
        DEPOLARIZE1(0.125) 0
    )CIRCUIT");
    ASSERT_EQ(circuit_to_dem(circuit), DetectorErrorModel(R"DEM(
        error(0.25) L0 L1
        logical_observable L2
        logical_observable L2
    )DEM"));
}

TEST(ErrorAnalyzer, tagged_noise) {
    ASSERT_EQ(circuit_to_dem(Circuit(R"CIRCUIT(
        R[test-tag-0] 0
        X_ERROR[test-tag-1](0.25) 0
        M[test-tag-2] 0
        DETECTOR[test-tag-3] rec[-1]
        OBSERVABLE_INCLUDE[test-tag-4](0) rec[-1]
        SHIFT_COORDS[test-tag-5](1)
    )CIRCUIT")), DetectorErrorModel(R"DEM(
        error[test-tag-1](0.25) D0 L0
        detector[test-tag-3] D0
        logical_observable[test-tag-4] L0
        shift_detectors[test-tag-5](1.0) 0
    )DEM"));

    ASSERT_EQ(circuit_to_dem(Circuit(R"CIRCUIT(
        OBSERVABLE_INCLUDE[test-tag-1](0)
        OBSERVABLE_INCLUDE[test-tag-2](0)
    )CIRCUIT")), DetectorErrorModel(R"DEM(
        logical_observable[test-tag-1] L0
        logical_observable[test-tag-2] L0
    )DEM"));

    ASSERT_EQ(circuit_to_dem(Circuit(R"CIRCUIT(
        R 0
        X_ERROR[test-tag-0](0.25) 0
        REPEAT[test-tag-1] 100 {
            X_ERROR[test-tag-2](0.125) 0
            MR 0
            DETECTOR[test-tag-3] rec[-1]
            OBSERVABLE_INCLUDE[test-tag-4](0) rec[-1]
        }
    )CIRCUIT"), {.decompose_errors=false, .flatten_loops=false}), DetectorErrorModel(R"DEM(
        error[test-tag-0](0.25) D0 L0
        repeat[test-tag-1] 99 {
            error[test-tag-2](0.125) D0 L0
            detector[test-tag-3] D0
            logical_observable[test-tag-4] L0
            shift_detectors 1
        }
        error[test-tag-2](0.125) D0 L0
        detector[test-tag-3] D0
        logical_observable[test-tag-4] L0
    )DEM"));
}
