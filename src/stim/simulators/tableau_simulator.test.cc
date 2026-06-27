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
#include "stim/mem/simd_word.test.h"
#include "stim/simulators/vector_simulator.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

static std::vector<GateTarget> qubit_targets(const std::vector<uint32_t> &targets) {
    std::vector<GateTarget> result;
    for (uint32_t t : targets) {
        result.push_back(GateTarget::qubit(t & ~TARGET_INVERTED_BIT, t & TARGET_INVERTED_BIT));
    }
    return result;
}
struct OpDat {
    std::vector<GateTarget> targets;
    OpDat(uint32_t u) : targets(qubit_targets({u})) {
    }
    OpDat(std::vector<uint32_t> u) : targets(qubit_targets(u)) {
    }
    operator CircuitInstruction() const {
        return {(GateType)0, {}, targets, ""};
    }
};

TEST_EACH_WORD_SIZE_W(TableauSimulator, identity, {
    auto s = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 1);
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{}));
    s.do_MZ({GateType::Z, {}, qubit_targets({0}), ""});
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false}));
    s.do_MZ({GateType::Z, {}, qubit_targets({0 | TARGET_INVERTED_BIT}), ""});
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false, true}));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, bit_flip, {
    auto s = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 1);
    s.do_H_XZ(OpDat(0));
    s.do_SQRT_Z(OpDat(0));
    s.do_SQRT_Z(OpDat(0));
    s.do_H_XZ(OpDat(0));
    s.do_MZ(OpDat(0));
    s.do_X(OpDat(0));
    s.do_MZ(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{true, false}));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, identity2, {
    auto s = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 2);
    s.do_MZ(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false}));
    s.do_MZ(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{false, false}));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, bit_flip_2, {
    auto s = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 2);
    s.do_H_XZ(OpDat(0));
    s.do_SQRT_Z(OpDat(0));
    s.do_SQRT_Z(OpDat(0));
    s.do_H_XZ(OpDat(0));
    s.do_MZ(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{true}));
    s.do_MZ(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage, (std::vector<bool>{true, false}));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, epr, {
    auto s = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 2);
    s.do_H_XZ(OpDat(0));
    s.do_ZCX(OpDat({0, 1}));
    ASSERT_EQ(s.is_deterministic_z(0), false);
    ASSERT_EQ(s.is_deterministic_z(1), false);
    s.do_MZ(OpDat(0));
    ASSERT_EQ(s.is_deterministic_z(0), true);
    ASSERT_EQ(s.is_deterministic_z(1), true);
    s.do_MZ(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage[0], s.measurement_record.storage[1]);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, big_determinism, {
    auto s = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 1000);
    s.do_H_XZ(OpDat(0));
    s.do_H_YZ(OpDat(1));
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
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, phase_kickback_consume_s_state, {
    for (size_t k = 0; k < 8; k++) {
        auto s = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 2);
        s.do_H_XZ(OpDat(1));
        s.do_SQRT_Z(OpDat(1));
        s.do_H_XZ(OpDat(0));
        s.do_ZCX(OpDat({0, 1}));
        ASSERT_EQ(s.is_deterministic_z(1), false);
        s.do_MZ(OpDat(1));
        auto v1 = s.measurement_record.storage.back();
        if (v1) {
            s.do_SQRT_Z(OpDat(0));
            s.do_SQRT_Z(OpDat(0));
        }
        s.do_SQRT_Z(OpDat(0));
        s.do_H_XZ(OpDat(0));
        ASSERT_EQ(s.is_deterministic_z(0), true);
        s.do_MZ(OpDat(0));
        ASSERT_EQ(s.measurement_record.storage.back(), true);
    }
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, phase_kickback_preserve_s_state, {
    auto s = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 2);

    // Prepare S state.
    s.do_H_XZ(OpDat(1));
    s.do_SQRT_Z(OpDat(1));

    // Prepare test input.
    s.do_H_XZ(OpDat(0));

    // Kickback.
    s.do_ZCX(OpDat({0, 1}));
    s.do_H_XZ(OpDat(1));
    s.do_ZCX(OpDat({0, 1}));
    s.do_H_XZ(OpDat(1));

    // Check.
    s.do_SQRT_Z(OpDat(0));
    s.do_H_XZ(OpDat(0));
    ASSERT_EQ(s.is_deterministic_z(0), true);
    s.do_MZ(OpDat(0));
    ASSERT_EQ(s.measurement_record.storage.back(), true);
    s.do_SQRT_Z(OpDat(1));
    s.do_H_XZ(OpDat(1));
    ASSERT_EQ(s.is_deterministic_z(1), true);
    s.do_MZ(OpDat(1));
    ASSERT_EQ(s.measurement_record.storage.back(), true);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, kickback_vs_stabilizer, {
    auto sim = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 3);
    sim.do_H_XZ(OpDat(2));
    sim.do_ZCX(OpDat({2, 0}));
    sim.do_ZCX(OpDat({2, 1}));
    sim.do_SQRT_Z(OpDat(0));
    sim.do_SQRT_Z(OpDat(1));
    sim.do_H_XZ(OpDat(0));
    sim.do_H_XZ(OpDat(1));
    sim.do_H_XZ(OpDat(2));
    ASSERT_EQ(
        sim.inv_state.str(),
        "+-xz-xz-xz-\n"
        "| +- +- ++\n"
        "| ZY __ _X\n"
        "| __ ZY _X\n"
        "| XX XX XZ");
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, s_state_distillation_low_depth, {
    for (size_t reps = 0; reps < 10; reps++) {
        auto sim = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 9);

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
            sim.do_H_XZ(OpDat(anc));
            for (const auto &k : stabilizer) {
                sim.do_ZCX(OpDat({anc, k}));
            }
            sim.do_H_XZ(OpDat(anc));
            ASSERT_EQ(sim.is_deterministic_z(anc), false);
            sim.do_MZ(OpDat(anc));
            bool v = sim.measurement_record.storage.back();
            if (v) {
                sim.do_X(OpDat(anc));
            }
            stabilizer_measurements.push_back(v);
        }

        std::vector<bool> qubit_measurements;
        for (size_t k = 0; k < 7; k++) {
            sim.do_SQRT_Z(OpDat(k));
            sim.do_H_XZ(OpDat(k));
            sim.do_MZ(OpDat(k));
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
            sim.do_Z(OpDat(7));
        }

        sim.do_SQRT_Z(OpDat(7));
        sim.do_H_XZ(OpDat(7));
        ASSERT_EQ(sim.is_deterministic_z(7), true);
        sim.do_MZ(OpDat(7));
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
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, s_state_distillation_low_space, {
    for (size_t rep = 0; rep < 10; rep++) {
        auto sim = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 5);

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
            sim.do_H_XZ(OpDat(anc));
            for (const auto &k : phasor) {
                sim.do_ZCX(OpDat({anc, k}));
            }
            sim.do_H_XZ(OpDat(anc));
            sim.do_SQRT_Z(OpDat(anc));
            sim.do_H_XZ(OpDat(anc));
            ASSERT_EQ(sim.is_deterministic_z(anc), false);
            sim.do_MZ(OpDat(anc));
            bool v = sim.measurement_record.storage.back();
            if (v) {
                for (const auto &k : phasor) {
                    sim.do_X(OpDat(k));
                }
                sim.do_X(OpDat(anc));
            }
        }

        for (size_t k = 0; k < 3; k++) {
            ASSERT_EQ(sim.is_deterministic_z(k), true);
            sim.do_MZ(OpDat(k));
            ASSERT_EQ(sim.measurement_record.storage.back(), false);
        }
        sim.do_SQRT_Z(OpDat(3));
        sim.do_H_XZ(OpDat(3));
        ASSERT_EQ(sim.is_deterministic_z(3), true);
        sim.do_MZ(OpDat(3));
        ASSERT_EQ(sim.measurement_record.storage.back(), true);
    }
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, unitary_gates_consistent_with_tableau_data, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto t = Tableau<W>::random(10, rng);
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 10);
    for (const auto &gate : GATE_DATA.items) {
        if (!gate.has_known_unitary_matrix()) {
            continue;
        }
        sim.inv_state = t;

        const auto &inverse_op_tableau = gate.inverse().tableau<W>();
        if (inverse_op_tableau.num_qubits == 2) {
            sim.do_gate({gate.id, {}, qubit_targets({7, 4}), ""});
            t.inplace_scatter_prepend(inverse_op_tableau, {7, 4});
        } else {
            sim.do_gate({gate.id, {}, qubit_targets({5}), ""});
            t.inplace_scatter_prepend(inverse_op_tableau, {5});
        }
        EXPECT_EQ(sim.inv_state, t) << gate.name;
    }
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, certain_errors_consistent_with_gates, {
    TableauSimulator<W> sim1(INDEPENDENT_TEST_RNG(), 2);
    TableauSimulator<W> sim2(INDEPENDENT_TEST_RNG(), 2);
    GateTarget targets[]{GateTarget{0}};
    double p0 = 0.0;
    double p1 = 1.0;
    CircuitInstruction d0{(GateType)0, {&p0}, {targets}, ""};
    CircuitInstruction d1{(GateType)0, {&p1}, {targets}, ""};

    sim1.do_X_ERROR(d1);
    sim2.do_X(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
    sim1.do_X_ERROR(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);

    sim1.do_Y_ERROR(d1);
    sim2.do_Y(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
    sim1.do_Y_ERROR(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);

    sim1.do_Z_ERROR(d1);
    sim2.do_Z(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
    sim1.do_Z_ERROR(d0);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, simulate, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto results = TableauSimulator<W>::sample_circuit(
        Circuit(
            "H 0\n"
            "CNOT 0 1\n"
            "M 0\n"
            "M 1\n"
            "M 2\n"),
        rng);
    ASSERT_EQ(results[0], results[1]);
    ASSERT_EQ(results[2], false);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, simulate_reset, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto results = TableauSimulator<W>::sample_circuit(
        Circuit(
            "X 0\n"
            "M 0\n"
            "R 0\n"
            "M 0\n"
            "R 0\n"
            "M 0\n"),
        rng);
    ASSERT_EQ(results[0], true);
    ASSERT_EQ(results[1], false);
    ASSERT_EQ(results[2], false);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, to_vector_sim, {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauSimulator<W> sim_tab(INDEPENDENT_TEST_RNG(), 2);
    VectorSimulator sim_vec(2);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.do_X(OpDat(0));
    sim_vec.apply(GateType::X, 0);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.do_H_XZ(OpDat(0));
    sim_vec.apply(GateType::H, 0);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.do_SQRT_Z(OpDat(0));
    sim_vec.apply(GateType::S, 0);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.do_ZCX(OpDat({0, 1}));
    sim_vec.apply(GateType::CX, 0, 1);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.inv_state = Tableau<W>::random(10, rng);
    sim_vec = sim_tab.to_vector_sim();
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));

    sim_tab.do_gate({GateType::XCX, {}, qubit_targets({4, 7}), ""});
    sim_vec.apply(GateType::XCX, 4, 7);
    ASSERT_TRUE(sim_tab.to_vector_sim().approximate_equals(sim_vec, true));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, to_state_vector, {
    auto v = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), 0).to_state_vector(true);
    ASSERT_EQ(v.size(), 1);
    auto r = v[0].real();
    auto i = v[0].imag();
    ASSERT_LT(r * r + i * i - 1, 1e-4);

    TableauSimulator<W> sim_tab(INDEPENDENT_TEST_RNG(), 3);
    auto sim_vec = sim_tab.to_vector_sim();
    VectorSimulator sim_vec2(3);
    sim_vec2.state = sim_tab.to_state_vector(true);
    ASSERT_TRUE(sim_vec.approximate_equals(sim_vec2, true));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, to_state_vector_endian, {
    VectorSimulator sim_vec0(3);
    VectorSimulator sim_vec2(3);
    sim_vec0.apply(GateType::H, 0);
    sim_vec2.apply(GateType::H, 2);

    TableauSimulator<W> sim_tab(INDEPENDENT_TEST_RNG(), 3);
    sim_tab.do_H_XZ(OpDat(2));

    VectorSimulator cmp(3);
    cmp.state = sim_tab.to_state_vector(true);
    ASSERT_TRUE(cmp.approximate_equals(sim_vec2, true));
    cmp.state = sim_tab.to_state_vector(false);
    ASSERT_TRUE(cmp.approximate_equals(sim_vec0, true));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, to_state_vector_canonical, {
    TableauSimulator<W> sim_tab(INDEPENDENT_TEST_RNG(), 3);
    sim_tab.do_H_XZ(OpDat(2));
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
})

template <size_t W>
bool vec_sim_corroborates_measurement_process(
    const Tableau<W> &state, const std::vector<uint32_t> &measurement_targets) {
    TableauSimulator<W> sim_tab(INDEPENDENT_TEST_RNG(), 2);
    sim_tab.inv_state = state;
    auto vec_sim = sim_tab.to_vector_sim();
    sim_tab.do_MZ(OpDat(measurement_targets));
    PauliString<W> buf(sim_tab.inv_state.num_qubits);
    size_t k = 0;
    for (auto t : measurement_targets) {
        buf.zs[t] = true;
        buf.sign = sim_tab.measurement_record.storage[k++];
        float f = vec_sim.template project<W>(buf);
        if (fabs(f - 0.5) > 1e-4 && fabsf(f - 1) > 1e-4) {
            return false;
        }
        buf.zs[t] = false;
    }
    return true;
}

TEST_EACH_WORD_SIZE_W(TableauSimulator, measurement_vs_vector_sim, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (size_t k = 0; k < 5; k++) {
        auto state = Tableau<W>::random(2, rng);
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 1}));
    }
    for (size_t k = 0; k < 5; k++) {
        auto state = Tableau<W>::random(4, rng);
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {2, 1}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 1, 2, 3}));
    }
    {
        auto state = Tableau<W>::random(8, rng);
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 1, 2, 3}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 6, 7}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {7, 3, 4}));
        ASSERT_TRUE(vec_sim_corroborates_measurement_process(state, {0, 1, 2, 3, 4, 5, 6, 7}));
    }
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, correlated_error, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bits<W> expected(5);

    expected.clear();
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            rng),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            rng),
        expected);

    expected.clear();
    expected[1] = true;
    expected[2] = true;
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            rng),
        expected);

    expected.clear();
    expected[2] = true;
    expected[3] = true;
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(1) X2 X3
        M 0 1 2 3
    )circuit"),
            rng),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            rng),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(1) X2 X3
        M 0 1 2 3
    )circuit"),
            rng),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    expected[3] = true;
    expected[4] = true;
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(1) X2 X3
        CORRELATED_ERROR(1) X3 X4
        M 0 1 2 3 4
    )circuit"),
            rng),
        expected);

    int hits[3]{};
    size_t n = 5000;
    for (size_t k = 0; k < n; k++) {
        auto sample = TableauSimulator<W>::sample_circuit(
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
    ASSERT_TRUE((0.125 - 0.08) * n < hits[1] && hits[1] < (0.125 + 0.08) * n);
    ASSERT_TRUE((0.28125 - 0.08) * n < hits[2] && hits[2] < (0.28125 + 0.08) * n);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, quantum_cannot_control_classical, {
    auto rng = INDEPENDENT_TEST_RNG();

    // Quantum controlling classical operation is not allowed.
    ASSERT_THROW(
        {
            TableauSimulator<W>::sample_circuit(
                Circuit(R"circuit(
            M 0
            CNOT 1 rec[-1]
        )circuit"),
                rng);
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            TableauSimulator<W>::sample_circuit(
                Circuit(R"circuit(
            M 0
            CY 1 rec[-1]
        )circuit"),
                rng);
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            TableauSimulator<W>::sample_circuit(
                Circuit(R"circuit(
            M 0
            YCZ rec[-1] 1
        )circuit"),
                rng);
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            TableauSimulator<W>::sample_circuit(
                Circuit(R"circuit(
            M 0
            XCZ rec[-1] 1
        )circuit"),
                rng);
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            TableauSimulator<W>::sample_circuit(
                Circuit(R"circuit(
            M 0
            SWAP 1 rec[-1]
        )circuit"),
                rng);
        },
        std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, classical_can_control_quantum, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bits<W> expected(5);
    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        M !0
        CX rec[-1] 1
        M 1
    )circuit"),
            rng),
        expected);
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        M !0
        CY rec[-1] 1
        M 1
    )circuit"),
            rng),
        expected);
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        M !0
        XCZ 1 rec[-1]
        M 1
    )circuit"),
            rng),
        expected);
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        M !0
        YCZ 1 rec[-1]
        M 1
    )circuit"),
            rng),
        expected);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, classical_control_cases, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bits<W> expected(5);
    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        M !0
        H 1
        CZ rec[-1] 1
        H 1
        M 1
    )circuit"),
            rng),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        M !0
        CY rec[-1] 1
        M 1
    )circuit"),
            rng),
        expected);

    expected.clear();
    expected[0] = false;
    expected[1] = false;
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        M 0
        CX rec[-1] 1
        M 1
    )circuit"),
            rng),
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = false;
    expected[2] = true;
    expected[3] = false;
    expected[4] = false;
    ASSERT_EQ(
        TableauSimulator<W>::sample_circuit(
            Circuit(R"circuit(
        X 0
        M 0
        R 0
        M 0
        CX rec[-2] 1
        CX rec[-1] 2
        M 1 2 3
    )circuit"),
            rng),
        expected);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, mr_repeated_target, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bits<W> expected(2);
    expected[0] = true;
    auto r = TableauSimulator<W>::sample_circuit(Circuit("X 0\nMR 0 0"), rng);
    ASSERT_EQ(r, expected);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, peek_bloch, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 3);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString<W>::from_str("+Z"));

    sim.do_H_XZ(OpDat(0));
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString<W>::from_str("+Z"));

    sim.do_X(OpDat(1));
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString<W>::from_str("+Z"));

    sim.do_H_YZ(OpDat(2));
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString<W>::from_str("+Y"));

    sim.do_X(OpDat(0));
    sim.do_X(OpDat(1));
    sim.do_X(OpDat(2));
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString<W>::from_str("-Y"));

    sim.do_Y(OpDat(0));
    sim.do_Y(OpDat(1));
    sim.do_Y(OpDat(2));
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString<W>::from_str("-Y"));

    sim.do_ZCZ(OpDat({0, 1}));
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString<W>::from_str("-Y"));

    sim.do_ZCZ(OpDat({1, 2}));
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString<W>::from_str("+Y"));

    sim.do_ZCZ(OpDat({0, 2}));
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+I"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString<W>::from_str("+I"));

    sim.do_X(OpDat(0));
    sim.do_X(OpDat(1));
    sim.do_X(OpDat(2));
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+I"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(2), PauliString<W>::from_str("+I"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, paulis, {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauSimulator<W> sim1(INDEPENDENT_TEST_RNG(), 300);
    TableauSimulator<W> sim2(INDEPENDENT_TEST_RNG(), 300);
    sim1.inv_state = Tableau<W>::random(300, rng);
    sim2.inv_state = sim1.inv_state;

    sim1.paulis(PauliString<W>(300));
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
    sim1.paulis(PauliString<W>(5));
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
    sim1.paulis(PauliString<W>(0));
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);

    sim1.paulis(PauliString<W>::from_str("IXYZ"));
    sim2.do_X(OpDat(1));
    sim2.do_Y(OpDat(2));
    sim2.do_Z(OpDat(3));
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, set_num_qubits, {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauSimulator<W> sim1(INDEPENDENT_TEST_RNG(), 10);
    TableauSimulator<W> sim2(INDEPENDENT_TEST_RNG(), 10);
    sim1.inv_state = Tableau<W>::random(10, rng);
    sim2.inv_state = sim1.inv_state;

    sim1.set_num_qubits(20);
    sim1.set_num_qubits(10);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);

    sim1.set_num_qubits(20);
    sim1.do_X(OpDat(10));
    sim1.do_Z(OpDat(11));
    sim1.do_H_XZ(OpDat(12));
    sim1.do_ZCX(OpDat({12, 13}));
    sim1.set_num_qubits(10);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);

    sim1.set_num_qubits(20);
    sim2.ensure_large_enough_for_qubits(20);
    ASSERT_EQ(sim1.inv_state, sim2.inv_state);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, set_num_qubits_reduce_random, {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 10);
    sim.inv_state = Tableau<W>::random(10, rng);
    sim.set_num_qubits(5);
    ASSERT_EQ(sim.inv_state.num_qubits, 5);
    ASSERT_TRUE(sim.inv_state.satisfies_invariants());
})

