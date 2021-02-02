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

#include "../simulators/gate_data.h"
#include "../test_util.test.h"

TEST(SimTableau, identity) {
    auto s = TableauSimulator(1, SHARED_TEST_RNG());
    ASSERT_EQ(s.recorded_measurement_results.size(), 0);
    s.measure(0);
    ASSERT_EQ(s.recorded_measurement_results.size(), 1);
    ASSERT_EQ(s.recorded_measurement_results.front(), false);
    s.recorded_measurement_results.pop();
    ASSERT_EQ(s.recorded_measurement_results.size(), 0);
    s.measure(OperationData({0}, {true}, 0));
    ASSERT_EQ(s.recorded_measurement_results.size(), 1);
    ASSERT_EQ(s.recorded_measurement_results.front(), true);
}

TEST(SimTableau, bit_flip) {
    auto s = TableauSimulator(1, SHARED_TEST_RNG());
    s.H_XZ(0);
    s.SQRT_Z(0);
    s.SQRT_Z(0);
    s.H_XZ(0);
    s.measure(0);
    s.X(0);
    s.measure(0);
    ASSERT_EQ(s.recorded_measurement_results.front(), true);
    s.recorded_measurement_results.pop();
    ASSERT_EQ(s.recorded_measurement_results.front(), false);
}

TEST(SimTableau, identity2) {
    auto s = TableauSimulator(2, SHARED_TEST_RNG());
    s.measure(0);
    ASSERT_EQ(s.recorded_measurement_results.front(), false);
    s.recorded_measurement_results.pop();
    s.measure({1});
    ASSERT_EQ(s.recorded_measurement_results.front(), false);
    s.recorded_measurement_results.pop();
}

TEST(SimTableau, bit_flip_2) {
    auto s = TableauSimulator(2, SHARED_TEST_RNG());
    s.H_XZ(0);
    s.SQRT_Z(0);
    s.SQRT_Z(0);
    s.H_XZ(0);
    s.measure(0);
    ASSERT_EQ(s.recorded_measurement_results.front(), true);
    s.recorded_measurement_results.pop();
    s.measure(1);
    ASSERT_EQ(s.recorded_measurement_results.front(), false);
    s.recorded_measurement_results.pop();
}

TEST(SimTableau, epr) {
    auto s = TableauSimulator(2, SHARED_TEST_RNG());
    s.H_XZ(0);
    s.ZCX({0, 1});
    ASSERT_EQ(s.is_deterministic(0), false);
    ASSERT_EQ(s.is_deterministic(1), false);
    s.measure(0);
    auto v1 = s.recorded_measurement_results.front();
    s.recorded_measurement_results.pop();
    ASSERT_EQ(s.is_deterministic(0), true);
    ASSERT_EQ(s.is_deterministic(1), true);
    s.measure(1);
    auto v2 = s.recorded_measurement_results.front();
    s.recorded_measurement_results.pop();
    ASSERT_EQ(v1, v2);
}

TEST(SimTableau, big_determinism) {
    auto s = TableauSimulator(1000, SHARED_TEST_RNG());
    s.H_XZ(0);
    ASSERT_FALSE(s.is_deterministic(0));
    for (size_t k = 1; k < 1000; k++) {
        ASSERT_TRUE(s.is_deterministic(k));
    }
}

TEST(SimTableau, phase_kickback_consume_s_state) {
    for (size_t k = 0; k < 8; k++) {
        auto s = TableauSimulator(2, SHARED_TEST_RNG());
        s.H_XZ(1);
        s.SQRT_Z(1);
        s.H_XZ(0);
        s.ZCX({0, 1});
        ASSERT_EQ(s.is_deterministic(1), false);
        s.measure(1);
        auto v1 = s.recorded_measurement_results.front();
        s.recorded_measurement_results.pop();
        if (v1) {
            s.SQRT_Z(0);
            s.SQRT_Z(0);
        }
        s.SQRT_Z(0);
        s.H_XZ(0);
        ASSERT_EQ(s.is_deterministic(0), true);
        s.measure(0);
        ASSERT_EQ(s.recorded_measurement_results.front(), true);
        s.recorded_measurement_results.pop();
    }
}

