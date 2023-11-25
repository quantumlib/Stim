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

#include "stim/circuit/gate_data.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.h"
#include "stim/circuit/stabilizer_flow.h"
#include "stim/mem/simd_word.test.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/test_util.test.h"

using namespace stim;

TEST(gate_data, lookup) {
    ASSERT_TRUE(GATE_DATA.has("H"));
    ASSERT_FALSE(GATE_DATA.has("H2345"));
    ASSERT_EQ(GATE_DATA.at("H").id, GATE_DATA.at("H_XZ").id);
    ASSERT_NE(GATE_DATA.at("H").id, GATE_DATA.at("H_XY").id);
    ASSERT_THROW(GATE_DATA.at("MISSING"), std::out_of_range);

    ASSERT_TRUE(GATE_DATA.has("h"));
    ASSERT_TRUE(GATE_DATA.has("Cnot"));

    ASSERT_TRUE(GATE_DATA.at("h").id == GATE_DATA.at("H").id);
    ASSERT_TRUE(GATE_DATA.at("H_xz").id == GATE_DATA.at("H").id);
}

TEST(gate_data, zero_flag_means_not_a_gate) {
    ASSERT_EQ((GateType)0, GateType::NOT_A_GATE);
    ASSERT_EQ(GATE_DATA[(GateType)0].id, (GateType)0);
    ASSERT_EQ(GATE_DATA[(GateType)0].flags, GateFlags::NO_GATE_FLAG);
    for (size_t k = 0; k < GATE_DATA.items.size(); k++) {
        const auto &g = GATE_DATA[(GateType)k];
        if (g.id != GateType::NOT_A_GATE) {
            EXPECT_NE(g.flags, GateFlags::NO_GATE_FLAG) << g.name;
        }
    }
}

TEST(gate_data, one_step_to_canonical_gate) {
    for (size_t k = 0; k < GATE_DATA.items.size(); k++) {
        const auto &g = GATE_DATA[(GateType)k];
        if (g.id != GateType::NOT_A_GATE) {
            EXPECT_TRUE(g.id == (GateType)k || GATE_DATA[g.id].id == g.id) << g.name;
        }
    }
}

TEST(gate_data, hash_matches_storage_location) {
    ASSERT_EQ((GateType)0, GateType::NOT_A_GATE);
    ASSERT_EQ(GATE_DATA[(GateType)0].id, (GateType)0);
    ASSERT_EQ(GATE_DATA[(GateType)0].flags, GateFlags::NO_GATE_FLAG);
    for (size_t k = 0; k < GATE_DATA.items.size(); k++) {
        const auto &g = GATE_DATA[(GateType)k];
        EXPECT_EQ(g.id, (GateType)k) << g.name;
        if (g.id != GateType::NOT_A_GATE) {
            EXPECT_EQ(GATE_DATA.hashed_name_to_gate_type_table[gate_name_to_hash(g.name)].id, g.id) << g.name;
        }
    }
}

template <size_t W>
std::pair<std::vector<PauliString<W>>, std::vector<PauliString<W>>> circuit_output_eq_val(const Circuit &circuit) {
    if (circuit.count_measurements() > 1) {
        throw std::invalid_argument("count_measurements > 1");
    }
    TableauSimulator<W> sim1(INDEPENDENT_TEST_RNG(), circuit.count_qubits(), -1);
    TableauSimulator<W> sim2(INDEPENDENT_TEST_RNG(), circuit.count_qubits(), +1);
    sim1.expand_do_circuit(circuit);
    sim2.expand_do_circuit(circuit);
    return {sim1.canonical_stabilizers(), sim2.canonical_stabilizers()};
}

template <size_t W>
bool is_decomposition_correct(const Gate &gate) {
    const char *decomposition = gate.h_s_cx_m_r_decomposition;
    if (decomposition == nullptr) {
        return false;
    }

    std::vector<uint32_t> qs{0};
    if (gate.flags & GATE_TARGETS_PAIRS) {
        qs.push_back(1);
    }

    Circuit epr;
    epr.safe_append_u("H", qs);
    for (auto q : qs) {
        epr.safe_append_u("CNOT", {q, q + 2});
    }

    Circuit circuit1 = epr;
    circuit1.safe_append_u(gate.name, qs);
    Circuit circuit2 = epr + Circuit(decomposition);

    // Reset gates make the ancillary qubits irrelevant because the final value is unrelated to the initial value.
    // So, for reset gates, discard the ancillary qubits.
    // CAUTION: this could give false positives if "partial reset" gates are added in the future.
    //          (E.g. a two qubit gate that resets only one of the qubits.)
    if ((gate.flags & GATE_IS_RESET) && !(gate.flags & GATE_PRODUCES_RESULTS)) {
        for (auto q : qs) {
            circuit1.safe_append_u("R", {q + 2});
            circuit2.safe_append_u("R", {q + 2});
        }
    }

    for (const auto &op : circuit2.operations) {
        if (op.gate_type != GateType::CX && op.gate_type != GateType::H && op.gate_type != GateType::S &&
            op.gate_type != GateType::M && op.gate_type != GateType::R) {
            return false;
        }
    }

    auto v1 = circuit_output_eq_val<W>(circuit1);
    auto v2 = circuit_output_eq_val<W>(circuit2);
    return v1 == v2;
}

