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

#include "stim/simulators/tableau_simulator.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.test.h"
#include "stim/test_util.test.h"

using namespace stim;

TEST(TableauSimulator, identity) {
    auto s = TableauSimulator(SHARED_TEST_RNG(), 1);
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{}));
    s.measure_z(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false}));
    s.measure_z(OpDat::flipped(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false, true}));
}

TEST(TableauSimulator, bit_flip) {
    auto s = TableauSimulator(SHARED_TEST_RNG(), 1);
    s.H_XZ(OpDat(0));
    s.SQRT_Z(OpDat(0));
    s.SQRT_Z(OpDat(0));
    s.H_XZ(OpDat(0));
    s.measure_z(OpDat(0));
    s.X(OpDat(0));
    s.measure_z(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{true, false}));
}

TEST(TableauSimulator, identity2) {
    auto s = TableauSimulator(SHARED_TEST_RNG(), 2);
    s.measure_z(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false}));
    s.measure_z(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false, false}));
}

TEST(TableauSimulator, bit_flip_2) {
    auto s = TableauSimulator(SHARED_TEST_RNG(), 2);
    s.H_XZ(OpDat(0));
    s.SQRT_Z(OpDat(0));
    s.SQRT_Z(OpDat(0));
    s.H_XZ(OpDat(0));
    s.measure_z(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{true}));
    s.measure_z(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{true, false}));
}

TEST(TableauSimulator, epr) {
    auto s = TableauSimulator(SHARED_TEST_RNG(), 2);
    s.H_XZ(OpDat(0));
    s.ZCX(OpDat({0, 1}));
    ASSERT_EQ(s.is_deterministic_z(0), false);
    ASSERT_EQ(s.is_deterministic_z(1), false);
    s.measure_z(OpDat(0));
    ASSERT_EQ(s.is_deterministic_z(0), true);
    ASSERT_EQ(s.is_deterministic_z(1), true);
    s.measure_z(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage[0], s.measurement_record.storage[1]);
}

TEST(TableauSimulator, big_determinism) {
    auto s = TableauSimulator(SHARED_TEST_RNG(), 1000);
    s.H_XZ(OpDat(0));
    s.H_YZ(OpDat(1));
    ASSERT_FALSE(s.is_deterministic_z(0));
    ASSERT_FALSE(s.is_deterministic_z(1));
    ASSERT_TRUE(s.is_deterministic_x(0));
    ASSERT_FALSE(s.is_deterministic_x(1));
    ASSERT_FALSE(s.is_deterministic_y(0));
    ASSERT_TRUE(s.is_deterministic_y(1));
    for (size_t k = 2; k < 1000; k++) {
        ASSERT_TRUE(s.is_deterministic_z(k));
        ASSERT_FALSE(s.is_deterministic_x(k));
        ASSERT_FALSE(s.is_deterministic_y(k));
    }
}

TEST(TableauSimulator, phase_kickback_consume_s_state) {
    for (size_t k = 0; k < 8; k++) {
        auto s = TableauSimulator(SHARED_TEST_RNG(), 2);
        s.H_XZ(OpDat(1));
        s.SQRT_Z(OpDat(1));
        s.H_XZ(OpDat(0));
        s.ZCX(OpDat({0, 1}));
        ASSERT_EQ(s.is_deterministic_z(1), false);
        s.measure_z(OpDat(1));
        auto v1 = s.measurement_record.storage.back();
        if (v1) {
            s.SQRT_Z(OpDat(0));
            s.SQRT_Z(OpDat(0));
        }
        s.SQRT_Z(OpDat(0));
        s.H_XZ(OpDat(0));
        ASSERT_EQ(s.is_deterministic_z(0), true);
        s.measure_z(OpDat(0));
        ASSERT_EQ(s.measurement_record.storage.back(), true);
    }
}

TEST(TableauSimulator, phase_kickback_preserve_s_state) {
    auto s = TableauSimulator(SHARED_TEST_RNG(), 2);

    // Prepare S state.
    s.H_XZ(OpDat(1));
    s.SQRT_Z(OpDat(1));

    // Prepare test input.
    s.H_XZ(OpDat(0));

    // Kickback.
    s.ZCX(OpDat({0, 1}));
    s.H_XZ(OpDat(1));
    s.ZCX(OpDat({0, 1}));
    s.H_XZ(OpDat(1));

    // Check.
    s.SQRT_Z(OpDat(0));
    s.H_XZ(OpDat(0));
    ASSERT_EQ(s.is_deterministic_z(0), true);
    s.measure_z(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage.back(), true);
    s.SQRT_Z(OpDat(1));
    s.H_XZ(OpDat(1));
    ASSERT_EQ(s.is_deterministic_z(1), true);
    s.measure_z(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage.back(), true);
}

TEST(TableauSimulator, kickback_vs_stabilizer) {
    auto sim = TableauSimulator(SHARED_TEST_RNG(), 3);
    sim.H_XZ(OpDat(2));
    sim.ZCX(OpDat({2, 0}));
    sim.ZCX(OpDat({2, 1}));
    sim.SQRT_Z(OpDat(0));
    sim.SQRT_Z(OpDat(1));
    sim.H_XZ(OpDat(0));
    sim.H_XZ(OpDat(1));
    sim.H_XZ(OpDat(2));
    ASSERT_EQ(
        sim.inv_state.str(),
        "+-xz-xz-xz-\n"
        "| +- +- ++\n"
        "| ZY __ _X\n"
        "| __ ZY _X\n"
        "| XX XX XZ");
}

TEST(TableauSimulator, s_state_distillation_low_depth) {
    for (size_t reps = 0; reps < 10; reps++) {
        auto sim = TableauSimulator(SHARED_TEST_RNG(), 9);

        std::vector<std::vector<uint8_t>> stabilizers = {
            {0, 1, 2, 3},
            {0, 1, 4, 5},
            {0, 2, 4, 6},
            {1, 2, 4, 7},
        };
        std::vector<std::unordered_map<std::string, std::vector<uint8_t>>> checks{
            {{"s", {0}}, {"q", stabilizers[0]}},
            {{"s", {1}}, {"q", stabilizers[1]}},
            {{"s", {2}}, {"q", stabilizers[2]}},
        };

        std::vector<bool> stabilizer_measurements;
        uint32_t anc = 8;
        for (const auto &stabilizer : stabilizers) {
            sim.H_XZ(OpDat(anc));
            for (const auto &k : stabilizer) {
                sim.ZCX(OpDat({anc, k}));
            }
            sim.H_XZ(OpDat(anc));
            ASSERT_EQ(sim.is_deterministic_z(anc), false);
            sim.measure_z(OpDat(anc));
            bool v = sim.measurement_record.storage.back();
            if (v) {
                sim.X(OpDat(anc));
            }
            stabilizer_measurements.push_back(v);
        }

        std::vector<bool> qubit_measurements;
        for (size_t k = 0; k < 7; k++) {
            sim.SQRT_Z(OpDat(k));
            sim.H_XZ(OpDat(k));
            sim.measure_z(OpDat(k));
            qubit_measurements.push_back(sim.measurement_record.storage.back());
        }

        bool sum = false;
        for (auto e : stabilizer_measurements) {
            sum ^= e;
        }
        for (auto e : qubit_measurements) {
            sum ^= e;
        }
        if (sum) {
            sim.Z(OpDat(7));
        }

        sim.SQRT_Z(OpDat(7));
        sim.H_XZ(OpDat(7));
        ASSERT_EQ(sim.is_deterministic_z(7), true);
        sim.measure_z(OpDat(7));
        ASSERT_EQ(sim.measurement_record.storage.back(), false);

        for (const auto &c : checks) {
            bool r = false;
            for (auto k : c.at("s")) {
                r ^= stabilizer_measurements[k];
            }
            for (auto k : c.at("q")) {
                r ^= qubit_measurements[k];
            }
            ASSERT_EQ(r, false);
        }
    }
}

