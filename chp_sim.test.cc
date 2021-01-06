#include "gtest/gtest.h"
#include "chp_sim.h"

TEST(ChpSim, identity) {
    auto s = ChpSim(1);
    ASSERT_EQ(s.measure(0), false);
}

TEST(ChpSim, bit_flip) {
    auto s = ChpSim(1);
    s.H(0);
    s.S(0);
    s.S(0);
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
    s.S(0);
    s.S(0);
    s.H(0);
    ASSERT_EQ(s.measure(0), true);
    ASSERT_EQ(s.measure(1), false);
}

TEST(ChpSim, epr) {
    auto s = ChpSim(2);
    s.H(0);
    s.CNOT(0, 1);
    ASSERT_EQ(s.is_deterministic(0), false);
    ASSERT_EQ(s.is_deterministic(1), false);
    auto v1 = s.measure(0);
    ASSERT_EQ(s.is_deterministic(0), true);
    ASSERT_EQ(s.is_deterministic(1), true);
    auto v2 = s.measure(1);
    ASSERT_EQ(v1, v2);
}

TEST(ChpSim, phase_kickback_consume_s_state) {
    for (size_t k = 0; k < 8; k++) {
        auto s = ChpSim(2);
        s.H(1);
        s.S(1);
        s.H(0);
        s.CNOT(0, 1);
        ASSERT_EQ(s.is_deterministic(1), false);
        auto v1 = s.measure(1);
        if (v1) {
            s.S(0);
            s.S(0);
        }
        s.S(0);
        s.H(0);
        ASSERT_EQ(s.is_deterministic(0), true);
        ASSERT_EQ(s.measure(0), true);
    }
}

TEST(ChpSim, phase_kickback_preserve_s_state) {
    auto s = ChpSim(2);

    // Prepare S state.
    s.H(1);
    s.S(1);

    // Prepare test input.
    s.H(0);

    // Kickback.
    s.CNOT(0, 1);
    s.H(1);
    s.CNOT(0, 1);
    s.H(1);

    // Check.
    s.S(0);
    s.H(0);
    ASSERT_EQ(s.is_deterministic(0), true);
    ASSERT_EQ(s.measure(0), true);
    s.S(1);
    s.H(1);
    ASSERT_EQ(s.is_deterministic(1), true);
    ASSERT_EQ(s.measure(1), true);
}

TEST(ChpSim, kickback_vs_stabilizer) {
    auto sim = ChpSim(3);
    sim.H(2);
    sim.CNOT(2, 0);
    sim.CNOT(2, 1);
    sim.S(0);
    sim.S(1);
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
                sim.CNOT(anc, k);
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
            sim.S(k);
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

        sim.S(7);
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
                sim.CNOT(anc, k);
            }
            sim.H(anc);
            sim.S(anc);
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
        sim.S(3);
        sim.H(3);
        ASSERT_EQ(sim.is_deterministic(3), true);
        ASSERT_EQ(sim.measure(3), true);
    }
}
