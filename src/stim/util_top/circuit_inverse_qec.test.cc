#include "stim/util_top/circuit_inverse_qec.h"

#include "gtest/gtest.h"

#include "stim/gen/gen_surface_code.h"
#include "stim/mem/simd_word.test.h"
#include "stim/util_top/circuit_to_detecting_regions.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, unitary, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        H 0
        ISWAP 0 1 1 2 3 2
        S 0 3 4
    )CIRCUIT"),
        {});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        S_DAG 4 3 0
        ISWAP_DAG 3 2 1 2 0 1
        H 0
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, r_m_det, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        R 0
        M 0
        DETECTOR rec[-1]
    )CIRCUIT"),
        {});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        R 0
        M 0
        DETECTOR rec[-1]
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, r_m_det_keep_m, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        R 0
        M 0
        DETECTOR rec[-1]
    )CIRCUIT"),
        {},
        true);
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        M 0
        M 0
        DETECTOR rec[-2] rec[-1]
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, two_to_one, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        R 0 1
        CX 0 1
        M 0 1
        DETECTOR rec[-1] rec[-2]
    )CIRCUIT"),
        {});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        R 1 0
        CX 0 1
        M 1 0
        DETECTOR rec[-2]
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, pass_through, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        R 0
        M 0
        MR 0
        DETECTOR rec[-1]
    )CIRCUIT"),
        {});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        MR 0
        M 0
        M 0
        DETECTOR rec[-1]
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, anticommute, {
    ASSERT_THROW(
        {
            circuit_inverse_qec<W>(
                Circuit(R"CIRCUIT(
            R 0
            MX 0
            MR 0
            DETECTOR rec[-1]
        )CIRCUIT"),
                {});
        },
        std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, noisy_mr, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        MR(0.125) 0 1 2 0 2 4
        MRX(0.25) 0
        MRY(0.375) 0
    )CIRCUIT"),
        {});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        MRY 0
        Z_ERROR(0.375) 0
        MRX 0
        Z_ERROR(0.25) 0
        MR 4 2 0
        X_ERROR(0.125) 4 2 0
        MR 2 1 0
        X_ERROR(0.125) 2 1 0
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, noisy_m, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        M(0.125) 0 1 2 0 2 4
        MX(0.25) 0
        MY(0.375) 0
    )CIRCUIT"),
        {});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        MY(0.375) 0
        MX(0.25) 0
        M(0.125) 4 2 0 2 1 0
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, noisy_mr_det, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        MR(0.125) 0
        TICK
        MR(0.25) 0
        MR(0.375) 0
        DETECTOR rec[-1]
    )CIRCUIT"),
        {});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        MR 0
        X_ERROR(0.375) 0
        MR 0
        X_ERROR(0.25) 0
        DETECTOR rec[-1]
        TICK
        MR 0
        X_ERROR(0.125) 0
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, m_det, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        R 0 1 2
        TICK
        M 0 1 2
        TICK
        M 0 1 2
        DETECTOR(2) rec[-1]
        DETECTOR(1) rec[-2]
    )CIRCUIT"),
        {});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        R 2 1
        M 0
        TICK
        M 2 1 0
        TICK
        M 2 1 0
        DETECTOR(2) rec[-3]
        DETECTOR(1) rec[-2]
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, mzz, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        MRY 0 1
        M 0
        TICK
        MZZ(0.125) 0 1 2 3
        TICK
        M 1
        MRY 0 1
        DETECTOR rec[-3] rec[-5] rec[-6]
    )CIRCUIT"),
        {});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        MRY 1 0
        R 1
        TICK
        MZZ(0.125) 2 3 0 1
        TICK
        M 0
        DETECTOR rec[-2] rec[-1]
        MRY 1 0
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, mpp, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        MPP !X0*X1 Y0*Y1 Z0*Z1
        DETECTOR rec[-1] rec[-2] rec[-3]
    )CIRCUIT"),
        {});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        MPP Z1*Z0 Y1*Y0 X1*!X0
        DETECTOR rec[-3] rec[-2] rec[-1]
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, flow_reverse, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        M 0
    )CIRCUIT"),
        {std::vector<Flow<W>>{Flow<W>::from_str("1 -> Z0 xor rec[-1]")}});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        M 0
    )CIRCUIT"));
    ASSERT_EQ(actual.second, (std::vector<Flow<W>>{Flow<W>::from_str("Z0 -> rec[-1]")}));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, flow_through_mzz, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        MZZ 0 1
    )CIRCUIT"),
        {std::vector<Flow<W>>{
            Flow<W>::from_str("X0*X1 -> Y0*Y1 xor rec[-1]"),
            Flow<W>::from_str("X0*X1 -> X0*X1"),
            Flow<W>::from_str("Z0 -> Z1 xor rec[-1]"),
            Flow<W>::from_str("Z0 -> Z0"),
        }});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        MZZ 0 1
    )CIRCUIT"));
    ASSERT_EQ(
        actual.second,
        (std::vector<Flow<W>>{
            Flow<W>::from_str("Y0*Y1 -> X0*X1 xor rec[-1]"),
            Flow<W>::from_str("X0*X1 -> X0*X1"),
            Flow<W>::from_str("Z1 -> Z0 xor rec[-1]"),
            Flow<W>::from_str("Z0 -> Z0"),
        }));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, flow_through_mzz_h_cx_s, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        MZZ 0 1
        H 0
        CX 0 1
        S 1
    )CIRCUIT"),
        {std::vector<Flow<W>>{
            Flow<W>::from_str("X0*X1 -> X0*Z1 xor rec[-1]"),
            Flow<W>::from_str("X0*X1 -> Z0*Y1"),
            Flow<W>::from_str("Z0 -> Z0*Z1 xor rec[-1]"),
            Flow<W>::from_str("Z0 -> X0*Y1"),
        }});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        S_DAG 1
        CX 0 1
        H 0
        MZZ 0 1
    )CIRCUIT"));
    ASSERT_EQ(
        actual.second,
        (std::vector<Flow<W>>{
            Flow<W>::from_str("X0*Z1 -> X0*X1 xor rec[-1]"),
            Flow<W>::from_str("Z0*Y1 -> X0*X1"),
            Flow<W>::from_str("Z0*Z1 -> Z0 xor rec[-1]"),
            Flow<W>::from_str("X0*Y1 -> Z0"),
        }));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, flow_flip, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
        MY 0
        MRX 0
        MR 1
        R 0
    )CIRCUIT"),
        {std::vector<Flow<W>>{
            Flow<W>::from_str("Y0*Z1 -> rec[-3] xor rec[-1]"),
            Flow<W>::from_str("1 -> Z0*Z1"),
            Flow<W>::from_str("1 -> Z1"),
            Flow<W>::from_str("1 -> Z0"),
        }});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        M 0
        MR 1
        MRX 0
        RY 0
    )CIRCUIT"));
    ASSERT_EQ(
        actual.second,
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> Y0*Z1"),
            Flow<W>::from_str("Z0*Z1 -> rec[-3] xor rec[-2]"),
            Flow<W>::from_str("Z1 -> rec[-2]"),
            Flow<W>::from_str("Z0 -> rec[-3]"),
        }));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, flow_past_end_of_circuit, {
    auto actual = circuit_inverse_qec<W>(
        Circuit(R"CIRCUIT(
            H 0
        )CIRCUIT"),
        {std::vector<Flow<W>>{
            Flow<W>::from_str("X300*Z0 -> X300*X0"),
        }});
    ASSERT_EQ(actual.first, Circuit(R"CIRCUIT(
        H 0
    )CIRCUIT"));
    ASSERT_EQ(
        actual.second,
        (std::vector<Flow<W>>{
            Flow<W>::from_str("X300*X0 -> X300*Z0"),
        }));
})