TEST(TableauSimulator, s_state_distillation_low_space) {
    for (size_t rep = 0; rep < 10; rep++) {
        auto sim = TableauSimulator(SHARED_TEST_RNG(), 5);

        std::vector<std::vector<uint8_t>> phasors = {
            {
                0,
            },
            {
                1,
            },
            {
                2,
            },
            {0, 1, 2},
            {0, 1, 3},
            {0, 2, 3},
            {1, 2, 3},
        };

        uint32_t anc = 4;
        for (const auto &phasor : phasors) {
            sim.H_XZ(OpDat(anc));
            for (const auto &k : phasor) {
                sim.ZCX(OpDat({anc, k}));
            }
            sim.H_XZ(OpDat(anc));
            sim.SQRT_Z(OpDat(anc));
            sim.H_XZ(OpDat(anc));
            ASSERT_EQ(sim.is_deterministic_z(anc), false);
            sim.measure_z(OpDat(anc));
            bool v = sim.measurement_record.storage.back();
            if (v) {
                for (const auto &k : phasor) {
                    sim.X(OpDat(k));
                }
                sim.X(OpDat(anc));
            }
        }

        for (size_t k = 0; k < 3; k++) {
            ASSERT_EQ(sim.is_deterministic_z(k), true);
            sim.measure_z(OpDat(k));
            ASSERT_EQ(sim.measurement_record.storage.back(), false);
        }
        sim.SQRT_Z(OpDat(3));
        sim.H_XZ(OpDat(3));
        ASSERT_EQ(sim.is_deterministic_z(3), true);
        sim.measure_z(OpDat(3));
        ASSERT_EQ(sim.measurement_record.storage.back(), true);
    }
}

TEST(TableauSimulator, unitary_gates_consistent_with_tableau_data) {
    auto t = Tableau::random(10, SHARED_TEST_RNG());
    TableauSimulator sim(SHARED_TEST_RNG(), 10);
    sim.inv_state = t;
    for (const auto &gate : GATE_DATA.gates()) {
        if (!(gate.flags & GATE_IS_UNITARY)) {
            continue;
        }

        const auto &action = gate.tableau_simulator_function;
        const auto &inverse_op_tableau = gate.inverse().tableau();
        if (inverse_op_tableau.num_qubits == 2) {
            (sim.*action)(OpDat({7, 4}));
            t.inplace_scatter_prepend(inverse_op_tableau, {7, 4});
        } else {
            (sim.*action)(OpDat(5));
            t.inplace_scatter_prepend(inverse_op_tableau, {5});
        }
        EXPECT_EQ(sim.inv_state, t) << gate.name;
    }
}

TEST(TableauSimulator, certain_errors_consistent_with_gates) {
    TableauSimulator sim1(SHARED_TEST_RNG(), 2);
    TableauSimulator sim2(SHARED_TEST_RNG(), 2);
    GateTarget targets[]{GateTarget{0}};
    double p0 = 0.0;
    double p1 = 1.0;
    OperationData d0{{&p0}, {targets}};
    OperationData d1{{&p1}, {targets}};

    sim1.X_ERROR(d1);
    sim2.X(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
    sim1.X_ERROR(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);

    sim1.Y_ERROR(d1);
    sim2.Y(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
    sim1.Y_ERROR(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);

    sim1.Z_ERROR(d1);
    sim2.Z(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
    sim1.Z_ERROR(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
}

TEST(TableauSimulator, simulate) {
    auto results = TableauSimulator::sample_circuit(
        Circuit("H 0\n"
                "CNOT 0 1\n"
                "M 0\n"
                "M 1\n"
                "M 2\n"),
        SHARED_TEST_RNG());
    ASSERT_EQ(results[0], results[1]);
    ASSERT_EQ(results[2], false);
}

TEST(TableauSimulator, simulate_reset) {
    auto results = TableauSimulator::sample_circuit(
        Circuit("X 0\n"
                "M 0\n"
                "R 0\n"
                "M 0\n"
                "R 0\n"
                "M 0\n"),
        SHARED_TEST_RNG());
    ASSERT_EQ(results[0], true);
    ASSERT_EQ(results[1], false);
    ASSERT_EQ(results[2], false);
}

TEST(TableauSimulator, to_vector_sim) {
    TableauSimulator sim_tab(SHARED_TEST_RNG(), 2);
    VectorSimulator sim_vec(2);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.X(OpDat(0));
    sim_vec.apply("X", 0);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.H_XZ(OpDat(0));
    sim_vec.apply("H_XZ", 0);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.SQRT_Z(OpDat(0));
    sim_vec.apply("SQRT_Z", 0);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.ZCX(OpDat({0, 1}));
    sim_vec.apply("ZCX", 0, 1);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.inv_state = Tableau::random(10, SHARED_TEST_RNG());
    sim_vec = sim_tab.to_vector_sim();
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    (sim_tab.*GATE_DATA.at("XCX").tableau_simulator_function)(OpDat({4, 7}));
    sim_vec.apply("XCX", 4, 7);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));
}

TEST(TableauSimulator, to_state_vector) {
    auto v = TableauSimulator(SHARED_TEST_RNG(), 0).to_state_vector(true);
    ASSERT_EQ(v.size(), 1);
    auto r = v[0].real();
    auto i = v[0].imag();
    ASSERT_LT(r * r + i * i - 1, 1e-4);

    TableauSimulator sim_tab(SHARED_TEST_RNG(), 3);
    auto sim_vec = sim_tab.to_vector_sim();
    VectorSimulator sim_vec2(3);
    sim_vec2.state = sim_tab.to_state_vector(true);
    ASSERT_TRUE(sim_vec.approximate_equals(sim_vec2, true));
}

TEST(TableauSimulator, to_state_vector_endian) {
    VectorSimulator sim_vec0(3);
    VectorSimulator sim_vec2(3);
    sim_vec0.apply("H", 0);
    sim_vec2.apply("H", 2);

    TableauSimulator sim_tab(SHARED_TEST_RNG(), 3);
    sim_tab.H_XZ(OpDat(2));

    VectorSimulator cmp(3);
    cmp.state = sim_tab.to_state_vector(true);
    ASSERT_TRUE(cmp.approximate_equals(sim_vec2, true));
    cmp.state = sim_tab.to_state_vector(false);
    ASSERT_TRUE(cmp.approximate_equals(sim_vec0, true));
}

TEST(TableauSimulator, to_state_vector_canonical) {
    TableauSimulator sim_tab(SHARED_TEST_RNG(), 3);
    sim_tab.H_XZ(OpDat(2));
    std::vector<float> expected;

    auto actual = sim_tab.to_state_vector(true);
    expected = {sqrtf(0.5), 0, 0, 0, sqrtf(0.5), 0, 0, 0};
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t k = 0; k < 8; k++) {
        ASSERT_LT(abs(actual[k] - expected[k]), 1e-4) << k;
    }

    actual = sim_tab.to_state_vector(false);
    expected = {sqrtf(0.5), sqrtf(0.5), 0, 0, 0, 0, 0, 0};
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t k = 0; k < 8; k++) {
        ASSERT_LT(abs(actual[k] - expected[k]), 1e-4) << k;
    }
}

