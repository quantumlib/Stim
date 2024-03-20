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

#include "stim/util_top/stabilizers_vs_amplitudes.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/test_util.test.h"

using namespace stim;

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