template <size_t W>
void scramble_stabilizers(TableauSimulator<W> &s) {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauTransposedRaii<W> tmp(s.inv_state);
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

TEST_EACH_WORD_SIZE_W(TableauSimulator, canonical_stabilizers, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 2);
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    ASSERT_EQ(
        sim.canonical_stabilizers(),
        (std::vector<PauliString<W>>{
            PauliString<W>::from_str("XX"),
            PauliString<W>::from_str("ZZ"),
        }));
    sim.do_SQRT_Y(OpDat({0, 1}));
    ASSERT_EQ(
        sim.canonical_stabilizers(),
        (std::vector<PauliString<W>>{
            PauliString<W>::from_str("XX"),
            PauliString<W>::from_str("ZZ"),
        }));
    sim.do_SQRT_X(OpDat({0, 1}));
    ASSERT_EQ(
        sim.canonical_stabilizers(),
        (std::vector<PauliString<W>>{
            PauliString<W>::from_str("XX"),
            PauliString<W>::from_str("-ZZ"),
        }));
    sim.set_num_qubits(3);
    ASSERT_EQ(
        sim.canonical_stabilizers(),
        (std::vector<PauliString<W>>{
            PauliString<W>::from_str("+XX_"),
            PauliString<W>::from_str("-ZZ_"),
            PauliString<W>::from_str("+__Z"),
        }));
    sim.do_ZCX(OpDat({2, 0}));
    ASSERT_EQ(
        sim.canonical_stabilizers(),
        (std::vector<PauliString<W>>{
            PauliString<W>::from_str("+XX_"),
            PauliString<W>::from_str("-ZZ_"),
            PauliString<W>::from_str("+__Z"),
        }));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, canonical_stabilizers_random, {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 4);
    sim.inv_state = Tableau<W>::random(4, rng);
    auto s1 = sim.canonical_stabilizers();
    scramble_stabilizers<W>(sim);
    auto s2 = sim.canonical_stabilizers();
    ASSERT_EQ(s1, s2);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, set_num_qubits_reduce_preserves_scrambled_stabilizers, {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 4);
    sim.inv_state = Tableau<W>::random(4, rng);
    auto s1 = sim.canonical_stabilizers();
    sim.inv_state.expand(8, 1.0);
    scramble_stabilizers<W>(sim);
    sim.set_num_qubits(4);
    auto s2 = sim.canonical_stabilizers();
    ASSERT_EQ(s1, s2);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_kickback_z, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 4);
    sim.do_H_XZ(OpDat({0, 2}));
    sim.do_ZCX(OpDat({0, 1, 2, 3}));
    auto k1 = sim.measure_kickback_z(GateTarget::qubit(1));
    auto k2 = sim.measure_kickback_z(GateTarget::qubit(2));
    auto k3 = sim.measure_kickback_z(GateTarget::qubit(3));
    ASSERT_EQ(k1.second, PauliString<W>::from_str("XX__"));
    ASSERT_EQ(k2.second, PauliString<W>::from_str("__XX"));
    ASSERT_EQ(k3.second, PauliString<W>(0));
    ASSERT_EQ(k2.first, k3.first);
    auto p = PauliString<W>::from_str("+Z");
    auto pn = PauliString<W>::from_str("-Z");
    ASSERT_EQ(sim.peek_bloch(0), k1.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(1), k1.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(2), k2.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(3), k2.first ? pn : p);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_kickback_x, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 4);
    sim.do_H_XZ(OpDat({0, 2}));
    sim.do_ZCX(OpDat({0, 1, 2, 3}));
    auto k1 = sim.measure_kickback_x(GateTarget::qubit(1));
    auto k2 = sim.measure_kickback_x(GateTarget::qubit(2));
    auto k3 = sim.measure_kickback_x(GateTarget::qubit(3));
    ASSERT_EQ(k1.second, PauliString<W>::from_str("ZZ__"));
    ASSERT_EQ(k2.second, PauliString<W>::from_str("__ZZ"));
    ASSERT_EQ(k3.second, PauliString<W>(0));
    ASSERT_EQ(k2.first, k3.first);
    auto p = PauliString<W>::from_str("+X");
    auto pn = PauliString<W>::from_str("-X");
    ASSERT_EQ(sim.peek_bloch(0), k1.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(1), k1.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(2), k2.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(3), k2.first ? pn : p);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_kickback_y, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 4);
    sim.do_H_XZ(OpDat({0, 2}));
    sim.do_ZCX(OpDat({0, 1, 2, 3}));
    auto k1 = sim.measure_kickback_y(GateTarget::qubit(1));
    auto k2 = sim.measure_kickback_y(GateTarget::qubit(2));
    auto k3 = sim.measure_kickback_y(GateTarget::qubit(3));
    ASSERT_EQ(k1.second, PauliString<W>::from_str("ZX__"));
    ASSERT_EQ(k2.second, PauliString<W>::from_str("__ZX"));
    ASSERT_EQ(k3.second, PauliString<W>(0));
    ASSERT_NE(k2.first, k3.first);
    auto p = PauliString<W>::from_str("+Y");
    auto pn = PauliString<W>::from_str("-Y");
    ASSERT_EQ(sim.peek_bloch(0), k1.first ? p : pn);
    ASSERT_EQ(sim.peek_bloch(1), k1.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(2), k2.first ? pn : p);
    ASSERT_EQ(sim.peek_bloch(3), k2.first ? p : pn);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_kickback_isolates, {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 4);
    sim.inv_state = Tableau<W>::random(4, rng);
    for (size_t k = 0; k < 4; k++) {
        auto result = sim.measure_kickback_z(GateTarget::qubit(k));
        for (size_t j = 0; j < result.second.num_qubits && j < k; j++) {
            ASSERT_FALSE(result.second.xs[j]);
            ASSERT_FALSE(result.second.zs[j]);
        }
    }
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, collapse_isolate_completely, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (size_t k = 0; k < 10; k++) {
        TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 6);
        sim.inv_state = Tableau<W>::random(6, rng);
        {
            TableauTransposedRaii<W> tmp(sim.inv_state);
            sim.collapse_isolate_qubit_z(2, tmp);
        }
        PauliString<W> x2 = sim.inv_state.xs[2];
        PauliString<W> z2 = sim.inv_state.zs[2];
        x2.sign = false;
        z2.sign = false;
        ASSERT_EQ(x2, PauliString<W>::from_str("__X___"));
        ASSERT_EQ(z2, PauliString<W>::from_str("__Z___"));
    }
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, reset_pure, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG(), 1);
    ASSERT_EQ(t.peek_bloch(0), PauliString<W>::from_str("+Z"));
    t.do_RY(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString<W>::from_str("+Y"));
    t.do_RX(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString<W>::from_str("+X"));
    t.do_RY(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString<W>::from_str("+Y"));
    t.do_RZ(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString<W>::from_str("+Z"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, reset_random, {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG(), 5);

    t.inv_state = Tableau<W>::random(5, rng);
    t.do_RX(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString<W>::from_str("+X"));

    t.inv_state = Tableau<W>::random(5, rng);
    t.do_RY(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString<W>::from_str("+Y"));

    t.inv_state = Tableau<W>::random(5, rng);
    t.do_RZ(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString<W>::from_str("+Z"));

    t.inv_state = Tableau<W>::random(5, rng);
    t.do_MRX(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString<W>::from_str("+X"));

    t.inv_state = Tableau<W>::random(5, rng);
    t.do_MRY(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString<W>::from_str("+Y"));

    t.inv_state = Tableau<W>::random(5, rng);
    t.do_MRZ(OpDat(0));
    ASSERT_EQ(t.peek_bloch(0), PauliString<W>::from_str("+Z"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, reset_x_entangled, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG(), 2);
    t.do_H_XZ(OpDat(0));
    t.do_ZCX(OpDat({0, 1}));
    t.do_RX(OpDat(0));
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign = false;
    ASSERT_EQ(p1, PauliString<W>::from_str("+X"));
    ASSERT_EQ(p2, PauliString<W>::from_str("+X"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, reset_y_entangled, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG(), 2);
    t.do_H_XZ(OpDat(0));
    t.do_ZCX(OpDat({0, 1}));
    t.do_RY(OpDat(0));
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign = false;
    ASSERT_EQ(p1, PauliString<W>::from_str("+Y"));
    ASSERT_EQ(p2, PauliString<W>::from_str("+Y"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, reset_z_entangled, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG(), 2);
    t.do_H_XZ(OpDat(0));
    t.do_ZCX(OpDat({0, 1}));
    t.do_RZ(OpDat(0));
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign = false;
    ASSERT_EQ(p1, PauliString<W>::from_str("+Z"));
    ASSERT_EQ(p2, PauliString<W>::from_str("+Z"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_x_entangled, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG(), 2);
    t.do_H_XZ(OpDat(0));
    t.do_ZCX(OpDat({0, 1}));
    t.do_MX(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p1.sign ^= b;
    p2.sign ^= b;
    ASSERT_EQ(p1, PauliString<W>::from_str("+X"));
    ASSERT_EQ(p2, PauliString<W>::from_str("+X"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_y_entangled, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG(), 2);
    t.do_H_XZ(OpDat(0));
    t.do_ZCX(OpDat({0, 1}));
    t.do_MY(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p1.sign ^= b;
    p2.sign ^= !b;
    ASSERT_EQ(p1, PauliString<W>::from_str("+Y"));
    ASSERT_EQ(p2, PauliString<W>::from_str("+Y"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_z_entangled, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG(), 2);
    t.do_H_XZ(OpDat(0));
    t.do_ZCX(OpDat({0, 1}));
    t.do_MZ(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p1.sign ^= b;
    p2.sign ^= b;
    ASSERT_EQ(p1, PauliString<W>::from_str("+Z"));
    ASSERT_EQ(p2, PauliString<W>::from_str("+Z"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_reset_x_entangled, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG(), 2);
    t.do_H_XZ(OpDat(0));
    t.do_ZCX(OpDat({0, 1}));
    t.do_MRX(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign ^= b;
    ASSERT_EQ(p1, PauliString<W>::from_str("+X"));
    ASSERT_EQ(p2, PauliString<W>::from_str("+X"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_reset_y_entangled, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG(), 2);
    t.do_H_XZ(OpDat(0));
    t.do_ZCX(OpDat({0, 1}));
    t.do_MRY(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign ^= !b;
    ASSERT_EQ(p1, PauliString<W>::from_str("+Y"));
    ASSERT_EQ(p2, PauliString<W>::from_str("+Y"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_reset_z_entangled, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG(), 2);
    t.do_H_XZ(OpDat(0));
    t.do_ZCX(OpDat({0, 1}));
    t.do_MRZ(OpDat(0));
    auto b = t.measurement_record.storage.back();
    auto p1 = t.peek_bloch(0);
    auto p2 = t.peek_bloch(1);
    p2.sign ^= b;
    ASSERT_EQ(p1, PauliString<W>::from_str("+Z"));
    ASSERT_EQ(p2, PauliString<W>::from_str("+Z"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, reset_vs_measurements, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto check = [&](const char *circuit, std::vector<bool> results) {
        simd_bits<W> ref(results.size());
        for (size_t k = 0; k < results.size(); k++) {
            ref[k] = results[k];
        }
        for (size_t reps = 0; reps < 5; reps++) {
            simd_bits<W> t = TableauSimulator<W>::sample_circuit(Circuit(circuit), rng);
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
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, sample_circuit_mutates_rng_state, {
    std::mt19937_64 rng(1234);
    TableauSimulator<W>::sample_circuit(Circuit("H 0\nM 0"), rng);
    ASSERT_NE(rng, std::mt19937_64(1234));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, sample_stream_mutates_rng_state, {
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    fprintf(in, "H 0\nM 0\n");
    rewind(in);

    std::mt19937_64 rng(2345);
    TableauSimulator<W>::sample_stream(in, out, SampleFormat::SAMPLE_FORMAT_B8, false, rng);

    ASSERT_NE(rng, std::mt19937_64(2345));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, noisy_measurement_x, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RX 0
        REPEAT 10000 {
            MX(0.05) 0
        }
    )CIRCUIT"));
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.safe_do_circuit(Circuit("MX 0"));
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RX 0 1
        Y 0 1
        REPEAT 5000 {
            MX(0.05) 0 1
        }
    )CIRCUIT"));
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.safe_do_circuit(Circuit("MX 0"));
    ASSERT_TRUE(t.measurement_record.storage.back());
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, noisy_measurement_y, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RY 0
        REPEAT 10000 {
            MY(0.05) 0
        }
    )CIRCUIT"));
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.safe_do_circuit(Circuit("MY 0"));
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RY 0 1
        X 0 1
        REPEAT 5000 {
            MY(0.05) 0 1
        }
    )CIRCUIT"));
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.safe_do_circuit(Circuit("MY 0"));
    ASSERT_TRUE(t.measurement_record.storage.back());
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, noisy_measurement_z, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RZ 0
        REPEAT 10000 {
            MZ(0.05) 0
        }
    )CIRCUIT"));
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.safe_do_circuit(Circuit("MZ 0"));
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RZ 0 1
        X 0 1
        REPEAT 5000 {
            MZ(0.05) 0 1
        }
    )CIRCUIT"));
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.safe_do_circuit(Circuit("MZ 0"));
    ASSERT_TRUE(t.measurement_record.storage.back());
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, noisy_measure_reset_x, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RX 0
        REPEAT 10000 {
            MRX(0.05) 0
        }
    )CIRCUIT"));
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.safe_do_circuit(Circuit("MX 0"));
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RX 0 1
        REPEAT 5000 {
            Z 0 1
            MRX(0.05) 0 1
        }
    )CIRCUIT"));
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.safe_do_circuit(Circuit("MX 0"));
    ASSERT_FALSE(t.measurement_record.storage.back());
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, noisy_measure_reset_y, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RY 0 1
        REPEAT 5000 {
            MRY(0.05) 0 1
        }
    )CIRCUIT"));
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.safe_do_circuit(Circuit("MY 0"));
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RY 0 1
        REPEAT 5000 {
            X 0 1
            MRY(0.05) 0 1
        }
    )CIRCUIT"));
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.safe_do_circuit(Circuit("MY 0"));
    ASSERT_FALSE(t.measurement_record.storage.back());
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, noisy_measure_reset_z, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RZ 0 1
        REPEAT 5000 {
            MRZ(0.05) 0 1
        }
    )CIRCUIT"));
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.safe_do_circuit(Circuit("MZ 0"));
    ASSERT_FALSE(t.measurement_record.storage.back());

    t.measurement_record.storage.clear();
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        RZ 0 1
        REPEAT 5000 {
            X 0 1
            MRZ(0.05) 0 1
        }
    )CIRCUIT"));
    m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 10000 - 700);
    ASSERT_LT(m1, 10000 - 300);
    t.safe_do_circuit(Circuit("MZ 0"));
    ASSERT_FALSE(t.measurement_record.storage.back());
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_pauli_product_bad, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
    t.safe_do_circuit(Circuit("MPP X0*X0 !X0*X0"));
    ASSERT_EQ(t.measurement_record.storage, (std::vector<bool>{false, true}));
    ASSERT_THROW({ t.safe_do_circuit(Circuit("MPP X0*Z0")); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_pauli_product_1, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        REPEAT 100 {
            RX 0
            RY 1
            RZ 2
            MPP X0 Y1 Z2 X0*Y1*Z2
        }
    )CIRCUIT"));
    ASSERT_EQ(t.measurement_record.storage.size(), 400);
    ASSERT_EQ(std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0), 0);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_pauli_product_4body, {
    for (size_t k = 0; k < 10; k++) {
        TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
        t.safe_do_circuit(Circuit(R"CIRCUIT(
            MPP X0*X1*X2*X3
            MX 0 1 2 3 4 5
            MPP X2*X3*X4*X5
            MPP Z0*Z1*Z4*Z5 !Y0*Y1*Y4*Y5
        )CIRCUIT"));
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
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_pauli_product_epr, {
    for (size_t k = 0; k < 10; k++) {
        TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
        t.safe_do_circuit(Circuit(R"CIRCUIT(
            MPP X0*X1 Z0*Z1 Y0*Y1
            CNOT 0 1
            H 0
            M 0 1
        )CIRCUIT"));
        auto x01 = t.measurement_record.storage[0];
        auto z01 = t.measurement_record.storage[1];
        auto y01 = t.measurement_record.storage[2];
        auto m0 = t.measurement_record.storage[3];
        auto m1 = t.measurement_record.storage[4];
        ASSERT_EQ(m0, x01);
        ASSERT_EQ(m1, z01);
        ASSERT_EQ(x01 ^ z01, y01 ^ 1);
    }
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_pauli_product_inversions, {
    for (size_t k = 0; k < 10; k++) {
        TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
        t.safe_do_circuit(Circuit(R"CIRCUIT(
            MPP !X0*!X1 !X0*X1 X0*!X1 X0*X1 X0 X1 !X0 !X1
        )CIRCUIT"));
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
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_pauli_product_noisy, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        H 0
        CNOT 0 1
        REPEAT 5000 {
            MPP(0.05) X0*X1 Z0*Z1
        }
    )CIRCUIT"));
    auto m1 = std::accumulate(t.measurement_record.storage.begin(), t.measurement_record.storage.end(), 0);
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);
    t.safe_do_circuit(Circuit("MPP Y0*Y1"));
    ASSERT_EQ(t.measurement_record.storage.back(), true);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, ignores_sweep_controls, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        X 0
        CNOT sweep[0] 0
        M 0
    )CIRCUIT"));
    ASSERT_EQ(t.measurement_record.lookback(1), true);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, peek_observable_expectation, {
    TableauSimulator<W> t(INDEPENDENT_TEST_RNG());
    t.safe_do_circuit(Circuit(R"CIRCUIT(
        H 0
        CNOT 0 1
        X 0
    )CIRCUIT"));

    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("XX")), 1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("YY")), 1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("ZZ")), -1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("-XX")), -1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("-ZZ")), 1);

    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("")), 1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("-I")), -1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("X")), 0);
    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("Z")), 0);
    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("Z_")), 0);
    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("ZZZ")), -1);
    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("XXX")), 0);
    ASSERT_EQ(t.peek_observable_expectation(PauliString<W>::from_str("ZZZZZZZZ")), -1);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, postselect_x, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 2);

    // Postselect from +X.
    sim.do_RX(OpDat(0));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));

    // Postselect from -X.
    sim.do_RX(OpDat(0));
    sim.do_Z(OpDat(0));
    ASSERT_THROW({ sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false); }, std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-X"));

    // Postselect from +Y.
    sim.do_RY(OpDat(0));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));

    // Postselect from -Y.
    sim.do_RY(OpDat(0));
    sim.do_X(OpDat(0));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));

    // Postselect from +Z.
    sim.do_RZ(OpDat(0));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));

    // Postselect from -Z.
    sim.do_RZ(OpDat(0));
    sim.do_X(OpDat(0));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));

    // Postselect entangled.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(1)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+X"));

    // Postselect opposite state entangled.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-X"));

    // Postselect both independent.
    sim.do_RZ(OpDat({0, 1}));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-X"));

    // Postselect both entangled.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-X"));

    // Contradiction reached during second postselection.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.do_Z(OpDat(0));
    ASSERT_THROW(
        { sim.postselect_x(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true); },
        std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-X"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+X"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, postselect_y, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 2);

    // Postselect from +X.
    sim.do_RX(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Y"));

    // Postselect from -X.
    sim.do_RX(OpDat(0));
    sim.do_Z(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Y"));

    // Postselect from +Y.
    sim.do_RY(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Y"));

    // Postselect from -Y.
    sim.do_RY(OpDat(0));
    sim.do_X(OpDat(0));
    ASSERT_THROW({ sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false); }, std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-Y"));

    // Postselect from +Z.
    sim.do_RZ(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Y"));

    // Postselect from -Z.
    sim.do_RZ(OpDat(0));
    sim.do_X(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Y"));

    // Postselect entangled.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(1)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-Y"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+Y"));

    // Postselect opposite state entangled.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Y"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Y"));

    // Postselect both independent.
    sim.do_RZ(OpDat({0, 1}));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-Y"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Y"));

    // Postselect both entangled.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.do_Z(OpDat(0));
    sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-Y"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Y"));

    // Contradiction reached during second postselection.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    ASSERT_THROW(
        { sim.postselect_y(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true); },
        std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-Y"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+Y"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, postselect_z, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 2);

    // Postselect from +X.
    sim.do_RX(OpDat(0));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Z"));

    // Postselect from -X.
    sim.do_RX(OpDat(0));
    sim.do_Z(OpDat(0));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Z"));

    // Postselect from +Y.
    sim.do_RY(OpDat(0));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Z"));

    // Postselect from -Y.
    sim.do_RY(OpDat(0));
    sim.do_X(OpDat(0));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Z"));

    // Postselect from +Z.
    sim.do_RZ(OpDat(0));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Z"));

    // Postselect from -Z.
    sim.do_RZ(OpDat(0));
    sim.do_X(OpDat(0));
    ASSERT_THROW({ sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0)}, false); }, std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-Z"));

    // Postselect entangled.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(1)}, false);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+Z"));

    // Postselect opposite state entangled.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Z"));

    // Postselect both independent.
    sim.do_RX(OpDat({0, 1}));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Z"));

    // Postselect both entangled.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-Z"));

    // Contradiction reached during second postselection.
    sim.do_RZ(OpDat({0, 1}));
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.do_X(OpDat(0));
    ASSERT_THROW(
        { sim.postselect_z(std::vector<GateTarget>{GateTarget::qubit(0), GateTarget::qubit(1)}, true); },
        std::invalid_argument);
    ASSERT_EQ(sim.peek_bloch(0), PauliString<W>::from_str("-Z"));
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+Z"));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, peek_x, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 3);
    ASSERT_EQ(sim.peek_x(0), 0);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), +1);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), +1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), 0);
    ASSERT_EQ(sim.peek_z(2), +1);

    sim.do_H_XZ(OpDat(0));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), +1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), 0);
    ASSERT_EQ(sim.peek_z(2), +1);

    sim.do_X(OpDat(1));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), 0);
    ASSERT_EQ(sim.peek_z(2), +1);

    sim.do_H_YZ(OpDat(2));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), +1);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.do_X(OpDat(0));
    sim.do_X(OpDat(1));
    sim.do_X(OpDat(2));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), +1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), -1);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.do_Y(OpDat(0));
    sim.do_Y(OpDat(1));
    sim.do_Y(OpDat(2));
    ASSERT_EQ(sim.peek_x(0), -1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), -1);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.do_ZCZ(OpDat({0, 1}));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), -1);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.do_ZCZ(OpDat({1, 2}));
    ASSERT_EQ(sim.peek_x(0), +1);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), +1);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.do_ZCZ(OpDat({0, 2}));
    ASSERT_EQ(sim.peek_x(0), 0);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), -1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), 0);
    ASSERT_EQ(sim.peek_z(2), 0);

    sim.do_X(OpDat(0));
    sim.do_X(OpDat(1));
    sim.do_X(OpDat(2));
    ASSERT_EQ(sim.peek_x(0), 0);
    ASSERT_EQ(sim.peek_y(0), 0);
    ASSERT_EQ(sim.peek_z(0), 0);
    ASSERT_EQ(sim.peek_x(1), 0);
    ASSERT_EQ(sim.peek_y(1), 0);
    ASSERT_EQ(sim.peek_z(1), +1);
    ASSERT_EQ(sim.peek_x(2), 0);
    ASSERT_EQ(sim.peek_y(2), 0);
    ASSERT_EQ(sim.peek_z(2), 0);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, apply_tableau, {
    auto cnot = GATE_DATA.at("CNOT").tableau<W>();
    auto s = GATE_DATA.at("S").tableau<W>();
    auto h = GATE_DATA.at("H").tableau<W>();
    auto cxyz = GATE_DATA.at("C_XYZ").tableau<W>();

    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 4);
    sim.apply_tableau(h, {1});
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+X"));
    sim.apply_tableau(s, {1});
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("+Y"));
    sim.apply_tableau(s, {1});
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-X"));
    sim.apply_tableau(cnot, {2, 1});
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("-X"));
    sim.apply_tableau(cnot, {1, 2});
    ASSERT_EQ(sim.peek_bloch(1), PauliString<W>::from_str("I"));
    ASSERT_EQ(sim.peek_observable_expectation(PauliString<W>::from_str("IXXI")), -1);
    ASSERT_EQ(sim.peek_observable_expectation(PauliString<W>::from_str("IZZI")), +1);

    sim.apply_tableau(cxyz, {2});
    sim.apply_tableau(cxyz.inverse(), {1});
    ASSERT_EQ(sim.peek_observable_expectation(PauliString<W>::from_str("IZYI")), -1);
    ASSERT_EQ(sim.peek_observable_expectation(PauliString<W>::from_str("IYXI")), +1);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, measure_pauli_string, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 4);
    sim.do_H_XZ(OpDat(0));
    sim.do_ZCX(OpDat({0, 1}));
    sim.do_X(OpDat(0));

    ASSERT_FALSE(sim.measure_pauli_string(PauliString<W>::from_str("XX"), 0.0));
    ASSERT_TRUE(sim.measure_pauli_string(PauliString<W>::from_str("XX"), 1.0));
    ASSERT_TRUE(sim.measure_pauli_string(PauliString<W>::from_str("-XX"), 0.0));
    ASSERT_FALSE(sim.measure_pauli_string(PauliString<W>::from_str("-XX"), 1.0));

    ASSERT_TRUE(sim.measure_pauli_string(PauliString<W>::from_str("ZZ"), 0.0));
    ASSERT_FALSE(sim.measure_pauli_string(PauliString<W>::from_str("ZZ"), 1.0));
    ASSERT_FALSE(sim.measure_pauli_string(PauliString<W>::from_str("-ZZ"), 0.0));
    ASSERT_TRUE(sim.measure_pauli_string(PauliString<W>::from_str("-ZZ"), 1.0));

    ASSERT_FALSE(sim.measure_pauli_string(PauliString<W>::from_str("YY"), 0.0));
    ASSERT_FALSE(sim.measure_pauli_string(PauliString<W>::from_str("XXZ"), 0.0));
    ASSERT_FALSE(sim.measure_pauli_string(PauliString<W>::from_str("__Z"), 0.0));

    auto b = sim.measure_pauli_string(PauliString<W>::from_str("XXX"), 0.0);
    ASSERT_EQ(sim.measure_pauli_string(PauliString<W>::from_str("XXX"), 0.0), b);
    ASSERT_EQ(sim.measure_pauli_string(PauliString<W>::from_str("-XXX"), 0.0), !b);
    ASSERT_EQ(sim.measure_pauli_string(PauliString<W>::from_str("XXX"), 1.0), !b);

    ASSERT_TRUE(sim.measure_pauli_string(PauliString<W>::from_str("XX"), 1.0));

    ASSERT_THROW({ sim.measure_pauli_string(PauliString<W>::from_str(""), -0.5); }, std::invalid_argument);
    ASSERT_THROW({ sim.measure_pauli_string(PauliString<W>::from_str(""), 2.5); }, std::invalid_argument);
    ASSERT_THROW({ sim.measure_pauli_string(PauliString<W>::from_str(""), NAN); }, std::invalid_argument);

    ASSERT_FALSE(sim.measure_pauli_string(PauliString<W>::from_str("+"), 0.0));
    ASSERT_TRUE(sim.measure_pauli_string(PauliString<W>::from_str("+"), 1.0));
    ASSERT_TRUE(sim.measure_pauli_string(PauliString<W>::from_str("-"), 0.0));
    ASSERT_FALSE(sim.measure_pauli_string(PauliString<W>::from_str("-"), 1.0));
    ASSERT_FALSE(sim.measure_pauli_string(PauliString<W>::from_str("____________Z"), 0.0));
    ASSERT_EQ(sim.inv_state.num_qubits, 13);

    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{0, 1, 1,  0,  1, 0, 0, 1, 0, 0, 0,
                                                                 b, b, !b, !b, 1, 0, 1, 1, 0, 0}));

    size_t t = 0;
    for (size_t k = 0; k < 10000; k++) {
        t += sim.measure_pauli_string(PauliString<W>::from_str("-ZZ"), 0.2);
    }
    ASSERT_GT(t / 10000.0, 0.05);
    ASSERT_LT(t / 10000.0, 0.35);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, amortized_resizing, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 5120);
    sim.ensure_large_enough_for_qubits(5121);
    ASSERT_GT(sim.inv_state.xs.xt.num_minor_bits_padded(), 5600);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, mpad, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 5);
    ASSERT_EQ(sim.inv_state, Tableau<W>(5));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{}));

    sim.safe_do_circuit(Circuit("MPAD 0"));
    ASSERT_EQ(sim.inv_state, Tableau<W>(5));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{0}));

    sim.safe_do_circuit(Circuit("MPAD 1"));
    ASSERT_EQ(sim.inv_state, Tableau<W>(5));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{0, 1}));

    sim.safe_do_circuit(Circuit("MPAD 0 0 1 1 0"));
    ASSERT_EQ(sim.inv_state, Tableau<W>(5));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{0, 1, 0, 0, 1, 1, 0}));
})