TEST_EACH_WORD_SIZE_W(gate_data, decompositions_are_correct, {
    for (const auto &g : GATE_DATA.items) {
        if (g.flags & GATE_IS_UNITARY) {
            EXPECT_TRUE(g.h_s_cx_m_r_decomposition != nullptr) << g.name;
        }
        if (g.h_s_cx_m_r_decomposition != nullptr && g.id != GateType::MPP) {
            EXPECT_TRUE(is_decomposition_correct<W>(g)) << g.name;
        }
    }
})

TEST_EACH_WORD_SIZE_W(gate_data, unitary_inverses_are_correct, {
    for (const auto &g : GATE_DATA.items) {
        if (g.flags & GATE_IS_UNITARY) {
            auto g_t_inv = g.tableau<W>().inverse(false);
            auto g_inv_t = GATE_DATA[g.best_candidate_inverse_id].tableau<W>();
            EXPECT_EQ(g_t_inv, g_inv_t) << g.name;
        }
    }
})

TEST_EACH_WORD_SIZE_W(gate_data, stabilizer_flows_are_correct, {
    for (const auto &g : GATE_DATA.items) {
        auto flows = g.flows<W>();
        if (flows.empty()) {
            continue;
        }
        std::vector<GateTarget> targets;
        if (g.id == GateType::MPP) {
            targets.push_back(GateTarget::x(0));
            targets.push_back(GateTarget::combiner());
            targets.push_back(GateTarget::y(1));
            targets.push_back(GateTarget::combiner());
            targets.push_back(GateTarget::z(2));
            targets.push_back(GateTarget::x(3));
            targets.push_back(GateTarget::combiner());
            targets.push_back(GateTarget::x(4));
        } else {
            targets.push_back(GateTarget::qubit(0));
            if (g.flags & GATE_TARGETS_PAIRS) {
                targets.push_back(GateTarget::qubit(1));
            }
        }

        Circuit c;
        c.safe_append(g.id, targets, {});
        auto rng = INDEPENDENT_TEST_RNG();
        auto r = sample_if_circuit_has_stabilizer_flows<W>(256, rng, c, flows);
        for (uint32_t fk = 0; fk < (uint32_t)flows.size(); fk++) {
            EXPECT_TRUE(r[fk]) << "gate " << g.name << " has an unsatisfied flow: " << flows[fk];
        }
    }
})

TEST_EACH_WORD_SIZE_W(gate_data, stabilizer_flows_are_also_correct_for_decomposed_circuit, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (const auto &g : GATE_DATA.items) {
        auto flows = g.flows<W>();
        if (flows.empty()) {
            continue;
        }
        std::vector<GateTarget> targets;
        if (g.id == GateType::MPP) {
            targets.push_back(GateTarget::x(0));
            targets.push_back(GateTarget::combiner());
            targets.push_back(GateTarget::y(1));
            targets.push_back(GateTarget::combiner());
            targets.push_back(GateTarget::z(2));
            targets.push_back(GateTarget::x(3));
            targets.push_back(GateTarget::combiner());
            targets.push_back(GateTarget::x(4));
        } else {
            targets.push_back(GateTarget::qubit(0));
            if (g.flags & GATE_TARGETS_PAIRS) {
                targets.push_back(GateTarget::qubit(1));
            }
        }

        Circuit c(g.h_s_cx_m_r_decomposition);
        auto r = sample_if_circuit_has_stabilizer_flows<W>(256, rng, c, flows);
        for (uint32_t fk = 0; fk < (uint32_t)flows.size(); fk++) {
            EXPECT_TRUE(r[fk]) << "gate " << g.name << " has a decomposition with an unsatisfied flow: " << flows[fk];
        }
    }
})

std::array<std::complex<float>, 4> canonicalize_global_phase(std::array<std::complex<float>, 4> v) {
    for (std::complex<float> pivot : v) {
        if (std::abs(pivot) > 1e-5) {
            for (auto &t : v) {
                t /= pivot;
            }
            return v;
        }
    }
    return v;
}