bool vec_sim_corroborates_measurement_process(const Tableau &state, const std::vector<uint32_t> &measurement_targets) {
    TableauSimulator sim_tab(SHARED_TEST_RNG(), 2);
    sim_tab.inv_state = state;
    auto vec_sim = sim_tab.to_vector_sim();
    sim_tab.measure_z(OpDat(measurement_targets));
    PauliString buf(sim_tab.inv_state.num_qubits);
    size_t k = 0;
    for (auto t : measurement_targets) {
        buf.zs[t] = true;
        buf.sign = sim_tab.measurement_record.storage[k++];
        float f = vec_sim.project(buf);
        if (fabs(f - 0.5) > 1e-4 && fabsf(f - 1) > 1e-4) {
            return false;
        }
        buf.zs[t] = false;
    }
    return true;
}

TEST(TableauSimulator, measurement_vs_vector_sim) {
    for (size_t k = 0; k < 10; k++) {
        Tableau state = Tableau::random(2, SHARED_TEST_RNG());
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 1}));
    }
    for (size_t k = 0; k < 10; k++) {
        Tableau state = Tableau::random(4, SHARED_TEST_RNG());
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {2, 1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 1, 2, 3}));
    }
    {
        Tableau state = Tableau::random(12, SHARED_TEST_RNG());
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 1, 2, 3}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 10, 11}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {11, 5, 7}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
    }
}

TEST(TableauSimulator, correlated_error) {
    simd_bits<MAX_BITWORD_WIDTH> expected(5);

    expected.clear();
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            SHARED_TEST_RNG()),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            SHARED_TEST_RNG()),
        expected);

    expected.clear();
    expected[1] = true;
    expected[2] = true;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            SHARED_TEST_RNG()),
        expected);

    expected.clear();
    expected[2] = true;
    expected[3] = true;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(1) X2 X3
        M 0 1 2 3
    )circuit"),
            SHARED_TEST_RNG()),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            SHARED_TEST_RNG()),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(1) X2 X3
        M 0 1 2 3
    )circuit"),
            SHARED_TEST_RNG()),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    expected[3] = true;
    expected[4] = true;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(1) X2 X3
        CORRELATED_ERROR(1) X3 X4
        M 0 1 2 3 4
    )circuit"),
            SHARED_TEST_RNG()),
        expected);

    int hits[3]{};
    size_t n = 10000;
    std::mt19937_64 rng(0);
    for (size_t k = 0; k < n; k++) {
        auto sample = TableauSimulator::sample_circuit(
            Circuit(R"circuit(
            CORRELATED_ERROR(0.5) X0
            ELSE_CORRELATED_ERROR(0.25) X1
            ELSE_CORRELATED_ERROR(0.75) X2
            M 0 1 2
        )circuit"),
            rng);
        hits[0] += sample[0];
        hits[1] += sample[1];
        hits[2] += sample[2];
    }
    ASSERT_TRUE(0.45 * n < hits[0] && hits[0] < 0.55 * n);
    ASSERT_TRUE((0.125 - 0.05) * n < hits[1] && hits[1] < (0.125 + 0.05) * n);
    ASSERT_TRUE((0.28125 - 0.05) * n < hits[2] && hits[2] < (0.28125 + 0.05) * n);
}

TEST(TableauSimulator, quantum_cannot_control_classical) {
    // Quantum controlling classical operation is not allowed.
    ASSERT_THROW(
        {
            TableauSimulator::sample_circuit(
                Circuit(R"circuit(
            M 0
            CNOT 1 rec[-1]
        )circuit"),
                SHARED_TEST_RNG());
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            TableauSimulator::sample_circuit(
                Circuit(R"circuit(
            M 0
            CY 1 rec[-1]
        )circuit"),
                SHARED_TEST_RNG());
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            TableauSimulator::sample_circuit(
                Circuit(R"circuit(
            M 0
            YCZ rec[-1] 1
        )circuit"),
                SHARED_TEST_RNG());
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            TableauSimulator::sample_circuit(
                Circuit(R"circuit(
            M 0
            XCZ rec[-1] 1
        )circuit"),
                SHARED_TEST_RNG());
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            TableauSimulator::sample_circuit(
                Circuit(R"circuit(
            M 0
            SWAP 1 rec[-1]
        )circuit"),
                SHARED_TEST_RNG());
        },
        std::invalid_argument);
}

TEST(TableauSimulator, classical_can_control_quantum) {
    simd_bits<MAX_BITWORD_WIDTH> expected(5);
    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        M !0
        CX rec[-1] 1
        M 1
    )circuit"),
            SHARED_TEST_RNG()),
        expected);
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        M !0
        CY rec[-1] 1
        M 1
    )circuit"),
            SHARED_TEST_RNG()),
        expected);
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        M !0
        XCZ 1 rec[-1]
        M 1
    )circuit"),
            SHARED_TEST_RNG()),
        expected);
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        M !0
        YCZ 1 rec[-1]
        M 1
    )circuit"),
            SHARED_TEST_RNG()),
        expected);
}

TEST(TableauSimulator, classical_control_cases) {
    simd_bits<MAX_BITWORD_WIDTH> expected(5);
    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        M !0
        H 1
        CZ rec[-1] 1
        H 1
        M 1
    )circuit"),
            SHARED_TEST_RNG()),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        M !0
        CY rec[-1] 1
        M 1
    )circuit"),
            SHARED_TEST_RNG()),
        expected);

    expected.clear();
    expected[0] = false;
    expected[1] = false;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        M 0
        CX rec[-1] 1
        M 1
    )circuit"),
            SHARED_TEST_RNG()),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = false;
    expected[2] = true;
    expected[3] = false;
    expected[4] = false;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit(R"circuit(
        X 0
        M 0
        R 0
        M 0
        CX rec[-2] 1
        CX rec[-1] 2
        M 1 2 3
    )circuit"),
            SHARED_TEST_RNG()),
        expected);
}

