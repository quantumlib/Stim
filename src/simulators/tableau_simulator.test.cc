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

#include "tableau_simulator.h"

#include <gtest/gtest.h>

#include "../circuit/circuit.test.h"
#include "../circuit/gate_data.h"
#include "../test_util.test.h"

using namespace stim_internal;

TEST(TableauSimulator, identity) {
    auto s = TableauSimulator(1, SHARED_TEST_RNG());
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{}));
    s.measure(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false}));
    s.measure(OpDat::flipped(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false, true}));
}

TEST(TableauSimulator, bit_flip) {
    auto s = TableauSimulator(1, SHARED_TEST_RNG());
    s.H_XZ(OpDat(0));
    s.SQRT_Z(OpDat(0));
    s.SQRT_Z(OpDat(0));
    s.H_XZ(OpDat(0));
    s.measure(OpDat(0));
    s.X(OpDat(0));
    s.measure(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{true, false}));
}

TEST(TableauSimulator, identity2) {
    auto s = TableauSimulator(2, SHARED_TEST_RNG());
    s.measure(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false}));
    s.measure(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false, false}));
}

TEST(TableauSimulator, bit_flip_2) {
    auto s = TableauSimulator(2, SHARED_TEST_RNG());
    s.H_XZ(OpDat(0));
    s.SQRT_Z(OpDat(0));
    s.SQRT_Z(OpDat(0));
    s.H_XZ(OpDat(0));
    s.measure(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{true}));
    s.measure(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{true, false}));
}

TEST(TableauSimulator, epr) {
    auto s = TableauSimulator(2, SHARED_TEST_RNG());
    s.H_XZ(OpDat(0));
    s.ZCX(OpDat({0, 1}));
    ASSERT_EQ(s.is_deterministic(0), false);
    ASSERT_EQ(s.is_deterministic(1), false);
    s.measure(OpDat(0));
    ASSERT_EQ(s.is_deterministic(0), true);
    ASSERT_EQ(s.is_deterministic(1), true);
    s.measure(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage[0], s.measurement_record.storage[1]);
}

TEST(TableauSimulator, big_determinism) {
    auto s = TableauSimulator(1000, SHARED_TEST_RNG());
    s.H_XZ(OpDat(0));
    ASSERT_FALSE(s.is_deterministic(0));
    for (size_t k = 1; k < 1000; k++) {
        ASSERT_TRUE(s.is_deterministic(k));
    }
}

TEST(TableauSimulator, phase_kickback_consume_s_state) {
    for (size_t k = 0; k < 8; k++) {
        auto s = TableauSimulator(2, SHARED_TEST_RNG());
        s.H_XZ(OpDat(1));
        s.SQRT_Z(OpDat(1));
        s.H_XZ(OpDat(0));
        s.ZCX(OpDat({0, 1}));
        ASSERT_EQ(s.is_deterministic(1), false);
        s.measure(OpDat(1));
        auto v1 = s.measurement_record.storage.back();
        if (v1) {
            s.SQRT_Z(OpDat(0));
            s.SQRT_Z(OpDat(0));
        }
        s.SQRT_Z(OpDat(0));
        s.H_XZ(OpDat(0));
        ASSERT_EQ(s.is_deterministic(0), true);
        s.measure(OpDat(0));
        ASSERT_EQ(s.measurement_record.storage.back(), true);
    }
}

TEST(TableauSimulator, phase_kickback_preserve_s_state) {
    auto s = TableauSimulator(2, SHARED_TEST_RNG());

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
    ASSERT_EQ(s.is_deterministic(0), true);
    s.measure(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage.back(), true);
    s.SQRT_Z(OpDat(1));
    s.H_XZ(OpDat(1));
    ASSERT_EQ(s.is_deterministic(1), true);
    s.measure(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage.back(), true);
}

TEST(TableauSimulator, kickback_vs_stabilizer) {
    auto sim = TableauSimulator(3, SHARED_TEST_RNG());
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
        auto sim = TableauSimulator(9, SHARED_TEST_RNG());

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
            ASSERT_EQ(sim.is_deterministic(anc), false);
            sim.measure(OpDat(anc));
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
            sim.measure(OpDat(k));
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
        ASSERT_EQ(sim.is_deterministic(7), true);
        sim.measure(OpDat(7));
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
        auto sim = TableauSimulator(5, SHARED_TEST_RNG());

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
            ASSERT_EQ(sim.is_deterministic(anc), false);
            sim.measure(OpDat(anc));
            bool v = sim.measurement_record.storage.back();
            if (v) {
                for (const auto &k : phasor) {
                    sim.X(OpDat(k));
                }
                sim.X(OpDat(anc));
            }
        }

        for (size_t k = 0; k < 3; k++) {
            ASSERT_EQ(sim.is_deterministic(k), true);
            sim.measure(OpDat(k));
            ASSERT_EQ(sim.measurement_record.storage.back(), false);
        }
        sim.SQRT_Z(OpDat(3));
        sim.H_XZ(OpDat(3));
        ASSERT_EQ(sim.is_deterministic(3), true);
        sim.measure(OpDat(3));
        ASSERT_EQ(sim.measurement_record.storage.back(), true);
    }
}

