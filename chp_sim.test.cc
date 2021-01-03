#include "gtest/gtest.h"
#include "chp_sim.h"

TEST(ChpSim, identity) {
    auto s = ChpSim(1);
    ASSERT_EQ(s.measure(0), false);
}

TEST(ChpSim, bit_flip) {
    auto s = ChpSim(1);
    s.hadamard(0);
    s.phase(0);
    s.phase(0);
    s.hadamard(0);
    ASSERT_EQ(s.measure(0), true);
}

TEST(ChpSim, identity2) {
    auto s = ChpSim(2);
    ASSERT_EQ(s.measure(0), false);
    ASSERT_EQ(s.measure(1), false);
}

TEST(ChpSim, bit_flip_2) {
    auto s = ChpSim(2);
    s.hadamard(0);
    s.phase(0);
    s.phase(0);
    s.hadamard(0);
    ASSERT_EQ(s.measure(0), true);
    ASSERT_EQ(s.measure(1), false);
}

TEST(ChpSim, epr) {
    auto s = ChpSim(2);
    s.hadamard(0);
    s.cnot(0, 1);
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
        s.hadamard(1);
        s.phase(1);
        s.hadamard(0);
        s.cnot(0, 1);
        ASSERT_EQ(s.is_deterministic(1), false);
        auto v1 = s.measure(1);
        if (v1) {
            s.phase(0);
            s.phase(0);
        }
        s.phase(0);
        s.hadamard(0);
        ASSERT_EQ(s.is_deterministic(0), true);
        ASSERT_EQ(s.measure(0), true);
    }
}

TEST(ChpSim, phase_kickback_preserve_s_state) {
    auto s = ChpSim(2);

    // Prepare S state.
    s.hadamard(1);
    s.phase(1);

    // Prepare test input.
    s.hadamard(0);

    // Kickback.
    s.cnot(0, 1);
    s.hadamard(1);
    s.cnot(0, 1);
    s.hadamard(1);

    // Check.
    s.phase(0);
    s.hadamard(0);
    ASSERT_EQ(s.is_deterministic(0), true);
    ASSERT_EQ(s.measure(0), true);
    s.phase(1);
    s.hadamard(1);
    ASSERT_EQ(s.is_deterministic(1), true);
    ASSERT_EQ(s.measure(1), true);
}

TEST(ChpSim, kickback_vs_stabilizer) {
    auto sim = ChpSim(3);
    sim.hadamard(2);
    sim.cnot(2, 0);
    sim.cnot(2, 1);
    sim.phase(0);
    sim.phase(1);
    sim.hadamard(0);
    sim.hadamard(1);
    sim.hadamard(2);
    ASSERT_EQ(sim.inv_state.str(),
              "Tableau {\n"
              "  qubit 0_x: +Z_X\n"
              "  qubit 0_y: -X__\n"
              "  qubit 1_x: +_ZX\n"
              "  qubit 1_y: -_X_\n"
              "  qubit 2_x: +__X\n"
              "  qubit 2_y: +XXY\n"
              "}");
}

TEST(ChpSim, s_state_distillation_low_depth) {
    for (size_t reps = 0; reps < 10; reps++) {
        std::cerr << reps << " rep\n";
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
            sim.hadamard(anc);
            for (const auto &k : stabilizer) {
                sim.cnot(anc, k);
            }
            sim.hadamard(anc);
            ASSERT_EQ(sim.is_deterministic(anc), false);
            auto v = sim.measure(anc);
            if (v) {
                sim.hadamard(anc);
                sim.phase(anc);
                sim.phase(anc);
                sim.hadamard(anc);
            }
            stabilizer_measurements.push_back(v);
        }

        std::vector<bool> qubit_measurements;
        for (size_t k = 0; k < 7; k++) {
            sim.phase(k);
            sim.hadamard(k);
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
            sim.phase(7);
            sim.phase(7);
        }

        sim.phase(7);
        sim.hadamard(7);
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

/*
TEST(ChpSim, s_state_distillation_low_space) {
    for xx in range(10):
        sim = ChpSim(5)

        phasors = [
            (0,),
            (1,),
            (2,),
            (0, 1, 2),
            (0, 1, 3),
            (0, 2, 3),
            (1, 2, 3),
        ]

        anc = 4
        for phasor in phasors:
            sim.hadamard(anc)
            for k in phasor:
                sim.cnot(anc, k)
            sim.hadamard(anc)
            sim.phase(anc)
            sim.hadamard(anc)
            v = sim.measure(anc)
            ASSERT_EQ(not v.determined
            if v.value:
                for k in phasor + (anc,):
                    sim.hadamard(k)
                    sim.phase(k)
                    sim.phase(k)
                    sim.hadamard(k)

        for k in range(3):
            ASSERT_EQ(sim.measure(k) == MeasureResult(value=False, determined=True)
        sim.phase(3)
        sim.hadamard(3)
        ASSERT_EQ(sim.measure(3) == MeasureResult(value=True, determined=True)


TEST(ChpSim, count_s_state_distillation_failure_cases) {
    def distill(errors: Set[int]) -> str:
        sim = ChpSim(5)

        phasors = [
            (0,),
            (1,),
            (2,),
            (0, 1, 2),
            (0, 1, 3),
            (0, 2, 3),
            (1, 2, 3),
        ]

        anc = 4
        for e, phasor in enumerate(phasors):
            for k in phasor:
                sim.hadamard(anc)
                sim.cnot(anc, k)
                sim.hadamard(anc)
            sim.phase(anc)

            if e in errors:
                sim.phase(anc)
                sim.phase(anc)

            sim.hadamard(anc)
            v = sim.measure(anc)
            if v.value:
                sim.hadamard(anc)
                sim.phase(anc)
                sim.phase(anc)
                sim.hadamard(anc)
            ASSERT_EQ(not v.determined
            if v.value:
                for k in phasor:
                    sim.hadamard(k)
                    sim.phase(k)
                    sim.phase(k)
                    sim.hadamard(k)

        sim.phase(3)
        sim.phase(3)
        sim.phase(3)
        sim.hadamard(3)
        result = sim.measure(3)
        sim.hadamard(3)
        sim.phase(3)
        checks = [sim.measure(k) for k in range(3)]
        ASSERT_EQ(result.determined
        ASSERT_EQ(all(e.determined for e in checks)
        good_result = result.value is False
        checks_passed = not any(e.value for e in checks)
        if checks_passed:
            if good_result:
                return 'good'
            else:
                return 'ERROR'
        else:
            if good_result:
                return 'victim'
            else:
                return 'caught'

    def classify(errs) -> collections.Counter:
        result = collections.Counter()
        for err in errs:
            result[distill(err)] += 1
        return result

    nones = list(itertools.combinations(range(7), 0))
    singles = list(itertools.combinations(range(7), 1))
    doubles = list(itertools.combinations(range(7), 2))
    triples = list(itertools.combinations(range(7), 3))

    ASSERT_EQ(classify(nones) == {'good': 1}
    ASSERT_EQ(classify(singles) == {'caught': 3, 'victim': 4}
    ASSERT_EQ(classify(doubles) == {'caught': 12, 'victim': 9}
    ASSERT_EQ(classify(triples) == {'caught': 12, 'victim': 16, 'ERROR': 7}

 */