#include "stim/util_top/stabilizers_to_tableau.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(conversions, stabilizers_to_tableau_fuzz, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (size_t n = 0; n < 10; n++) {
        auto t = Tableau<W>::random(n, rng);
        std::vector<PauliString<W>> expected_stabilizers;
        for (size_t k = 0; k < n; k++) {
            expected_stabilizers.push_back(t.zs[k]);
        }
        auto actual = stabilizers_to_tableau<W>(expected_stabilizers, false, false, false);
        for (size_t k = 0; k < n; k++) {
            ASSERT_EQ(actual.zs[k], expected_stabilizers[k]);
        }

        ASSERT_TRUE(actual.satisfies_invariants());
    }
})

TEST_EACH_WORD_SIZE_W(conversions, stabilizers_to_tableau_partial_fuzz, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (size_t n = 0; n < 10; n++) {
        for (size_t skipped = 1; skipped < n && skipped < 4; skipped++) {
            auto t = Tableau<W>::random(n, rng);
            std::vector<PauliString<W>> expected_stabilizers;
            for (size_t k = 0; k < n - skipped; k++) {
                expected_stabilizers.push_back(t.zs[k]);
            }
            ASSERT_THROW(
                { stabilizers_to_tableau<W>(expected_stabilizers, false, false, false); }, std::invalid_argument);
            auto actual = stabilizers_to_tableau<W>(expected_stabilizers, false, true, false);
            for (size_t k = 0; k < n - skipped; k++) {
                ASSERT_EQ(actual.zs[k], expected_stabilizers[k]);
            }

            ASSERT_TRUE(actual.satisfies_invariants());

            auto inverted = stabilizers_to_tableau<W>(expected_stabilizers, false, true, true);
            ASSERT_EQ(actual.inverse(), inverted);
        }
    }
})

TEST_EACH_WORD_SIZE_W(conversions, stabilizers_to_tableau_overconstrained, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (size_t n = 4; n < 10; n++) {
        auto t = Tableau<W>::random(n, rng);
        std::vector<PauliString<W>> expected_stabilizers;
        expected_stabilizers.push_back(PauliString<W>(n));
        expected_stabilizers.push_back(PauliString<W>(n));
        uint8_t s = 0;
        s += expected_stabilizers.back().ref().inplace_right_mul_returning_log_i_scalar(t.zs[1]);
        s += expected_stabilizers.back().ref().inplace_right_mul_returning_log_i_scalar(t.zs[3]);
        if (s & 2) {
            expected_stabilizers.back().sign ^= true;
        }
        for (size_t k = 0; k < n; k++) {
            expected_stabilizers.push_back(t.zs[k]);
        }
        ASSERT_THROW({ stabilizers_to_tableau<W>(expected_stabilizers, false, false, false); }, std::invalid_argument);
        auto actual = stabilizers_to_tableau<W>(expected_stabilizers, true, false, false);
        for (size_t k = 0; k < n; k++) {
            ASSERT_EQ(actual.zs[k], expected_stabilizers[k + 1 + (k > 3)]);
        }

        ASSERT_TRUE(actual.satisfies_invariants());
    }
})

TEST_EACH_WORD_SIZE_W(conversions, stabilizers_to_tableau_bell_pair, {
    std::vector<stim::PauliString<W>> input_stabilizers;
    input_stabilizers.push_back(PauliString<W>::from_str("XX"));
    input_stabilizers.push_back(PauliString<W>::from_str("ZZ"));
    auto actual = stabilizers_to_tableau<W>(input_stabilizers, false, false, false);
    Tableau<W> expected(2);
    expected.zs[0] = PauliString<W>::from_str("XX");
    expected.zs[1] = PauliString<W>::from_str("ZZ");
    expected.xs[0] = PauliString<W>::from_str("Z_");
    expected.xs[1] = PauliString<W>::from_str("_X");
    ASSERT_EQ(actual, expected);

    input_stabilizers.push_back(PauliString<W>::from_str("-YY"));
    ASSERT_THROW({ stabilizers_to_tableau<W>(input_stabilizers, false, false, false); }, std::invalid_argument);
    actual = stabilizers_to_tableau<W>(input_stabilizers, true, false, false);
    ASSERT_EQ(actual, expected);

    input_stabilizers[2] = PauliString<W>::from_str("+YY");
    // Sign is wrong!
    ASSERT_THROW({ stabilizers_to_tableau<W>(input_stabilizers, true, true, false); }, std::invalid_argument);

    input_stabilizers[2] = PauliString<W>::from_str("+Z_");
    // Anticommutes!
    ASSERT_THROW({ stabilizers_to_tableau<W>(input_stabilizers, true, true, false); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(conversions, stabilizer_to_tableau_detect_anticommutation, {
    std::vector<stim::PauliString<W>> input_stabilizers;
    input_stabilizers.push_back(PauliString<W>::from_str("YY"));
    input_stabilizers.push_back(PauliString<W>::from_str("YX"));
    ASSERT_THROW({ stabilizers_to_tableau<W>(input_stabilizers, false, false, false); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(conversions, stabilizer_to_tableau_size_affecting_redundancy, {
    std::vector<stim::PauliString<W>> input_stabilizers;
    input_stabilizers.push_back(PauliString<W>::from_str("X_"));
    input_stabilizers.push_back(PauliString<W>::from_str("_X"));
    for (size_t k = 0; k < 150; k++) {
        input_stabilizers.push_back(PauliString<W>::from_str("__"));
    }
    auto t = stabilizers_to_tableau<W>(input_stabilizers, true, true, false);
    ASSERT_EQ(t.num_qubits, 2);
    ASSERT_EQ(t.zs[0], PauliString<W>::from_str("X_"));
    ASSERT_EQ(t.zs[1], PauliString<W>::from_str("_X"));
})