void expect_unitaries_close_up_global_phase(
    Gate g, std::array<std::complex<float>, 4> u1, std::array<std::complex<float>, 4> u2) {
    u1 = canonicalize_global_phase(u1);
    u2 = canonicalize_global_phase(u2);
    for (size_t k = 0; k < 4; k++) {
        if (std::abs(u1[k] - u2[k]) > 1e-5) {
            std::stringstream out;
            out << g.name << ":\n";
            for (size_t k2 = 0; k2 < 4; k2++) {
                out << "    " << u1[k2] << " vs " << u2[k2] << "\n";
            }
            EXPECT_EQ(u1, u2) << out.str() << "\n" << comma_sep(g.to_euler_angles(), ",");
            return;
        }
    }
    EXPECT_TRUE(true);
}

std::array<std::complex<float>, 4> reconstruct_unitary_from_euler_angles(Gate g) {
    auto xyz = g.to_euler_angles();
    auto c = cosf(xyz[0] / 2);
    auto s = sinf(xyz[0] / 2);
    return {
        c,
        -s * std::exp(std::complex<float>{0, xyz[2]}),
        s * std::exp(std::complex<float>{0, xyz[1]}),
        c * std::exp(std::complex<float>{0, xyz[1] + xyz[2]}),
    };
}

std::array<std::complex<float>, 4> reconstruct_unitary_from_data(Gate g) {
    return {
        g.unitary_data[0][0],
        g.unitary_data[0][1],
        g.unitary_data[1][0],
        g.unitary_data[1][1],
    };
}

std::array<std::complex<float>, 4> reconstruct_unitary_from_axis_angle(Gate g) {
    auto xyz_a = g.to_axis_angle();
    auto x = xyz_a[0];
    auto y = xyz_a[1];
    auto z = xyz_a[2];
    auto a = xyz_a[3];
    auto c = cosf(a / 2);
    auto s = -sinf(a / 2);
    return {
        std::complex<float>{c, s * z},
        std::complex<float>{s * y, s * x},
        std::complex<float>{-s * y, s * x},
        std::complex<float>{c, -s * z},
    };
}

std::array<std::complex<float>, 4> reconstruct_unitary_from_euler_angles_via_vector_sim_for_axis_reference(Gate g) {
    auto xyz = g.to_euler_angles();
    std::array<int, 3> half_turns;

    for (size_t k = 0; k < 3; k++) {
        half_turns[k] = (int)(roundf(xyz[k] / 3.14159265359f * 2)) & 3;
    }
    std::array<const char *, 4> y_rots{"I", "SQRT_Y", "Y", "SQRT_Y_DAG"};
    std::array<const char *, 4> z_rots{"I", "S", "Z", "S_DAG"};

    // Recover the unitary matrix via the state channel duality.
    Circuit c;
    c.safe_append_u("H", {0});
    c.safe_append_u("CX", {0, 1});
    c.safe_append_u(z_rots[half_turns[2]], {1});
    c.safe_append_u(y_rots[half_turns[0]], {1});
    c.safe_append_u(z_rots[half_turns[1]], {1});
    VectorSimulator v(2);
    v.do_unitary_circuit(c);

    return {v.state[0], v.state[1], v.state[2], v.state[3]};
}

TEST(gate_data, to_euler_angles) {
    for (const auto &g : GATE_DATA.items) {
        if ((g.flags & GATE_IS_UNITARY) && (g.flags & GATE_IS_SINGLE_QUBIT_GATE)) {
            auto u1 = reconstruct_unitary_from_data(g);
            auto u2 = reconstruct_unitary_from_euler_angles(g);
            expect_unitaries_close_up_global_phase(g, u1, u2);
        }
    }
}

TEST(gate_data, to_axis_angle) {
    for (const auto &g : GATE_DATA.items) {
        if ((g.flags & GATE_IS_UNITARY) && (g.flags & GATE_IS_SINGLE_QUBIT_GATE)) {
            auto u1 = reconstruct_unitary_from_data(g);
            auto u2 = reconstruct_unitary_from_axis_angle(g);
            expect_unitaries_close_up_global_phase(g, u1, u2);
        }
    }
}

TEST(gate_data, to_euler_angles_axis_reference) {
    for (const auto &g : GATE_DATA.items) {
        if ((g.flags & GATE_IS_UNITARY) && (g.flags & GATE_IS_SINGLE_QUBIT_GATE)) {
            auto u1 = reconstruct_unitary_from_data(g);
            auto u2 = reconstruct_unitary_from_euler_angles_via_vector_sim_for_axis_reference(g);
            expect_unitaries_close_up_global_phase(g, u1, u2);
        }
    }
}
