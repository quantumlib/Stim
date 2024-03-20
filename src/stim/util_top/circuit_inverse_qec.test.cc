#include "stim/util_top/circuit_inverse_qec.h"

#include "gtest/gtest.h"
#include "stim/mem/simd_word.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, unitary, {
    auto actual = circuit_inverse_qec<W>(Circuit(R"CIRCUIT(
        H 0
        ISWAP 0 1 1 2 3 2
        S 0 3 4
    )CIRCUIT"), {});
    ASSERT_EQ(actual, Circuit(R"CIRCUIT(
        S_DAG 4 3 0
        ISWAP_DAG 3 2 1 2 0 1
        H 0
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, r_m_det, {
    auto actual = circuit_inverse_qec<W>(Circuit(R"CIRCUIT(
        R 0
        M 0
        DETECTOR rec[-1]
    )CIRCUIT"), {});
    ASSERT_EQ(actual, Circuit(R"CIRCUIT(
        R 0
        M 0
        DETECTOR rec[-1]
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, two_to_one, {
    auto actual = circuit_inverse_qec<W>(Circuit(R"CIRCUIT(
        R 0 1
        CX 0 1
        M 0 1
        DETECTOR rec[-1] rec[-2]
    )CIRCUIT"), {});
    ASSERT_EQ(actual, Circuit(R"CIRCUIT(
        R 1 0
        CX 0 1
        M 1 0
        DETECTOR rec[-2]
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, pass_through, {
    auto actual = circuit_inverse_qec<W>(Circuit(R"CIRCUIT(
        R 0
        M 0
        MR 0
        DETECTOR rec[-1]
    )CIRCUIT"), {});
    ASSERT_EQ(actual, Circuit(R"CIRCUIT(
        MR 0
        M 0
        M 0
        DETECTOR rec[-1]
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, anticommute, {
    ASSERT_THROW({
        circuit_inverse_qec<W>(Circuit(R"CIRCUIT(
            R 0
            MX 0
            MR 0
            DETECTOR rec[-1]
        )CIRCUIT"), {});
    }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, noisy_mr, {
    auto actual = circuit_inverse_qec<W>(Circuit(R"CIRCUIT(
        MR(0.125) 0 1 2 0 2 4
        MRX(0.25) 0
        MRY(0.375) 0
    )CIRCUIT"), {});
    ASSERT_EQ(actual, Circuit(R"CIRCUIT(
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
    auto actual = circuit_inverse_qec<W>(Circuit(R"CIRCUIT(
        M(0.125) 0 1 2 0 2 4
        MX(0.25) 0
        MY(0.375) 0
    )CIRCUIT"), {});
    ASSERT_EQ(actual, Circuit(R"CIRCUIT(
        MY(0.375) 0
        MX(0.25) 0
        M(0.125) 4 2 0 2 1 0
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(circuit_inverse_qec, noisy_mr_det, {
    auto actual = circuit_inverse_qec<W>(Circuit(R"CIRCUIT(
        MR(0.125) 0
        TICK
        MR(0.25) 0
        MR(0.375) 0
        DETECTOR rec[-1]
    )CIRCUIT"), {});
    ASSERT_EQ(actual, Circuit(R"CIRCUIT(
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
    auto actual = circuit_inverse_qec<W>(Circuit(R"CIRCUIT(
        R 0 1 2
        TICK
        M 0 1 2
        TICK
        M 0 1 2
        DETECTOR(2) rec[-1]
        DETECTOR(1) rec[-2]
    )CIRCUIT"), {});
    ASSERT_EQ(actual, Circuit(R"CIRCUIT(
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
