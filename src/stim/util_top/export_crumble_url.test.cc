#include "stim/util_top/export_crumble_url.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.test.h"
#include "stim/search/graphlike/algo.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/error_matcher.h"

using namespace stim;

TEST(export_crumble, all_operations) {
    auto actual = export_crumble_url(generate_test_circuit_with_all_operations());
    auto expected =
        "https://algassert.com/crumble#circuit="
        "Q(1,2,3)0;"
        "I_0;"
        "X_1;"
        "Y_2;"
        "Z_3;"
        "TICK;"
        "C_XYZ_0;"
        "C_NXYZ_1;"
        "C_XNYZ_2;"
        "C_XYNZ_3;"
        "C_ZYX_4;"
        "C_NZYX_5;"
        "C_ZNYX_6;"
        "C_ZYNX_7;"
        "H_XY_0;"
        "H_1;"
        "H_YZ_2;"
        "H_NXY_3;"
        "H_NXZ_4;"
        "H_NYZ_5;"
        "SQRT_X_0;"
        "SQRT_X_DAG_1;"
        "SQRT_Y_2;"
        "SQRT_Y_DAG_3;"
        "S_4;"
        "S_DAG_5;"
        "TICK;"
        "CXSWAP_0_1;"
        "ISWAP_2_3;"
        "ISWAP_DAG_4_5;"
        "SWAP_6_7;"
        "SWAPCX_8_9;"
        "CZSWAP_10_11;"
        "SQRT_XX_0_1;"
        "SQRT_XX_DAG_2_3;"
        "SQRT_YY_4_5;"
        "SQRT_YY_DAG_6_7;"
        "SQRT_ZZ_8_9;"
        "SQRT_ZZ_DAG_10_11;"
        "II_12_13;"
        "XCX_0_1;"
        "XCY_2_3;"
        "XCZ_4_5;"
        "YCX_6_7;"
        "YCY_8_9;"
        "YCZ_10_11;"
        "CX_12_13;"
        "CY_14_15;"
        "CZ_16_17;"
        "TICK;"
        "E(0.01)X1_Y2_Z3;"
        "ELSE_CORRELATED_ERROR(0.02)X4_Y7_Z6;"
        "DEPOLARIZE1(0.02)0;"
        "DEPOLARIZE2(0.03)1_2;"
        "PAULI_CHANNEL_1(0.01,0.02,0.03)3;"
        "PAULI_CHANNEL_2(0.001,0.002,0.003,0.004,0.005,0.006,0.007,0.008,0.009,0.01,0.011,0.012,0.013,0.014,0.015)4_5;"
        "X_ERROR(0.01)0;"
        "Y_ERROR(0.02)1;"
        "Z_ERROR(0.03)2;"
        "HERALDED_ERASE(0.04)3;"
        "HERALDED_PAULI_CHANNEL_1(0.01,0.02,0.03,0.04)6;"
        "I_ERROR(0.06)7;"
        "II_ERROR(0.07)8_9;"
        "TICK;"
        "MPP_X0*Y1*Z2_Z0*Z1;"
        "SPP_X0*Y1*Z2_X3;"
        "SPP_DAG_X0*Y1*Z2_X2;"
        "TICK;"
        "MRX_0;"
        "MRY_1;"
        "MR_2;"
        "MX_3;"
        "MY_4;"
        "M_5_6;"
        "RX_7;"
        "RY_8;"
        "R_9;"
        "TICK;"
        "MXX_0_1_2_3;"
        "MYY_4_5;"
        "MZZ_6_7;"
        "TICK;"
        "REPEAT_3_{;"
        "H_0;"
        "CX_0_1;"
        "S_1;"
        "TICK;"
        "};"
        "TICK;"
        "MR_0;"
        "X_ERROR(0.1)0;"
        "MR(0.01)0;"
        "SHIFT_COORDS(1,2,3);"
        "DT(1,2,3)rec[-1];"
        "OI(0)rec[-1];"
        "MPAD_0_1_0;"
        "OI(1)Z2_Z3;"
        "TICK;"
        "MRX_!0;"
        "MY_!1;"
        "MZZ_!2_3;"
        "OI(1)rec[-1];"
        "MYY_!4_!5;"
        "MPP_X6*!Y7*Z8;"
        "TICK;"
        "CX_rec[-1]_0;"
        "CY_sweep[0]_1;"
        "CZ_2_rec[-1]";
    ASSERT_EQ(actual, expected);
}

TEST(export_crumble, graphlike_error) {
    Circuit circuit(R"CIRCUIT(
        R 0 1 2 3
        X_ERROR(0.125) 0 1
        M 0 1
        M(0.125) 2 3
        DETECTOR rec[-1] rec[-2]
        DETECTOR rec[-2] rec[-3]
        DETECTOR rec[-3] rec[-4]
        OBSERVABLE_INCLUDE(0) rec[-1]
    )CIRCUIT");
    DetectorErrorModel dem =
        ErrorAnalyzer::circuit_to_detector_error_model(circuit, false, true, false, 1, false, false);
    DetectorErrorModel filter = shortest_graphlike_undetectable_logical_error(dem, false);
    auto error = ErrorMatcher::explain_errors_from_circuit(circuit, &filter, false);

    auto actual = export_crumble_url(circuit, true, {{0, error}});
    auto expected =
        "https://algassert.com/crumble#circuit="
        "R_0_1_2_3;"
        "TICK;"
        "MARKX(0)1;"
        "MARKX(0)0;"
        "TICK;"
        "X_ERROR(0.125)0_1;"
        "M_0_1;"
        "TICK;"
        "MARKX(0)2;"
        "MARKX(0)3;"
        "TICK;"
        "M(0.125)2_3;"
        "OI(0)rec[-1]";
    ASSERT_EQ(actual, expected);
}