TEST(TableauSimulator, mr_repeated_target) {
    simd_bits<MAX_BITWORD_WIDTH> expected(2);
    expected[0] = true;
    auto r = TableauSimulator::sample_circuit(Circuit("X 0\nMR 0 0"), SHARED_TEST_RNG());
    ASSERT_EQ(r, expected);
}

TEST(TableauSimulator, peek_bloch) {
    TableauSimulator sim(SHARED_TEST_RNG(), 3);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString::from_str("+Z"));

    sim.H_XZ(OpDat(0));
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString::from_str("+Z"));

    sim.X(OpDat(1));
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString::from_str("+Z"));

    sim.H_YZ(OpDat(2));
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString::from_str("+Y"));

    sim.X(OpDat(0));
    sim.X(OpDat(1));
    sim.X(OpDat(2));
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString::from_str("-Y"));

    sim.Y(OpDat(0));
    sim.Y(OpDat(1));
    sim.Y(OpDat(2));
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString::from_str("-Y"));

    sim.ZCZ(OpDat({0, 1}));
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString::from_str("-Y"));

    sim.ZCZ(OpDat({1, 2}));
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString::from_str("+Y"));

    sim.ZCZ(OpDat({0, 2}));
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+I"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString::from_str("+I"));

    sim.X(OpDat(0));
    sim.X(OpDat(1));
    sim.X(OpDat(2));
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+I"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString::from_str("+I"));
}

TEST(TableauSimulator, paulis) {
    TableauSimulator sim1(SHARED_TEST_RNG(), 500);
    TableauSimulator sim2(SHARED_TEST_RNG(), 500);
    sim1.inv_state = Tableau::random(500, SHARED_TEST_RNG());
    sim2.inv_state = sim1.inv_state;

    sim1.paulis(PauliString(500));
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
    sim1.paulis(PauliString(5));
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
    sim1.paulis(PauliString(0));
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);

    sim1.paulis(PauliString::from_str("IXYZ"));
    sim2.X(OpDat(1));
    sim2.Y(OpDat(2));
    sim2.Z(OpDat(3));
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
}

TEST(TableauSimulator, set_num_qubits) {
    TableauSimulator sim1(SHARED_TEST_RNG(), 10);
    TableauSimulator sim2(SHARED_TEST_RNG(), 10);
    sim1.inv_state = Tableau::random(10, SHARED_TEST_RNG());
    sim2.inv_state = sim1.inv_state;

    sim1.set_num_qubits(20);
    sim1.set_num_qubits(10);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);

    sim1.set_num_qubits(20);
    sim1.X(OpDat(10));
    sim1.Z(OpDat(11));
    sim1.H_XZ(OpDat(12));
    sim1.ZCX(OpDat({12, 13}));
    sim1.set_num_qubits(10);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);

    sim1.set_num_qubits(20);
    sim2.ensure_large_enough_for_qubits(20);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
}

TEST(TableauSimulator, set_num_qubits_reduce_random) {
    TableauSimulator sim(SHARED_TEST_RNG(), 10);
    sim.inv_state = Tableau::random(10, SHARED_TEST_RNG());
    sim.set_num_qubits(5);
    ASSERT_EQ(sim.inv_state.num_qubits, 5);
    ASSERT_TRUE(sim.inv_state.satisfies_invariants());
}

void scramble_stabilizers(TableauSimulator &s) {
    auto &rng = SHARED_TEST_RNG();
    TableauTransposedRaii tmp(s.inv_state);
    for (size_t i = 0; i < s.inv_state.num_qubits; i++) {
        for (size_t j = i + 1; j < s.inv_state.num_qubits; j++) {
            if (rng() & 1) {
                tmp.append_ZCX(i, j);
            }
            if (rng() & 1) {
                tmp.append_ZCX(j, i);
            }
            if (rng() & 1) {
                tmp.append_ZCZ(i, j);
            }
        }
        if (rng() & 1) {
            tmp.append_S(i);
        }
    }
}

TEST(TableauSimulator, canonical_stabilizers) {
    TableauSimulator sim(SHARED_TEST_RNG(), 2);
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    ASSERT_EQ(
        sim.canonical_stabilizers(),
        (std::vector<PauliString>{
            PauliString::from_str("XX"),
            PauliString::from_str("ZZ"),
        }));
    sim.SQRT_Y(OpDat({0, 1}));
    ASSERT_EQ(
        sim.canonical_stabilizers(),
        (std::vector<PauliString>{
            PauliString::from_str("XX"),
            PauliString::from_str("ZZ"),
        }));
    sim.SQRT_X(OpDat({0, 1}));
    ASSERT_EQ(
        sim.canonical_stabilizers(),
        (std::vector<PauliString>{
            PauliString::from_str("XX"),
            PauliString::from_str("-ZZ"),
        }));
    sim.set_num_qubits(3);
    ASSERT_EQ(
        sim.canonical_stabilizers(),
        (std::vector<PauliString>{
            PauliString::from_str("+XX_"),
            PauliString::from_str("-ZZ_"),
            PauliString::from_str("+__Z"),
        }));
    sim.ZCX(OpDat({2, 0}));
    ASSERT_EQ(
        sim.canonical_stabilizers(),
        (std::vector<PauliString>{
            PauliString::from_str("+XX_"),
            PauliString::from_str("-ZZ_"),
            PauliString::from_str("+__Z"),
        }));
}

TEST(TableauSimulator, canonical_stabilizers_random) {
    TableauSimulator sim(SHARED_TEST_RNG(), 4);
    sim.inv_state = Tableau::random(4, SHARED_TEST_RNG());
    auto s1 = sim.canonical_stabilizers();
    scramble_stabilizers(sim);
    auto s2 = sim.canonical_stabilizers();
    ASSERT_EQ(s1, s2);
}

TEST(TableauSimulator, set_num_qubits_reduce_preserves_scrambled_stabilizers) {
    auto &rng = SHARED_TEST_RNG();
    TableauSimulator sim(rng, 4);
    sim.inv_state = Tableau::random(4, SHARED_TEST_RNG());
    auto s1 = sim.canonical_stabilizers();
    sim.inv_state.expand(8);
    scramble_stabilizers(sim);
    sim.set_num_qubits(4);
    auto s2 = sim.canonical_stabilizers();
    ASSERT_EQ(s1, s2);
}

TEST(TableauSimulator, measure_kickback_z) {
    TableauSimulator sim(SHARED_TEST_RNG(), 4);
    sim.H_XZ(OpDat({0, 2}));
    sim.ZCX(OpDat({0, 1, 2, 3}));
    auto k1 = sim.measure_kickback_z(GateTarget::qubit(1));
    auto k2 = sim.measure_kickback_z(GateTarget::qubit(2));
    auto k3 = sim.measure_kickback_z(GateTarget::qubit(3));
    ASSERT_EQ(k1.second, PauliString::from_str("XX__"));
    ASSERT_EQ(k2.second, PauliString::from_str("__XX"));
    ASSERT_EQ(k3.second, PauliString(0));
    ASSERT_EQ(k2.first, k3.first);
    auto p = PauliString::from_str("+Z");
    auto pn = PauliString::from_str("-Z");
    ASSERT_EQ(sim.peek_bloch(0), k1.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(1), k1.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(2), k2.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(3), k2.first ? pn : p);
}

