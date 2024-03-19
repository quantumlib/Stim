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

#include "stim/stabilizers/conversions.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/test_util.test.h"

using namespace stim;

TEST(conversions, is_power_of_2) {
    ASSERT_FALSE(is_power_of_2(0));
    ASSERT_TRUE(is_power_of_2(1));
    ASSERT_TRUE(is_power_of_2(2));
    ASSERT_FALSE(is_power_of_2(3));
    ASSERT_TRUE(is_power_of_2(4));
    ASSERT_FALSE(is_power_of_2(5));
    ASSERT_FALSE(is_power_of_2(6));
    ASSERT_FALSE(is_power_of_2(7));
    ASSERT_TRUE(is_power_of_2(8));
    ASSERT_FALSE(is_power_of_2(9));
}

TEST(conversions, floor_lg2) {
    ASSERT_EQ(floor_lg2(1), 0);
    ASSERT_EQ(floor_lg2(2), 1);
    ASSERT_EQ(floor_lg2(3), 1);
    ASSERT_EQ(floor_lg2(4), 2);
    ASSERT_EQ(floor_lg2(5), 2);
    ASSERT_EQ(floor_lg2(6), 2);
    ASSERT_EQ(floor_lg2(7), 2);
    ASSERT_EQ(floor_lg2(8), 3);
    ASSERT_EQ(floor_lg2(9), 3);
}

TEST_EACH_WORD_SIZE_W(conversions, stabilizer_state_vector_to_circuit_basic, {
    ASSERT_THROW(
        stabilizer_state_vector_to_circuit<W>(
            {
                {0.5},
            },
            false),
        std::invalid_argument);

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {1},
            },
            false),
        stim::Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {-1},
            },
            false),
        stim::Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {0, 1},
                {0},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        I 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {0},
                {1},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        X 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {sqrtf(0.5)},
                {sqrtf(0.5)},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        H 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {0},
                {1},
                {0},
                {0},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        X 1
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {0},
                {0},
                {1},
                {0},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        X 0
        I 1
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {0},
                {1},
                {0},
                {0},
            },
            true),
        stim::Circuit(R"CIRCUIT(
        X 0
        I 1
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {0},
                {0},
                {1},
                {0},
            },
            true),
        stim::Circuit(R"CIRCUIT(
        X 1
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {sqrtf(0.5)},
                {0, sqrtf(0.5)},
            },
            true),
        stim::Circuit(R"CIRCUIT(
        H 0
        S 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {sqrtf(0.5)},
                {0, -sqrtf(0.5)},
            },
            true),
        stim::Circuit(R"CIRCUIT(
        H 0
        S_DAG 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit<W>(
            {
                {sqrtf(0.5)},
                {-sqrtf(0.5)},
            },
            true),
        stim::Circuit(R"CIRCUIT(
        H 0
        Z 0
    )CIRCUIT"));
})

TEST_EACH_WORD_SIZE_W(conversions, stabilizer_state_vector_to_circuit_fuzz_round_trip, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (const auto &little_endian : std::vector<bool>{false, true}) {
        for (size_t n = 0; n < 5; n++) {
            // Pick a random stabilizer state.
            TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), n);
            sim.inv_state = Tableau<W>::random(n, rng);
            auto desired_vec = sim.to_state_vector(little_endian);

            // Round trip through a circuit.
            auto circuit = stabilizer_state_vector_to_circuit<W>(desired_vec, little_endian);
            auto actual_vec = circuit_to_output_state_vector<W>(circuit, little_endian);
            ASSERT_EQ(actual_vec, desired_vec) << "little_endian=" << little_endian << ", n=" << n;
        }
    }
})


TEST_EACH_WORD_SIZE_W(conversions, circuit_to_output_state_vector, {
    ASSERT_EQ(circuit_to_output_state_vector<W>(Circuit(""), false), (std::vector<std::complex<float>>{{1}}));
    ASSERT_EQ(
        circuit_to_output_state_vector<W>(Circuit("H 0 1"), false),
        (std::vector<std::complex<float>>{{0.5}, {0.5}, {0.5}, {0.5}}));
    ASSERT_EQ(
        circuit_to_output_state_vector<W>(Circuit("X 1"), false),
        (std::vector<std::complex<float>>{{0}, {1}, {0}, {0}}));
    ASSERT_EQ(
        circuit_to_output_state_vector<W>(Circuit("X 1"), true),
        (std::vector<std::complex<float>>{{0}, {0}, {1}, {0}}));
})

TEST_EACH_WORD_SIZE_W(conversions, unitary_to_tableau_vs_gate_data, {
    for (const auto &gate : GATE_DATA.items) {
        if (gate.flags & GATE_IS_UNITARY) {
            EXPECT_EQ(unitary_to_tableau<W>(gate.unitary(), true), gate.tableau<W>()) << gate.name;
        }
    }
})

TEST_EACH_WORD_SIZE_W(conversions, tableau_to_unitary_vs_gate_data, {
    VectorSimulator v1(2);
    VectorSimulator v2(2);
    for (const auto &gate : GATE_DATA.items) {
        if (gate.flags & GATE_IS_UNITARY) {
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
