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

TEST(conversions, unitary_circuit_inverse) {
    ASSERT_EQ(
        unitary_circuit_inverse(Circuit(R"CIRCUIT(
        H 0
        ISWAP 0 1 1 2 3 2
        S 0 3 4
    )CIRCUIT")),
        Circuit(R"CIRCUIT(
        S_DAG 4 3 0
        ISWAP_DAG 3 2 1 2 0 1
        H 0
    )CIRCUIT"));

    ASSERT_THROW({ unitary_circuit_inverse(Circuit("M 0")); }, std::invalid_argument);
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
        for (size_t n = 0; n < 10; n++) {
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

TEST_EACH_WORD_SIZE_W(conversions, circuit_to_tableau_ignoring_gates, {
    Circuit unitary(R"CIRCUIT(
        I 0
        X 0
        Y 0
        Z 0
        C_XYZ 0
        C_ZYX 0
        H 0
        H_XY 0
        H_XZ 0
        H_YZ 0
        S 0
        SQRT_X 0
        SQRT_X_DAG 0
        SQRT_Y 0
        SQRT_Y_DAG 0
        SQRT_Z 0
        SQRT_Z_DAG 0
        S_DAG 0
        CNOT 0 1
        CX 0 1
        CY 0 1
        CZ 0 1
        ISWAP 0 1
        ISWAP_DAG 0 1
        SQRT_XX 0 1
        SQRT_XX_DAG 0 1
        SQRT_YY 0 1
        SQRT_YY_DAG 0 1
        SQRT_ZZ 0 1
        SQRT_ZZ_DAG 0 1
        SWAP 0 1
        XCX 0 1
        XCY 0 1
        XCZ 0 1
        YCX 0 1
        YCY 0 1
        YCZ 0 1
        ZCX 0 1
        ZCY 0 1
        ZCZ 0 1
    )CIRCUIT");
    Circuit noise(R"CIRCUIT(
        CORRELATED_ERROR(0.1) X0
        DEPOLARIZE1(0.1) 0
        DEPOLARIZE2(0.1) 0 1
        E(0.1) X0
        ELSE_CORRELATED_ERROR(0.1) Y1
        PAULI_CHANNEL_1(0.1,0.2,0.3) 0
        PAULI_CHANNEL_2(0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01) 0 1
        X_ERROR(0.1) 0
        Y_ERROR(0.1) 0
        Z_ERROR(0.1) 0
    )CIRCUIT");
    Circuit measure(R"CIRCUIT(
        M 0
        MPP X0
        MX 0
        MY 0
        MZ 0
    )CIRCUIT");
    Circuit reset(R"CIRCUIT(
        R 0
        RX 0
        RY 0
        RZ 0
    )CIRCUIT");
    Circuit measure_reset(R"CIRCUIT(
        MR 0
        MRX 0
        MRY 0
        MRZ 0
    )CIRCUIT");
    Circuit annotations(R"CIRCUIT(
        REPEAT 10 {
            I 0
        }
        DETECTOR(1, 2)
        OBSERVABLE_INCLUDE(1)
        QUBIT_COORDS(0,1,2) 0
        SHIFT_COORDS(2, 3, 4)
        TICK
    )CIRCUIT");

    ASSERT_EQ(circuit_to_tableau<W>(unitary, false, false, false).num_qubits, 2);

    ASSERT_THROW({ circuit_to_tableau<W>(noise, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau<W>(noise, false, true, true); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau<W>(noise, true, false, false), Tableau<W>(2));

    ASSERT_THROW({ circuit_to_tableau<W>(measure, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau<W>(measure, true, false, true); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau<W>(measure, false, true, false), Tableau<W>(1));

    ASSERT_THROW({ circuit_to_tableau<W>(reset, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau<W>(reset, true, true, false); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau<W>(reset, false, false, true), Tableau<W>(1));

    ASSERT_THROW({ circuit_to_tableau<W>(measure_reset, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau<W>(measure_reset, true, false, true); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau<W>(measure_reset, true, true, false); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau<W>(measure_reset, false, true, true), Tableau<W>(1));

    ASSERT_EQ(circuit_to_tableau<W>(annotations, false, false, false), Tableau<W>(1));

    ASSERT_EQ(
        circuit_to_tableau<W>(annotations + measure_reset + measure + reset + unitary + noise, true, true, true)
            .num_qubits,
        2);
})

TEST_EACH_WORD_SIZE_W(conversions, circuit_to_tableau, {
    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
        )CIRCUIT"),
            false,
            false,
            false),
        Tableau<W>(0));

    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
            REPEAT 10 {
                X 0
                TICK
            }
        )CIRCUIT"),
            false,
            false,
            false),
        Tableau<W>(1));

    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
            REPEAT 11 {
                X 0
                TICK
            }
        )CIRCUIT"),
            false,
            false,
            false),
        GATE_DATA.at("X").tableau<W>());

    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
            S 0
        )CIRCUIT"),
            false,
            false,
            false),
        GATE_DATA.at("S").tableau<W>());

    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
            SQRT_Y_DAG 1
            CZ 0 1
            SQRT_Y 1
        )CIRCUIT"),
            false,
            false,
            false),
        GATE_DATA.at("CX").tableau<W>());

    ASSERT_EQ(
        circuit_to_tableau<W>(
            Circuit(R"CIRCUIT(
            R 0
            X_ERROR(0.1) 0
            SQRT_Y_DAG 1
            CZ 0 1
            SQRT_Y 1
            M 0
        )CIRCUIT"),
            true,
            true,
            true),
        GATE_DATA.at("CX").tableau<W>());
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