template <size_t W>
void expect_same_final_state(const Tableau<W> &start, const Circuit &c1, const Circuit &c2, bool unsigned_stabilizers) {
    size_t n = start.num_qubits;
    TableauSimulator<W> sim1(INDEPENDENT_TEST_RNG(), n);
    TableauSimulator<W> sim2(INDEPENDENT_TEST_RNG(), n);
    sim1.inv_state = start;
    sim2.inv_state = start;
    sim1.safe_do_circuit(c1);
    sim2.safe_do_circuit(c2);
    auto t1 = sim1.canonical_stabilizers();
    auto t2 = sim2.canonical_stabilizers();
    if (unsigned_stabilizers) {
        for (auto &e1 : t1) {
            e1.sign = false;
        }
        for (auto &e2 : t2) {
            e2.sign = false;
        }
    }
    EXPECT_EQ(t1, t2);
}

TEST_EACH_WORD_SIZE_W(TableauSimulator, mxx_myy_mzz_vs_mpp_unsigned, {
    auto rng = INDEPENDENT_TEST_RNG();
    expect_same_final_state(
        Tableau<W>::random(5, rng), Circuit("MXX 1 3 1 2 3 4"), Circuit("MPP X1*X3 X1*X2 X3*X4"), true);
    expect_same_final_state(
        Tableau<W>::random(5, rng), Circuit("MYY 1 3 1 2 3 4"), Circuit("MPP Y1*Y3 Y1*Y2 Y3*Y4"), true);
    expect_same_final_state(
        Tableau<W>::random(5, rng), Circuit("MZZ 1 3 1 2 3 4"), Circuit("MPP Z1*Z3 Z1*Z2 Z3*Z4"), true);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, mxx, {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 5);
    sim.safe_do_circuit(Circuit("RX 0 1"));
    sim.safe_do_circuit(Circuit("MXX 0 1"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{false}));
    sim.measurement_record.storage.clear();

    sim.inv_state = Tableau<W>::random(5, rng);
    sim.safe_do_circuit(Circuit("MXX 1 3"));
    bool x13 = sim.measurement_record.storage.back();
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MXX 1 3"));
    sim.safe_do_circuit(Circuit("MXX 1 !3"));
    sim.safe_do_circuit(Circuit("MXX !1 3"));
    sim.safe_do_circuit(Circuit("MXX !1 !3"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{x13, !x13, !x13, x13}));
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MXX 2 3"));
    bool x23 = sim.measurement_record.storage.back();
    bool x12 = x13 ^ x23;
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MXX 1 2"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{x12}));
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MXX 3 4"));
    bool x34 = sim.measurement_record.storage.back();
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MXX 1 2 3 4 2 3 1 3"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{x12, x34, x23, x13}));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, myy, {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 5);
    sim.safe_do_circuit(Circuit("RY 0 1"));
    sim.safe_do_circuit(Circuit("MYY 0 1"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{false}));
    sim.measurement_record.storage.clear();

    sim.inv_state = Tableau<W>::random(5, rng);
    sim.safe_do_circuit(Circuit("MYY 1 3"));
    bool x13 = sim.measurement_record.storage.back();
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MYY 1 3"));
    sim.safe_do_circuit(Circuit("MYY 1 !3"));
    sim.safe_do_circuit(Circuit("MYY !1 3"));
    sim.safe_do_circuit(Circuit("MYY !1 !3"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{x13, !x13, !x13, x13}));
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MYY 2 3"));
    bool x23 = sim.measurement_record.storage.back();
    bool x12 = x13 ^ x23;
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MYY 1 2"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{x12}));
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MYY 3 4"));
    bool x34 = sim.measurement_record.storage.back();
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MYY 1 2 3 4 2 3 1 3"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{x12, x34, x23, x13}));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, mzz, {
    auto rng = INDEPENDENT_TEST_RNG();
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 5);
    sim.safe_do_circuit(Circuit("RZ 0 1"));
    sim.safe_do_circuit(Circuit("MZZ 0 1"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{false}));
    sim.measurement_record.storage.clear();

    sim.inv_state = Tableau<W>::random(5, rng);
    sim.safe_do_circuit(Circuit("MZZ 1 3"));
    bool x13 = sim.measurement_record.storage.back();
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MZZ 1 3"));
    sim.safe_do_circuit(Circuit("MZZ 1 !3"));
    sim.safe_do_circuit(Circuit("MZZ !1 3"));
    sim.safe_do_circuit(Circuit("MZZ !1 !3"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{x13, !x13, !x13, x13}));
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MZZ 2 3"));
    bool x23 = sim.measurement_record.storage.back();
    bool x12 = x13 ^ x23;
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MZZ 1 2"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{x12}));
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MZZ 3 4"));
    bool x34 = sim.measurement_record.storage.back();
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit("MZZ 1 2 3 4 2 3 1 3"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{x12, x34, x23, x13}));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, runs_on_general_circuit, {
    auto circuit = generate_test_circuit_with_all_operations();
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 1);
    sim.safe_do_circuit(circuit);
    ASSERT_GT(sim.inv_state.xs.num_qubits, 1);
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, heralded_erase, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 1);
    sim.safe_do_circuit(Circuit(R"CIRCUIT(
        HERALDED_ERASE(0) 0 1 2 3 10 11 12 13
    )CIRCUIT"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{0, 0, 0, 0, 0, 0, 0, 0}));
    sim.measurement_record.storage.clear();
    ASSERT_EQ(sim.inv_state, Tableau<W>(14));

    sim.safe_do_circuit(Circuit(R"CIRCUIT(
        HERALDED_ERASE(1) 0 1 2 3 4 5 6 10 11 12 13
    )CIRCUIT"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}));
    ASSERT_NE(sim.inv_state, Tableau<W>(14));
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, heralded_pauli_channel_1, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 1);
    Tableau<W> expected(14);

    sim.safe_do_circuit(Circuit(R"CIRCUIT(
        HERALDED_PAULI_CHANNEL_1(0, 0, 0, 0) 0 1 2 3 10 11 12 13
    )CIRCUIT"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{0, 0, 0, 0, 0, 0, 0, 0}));
    ASSERT_EQ(sim.inv_state, Tableau<W>(14));
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit(R"CIRCUIT(
        HERALDED_PAULI_CHANNEL_1(1, 0, 0, 0) 0 1 2 3 4 5 6 10 11 12 13
    )CIRCUIT"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}));
    ASSERT_EQ(sim.inv_state, Tableau<W>(14));
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit(R"CIRCUIT(
        HERALDED_PAULI_CHANNEL_1(0, 1, 0, 0) 13
    )CIRCUIT"));
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{true}));
    expected.prepend_X(13);
    ASSERT_EQ(sim.inv_state, expected);
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit(R"CIRCUIT(
        HERALDED_PAULI_CHANNEL_1(0, 0, 1, 0) 5 10
    )CIRCUIT"));
    expected.prepend_Y(5);
    expected.prepend_Y(10);
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{1, 1}));
    ASSERT_EQ(sim.inv_state, expected);
    sim.measurement_record.storage.clear();

    sim.safe_do_circuit(Circuit(R"CIRCUIT(
        HERALDED_PAULI_CHANNEL_1(0, 0, 0, 1) 1 10 11
    )CIRCUIT"));
    expected.prepend_Z(1);
    expected.prepend_Z(10);
    expected.prepend_Z(11);
    ASSERT_EQ(sim.measurement_record.storage, (std::vector<bool>{1, 1, 1}));
    ASSERT_EQ(sim.inv_state, expected);
    sim.measurement_record.storage.clear();
})