TEST(SimTableau, phase_kickback_preserve_s_state) {
    auto s = TableauSimulator(2, SHARED_TEST_RNG());

    // Prepare S state.
    s.H_XZ(1);
    s.SQRT_Z(1);

    // Prepare test input.
    s.H_XZ(0);

    // Kickback.
    s.ZCX({0, 1});
    s.H_XZ(1);
    s.ZCX({0, 1});
    s.H_XZ(1);

    // Check.
    s.SQRT_Z(0);
    s.H_XZ(0);
    ASSERT_EQ(s.is_deterministic(0), true);
    s.measure(0);
    ASSERT_EQ(s.recorded_measurement_results.front(), true);
    s.recorded_measurement_results.pop();
    s.SQRT_Z(1);
    s.H_XZ(1);
    ASSERT_EQ(s.is_deterministic(1), true);
    s.measure(1);
    ASSERT_EQ(s.recorded_measurement_results.front(), true);
    s.recorded_measurement_results.pop();
}

TEST(SimTableau, kickback_vs_stabilizer) {
    auto sim = TableauSimulator(3, SHARED_TEST_RNG());
    sim.H_XZ(2);
    sim.ZCX({2, 0});
    sim.ZCX({2, 1});
    sim.SQRT_Z(0);
    sim.SQRT_Z(1);
    sim.H_XZ(0);
    sim.H_XZ(1);
    sim.H_XZ(2);
    ASSERT_EQ(
        sim.inv_state.str(),
        "+-xz-xz-xz-\n"
        "| +- +- ++\n"
        "| ZY __ _X\n"
        "| __ ZY _X\n"
        "| XX XX XZ");
}

TEST(SimTableau, s_state_distillation_low_depth) {
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
        size_t anc = 8;
        for (const auto &stabilizer : stabilizers) {
            sim.H_XZ({anc});
            for (const auto &k : stabilizer) {
                sim.ZCX({anc, k});
            }
            sim.H_XZ({anc});
            ASSERT_EQ(sim.is_deterministic(anc), false);
            sim.measure({anc});
            bool v = sim.recorded_measurement_results.front();
            sim.recorded_measurement_results.pop();
            if (v) {
                sim.X({anc});
            }
            stabilizer_measurements.push_back(v);
        }

        std::vector<bool> qubit_measurements;
        for (size_t k = 0; k < 7; k++) {
            sim.SQRT_Z({k});
            sim.H_XZ({k});
            sim.measure({k});
            qubit_measurements.push_back(sim.recorded_measurement_results.front());
            sim.recorded_measurement_results.pop();
        }

        bool sum = false;
        for (auto e : stabilizer_measurements) {
            sum ^= e;
        }
        for (auto e : qubit_measurements) {
            sum ^= e;
        }
        if (sum) {
            sim.Z({7});
        }

        sim.SQRT_Z({7});
        sim.H_XZ({7});
        ASSERT_EQ(sim.is_deterministic(7), true);
        sim.measure({7});
        ASSERT_EQ(sim.recorded_measurement_results.front(), false);
        sim.recorded_measurement_results.pop();

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

TEST(SimTableau, s_state_distillation_low_space) {
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

        size_t anc = 4;
        for (const auto &phasor : phasors) {
            sim.H_XZ({anc});
            for (const auto &k : phasor) {
                sim.ZCX({anc, k});
            }
            sim.H_XZ({anc});
            sim.SQRT_Z({anc});
            sim.H_XZ({anc});
            ASSERT_EQ(sim.is_deterministic(anc), false);
            sim.measure({anc});
            bool v = sim.recorded_measurement_results.front();
            sim.recorded_measurement_results.pop();
            if (v) {
                for (const auto &k : phasor) {
                    sim.X({k});
                }
                sim.X({anc});
            }
        }

        for (size_t k = 0; k < 3; k++) {
            ASSERT_EQ(sim.is_deterministic(k), true);
            sim.measure({k});
            ASSERT_EQ(sim.recorded_measurement_results.front(), false);
            sim.recorded_measurement_results.pop();
        }
        sim.SQRT_Z({3});
        sim.H_XZ({3});
        ASSERT_EQ(sim.is_deterministic(3), true);
        sim.measure({3});
        ASSERT_EQ(sim.recorded_measurement_results.front(), true);
        sim.recorded_measurement_results.pop();
    }
}

