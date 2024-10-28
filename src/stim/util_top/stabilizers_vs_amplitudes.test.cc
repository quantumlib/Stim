#include "stim/util_top/stabilizers_vs_amplitudes.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(conversions, unitary_to_tableau_vs_gate_data, {
    for (const auto &gate : GATE_DATA.items) {
        if (gate.has_known_unitary_matrix()) {
            EXPECT_EQ(unitary_to_tableau<W>(gate.unitary(), true), gate.tableau<W>()) << gate.name;
        }
    }
})

TEST_EACH_WORD_SIZE_W(conversions, tableau_to_unitary_vs_gate_data, {
    VectorSimulator v1(2);
    VectorSimulator v2(2);
    for (const auto &gate : GATE_DATA.items) {
        if (gate.has_known_unitary_matrix()) {
            auto actual = tableau_to_unitary<W>(gate.tableau<W>(), true);
            auto expected = gate.unitary();
            v1.state.clear();
            for (const auto &row : actual) {
                v1.state.insert(v1.state.end(), row.begin(), row.end());
            }
            v2.state.clear();
            for (const auto &row : expected) {
                v2.state.insert(v2.state.end(), row.begin(), row.end());
            }
            for (auto &v : v1.state) {
                v /= sqrtf(actual.size());
            }
            for (auto &v : v2.state) {
                v /= sqrtf(actual.size());
            }
            ASSERT_TRUE(v1.approximate_equals(v2, true)) << gate.name;
        }
    }
})

TEST_EACH_WORD_SIZE_W(conversions, unitary_vs_tableau_basic, {
    ASSERT_EQ(unitary_to_tableau<W>(GATE_DATA.at("XCZ").unitary(), false), GATE_DATA.at("ZCX").tableau<W>());
    ASSERT_EQ(unitary_to_tableau<W>(GATE_DATA.at("XCZ").unitary(), true), GATE_DATA.at("XCZ").tableau<W>());
    ASSERT_EQ(unitary_to_tableau<W>(GATE_DATA.at("ZCX").unitary(), false), GATE_DATA.at("XCZ").tableau<W>());
    ASSERT_EQ(unitary_to_tableau<W>(GATE_DATA.at("ZCX").unitary(), true), GATE_DATA.at("ZCX").tableau<W>());

    ASSERT_EQ(unitary_to_tableau<W>(GATE_DATA.at("XCY").unitary(), false), GATE_DATA.at("YCX").tableau<W>());
    ASSERT_EQ(unitary_to_tableau<W>(GATE_DATA.at("XCY").unitary(), true), GATE_DATA.at("XCY").tableau<W>());
    ASSERT_EQ(unitary_to_tableau<W>(GATE_DATA.at("YCX").unitary(), false), GATE_DATA.at("XCY").tableau<W>());
    ASSERT_EQ(unitary_to_tableau<W>(GATE_DATA.at("YCX").unitary(), true), GATE_DATA.at("YCX").tableau<W>());
})

TEST_EACH_WORD_SIZE_W(conversions, unitary_to_tableau_fuzz_vs_tableau_to_unitary, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (bool little_endian : std::vector<bool>{false, true}) {
        for (size_t n = 0; n < 6; n++) {
            auto desired = Tableau<W>::random(n, rng);
            auto unitary = tableau_to_unitary<W>(desired, little_endian);
            auto actual = unitary_to_tableau<W>(unitary, little_endian);
            ASSERT_EQ(actual, desired) << "little_endian=" << little_endian << ", n=" << n;
        }
    }
})

TEST_EACH_WORD_SIZE_W(conversions, unitary_to_tableau_fail, {
    ASSERT_THROW(
        { unitary_to_tableau<W>({{{1}, {0}}, {{0}, {sqrtf(0.5), sqrtf(0.5)}}}, false); }, std::invalid_argument);
    ASSERT_THROW(
        {
            unitary_to_tableau<W>(
                {
                    {1, 0, 0, 0},
                    {0, 1, 0, 0},
                    {0, 0, 1, 0},
                    {0, 0, 0, {0, 1}},
                },
                false);
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            unitary_to_tableau<W>(
                {
                    {1, 0, 0, 0, 0, 0, 0, 0},
                    {0, 1, 0, 0, 0, 0, 0, 0},
                    {0, 0, 1, 0, 0, 0, 0, 0},
                    {0, 0, 0, 1, 0, 0, 0, 0},
                    {0, 0, 0, 0, 1, 0, 0, 0},
                    {0, 0, 0, 0, 0, 1, 0, 0},
                    {0, 0, 0, 0, 0, 0, 0, 1},
                    {0, 0, 0, 0, 0, 0, 1, 0},
                },
                false);
        },
        std::invalid_argument);
})
