#include "gtest/gtest.h"
#include "chp_sim.h"

TEST(ChpSim, identity) {
    auto s = ChpSim(1);
    ASSERT_EQ(s.measure(0), false);
}

TEST(ChpSim, bit_flip) {
    auto s = ChpSim(1);
    s.H(0);
    s.SQRT_Z(0);
    s.SQRT_Z(0);
    s.H(0);
    ASSERT_EQ(s.measure(0), true);
    s.X(0);
    ASSERT_EQ(s.measure(0), false);
}

TEST(ChpSim, identity2) {
    auto s = ChpSim(2);
    ASSERT_EQ(s.measure(0), false);
    ASSERT_EQ(s.measure(1), false);
}

TEST(ChpSim, bit_flip_2) {
    auto s = ChpSim(2);
    s.H(0);
    s.SQRT_Z(0);
    s.SQRT_Z(0);
    s.H(0);
    ASSERT_EQ(s.measure(0), true);
    ASSERT_EQ(s.measure(1), false);
}

TEST(ChpSim, epr) {
    auto s = ChpSim(2);
    s.H(0);
    s.CX(0, 1);
    ASSERT_EQ(s.is_deterministic(0), false);
    ASSERT_EQ(s.is_deterministic(1), false);
    auto v1 = s.measure(0);
    ASSERT_EQ(s.is_deterministic(0), true);
    ASSERT_EQ(s.is_deterministic(1), true);
    auto v2 = s.measure(1);
    ASSERT_EQ(v1, v2);
}

TEST(ChpSim, big_determinism) {
    auto s = ChpSim(1000);
    s.H(0);
    ASSERT_FALSE(s.is_deterministic(0));
    for (size_t k = 1; k < 1000; k++) {
        ASSERT_TRUE(s.is_deterministic(k));
    }
}

TEST(ChpSim, phase_kickback_consume_s_state) {
    for (size_t k = 0; k < 8; k++) {
        auto s = ChpSim(2);
        s.H(1);
        s.SQRT_Z(1);
        s.H(0);
        s.CX(0, 1);
        ASSERT_EQ(s.is_deterministic(1), false);
        auto v1 = s.measure(1);
        if (v1) {
            s.SQRT_Z(0);
            s.SQRT_Z(0);
        }
        s.SQRT_Z(0);
        s.H(0);
        ASSERT_EQ(s.is_deterministic(0), true);
        ASSERT_EQ(s.measure(0), true);
    }
}

TEST(ChpSim, phase_kickback_preserve_s_state) {
    auto s = ChpSim(2);

    // Prepare S state.
    s.H(1);
    s.SQRT_Z(1);

    // Prepare test input.
    s.H(0);

    // Kickback.
    s.CX(0, 1);
    s.H(1);
    s.CX(0, 1);
    s.H(1);

    // Check.
    s.SQRT_Z(0);
    s.H(0);
    ASSERT_EQ(s.is_deterministic(0), true);
    ASSERT_EQ(s.measure(0), true);
    s.SQRT_Z(1);
    s.H(1);
    ASSERT_EQ(s.is_deterministic(1), true);
    ASSERT_EQ(s.measure(1), true);
}

TEST(ChpSim, kickback_vs_stabilizer) {
    auto sim = ChpSim(3);
    sim.H(2);
    sim.CX(2, 0);
    sim.CX(2, 1);
    sim.SQRT_Z(0);
    sim.SQRT_Z(1);
    sim.H(0);
    sim.H(1);
    sim.H(2);
    ASSERT_EQ(sim.inv_state.str(),
              "Tableau {\n"
              "  qubit 0_x: +Z_X\n"
              "  qubit 0_z: -Y_X\n"
              "  qubit 1_x: +_ZX\n"
              "  qubit 1_z: -_YX\n"
              "  qubit 2_x: +__X\n"
              "  qubit 2_z: +XXZ\n"
              "}");
}