TEST(TableauSimulator, measure_kickback_x) {
    TableauSimulator sim(SHARED_TEST_RNG(), 4);
    sim.H_XZ(OpDat({0, 2}));
    sim.ZCX(OpDat({0, 1, 2, 3}));
    auto k1 = sim.measure_kickback_x(GateTarget::qubit(1));
    auto k2 = sim.measure_kickback_x(GateTarget::qubit(2));
    auto k3 = sim.measure_kickback_x(GateTarget::qubit(3));
    ASSERT_EQ(k1.second, PauliString::from_str("ZZ__"));
    ASSERT_EQ(k2.second, PauliString::from_str("__ZZ"));
    ASSERT_EQ(k3.second, PauliString(0));
    ASSERT_EQ(k2.first, k3.first);
    auto p = PauliString::from_str("+X");
    auto pn = PauliString::from_str("-X");
    ASSERT_EQ(sim.peek_bloch(0), k1.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(1), k1.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(2), k2.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(3), k2.first ? pn : p);
}

TEST(TableauSimulator, measure_kickback_y) {
    TableauSimulator sim(SHARED_TEST_RNG(), 4);
    sim.H_XZ(OpDat({0, 2}));
    sim.ZCX(OpDat({0, 1, 2, 3}));
    auto k1 = sim.measure_kickback_y(GateTarget::qubit(1));
    auto k2 = sim.measure_kickback_y(GateTarget::qubit(2));
    auto k3 = sim.measure_kickback_y(GateTarget::qubit(3));
    ASSERT_EQ(k1.second, PauliString::from_str("ZX__"));
    ASSERT_EQ(k2.second, PauliString::from_str("__ZX"));
    ASSERT_EQ(k3.second, PauliString(0));
    ASSERT_NE(k2.first, k3.first);
    auto p = PauliString::from_str("+Y");
    auto pn = PauliString::from_str("-Y");
    ASSERT_EQ(sim.peek_bloch(0), k1.first ? p : pn);
    ASSERT_EQ(sim.peek_bloch(1), k1.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(2), k2.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(3), k2.first ? p : pn);
}

TEST(TableauSimulator, measure_kickback_isolates) {
    TableauSimulator sim(SHARED_TEST_RNG(), 4);
    sim.inv_state = Tableau::random(4, SHARED_TEST_RNG());
    for (size_t k = 0; k < 4; k++) {
        auto result = sim.measure_kickback_z(GateTarget::qubit(k));
        for (size_t j = 0; j < result.second.num_qubits && j < k; j++) {
            ASSERT_FALSE(result.second.xs[j]);
            ASSERT_FALSE(result.second.zs[j]);
        }
    }
}

TEST(TableauSimulator, collapse_isolate_completely) {
    for (size_t k = 0; k < 10; k++) {
        TableauSimulator sim(SHARED_TEST_RNG(), 6);
        sim.inv_state = Tableau::random(6, SHARED_TEST_RNG());
        {
            TableauTransposedRaii tmp(sim.inv_state);
            sim.collapse_isolate_qubit_z(2, tmp);
        }
        PauliString x2 = sim.inv_state.xs[2];
        PauliString z2 = sim.inv_state.zs[2];
        x2.sign = false;
        z2.sign = false;
        ASSERT_EQ(x2, PauliString::from_str("__X___"));
        ASSERT_EQ(z2, PauliString::from_str("__Z___"));
    }
}

TEST(TableauSimulator, reset_pure) {
    TableauSimulator t(SHARED_TEST_RNG(), 1);
    ASSERT_EQ(t.peek_bloch(0), PauliString::from_str("+Z"));
    t.reset_y(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString::from_str("+Y"));
    t.reset_x(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString::from_str("+X"));
    t.reset_y(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString::from_str("+Y"));
    t.reset_z(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString::from_str("+Z"));
}

TEST(TableauSimulator, reset_random) {
    TableauSimulator t(SHARED_TEST_RNG(), 5);

    t.inv_state = Tableau::random(5, SHARED_TEST_RNG());
    t.reset_x(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString::from_str("+X"));

    t.inv_state = Tableau::random(5, SHARED_TEST_RNG());
    t.reset_y(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString::from_str("+Y"));

    t.inv_state = Tableau::random(5, SHARED_TEST_RNG());
    t.reset_z(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString::from_str("+Z"));

    t.inv_state = Tableau::random(5, SHARED_TEST_RNG());
    t.measure_reset_x(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString::from_str("+X"));

    t.inv_state = Tableau::random(5, SHARED_TEST_RNG());
    t.measure_reset_y(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString::from_str("+Y"));

    t.inv_state = Tableau::random(5, SHARED_TEST_RNG());
    t.measure_reset_z(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString::from_str("+Z"));
}

TEST(TableauSimulator, reset_x_entangled) {
    TableauSimulator t(SHARED_TEST_RNG(), 2);
    t.H_XZ(OpDat(0));
    t.ZCX(OpDat({0, 1}));
    t.reset_x(OpDat(0));
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign = false;
    ASSERT_EQ(p1, PauliString::from_str("+X"));
    ASSERT_EQ(p2, PauliString::from_str("+X"));
}

TEST(TableauSimulator, reset_y_entangled) {
    TableauSimulator t(SHARED_TEST_RNG(), 2);
    t.H_XZ(OpDat(0));
    t.ZCX(OpDat({0, 1}));
    t.reset_y(OpDat(0));
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign = false;
    ASSERT_EQ(p1, PauliString::from_str("+Y"));
    ASSERT_EQ(p2, PauliString::from_str("+Y"));
}

TEST(TableauSimulator, reset_z_entangled) {
    TableauSimulator t(SHARED_TEST_RNG(), 2);
    t.H_XZ(OpDat(0));
    t.ZCX(OpDat({0, 1}));
    t.reset_z(OpDat(0));
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign = false;
    ASSERT_EQ(p1, PauliString::from_str("+Z"));
    ASSERT_EQ(p2, PauliString::from_str("+Z"));
}