TEST(TableauSimulator, unitary_gates_consistent_with_tableau_data) {
    auto t = Tableau::random(10, SHARED_TEST_RNG());
    TableauSimulator sim(10, SHARED_TEST_RNG());
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
        ASSERT_EQ(sim.inv_state, t) << gate.name;
    }
}

TEST(TableauSimulator, certain_errors_consistent_with_gates) {
    TableauSimulator sim1(2, SHARED_TEST_RNG());
    TableauSimulator sim2(2, SHARED_TEST_RNG());
    uint32_t targets[] {0};
    OperationData d0{0, {targets, targets + 1}};
    OperationData d1{1.0, {targets, targets + 1}};

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
        Circuit::from_text("H 0\n"
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
        Circuit::from_text("X 0\n"
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
    TableauSimulator sim_tab(2, SHARED_TEST_RNG());
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

bool vec_sim_corroborates_measurement_process(const Tableau &state, const std::vector<uint32_t> &measurement_targets) {
    TableauSimulator sim_tab(2, SHARED_TEST_RNG());
    sim_tab.inv_state = state;
    auto vec_sim = sim_tab.to_vector_sim();
    sim_tab.measure(OpDat(measurement_targets));
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
    simd_bits expected(5);

    expected.clear();
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit::from_text(R"circuit(
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
            Circuit::from_text(R"circuit(
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
            Circuit::from_text(R"circuit(
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
            Circuit::from_text(R"circuit(
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
            Circuit::from_text(R"circuit(
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
            Circuit::from_text(R"circuit(
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
            Circuit::from_text(R"circuit(
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
            Circuit::from_text(R"circuit(
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
                Circuit::from_text(R"circuit(
            M 0
            CNOT 1 rec[-1]
        )circuit"),
                SHARED_TEST_RNG());
        },
        std::out_of_range);
    ASSERT_THROW(
        {
            TableauSimulator::sample_circuit(
                Circuit::from_text(R"circuit(
            M 0
            CY 1 rec[-1]
        )circuit"),
                SHARED_TEST_RNG());
        },
        std::out_of_range);
    ASSERT_THROW(
        {
            TableauSimulator::sample_circuit(
                Circuit::from_text(R"circuit(
            M 0
            YCZ rec[-1] 1
        )circuit"),
                SHARED_TEST_RNG());
        },
        std::out_of_range);
    ASSERT_THROW(
        {
            TableauSimulator::sample_circuit(
                Circuit::from_text(R"circuit(
            M 0
            XCZ rec[-1] 1
        )circuit"),
                SHARED_TEST_RNG());
        },
        std::out_of_range);
    ASSERT_THROW(
        {
            TableauSimulator::sample_circuit(
                Circuit::from_text(R"circuit(
            M 0
            SWAP 1 rec[-1]
        )circuit"),
                SHARED_TEST_RNG());
        },
        std::out_of_range);
}

TEST(TableauSimulator, classical_can_control_quantum) {
    simd_bits expected(5);
    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit::from_text(R"circuit(
        M !0
        CX rec[-1] 1
        M 1
    )circuit"),
            SHARED_TEST_RNG()),
        expected);
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit::from_text(R"circuit(
        M !0
        CY rec[-1] 1
        M 1
    )circuit"),
            SHARED_TEST_RNG()),
        expected);
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit::from_text(R"circuit(
        M !0
        XCZ 1 rec[-1]
        M 1
    )circuit"),
            SHARED_TEST_RNG()),
        expected);
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit::from_text(R"circuit(
        M !0
        YCZ 1 rec[-1]
        M 1
    )circuit"),
            SHARED_TEST_RNG()),
        expected);
}