TEST(ChpSim, s_state_distillation_low_depth) {
    for (size_t reps = 0; reps < 10; reps++) {
        auto sim = ChpSim(9);

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
            sim.H(anc);
            for (const auto &k : stabilizer) {
                sim.CX(anc, k);
            }
            sim.H(anc);
            ASSERT_EQ(sim.is_deterministic(anc), false);
            auto v = sim.measure(anc);
            if (v) {
                sim.X(anc);
            }
            stabilizer_measurements.push_back(v);
        }

        std::vector<bool> qubit_measurements;
        for (size_t k = 0; k < 7; k++) {
            sim.SQRT_Z(k);
            sim.H(k);
            qubit_measurements.push_back(sim.measure(k));
        }

        bool sum = false;
        for (auto e : stabilizer_measurements) {
            sum ^= e;
        }
        for (auto e : qubit_measurements) {
            sum ^= e;
        }
        if (sum) {
            sim.Z(7);
        }

        sim.SQRT_Z(7);
        sim.H(7);
        ASSERT_EQ(sim.is_deterministic(7), true);
        ASSERT_EQ(sim.measure(7), false);

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

TEST(ChpSim, s_state_distillation_low_space) {
    for (size_t rep = 0; rep < 10; rep++) {
        auto sim = ChpSim(5);

        std::vector<std::vector<uint8_t>> phasors = {
                {0,},
                {1,},
                {2,},
                {0, 1, 2},
                {0, 1, 3},
                {0, 2, 3},
                {1, 2, 3},
        };

        size_t anc = 4;
        for (const auto &phasor : phasors) {
            sim.H(anc);
            for (const auto &k : phasor) {
                sim.CX(anc, k);
            }
            sim.H(anc);
            sim.SQRT_Z(anc);
            sim.H(anc);
            ASSERT_EQ(sim.is_deterministic(anc), false);
            auto v = sim.measure(anc);
            if (v) {
                for (const auto &k : phasor) {
                    sim.X(k);
                }
                sim.X(anc);
            }
        }

        for (size_t k = 0; k < 3; k++) {
            ASSERT_EQ(sim.is_deterministic(k), true);
            ASSERT_EQ(sim.measure(k), false);
        }
        sim.SQRT_Z(3);
        sim.H(3);
        ASSERT_EQ(sim.is_deterministic(3), true);
        ASSERT_EQ(sim.measure(3), true);
    }
}

TEST(ChpSim, single_qubit_gates_consistent_with_tableau_data) {
    auto t = Tableau::random(10);
    ChpSim sim(10);
    ChpSim sim2(10);
    sim.inv_state = t;
    sim2.inv_state = t;
    for (const auto &kv : SINGLE_QUBIT_GATE_FUNCS) {
        const auto &name = kv.first;
        if (name == "M" || name == "R") {
            continue;
        }
        const auto &action = kv.second;
        const auto &inverse_op_tableau = GATE_TABLEAUS.at(GATE_INVERSE_NAMES.at(name));
        action(sim, 5);
        sim2.op(name, {5});
        t.inplace_scatter_prepend(inverse_op_tableau, {5});
        ASSERT_EQ(sim.inv_state, t) << name;
        ASSERT_EQ(sim.inv_state, sim2.inv_state) << name;
    }
    for (const auto &kv : TWO_QUBIT_GATE_FUNCS) {
        const auto &name = kv.first;
        const auto &action = kv.second;
        const auto &inverse_op_tableau = GATE_TABLEAUS.at(GATE_INVERSE_NAMES.at(name));
        action(sim, 7, 4);
        sim2.op(name, {7, 4});
        t.inplace_scatter_prepend(inverse_op_tableau, {7, 4});
        ASSERT_EQ(sim.inv_state, t) << name;
        ASSERT_EQ(sim.inv_state, sim2.inv_state) << name;
    }
}

TEST(ChpSim, simulate) {
    auto results = ChpSim::simulate(Circuit::from_text(
            "H 0\n"
            "CNOT 0 1\n"
            "M 0\n"
            "M 1\n"
            "M 2\n"));
    ASSERT_EQ(results[0], results[1]);
    ASSERT_EQ(results[2], false);
}

TEST(ChpSim, simulate_reset) {
    auto results = ChpSim::simulate(Circuit::from_text(
            "X 0\n"
            "M 0\n"
            "R 0\n"
            "M 0\n"
            "R 0\n"
            "M 0\n"));
    ASSERT_EQ(results[0], true);
    ASSERT_EQ(results[1], false);
    ASSERT_EQ(results[2], false);
}

TEST(ChpSim, to_vector_sim) {
    ChpSim chp_sim(2);
    VectorSim vec_sim(2);
    ASSERT_TRUE(chp_sim.to_vector_sim().approximate_equals(vec_sim, true));

    chp_sim.X(0);
    vec_sim.apply("X", 0);
    ASSERT_TRUE(chp_sim.to_vector_sim().approximate_equals(vec_sim, true));

    chp_sim.H(0);
    vec_sim.apply("H", 0);
    ASSERT_TRUE(chp_sim.to_vector_sim().approximate_equals(vec_sim, true));

    chp_sim.SQRT_Z(0);
    vec_sim.apply("SQRT_Z", 0);
    ASSERT_TRUE(chp_sim.to_vector_sim().approximate_equals(vec_sim, true));

    chp_sim.CX(0, 1);
    vec_sim.apply("CX", 0, 1);
    ASSERT_TRUE(chp_sim.to_vector_sim().approximate_equals(vec_sim, true));

    chp_sim.inv_state = Tableau::random(10);
    vec_sim = chp_sim.to_vector_sim();
    ASSERT_TRUE(chp_sim.to_vector_sim().approximate_equals(vec_sim, true));

    chp_sim.op("XCX", {4, 7});
    vec_sim.apply("XCX", 4, 7);
    ASSERT_TRUE(chp_sim.to_vector_sim().approximate_equals(vec_sim, true));
}

bool vec_sim_corroborates_measurement_process(const ChpSim &sim, std::vector<size_t> measurement_targets) {
    ChpSim chp_sim = sim;
    auto vec_sim = chp_sim.to_vector_sim();
    auto results = chp_sim.measure_many(measurement_targets);
    PauliStringVal buf(chp_sim.inv_state.num_qubits);
    auto p = buf.ptr();
    for (size_t k = 0; k < measurement_targets.size(); k++) {
        p.set_z_bit(measurement_targets[k], true);
        p.bit_ptr_sign.set(results[k]);
        float f = vec_sim.project(p);
        if (fabsf(f - 0.5) > 1e-4 && fabsf(f - 1) > 1e-4) {
            return false;
        }
        p.set_z_bit(measurement_targets[k], false);
    }
    return true;
}

TEST(ChpSim, measurement_vs_vector_sim) {
    for (size_t k = 0; k < 10; k++) {
        ChpSim chp_sim(2);
        chp_sim.inv_state = Tableau::random(2);
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(chp_sim, {0}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(chp_sim, {1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(chp_sim, {0, 1}));
    }
    for (size_t k = 0; k < 10; k++) {
        ChpSim chp_sim(4);
        chp_sim.inv_state = Tableau::random(4);
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(chp_sim, {0, 1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(chp_sim, {2, 1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(chp_sim, {0, 1, 2, 3}));
    }
    {
        ChpSim chp_sim(12);
        chp_sim.inv_state = Tableau::random(12);
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(chp_sim, {0, 1, 2, 3}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(chp_sim, {0, 10, 11}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(chp_sim, {11, 5, 7}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(chp_sim, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
    }
}