TEST(TableauSimulator, measure_x_entangled) {
    TableauSimulator t(SHARED_TEST_RNG(), 2);
    t.H_XZ(OpDat(0));
    t.ZCX(OpDat({0, 1}));
    t.measure_x(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p1.sign ^= b;
    p2.sign ^= b;
    ASSERT_EQ(p1, PauliString::from_str("+X"));
    ASSERT_EQ(p2, PauliString::from_str("+X"));
}

TEST(TableauSimulator, measure_y_entangled) {
    TableauSimulator t(SHARED_TEST_RNG(), 2);
    t.H_XZ(OpDat(0));
    t.ZCX(OpDat({0, 1}));
    t.measure_y(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p1.sign ^= b;
    p2.sign ^= !b;
    ASSERT_EQ(p1, PauliString::from_str("+Y"));
    ASSERT_EQ(p2, PauliString::from_str("+Y"));
}

TEST(TableauSimulator, measure_z_entangled) {
    TableauSimulator t(SHARED_TEST_RNG(), 2);
    t.H_XZ(OpDat(0));
    t.ZCX(OpDat({0, 1}));
    t.measure_z(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p1.sign ^= b;
    p2.sign ^= b;
    ASSERT_EQ(p1, PauliString::from_str("+Z"));
    ASSERT_EQ(p2, PauliString::from_str("+Z"));
}

TEST(TableauSimulator, measure_reset_x_entangled) {
    TableauSimulator t(SHARED_TEST_RNG(), 2);
    t.H_XZ(OpDat(0));
    t.ZCX(OpDat({0, 1}));
    t.measure_reset_x(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign ^= b;
    ASSERT_EQ(p1, PauliString::from_str("+X"));
    ASSERT_EQ(p2, PauliString::from_str("+X"));
}

TEST(TableauSimulator, measure_reset_y_entangled) {
    TableauSimulator t(SHARED_TEST_RNG(), 2);
    t.H_XZ(OpDat(0));
    t.ZCX(OpDat({0, 1}));
    t.measure_reset_y(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign ^= !b;
    ASSERT_EQ(p1, PauliString::from_str("+Y"));
    ASSERT_EQ(p2, PauliString::from_str("+Y"));
}

TEST(TableauSimulator, measure_reset_z_entangled) {
    TableauSimulator t(SHARED_TEST_RNG(), 2);
    t.H_XZ(OpDat(0));
    t.ZCX(OpDat({0, 1}));
    t.measure_reset_z(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign ^= b;
    ASSERT_EQ(p1, PauliString::from_str("+Z"));
    ASSERT_EQ(p2, PauliString::from_str("+Z"));
}

TEST(TableauSimulator, resets_vs_measurements) {
    auto check = [&](const char *circuit, std::vector<bool> results) {
        simd_bits<MAX_BITWORD_WIDTH> ref(results.size());
        for (size_t k = 0; k < results.size(); k++) {
            ref[k] = results[k];
        }
        for (size_t reps = 0; reps < 5; reps++) {
            simd_bits<MAX_BITWORD_WIDTH> t = TableauSimulator::sample_circuit(Circuit(circuit), SHARED_TEST_RNG());
            if (t != ref) {
                return false;
            }
        }
        return true;
    };

    ASSERT_TRUE(check(
        R"circuit(
        RX 0
        RY 1
        RZ 2
        H_XZ 0
        H_YZ 1
        M 0 1 2
    )circuit",
        {
            false,
            false,
            false,
        }));

    ASSERT_TRUE(check(
        R"circuit(
        H_XZ 0 1 2
        H_YZ 3 4 5
        X_ERROR(1) 0 3 6
        Y_ERROR(1) 1 4 7
        Z_ERROR(1) 2 5 8
        MX 0 1 2
        MY 3 4 5
        MZ 6 7 8
    )circuit",
        {
            false,
            true,
            true,
            true,
            false,
            true,
            true,
            true,
            false,
        }));

    ASSERT_TRUE(check(
        R"circuit(
        H_XZ 0 1 2
        H_YZ 3 4 5
        X_ERROR(1) 0 3 6
        Y_ERROR(1) 1 4 7
        Z_ERROR(1) 2 5 8
        MX !0 !1 !2
        MY !3 !4 !5
        MZ !6 !7 !8
    )circuit",
        {
            true,
            false,
            false,
            false,
            true,
            false,
            false,
            false,
            true,
        }));

    ASSERT_TRUE(check(
        R"circuit(
        H_XZ 0 1 2
        H_YZ 3 4 5
        X_ERROR(1) 0 3 6
        Y_ERROR(1) 1 4 7
        Z_ERROR(1) 2 5 8
        MRX 0 1 2
        MRY 3 4 5
        MRZ 6 7 8
        H_XZ 0
        H_YZ 3
        M 0 3 6
    )circuit",
        {
            false,
            true,
            true,
            true,
            false,
            true,
            true,
            true,
            false,
            false,
            false,
            false,
        }));

    ASSERT_TRUE(check(
        R"circuit(
        H_XZ 0 1 2
        H_YZ 3 4 5
        X_ERROR(1) 0 3 6
        Y_ERROR(1) 1 4 7
        Z_ERROR(1) 2 5 8
        MRX !0 !1 !2
        MRY !3 !4 !5
        MRZ !6 !7 !8
        H_XZ 0
        H_YZ 3
        M 0 3 6
    )circuit",
        {
            true,
            false,
            false,
            false,
            true,
            false,
            false,
            false,
            true,
            false,
            false,
            false,
        }));

    ASSERT_TRUE(check(
        R"circuit(
        H_XZ 0
        H_YZ 1
        Z_ERROR(1) 0 1
        X_ERROR(1) 2
        MRX 0 0
        MRY 1 1
        MRZ 2 2
    )circuit",
        {
            true,
            false,
            true,
            false,
            true,
            false,
        }));
}

TEST(TableauSimulator, sample_circuit_mutates_rng_state) {
  std::mt19937_64 rng(1234);
  TableauSimulator::sample_circuit(Circuit("H 0\nM 0"), rng);
  ASSERT_NE(rng, std::mt19937_64(1234));
}

TEST(TableauSimulator, sample_stream_mutates_rng_state) {
  FILE *in = tmpfile();
  FILE *out = tmpfile();
  fprintf(in, "H 0\nM 0\n");
  rewind(in);

  std::mt19937_64 rng(2345);
  TableauSimulator::sample_stream(in, out, SAMPLE_FORMAT_B8, false, rng);

  ASSERT_NE(rng, std::mt19937_64(2345));
}

TEST(TableauSimulator, noisy_measurement_x) {
    TableauSimulator t(SHARED_TEST_RNG());
    t.expand_do_circuit(R"CIRCUIT(
        RX 0
        REPEAT 10000 {
            MX(0.05) 0
        }
    )CIRCUIT");
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.expand_do_circuit("MX 0");
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.expand_do_circuit(R"CIRCUIT(
        RX 0 1
        Y 0 1
        REPEAT 5000 {
            MX(0.05) 0 1
        }
    )CIRCUIT");
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.expand_do_circuit("MX 0");
    ASSERT_TRUE(t.measurement_record.storage.back());
}

TEST(TableauSimulator, noisy_measurement_y) {
    TableauSimulator t(SHARED_TEST_RNG());
    t.expand_do_circuit(R"CIRCUIT(
        RY 0
        REPEAT 10000 {
            MY(0.05) 0
        }
    )CIRCUIT");
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.expand_do_circuit("MY 0");
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.expand_do_circuit(R"CIRCUIT(
        RY 0 1
        X 0 1
        REPEAT 5000 {
            MY(0.05) 0 1
        }
    )CIRCUIT");
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.expand_do_circuit("MY 0");
    ASSERT_TRUE(t.measurement_record.storage.back());
}