TEST_EACH_WORD_SIZE_W(conversions, tableau_to_circuit_fuzz_vs_circuit_to_tableau, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (size_t n = 0; n < 10; n++) {
        auto desired = Tableau<W>::random(n, rng);
        Circuit circuit = tableau_to_circuit<W>(desired, "elimination");
        auto actual = circuit_to_tableau<W>(circuit, false, false, false);
        ASSERT_EQ(actual, desired);

        for (const auto &op : circuit.operations) {
            ASSERT_TRUE(op.gate_type == GateType::S || op.gate_type == GateType::H || op.gate_type == GateType::CX)
                << op;
        }
    }
})

TEST_EACH_WORD_SIZE_W(conversions, tableau_to_circuit, {
    ASSERT_EQ(tableau_to_circuit<W>(GATE_DATA.at("I").tableau<W>(), "elimination"), Circuit(R"CIRCUIT(
            H 0
            H 0
        )CIRCUIT"));

    ASSERT_EQ(tableau_to_circuit<W>(GATE_DATA.at("X").tableau<W>(), "elimination"), Circuit(R"CIRCUIT(
            H 0
            S 0
            S 0
            H 0
        )CIRCUIT"));

    ASSERT_EQ(tableau_to_circuit<W>(GATE_DATA.at("S").tableau<W>(), "elimination"), Circuit(R"CIRCUIT(
            S 0
        )CIRCUIT"));

    ASSERT_EQ(tableau_to_circuit<W>(GATE_DATA.at("ISWAP").tableau<W>(), "elimination"), Circuit(R"CIRCUIT(
            CX 1 0 0 1 1 0
            S 0
            H 1
            CX 0 1
            H 1
            S 1
        )CIRCUIT"));
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

TEST(conversions, independent_to_disjoint_xyz_errors) {
    double out_x;
    double out_y;
    double out_z;

    independent_to_disjoint_xyz_errors(0.5, 0.5, 0.5, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 1 / 4.0, 1e-6);
    ASSERT_NEAR(out_y, 1 / 4.0, 1e-6);
    ASSERT_NEAR(out_z, 1 / 4.0, 1e-6);

    independent_to_disjoint_xyz_errors(0.1, 0, 0, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0.1, 1e-6);
    ASSERT_NEAR(out_y, 0, 1e-6);
    ASSERT_NEAR(out_z, 0, 1e-6);

    independent_to_disjoint_xyz_errors(0, 0.2, 0, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0, 1e-6);
    ASSERT_NEAR(out_y, 0.2, 1e-6);
    ASSERT_NEAR(out_z, 0, 1e-6);

    independent_to_disjoint_xyz_errors(0, 0, 0.05, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0, 1e-6);
    ASSERT_NEAR(out_y, 0, 1e-6);
    ASSERT_NEAR(out_z, 0.05, 1e-6);

    independent_to_disjoint_xyz_errors(0.1, 0.1, 0, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0.1 - 0.01, 1e-6);
    ASSERT_NEAR(out_y, 0.1 - 0.01, 1e-6);
    ASSERT_NEAR(out_z, 0.01, 1e-6);
}

TEST(conversions, disjoint_to_independent_xyz_errors_approx) {
    double out_x;
    double out_y;
    double out_z;

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.4, 0.0, 0.0, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.4, 1e-6);
    ASSERT_NEAR(out_y, 0.0, 1e-6);
    ASSERT_NEAR(out_z, 0.0, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.5, 0.0, 0.0, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.5, 1e-6);
    ASSERT_NEAR(out_y, 0.0, 1e-6);
    ASSERT_NEAR(out_z, 0.0, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.6, 0.0, 0.0, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.6, 1e-6);
    ASSERT_NEAR(out_y, 0.0, 1e-6);
    ASSERT_NEAR(out_z, 0.0, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.25, 0.25, 0.25, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.5, 1e-6);
    ASSERT_NEAR(out_y, 0.5, 1e-6);
    ASSERT_NEAR(out_z, 0.5, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.1, 0, 0, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.1, 1e-6);
    ASSERT_NEAR(out_y, 0, 1e-6);
    ASSERT_NEAR(out_z, 0, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0, 0.2, 0, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0, 1e-6);
    ASSERT_NEAR(out_y, 0.2, 1e-6);
    ASSERT_NEAR(out_z, 0, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0, 0, 0.05, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0, 1e-6);
    ASSERT_NEAR(out_y, 0, 1e-6);
    ASSERT_NEAR(out_z, 0.05, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.1 - 0.01, 0.1 - 0.01, 0.01, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.1, 1e-6);
    ASSERT_NEAR(out_y, 0.1, 1e-6);
    ASSERT_NEAR(out_z, 0, 1e-6);

    ASSERT_FALSE(try_disjoint_to_independent_xyz_errors_approx(0.2, 0.2, 0, &out_x, &out_y, &out_z));
    independent_to_disjoint_xyz_errors(out_x, out_y, out_z, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0.2, 1e-6);
    ASSERT_NEAR(out_y, 0.2, 1e-6);
    ASSERT_LT(out_z, 0.08);

    ASSERT_FALSE(try_disjoint_to_independent_xyz_errors_approx(0.2, 0.1, 0, &out_x, &out_y, &out_z));
    independent_to_disjoint_xyz_errors(out_x, out_y, out_z, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0.2, 1e-6);
    ASSERT_NEAR(out_y, 0.1, 1e-6);
    ASSERT_LT(out_z, 0.03);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.3 * 0.6, 0.4 * 0.7, 0.3 * 0.4, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.3, 1e-6);
    ASSERT_NEAR(out_y, 0.4, 1e-6);
    ASSERT_NEAR(out_z, 0.0, 1e-6);
    independent_to_disjoint_xyz_errors(out_x, out_y, out_z, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0.3 * 0.6, 1e-6);
    ASSERT_NEAR(out_y, 0.4 * 0.7, 1e-6);
    ASSERT_NEAR(out_z, 0.3 * 0.4, 1e-6);
}

TEST(conversions, fuzz_depolarize1_consistency) {
    auto rng = INDEPENDENT_TEST_RNG();
    std::uniform_real_distribution<double> dis(0, 1);
    for (size_t k = 0; k < 10; k++) {
        double p = dis(rng) * 0.75;
        double p2 = depolarize1_probability_to_independent_per_channel_probability(p);
        double x2, y2, z2;
        ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(p / 3, p / 3, p / 3, &x2, &y2, &z2));
        ASSERT_NEAR(x2, p2, 1e-6) << "p=" << p;
        ASSERT_NEAR(y2, p2, 1e-6) << "p=" << p;
        ASSERT_NEAR(z2, p2, 1e-6) << "p=" << p;

        double p3 = independent_per_channel_probability_to_depolarize1_probability(p2);
        double x3, y3, z3;
        independent_to_disjoint_xyz_errors(x2, y2, z2, &x3, &y3, &z3);
        ASSERT_NEAR(x3, p / 3, 1e-6) << "p=" << p;
        ASSERT_NEAR(y3, p / 3, 1e-6) << "p=" << p;
        ASSERT_NEAR(z3, p / 3, 1e-6) << "p=" << p;
        ASSERT_NEAR(p3, p, 1e-6) << "p=" << p;
    }
}

TEST(conversions, fuzz_depolarize2_consistency) {
    auto rng = INDEPENDENT_TEST_RNG();
    std::uniform_real_distribution<double> dis(0, 1);
    for (size_t k = 0; k < 10; k++) {
        double p = dis(rng) * 0.75;
        double p2 = depolarize2_probability_to_independent_per_channel_probability(p);
        double p3 = independent_per_channel_probability_to_depolarize2_probability(p2);
        ASSERT_NEAR(p3, p, 1e-6) << "p=" << p;
    }
}

TEST(conversions, independent_vs_disjoint_xyz_errors_round_trip_fuzz) {
    auto rng = INDEPENDENT_TEST_RNG();
    std::uniform_real_distribution<double> dis(0, 1);
    for (size_t k = 0; k < 25; k++) {
        double x;
        double y;
        double z;
        do {
            x = dis(rng);
            y = dis(rng);
            z = dis(rng);
        } while (x + y + z >= 0.999);
        double x2, y2, z2;
        independent_to_disjoint_xyz_errors(x, y, z, &x2, &y2, &z2);
        double x3, y3, z3;
        ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(x2, y2, z2, &x3, &y3, &z3));
        ASSERT_NEAR(x, x3, 1e-6) << "x=" << x << ",y=" << y << ",z=" << z;
        ASSERT_NEAR(y, y3, 1e-6) << "x=" << x << ",y=" << y << ",z=" << z;
        ASSERT_NEAR(z, z3, 1e-6) << "x=" << x << ",y=" << y << ",z=" << z;
    }
}

TEST_EACH_WORD_SIZE_W(conversions, fuzz_mpp_circuit_produces_correct_state, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto tableau = Tableau<W>::random(10, rng);
    auto circuit = tableau_to_circuit_mpp_method<W>(tableau, false);
    TableauSimulator<W> sim(std::move(rng), 10);
    sim.safe_do_circuit(circuit);
    auto expected = tableau.stabilizers(true);
    auto actual = sim.canonical_stabilizers();
    ASSERT_EQ(actual, expected);
})

TEST_EACH_WORD_SIZE_W(conversions, fuzz_mpp_circuit_produces_correct_state_unsigned, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto tableau = Tableau<W>::random(10, rng);
    auto circuit = tableau_to_circuit_mpp_method<W>(tableau, true);
    TableauSimulator<W> sim(std::move(rng), 10);
    sim.safe_do_circuit(circuit);
    auto expected = tableau.stabilizers(true);
    auto actual = sim.canonical_stabilizers();
    for (auto &e : expected) {
        e.sign = false;
    }
    for (auto &e : actual) {
        e.sign = false;
    }
    ASSERT_EQ(actual, expected);
})

