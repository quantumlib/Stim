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

TEST(conversions, stabilizer_state_vector_to_circuit_basic) {
    ASSERT_THROW(
        stabilizer_state_vector_to_circuit(
            {
                {0.5},
            },
            false),
        std::invalid_argument);

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {1},
            },
            false),
        stim::Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {-1},
            },
            false),
        stim::Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {0, 1},
                {0},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        I 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {0},
                {1},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        X 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {sqrtf(0.5)},
                {sqrtf(0.5)},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        H 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
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
        stabilizer_state_vector_to_circuit(
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
        stabilizer_state_vector_to_circuit(
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
        stabilizer_state_vector_to_circuit(
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
        stabilizer_state_vector_to_circuit(
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
        stabilizer_state_vector_to_circuit(
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
        stabilizer_state_vector_to_circuit(
            {
                {sqrtf(0.5)},
                {-sqrtf(0.5)},
            },
            true),
        stim::Circuit(R"CIRCUIT(
        H 0
        Z 0
    )CIRCUIT"));
}

TEST(conversions, stabilizer_state_vector_to_circuit_fuzz_round_trip) {
    for (const auto &little_endian : std::vector<bool>{false, true}) {
        for (size_t n = 0; n < 10; n++) {
            // Pick a random stabilizer state.
            TableauSimulator sim(SHARED_TEST_RNG(), n);
            sim.inv_state = Tableau::random(n, SHARED_TEST_RNG());
            auto desired_vec = sim.to_state_vector(little_endian);

            // Round trip through a circuit.
            auto circuit = stabilizer_state_vector_to_circuit(desired_vec, little_endian);
            auto actual_vec = circuit_to_output_state_vector(circuit, little_endian);
            ASSERT_EQ(actual_vec, desired_vec) << "little_endian=" << little_endian << ", n=" << n;
        }
    }
}

TEST(conversions, circuit_to_tableau_ignoring_gates) {
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

    ASSERT_EQ(circuit_to_tableau(unitary, false, false, false).num_qubits, 2);

    ASSERT_THROW({ circuit_to_tableau(noise, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau(noise, false, true, true); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau(noise, true, false, false), Tableau(2));

    ASSERT_THROW({ circuit_to_tableau(measure, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau(measure, true, false, true); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau(measure, false, true, false), Tableau(1));

    ASSERT_THROW({ circuit_to_tableau(reset, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau(reset, true, true, false); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau(reset, false, false, true), Tableau(1));

    ASSERT_THROW({ circuit_to_tableau(measure_reset, false, false, false); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau(measure_reset, true, false, true); }, std::invalid_argument);
    ASSERT_THROW({ circuit_to_tableau(measure_reset, true, true, false); }, std::invalid_argument);
    ASSERT_EQ(circuit_to_tableau(measure_reset, false, true, true), Tableau(1));

    ASSERT_EQ(circuit_to_tableau(annotations, false, false, false), Tableau(1));

    ASSERT_EQ(
        circuit_to_tableau(annotations + measure_reset + measure + reset + unitary + noise, true, true, true)
            .num_qubits,
        2);
}

TEST(conversions, circuit_to_tableau) {
    ASSERT_EQ(
        circuit_to_tableau(
            Circuit(R"CIRCUIT(
        )CIRCUIT"),
            false,
            false,
            false),
        Tableau(0));

    ASSERT_EQ(
        circuit_to_tableau(
            Circuit(R"CIRCUIT(
            REPEAT 10 {
                X 0
                TICK
            }
        )CIRCUIT"),
            false,
            false,
            false),
        Tableau(1));

    ASSERT_EQ(
        circuit_to_tableau(
            Circuit(R"CIRCUIT(
            REPEAT 11 {
                X 0
                TICK
            }
        )CIRCUIT"),
            false,
            false,
            false),
        GATE_DATA.at("X").tableau());

    ASSERT_EQ(
        circuit_to_tableau(
            Circuit(R"CIRCUIT(
            S 0
        )CIRCUIT"),
            false,
            false,
            false),
        GATE_DATA.at("S").tableau());

    ASSERT_EQ(
        circuit_to_tableau(
            Circuit(R"CIRCUIT(
            SQRT_Y_DAG 1
            CZ 0 1
            SQRT_Y 1
        )CIRCUIT"),
            false,
            false,
            false),
        GATE_DATA.at("CX").tableau());

    ASSERT_EQ(
        circuit_to_tableau(
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
        GATE_DATA.at("CX").tableau());
}

TEST(conversions, circuit_to_output_state_vector) {
    ASSERT_EQ(circuit_to_output_state_vector(Circuit(""), false), (std::vector<std::complex<float>>{{1}}));
    ASSERT_EQ(
        circuit_to_output_state_vector(Circuit("H 0 1"), false),
        (std::vector<std::complex<float>>{{0.5}, {0.5}, {0.5}, {0.5}}));
    ASSERT_EQ(
        circuit_to_output_state_vector(Circuit("X 1"), false), (std::vector<std::complex<float>>{{0}, {1}, {0}, {0}}));
    ASSERT_EQ(
        circuit_to_output_state_vector(Circuit("X 1"), true), (std::vector<std::complex<float>>{{0}, {0}, {1}, {0}}));
}

TEST(conversions, tableau_to_circuit_fuzz_vs_circuit_to_tableau) {
    for (size_t n = 0; n < 10; n++) {
        Tableau desired = Tableau::random(n, SHARED_TEST_RNG());
        Circuit circuit = tableau_to_circuit(desired, "elimination");
        Tableau actual = circuit_to_tableau(circuit, false, false, false);
        ASSERT_EQ(actual, desired);

        for (const auto &op : circuit.operations) {
            ASSERT_TRUE(
                op.gate == &GATE_DATA.at("S") || op.gate == &GATE_DATA.at("H") || op.gate == &GATE_DATA.at("CX"))
                << op;
        }
    }
}

TEST(conversions, tableau_to_circuit) {
    ASSERT_EQ(tableau_to_circuit(GATE_DATA.at("I").tableau(), "elimination"), Circuit(R"CIRCUIT(
            H 0
            H 0
        )CIRCUIT"));

    ASSERT_EQ(tableau_to_circuit(GATE_DATA.at("X").tableau(), "elimination"), Circuit(R"CIRCUIT(
            H 0
            S 0
            S 0
            H 0
        )CIRCUIT"));

    ASSERT_EQ(tableau_to_circuit(GATE_DATA.at("S").tableau(), "elimination"), Circuit(R"CIRCUIT(
            S 0
        )CIRCUIT"));

    ASSERT_EQ(tableau_to_circuit(GATE_DATA.at("ISWAP").tableau(), "elimination"), Circuit(R"CIRCUIT(
            CX 1 0 0 1 1 0
            S 0
            H 1
            CX 0 1
            H 1
            S 1
        )CIRCUIT"));
}

TEST(conversions, unitary_to_tableau_vs_gate_data) {
    for (const auto &gate : GATE_DATA.gates()) {
        if (gate.flags & GATE_IS_UNITARY) {
            ASSERT_EQ(unitary_to_tableau(gate.unitary(), true), gate.tableau()) << gate.name;
        }
    }
}

TEST(conversions, tableau_to_unitary_vs_gate_data) {
    VectorSimulator v1(2);
    VectorSimulator v2(2);
    for (const auto &gate : GATE_DATA.gates()) {
        if (gate.flags & GATE_IS_UNITARY) {
            auto actual = tableau_to_unitary(gate.tableau(), true);
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
}

TEST(conversions, unitary_vs_tableau_basic) {
    ASSERT_EQ(unitary_to_tableau(GATE_DATA.at("XCZ").unitary(), false), GATE_DATA.at("ZCX").tableau());
    ASSERT_EQ(unitary_to_tableau(GATE_DATA.at("XCZ").unitary(), true), GATE_DATA.at("XCZ").tableau());
    ASSERT_EQ(unitary_to_tableau(GATE_DATA.at("ZCX").unitary(), false), GATE_DATA.at("XCZ").tableau());
    ASSERT_EQ(unitary_to_tableau(GATE_DATA.at("ZCX").unitary(), true), GATE_DATA.at("ZCX").tableau());

    ASSERT_EQ(unitary_to_tableau(GATE_DATA.at("XCY").unitary(), false), GATE_DATA.at("YCX").tableau());
    ASSERT_EQ(unitary_to_tableau(GATE_DATA.at("XCY").unitary(), true), GATE_DATA.at("XCY").tableau());
    ASSERT_EQ(unitary_to_tableau(GATE_DATA.at("YCX").unitary(), false), GATE_DATA.at("XCY").tableau());
    ASSERT_EQ(unitary_to_tableau(GATE_DATA.at("YCX").unitary(), true), GATE_DATA.at("YCX").tableau());
}

TEST(conversions, unitary_to_tableau_fuzz_vs_tableau_to_unitary) {
    for (bool little_endian : std::vector<bool>{false, true}) {
        for (size_t n = 0; n < 6; n++) {
            Tableau desired = Tableau::random(n, SHARED_TEST_RNG());
            auto unitary = tableau_to_unitary(desired, little_endian);
            auto actual = unitary_to_tableau(unitary, little_endian);
            ASSERT_EQ(actual, desired) << "little_endian=" << little_endian << ", n=" << n;
        }
    }
}

TEST(conversions, unitary_to_tableau_fail) {
    ASSERT_THROW({ unitary_to_tableau({{{1}, {0}}, {{0}, {sqrtf(0.5), sqrtf(0.5)}}}, false); }, std::invalid_argument);
    ASSERT_THROW(
        {
            unitary_to_tableau(
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
            unitary_to_tableau(
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
}

TEST(conversions, stabilizers_to_tableau_fuzz) {
    for (size_t n = 0; n < 10; n++) {
        Tableau t = Tableau::random(n, SHARED_TEST_RNG());
        std::vector<stim::PauliString> expected_stabilizers;
        for (size_t k = 0; k < n; k++) {
            expected_stabilizers.push_back(t.zs[k]);
        }
        auto actual = stabilizers_to_tableau(expected_stabilizers, false, false, false);
        for (size_t k = 0; k < n; k++) {
            ASSERT_EQ(actual.zs[k], expected_stabilizers[k]);
        }

        ASSERT_TRUE(actual.satisfies_invariants());
    }
}

TEST(conversions, stabilizers_to_tableau_partial_fuzz) {
    for (size_t n = 0; n < 10; n++) {
        for (size_t skipped = 1; skipped < n && skipped < 4; skipped++) {
            Tableau t = Tableau::random(n, SHARED_TEST_RNG());
            std::vector<stim::PauliString> expected_stabilizers;
            for (size_t k = 0; k < n - skipped; k++) {
                expected_stabilizers.push_back(t.zs[k]);
            }
            ASSERT_THROW({ stabilizers_to_tableau(expected_stabilizers, false, false, false); }, std::invalid_argument);
            auto actual = stabilizers_to_tableau(expected_stabilizers, false, true, false);
            for (size_t k = 0; k < n - skipped; k++) {
                ASSERT_EQ(actual.zs[k], expected_stabilizers[k]);
            }

            ASSERT_TRUE(actual.satisfies_invariants());

            auto inverted = stabilizers_to_tableau(expected_stabilizers, false, true, true);
            ASSERT_EQ(actual.inverse(), inverted);
        }
    }
}

TEST(conversions, stabilizers_to_tableau_overconstrained) {
    for (size_t n = 4; n < 10; n++) {
        Tableau t = Tableau::random(n, SHARED_TEST_RNG());
        std::vector<stim::PauliString> expected_stabilizers;
        expected_stabilizers.push_back(PauliString(n));
        expected_stabilizers.push_back(PauliString(n));
        uint8_t s = 0;
        s += expected_stabilizers.back().ref().inplace_right_mul_returning_log_i_scalar(t.zs[1]);
        s += expected_stabilizers.back().ref().inplace_right_mul_returning_log_i_scalar(t.zs[3]);
        if (s & 2) {
            expected_stabilizers.back().sign ^= true;
        }
        for (size_t k = 0; k < n; k++) {
            expected_stabilizers.push_back(t.zs[k]);
        }
        ASSERT_THROW({ stabilizers_to_tableau(expected_stabilizers, false, false, false); }, std::invalid_argument);
        auto actual = stabilizers_to_tableau(expected_stabilizers, true, false, false);
        for (size_t k = 0; k < n; k++) {
            ASSERT_EQ(actual.zs[k], expected_stabilizers[k + 1 + (k > 3)]);
        }

        ASSERT_TRUE(actual.satisfies_invariants());
    }
}

TEST(conversions, stabilizers_to_tableau_bell_pair) {
    std::vector<stim::PauliString> input_stabilizers;
    input_stabilizers.push_back(PauliString::from_str("XX"));
    input_stabilizers.push_back(PauliString::from_str("ZZ"));
    auto actual = stabilizers_to_tableau(input_stabilizers, false, false, false);
    Tableau expected(2);
    expected.zs[0] = PauliString::from_str("XX");
    expected.zs[1] = PauliString::from_str("ZZ");
    expected.xs[0] = PauliString::from_str("Z_");
    expected.xs[1] = PauliString::from_str("_X");
    ASSERT_EQ(actual, expected);

    input_stabilizers.push_back(PauliString::from_str("-YY"));
    ASSERT_THROW({ stabilizers_to_tableau(input_stabilizers, false, false, false); }, std::invalid_argument);
    actual = stabilizers_to_tableau(input_stabilizers, true, false, false);
    ASSERT_EQ(actual, expected);

    input_stabilizers[2] = PauliString::from_str("+YY");
    // Sign is wrong!
    ASSERT_THROW({ stabilizers_to_tableau(input_stabilizers, true, true, false); }, std::invalid_argument);

    input_stabilizers[2] = PauliString::from_str("+Z_");
    // Anticommutes!
    ASSERT_THROW({ stabilizers_to_tableau(input_stabilizers, true, true, false); }, std::invalid_argument);
}