TEST(TableauSimulator, classical_control_cases) {
    simd_bits expected(5);
    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator::sample_circuit(
            Circuit::from_text(R"circuit(
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
            Circuit::from_text(R"circuit(
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
            Circuit::from_text(R"circuit(
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
            Circuit::from_text(R"circuit(
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
    simd_bits expected(2);
    expected[0] = true;
    auto r = TableauSimulator::sample_circuit(Circuit::from_text("X 0\nMR 0 0"), SHARED_TEST_RNG());
    ASSERT_EQ(r, expected);
}

TEST(TableauSimulator, peek_bloch) {
    TableauSimulator sim(3, SHARED_TEST_RNG());
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
    TableauSimulator sim1(500, SHARED_TEST_RNG());
    TableauSimulator sim2(500, SHARED_TEST_RNG());
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
    TableauSimulator sim1(10, SHARED_TEST_RNG());
    TableauSimulator sim2(10, SHARED_TEST_RNG());
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
    TableauSimulator sim(10, SHARED_TEST_RNG());
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
    TableauSimulator sim(2, SHARED_TEST_RNG());
    sim.H_XZ(OpDat(0));
    sim.ZCX(OpDat({0, 1}));
    ASSERT_EQ(sim.canonical_stabilizers(), (std::vector<PauliString>{
        PauliString::from_str("XX"),
        PauliString::from_str("ZZ"),
    }));
    sim.SQRT_Y(OpDat({0, 1}));
    ASSERT_EQ(sim.canonical_stabilizers(), (std::vector<PauliString>{
        PauliString::from_str("XX"),
        PauliString::from_str("ZZ"),
    }));
    sim.SQRT_X(OpDat({0, 1}));
    ASSERT_EQ(sim.canonical_stabilizers(), (std::vector<PauliString>{
        PauliString::from_str("XX"),
        PauliString::from_str("-ZZ"),
    }));
    sim.set_num_qubits(3);
    ASSERT_EQ(sim.canonical_stabilizers(), (std::vector<PauliString>{
        PauliString::from_str("+XX_"),
        PauliString::from_str("-ZZ_"),
        PauliString::from_str("+__Z"),
    }));
    sim.ZCX(OpDat({2, 0}));
    ASSERT_EQ(sim.canonical_stabilizers(), (std::vector<PauliString>{
        PauliString::from_str("+XX_"),
        PauliString::from_str("-ZZ_"),
        PauliString::from_str("+__Z"),
    }));
}

TEST(TableauSimulator, canonical_stabilizers_random) {
    TableauSimulator sim(4, SHARED_TEST_RNG());
    sim.inv_state = Tableau::random(4, SHARED_TEST_RNG());
    auto s1 = sim.canonical_stabilizers();
    scramble_stabilizers(sim);
    auto s2 = sim.canonical_stabilizers();
    ASSERT_EQ(s1, s2);
}

TEST(TableauSimulator, set_num_qubits_reduce_preserves_scrambled_stabilizers) {
    auto &rng = SHARED_TEST_RNG();
    TableauSimulator sim(4, rng);
    sim.inv_state = Tableau::random(4, SHARED_TEST_RNG());
    auto s1 = sim.canonical_stabilizers();
    sim.inv_state.expand(8);
    scramble_stabilizers(sim);
    sim.set_num_qubits(4);
    auto s2 = sim.canonical_stabilizers();
    ASSERT_EQ(s1, s2);
}

TEST(TableauSimulator, measure_kickback) {
    TableauSimulator sim(4, SHARED_TEST_RNG());
    sim.H_XZ(OpDat({0, 2}));
    sim.ZCX(OpDat({0, 1, 2, 3}));
    auto k1 = sim.measure_kickback(1);
    auto k2 = sim.measure_kickback(2);
    auto k3 = sim.measure_kickback(3);
    ASSERT_EQ(k1.second, PauliString::from_str("XX__"));
    ASSERT_EQ(k2.second, PauliString::from_str("__XX"));
    ASSERT_EQ(k3.second, PauliString(0));
    ASSERT_EQ(k2.first, k3.first);
}

TEST(TableauSimulator, measure_kickback_isolates) {
    TableauSimulator sim(4, SHARED_TEST_RNG());
    sim.inv_state = Tableau::random(4, SHARED_TEST_RNG());
    for (size_t k = 0; k < 4; k++) {
        auto result = sim.measure_kickback(k);
        for (size_t j = 0; j < result.second.num_qubits && j < k; j++) {
            ASSERT_FALSE(result.second.xs[j]);
            ASSERT_FALSE(result.second.zs[j]);
        }
    }
}

TEST(TableauSimulator, collapse_isolate_completely) {
    for (size_t k = 0; k < 10; k++) {
        TableauSimulator sim(6, SHARED_TEST_RNG());
        sim.inv_state = Tableau::random(6, SHARED_TEST_RNG());
        {
            TableauTransposedRaii tmp(sim.inv_state);
            sim.collapse_isolate_qubit(2, tmp);
        }
        PauliString x2 = sim.inv_state.xs[2];
        PauliString z2 = sim.inv_state.zs[2];
        x2.sign = false;
        z2.sign = false;
        ASSERT_EQ(x2, PauliString::from_str("__X___"));
        ASSERT_EQ(z2, PauliString::from_str("__Z___"));
    }
}