TEST_EACH_WORD_SIZE_W(TableauSimulator, postselect_observable, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 0);
    sim.postselect_observable(PauliString<W>("ZZ"), false);
    sim.postselect_observable(PauliString<W>("XX"), false);

    auto initial_state = sim.inv_state;
    ASSERT_THROW({ sim.postselect_observable(PauliString<W>("YY"), false); }, std::invalid_argument);

    sim.postselect_observable(PauliString<W>("ZZ"), false);
    ASSERT_EQ(sim.inv_state, initial_state);

    sim.postselect_observable(PauliString<W>("ZZ"), false);
    ASSERT_EQ(sim.inv_state, initial_state);

    ASSERT_THROW({ sim.postselect_observable(PauliString<W>("ZZ"), true); }, std::invalid_argument);
    ASSERT_EQ(sim.inv_state, initial_state);

    ASSERT_THROW({ sim.postselect_observable(PauliString<W>("ZZ"), true); }, std::invalid_argument);
    ASSERT_EQ(sim.inv_state, initial_state);

    sim.postselect_observable(PauliString<W>("ZZ"), false);
    ASSERT_EQ(sim.inv_state, initial_state);

    sim.postselect_observable(PauliString<W>("XX"), false);
    ASSERT_EQ(sim.inv_state, initial_state);

    sim.postselect_observable(PauliString<W>("-YY"), false);
    ASSERT_EQ(sim.inv_state, initial_state);

    sim.postselect_observable(PauliString<W>("YY"), true);
    ASSERT_EQ(sim.inv_state, initial_state);

    sim.postselect_observable(PauliString<W>("XZ"), true);
    ASSERT_NE(sim.inv_state, initial_state);
})