TEST(TableauSimulator, noisy_measurement_z) {
    TableauSimulator t(SHARED_TEST_RNG());
    t.expand_do_circuit(R"CIRCUIT(
        RZ 0
        REPEAT 10000 {
            MZ(0.05) 0
        }
    )CIRCUIT");
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.expand_do_circuit("MZ 0");
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.expand_do_circuit(R"CIRCUIT(
        RZ 0 1
        X 0 1
        REPEAT 5000 {
            MZ(0.05) 0 1
        }
    )CIRCUIT");
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.expand_do_circuit("MZ 0");
    ASSERT_TRUE(t.measurement_record.storage.back());
}

TEST(TableauSimulator, noisy_measure_reset_x) {
    TableauSimulator t(SHARED_TEST_RNG());
    t.expand_do_circuit(R"CIRCUIT(
        RX 0
        REPEAT 10000 {
            MRX(0.05) 0
        }
    )CIRCUIT");
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.expand_do_circuit("MX 0");
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.expand_do_circuit(R"CIRCUIT(
        RX 0 1
        REPEAT 5000 {
            Z 0 1
            MRX(0.05) 0 1
        }
    )CIRCUIT");
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.expand_do_circuit("MX 0");
    ASSERT_FALSE(t.measurement_record.storage.back());
}

TEST(TableauSimulator, noisy_measure_reset_y) {
    TableauSimulator t(SHARED_TEST_RNG());
    t.expand_do_circuit(R"CIRCUIT(
        RY 0 1
        REPEAT 5000 {
            MRY(0.05) 0 1
        }
    )CIRCUIT");
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.expand_do_circuit("MY 0");
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.expand_do_circuit(R"CIRCUIT(
        RY 0 1
        REPEAT 5000 {
            X 0 1
            MRY(0.05) 0 1
        }
    )CIRCUIT");
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.expand_do_circuit("MY 0");
    ASSERT_FALSE(t.measurement_record.storage.back());
}

TEST(TableauSimulator, noisy_measure_reset_z) {
    TableauSimulator t(SHARED_TEST_RNG());
    t.expand_do_circuit(R"CIRCUIT(
        RZ 0 1
        REPEAT 5000 {
            MRZ(0.05) 0 1
        }
    )CIRCUIT");
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.expand_do_circuit("MZ 0");
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.expand_do_circuit(R"CIRCUIT(
        RZ 0 1
        REPEAT 5000 {
            X 0 1
            MRZ(0.05) 0 1
        }
    )CIRCUIT");
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.expand_do_circuit("MZ 0");
    ASSERT_FALSE(t.measurement_record.storage.back());
}

TEST(TableauSimulator, measure_pauli_product_bad) {
    TableauSimulator t(SHARED_TEST_RNG());
    ASSERT_THROW({ t.expand_do_circuit("MPP X0*X0"); }, std::invalid_argument);
    ASSERT_THROW({ t.expand_do_circuit("MPP X0*Z0"); }, std::invalid_argument);
}

TEST(TableauSimulator, measure_pauli_product_1) {
    TableauSimulator t(SHARED_TEST_RNG());
    t.expand_do_circuit(R"CIRCUIT(
        REPEAT 100 {
            RX 0
            RY 1
            RZ 2
            MPP X0 Y1 Z2 X0*Y1*Z2
        }
    )CIRCUIT");
    ASSERT_EQ(t.measurement_record.storage.size(), 400);
    ASSERT_EQ(std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0), 0);
}

TEST(TableauSimulator, measure_pauli_product_4body) {
    for (size_t k = 0; k < 10; k++) {
        TableauSimulator t(SHARED_TEST_RNG());
        t.expand_do_circuit(R"CIRCUIT(
            MPP X0*X1*X2*X3
            MX 0 1 2 3 4 5
            MPP X2*X3*X4*X5
            MPP Z0*Z1*Z4*Z5 !Y0*Y1*Y4*Y5
        )CIRCUIT");
        auto x0123 = t.measurement_record.storage[0];
        auto x0 = t.measurement_record.storage[1];
        auto x1 = t.measurement_record.storage[2];
        auto x2 = t.measurement_record.storage[3];
        auto x3 = t.measurement_record.storage[4];
        auto x4 = t.measurement_record.storage[5];
        auto x5 = t.measurement_record.storage[6];
        auto x2345 = t.measurement_record.storage[7];
        auto mz0145 = t.measurement_record.storage[8];
        auto y0145 = t.measurement_record.storage[9];
        ASSERT_EQ(x0123, x0 ^ x1 ^ x2 ^ x3);
        ASSERT_EQ(x2345, x2 ^ x3 ^ x4 ^ x5);
        ASSERT_EQ(y0145 ^ mz0145 ^ 1, x0123 ^ x2345);
    }
}

TEST(TableauSimulator, measure_pauli_product_epr) {
    for (size_t k = 0; k < 10; k++) {
        TableauSimulator t(SHARED_TEST_RNG());
        t.expand_do_circuit(R"CIRCUIT(
            MPP X0*X1 Z0*Z1 Y0*Y1
            CNOT 0 1
            H 0
            M 0 1
        )CIRCUIT");
        auto x01 = t.measurement_record.storage[0];
        auto z01 = t.measurement_record.storage[1];
        auto y01 = t.measurement_record.storage[2];
        auto m0 = t.measurement_record.storage[3];
        auto m1 = t.measurement_record.storage[4];
        ASSERT_EQ(m0, x01);
        ASSERT_EQ(m1, z01);
        ASSERT_EQ(x01 ^ z01, y01 ^ 1);
    }
}

TEST(TableauSimulator, measure_pauli_product_inversions) {
    for (size_t k = 0; k < 10; k++) {
        TableauSimulator t(SHARED_TEST_RNG());
        t.expand_do_circuit(R"CIRCUIT(
            MPP !X0*!X1 !X0*X1 X0*!X1 X0*X1 X0 X1 !X0 !X1
        )CIRCUIT");
        auto a = t.measurement_record.storage[0];
        auto b = t.measurement_record.storage[1];
        auto c = t.measurement_record.storage[2];
        auto d = t.measurement_record.storage[3];
        auto e = t.measurement_record.storage[4];
        auto f = t.measurement_record.storage[5];
        auto g = t.measurement_record.storage[6];
        auto h = t.measurement_record.storage[7];
        ASSERT_EQ(a, d);
        ASSERT_EQ(b, c);
        ASSERT_NE(a, b);
        ASSERT_EQ(a, e ^ f);
        ASSERT_NE(e, g);
        ASSERT_NE(f, h);
    }
}

TEST(TableauSimulator, measure_pauli_product_noisy) {
    TableauSimulator t(SHARED_TEST_RNG());
    t.expand_do_circuit(R"CIRCUIT(
        H 0
        CNOT 0 1
        REPEAT 5000 {
            MPP(0.05) X0*X1 Z0*Z1
        }
    )CIRCUIT");
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.expand_do_circuit("MPP Y0*Y1");
    ASSERT_EQ(t.measurement_record.storage.back(), true);
}