TEST_EACH_WORD_SIZE_W(conversions, perfect_code_mpp_circuit, {
    Tableau<W> tableau(5);

    tableau.zs[0] = PauliString<W>("XZZX_");
    tableau.zs[1] = PauliString<W>("_XZZX");
    tableau.zs[2] = PauliString<W>("X_XZZ");
    tableau.zs[3] = PauliString<W>("ZX_XZ");
    tableau.zs[4] = PauliString<W>("ZZZZZ");

    tableau.xs[0] = PauliString<W>("Z_Z__");
    tableau.xs[1] = PauliString<W>("ZZZZ_");
    tableau.xs[2] = PauliString<W>("ZZ_ZZ");
    tableau.xs[3] = PauliString<W>("_Z__Z");
    tableau.xs[4] = PauliString<W>("XXXXX");

    ASSERT_TRUE(tableau.satisfies_invariants());

    ASSERT_EQ(tableau_to_circuit_mpp_method<W>(tableau, true), Circuit(R"CIRCUIT(
        MPP X0*Z1*Z2*X3 X1*Z2*Z3*X4 X0*X2*Z3*Z4 Z0*X1*X3*Z4 Z0*Z1*Z2*Z3*Z4
    )CIRCUIT"));

    ASSERT_EQ(tableau_to_circuit_mpp_method<W>(tableau, false), Circuit(R"CIRCUIT(
        MPP X0*Z1*Z2*X3 X1*Z2*Z3*X4 X0*X2*Z3*Z4 Z0*X1*X3*Z4 Z0*Z1*Z2*Z3*Z4
        CX rec[-1] 0 rec[-1] 1 rec[-1] 2 rec[-1] 3 rec[-1] 4
        CZ rec[-5] 0 rec[-5] 2 rec[-4] 0 rec[-4] 1 rec[-4] 2 rec[-4] 3 rec[-3] 0 rec[-3] 1 rec[-3] 3 rec[-3] 4 rec[-2] 1 rec[-2] 4
    )CIRCUIT"));
})