TEST(SimTableau, unitary_gates_consistent_with_tableau_data) {
    auto t = Tableau::random(10, SHARED_TEST_RNG());
    TableauSimulator sim(10, SHARED_TEST_RNG());
    TableauSimulator sim2(10, SHARED_TEST_RNG());
    sim.inv_state = t;
    sim2.inv_state = t;
    for (const auto &kv : GATE_INVERSE_NAMES) {
        const auto &name = kv.first;
        const auto &action = SIM_TABLEAU_GATE_FUNC_DATA.at(name);
        const auto &inverse_op_tableau = Tableau::named_gate(GATE_INVERSE_NAMES.at(name));
        if (inverse_op_tableau.num_qubits == 2) {
            action(sim, {7, 4});
            sim2.apply(Tableau::named_gate(name), {7, 4});
            t.inplace_scatter_prepend(inverse_op_tableau, {7, 4});
        } else {
            action(sim, {5});
            sim2.apply(Tableau::named_gate(name), {5});
            t.inplace_scatter_prepend(inverse_op_tableau, {5});
        }
        ASSERT_EQ(sim.inv_state, t) << name;
        ASSERT_EQ(sim.inv_state, sim2.inv_state) << name;
    }
}

TEST(SimTableau, simulate) {
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

TEST(SimTableau, simulate_reset) {
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

TEST(SimTableau, to_vector_sim) {
    TableauSimulator sim_tab(2, SHARED_TEST_RNG());
    VectorSimulator sim_vec(2);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.X(0);
    sim_vec.apply("X", 0);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.H_XZ(0);
    sim_vec.apply("H_XZ", 0);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.SQRT_Z(0);
    sim_vec.apply("SQRT_Z", 0);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.ZCX({0, 1});
    sim_vec.apply("ZCX", 0, 1);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.inv_state = Tableau::random(10, SHARED_TEST_RNG());
    sim_vec = sim_tab.to_vector_sim();
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.apply("XCX", {4, 7});
    sim_vec.apply("XCX", 4, 7);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));
}

bool vec_sim_corroborates_measurement_process(const TableauSimulator &sim, std::vector<size_t> measurement_targets) {
    TableauSimulator sim_tab = sim;
    auto vec_sim = sim_tab.to_vector_sim();
    sim_tab.measure(measurement_targets);
    PauliString buf(sim_tab.inv_state.num_qubits);
    for (size_t k = 0; k < measurement_targets.size(); k++) {
        buf.zs[measurement_targets[k]] = true;
        buf.sign = sim_tab.recorded_measurement_results.front();
        sim_tab.recorded_measurement_results.pop();
        float f = vec_sim.project(buf);
        if (fabsf(f - 0.5) > 1e-4 && fabsf(f - 1) > 1e-4) {
            return false;
        }
        buf.zs[measurement_targets[k]] = false;
    }
    return true;
}

TEST(SimTableau, measurement_vs_vector_sim) {
    for (size_t k = 0; k < 10; k++) {
        TableauSimulator sim_tab(2, SHARED_TEST_RNG());
        sim_tab.inv_state = Tableau::random(2, SHARED_TEST_RNG());
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(sim_tab, {0}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(sim_tab, {1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(sim_tab, {0, 1}));
    }
    for (size_t k = 0; k < 10; k++) {
        TableauSimulator sim_tab(4, SHARED_TEST_RNG());
        sim_tab.inv_state = Tableau::random(4, SHARED_TEST_RNG());
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(sim_tab, {0, 1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(sim_tab, {2, 1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(sim_tab, {0, 1, 2, 3}));
    }
    {
        TableauSimulator sim_tab(12, SHARED_TEST_RNG());
        sim_tab.inv_state = Tableau::random(12, SHARED_TEST_RNG());
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(sim_tab, {0, 1, 2, 3}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(sim_tab, {0, 10, 11}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(sim_tab, {11, 5, 7}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(sim_tab, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
    }
}