TEST(TableauSimulator, ignores_sweep_controls) {
    TableauSimulator t(SHARED_TEST_RNG());
    t.expand_do_circuit(R"CIRCUIT(
        X 0
        CNOT sweep[0] 0
        M 0
    )CIRCUIT");
    ASSERT_EQ(t.measurement_record.lookback(1), true);
}

TEST(TableauSimulator, peek_observable_expectation) {
    TableauSimulator t(SHARED_TEST_RNG());
    t.expand_do_circuit(R"CIRCUIT(
        H 0
        CNOT 0 1
        X 0
    )CIRCUIT");

    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("XX")), 1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("YY")), 1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("ZZ")), -1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("-XX")), -1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("-ZZ")), 1);

    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("")), 1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("-I")), -1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("X")), 0);
    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("Z")), 0);
    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("Z_")), 0);
    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("ZZZ")), -1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("XXX")), 0);
    ASSERT_EQ(t.peek_observable_expectation(PauliString::from_str("ZZZZZZZZ")), -1);
}

TEST(TableauSimulator, postselect_x) {
    TableauSimulator sim(SHARED_TEST_RNG(), 2);

    // Postselect from +X.
    sim.reset_x(OpDat(0));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));

    // Postselect from -X.
    sim.reset_x(OpDat(0));
    sim.Z(OpDat(0));
    ASSERT_THROW({ sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false); }, std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-X"));

    // Postselect from +Y.
    sim.reset_y(OpDat(0));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));

    // Postselect from -Y.
    sim.reset_y(OpDat(0));
    sim.X(OpDat(0));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));

    // Postselect from +Z.
    sim.reset_z(OpDat(0));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));

    // Postselect from -Z.
    sim.reset_z(OpDat(0));
    sim.X(OpDat(0));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));

    // Postselect entangled.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(1)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+X"));

    // Postselect opposite state entangled.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-X"));

    // Postselect both independent.
    sim.reset_z(OpDat({0, 1}));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-X"));

    // Postselect both entangled.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-X"));

    // Contradiction reached during second postselection.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    sim.Z(OpDat(0));
    ASSERT_THROW(
        {
            sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
        },
        std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+X"));
}

TEST(TableauSimulator, postselect_y) {
    TableauSimulator sim(SHARED_TEST_RNG(), 2);

    // Postselect from +X.
    sim.reset_x(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Y"));

    // Postselect from -X.
    sim.reset_x(OpDat(0));
    sim.Z(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Y"));

    // Postselect from +Y.
    sim.reset_y(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Y"));

    // Postselect from -Y.
    sim.reset_y(OpDat(0));
    sim.X(OpDat(0));
    ASSERT_THROW({ sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false); }, std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-Y"));

    // Postselect from +Z.
    sim.reset_z(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Y"));

    // Postselect from -Z.
    sim.reset_z(OpDat(0));
    sim.X(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Y"));

    // Postselect entangled.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(1)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-Y"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+Y"));

    // Postselect opposite state entangled.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Y"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Y"));

    // Postselect both independent.
    sim.reset_z(OpDat({0, 1}));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-Y"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Y"));

    // Postselect both entangled.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    sim.Z(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-Y"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Y"));

    // Contradiction reached during second postselection.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    ASSERT_THROW(
        {
            sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
        },
        std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-Y"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+Y"));
}

TEST(TableauSimulator, postselect_z) {
    TableauSimulator sim(SHARED_TEST_RNG(), 2);

    // Postselect from +X.
    sim.reset_x(OpDat(0));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Z"));

    // Postselect from -X.
    sim.reset_x(OpDat(0));
    sim.Z(OpDat(0));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Z"));

    // Postselect from +Y.
    sim.reset_y(OpDat(0));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Z"));

    // Postselect from -Y.
    sim.reset_y(OpDat(0));
    sim.X(OpDat(0));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Z"));

    // Postselect from +Z.
    sim.reset_z(OpDat(0));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Z"));

    // Postselect from -Z.
    sim.reset_z(OpDat(0));
    sim.X(OpDat(0));
    ASSERT_THROW({ sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false); }, std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-Z"));

    // Postselect entangled.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(1)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+Z"));

    // Postselect opposite state entangled.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Z"));

    // Postselect both independent.
    sim.reset_x(OpDat({0, 1}));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Z"));

    // Postselect both entangled.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-Z"));

    // Contradiction reached during second postselection.
    sim.reset_z(OpDat({0, 1}));
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    sim.X(OpDat(0));
    ASSERT_THROW(
        {
            sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
        },
        std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+Z"));
}

TEST(TableauSimulator, peek_x) {
    TableauSimulator sim(SHARED_TEST_RNG(), 3);
    ASSERT_EQ(sim.peek_x(0), 0);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), +1);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), +1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), 0);
    ASSERT_EQ(sim.peek_z(2), +1);

    sim.H_XZ(OpDat(0));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), +1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), 0);
    ASSERT_EQ(sim.peek_z(2), +1);

    sim.X(OpDat(1));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), 0);
    ASSERT_EQ(sim.peek_z(2), +1);

    sim.H_YZ(OpDat(2));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), +1);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.X(OpDat(0));
    sim.X(OpDat(1));
    sim.X(OpDat(2));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), +1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), -1);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.Y(OpDat(0));
    sim.Y(OpDat(1));
    sim.Y(OpDat(2));
    ASSERT_EQ(sim.peek_x(0), -1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), -1);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.ZCZ(OpDat({0, 1}));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), -1);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.ZCZ(OpDat({1, 2}));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), +1);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.ZCZ(OpDat({0, 2}));
    ASSERT_EQ(sim.peek_x(0), 0);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), 0);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.X(OpDat(0));
    sim.X(OpDat(1));
    sim.X(OpDat(2));
    ASSERT_EQ(sim.peek_x(0), 0);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), +1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), 0);
    ASSERT_EQ(sim.peek_z(2), 0);
}

TEST(TableauSimulator, apply_tableau) {
    auto cnot = GATE_DATA.at("CNOT").tableau();
    auto s = GATE_DATA.at("S").tableau();
    auto h = GATE_DATA.at("H").tableau();
    auto cxyz = GATE_DATA.at("C_XYZ").tableau();

    TableauSimulator sim(SHARED_TEST_RNG(), 4);
    sim.apply_tableau(h, {1});
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+X"));
    sim.apply_tableau(s, {1});
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("+Y"));
    sim.apply_tableau(s, {1});
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-X"));
    sim.apply_tableau(cnot, {2, 1});
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("-X"));
    sim.apply_tableau(cnot, {1, 2});
    ASSERT_EQ(sim.peek_bloch(1), PauliString::from_str("I"));
    ASSERT_EQ(sim.peek_observable_expectation(PauliString::from_str("IXXI")), -1);
    ASSERT_EQ(sim.peek_observable_expectation(PauliString::from_str("IZZI")), +1);

    sim.apply_tableau(cxyz, {2});
    sim.apply_tableau(cxyz.inverse(), {1});
    ASSERT_EQ(sim.peek_observable_expectation(PauliString::from_str("IZYI")), -1);
    ASSERT_EQ(sim.peek_observable_expectation(PauliString::from_str("IYXI")), +1);
}
