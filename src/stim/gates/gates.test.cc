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

#include "stim/gates/gates.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.h"
#include "stim/cmd/command_help.h"
#include "stim/mem/simd_word.test.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/util_bot/str_util.h"
#include "stim/util_bot/test_util.test.h"
#include "stim/util_top/circuit_flow_generators.h"
#include "stim/util_top/has_flow.h"

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
static std::pair<std::vector<PauliString<W>>, std::vector<PauliString<W>>> circuit_output_eq_val(
    const Circuit &circuit) {
    // CAUTION: this is not 100% reliable when measurement count is larger than 1.
    TableauSimulator<W> sim1(INDEPENDENT_TEST_RNG(), circuit.count_qubits(), -1);
    TableauSimulator<W> sim2(INDEPENDENT_TEST_RNG(), circuit.count_qubits(), +1);
    sim1.safe_do_circuit(circuit);
    sim2.safe_do_circuit(circuit);
    return {sim1.canonical_stabilizers(), sim2.canonical_stabilizers()};
}

template <size_t W>
bool is_decomposition_correct(const Gate &gate) {
    const char *decomposition = gate.h_s_cx_m_r_decomposition;
    if (decomposition == nullptr) {
        return false;
    }

    Circuit original;
    original.safe_append(CircuitInstruction(gate.id, {}, gate_decomposition_help_targets_for_gate_type(gate.id), ""));
    uint32_t n = original.count_qubits();

    Circuit epr;
    for (uint32_t q = 0; q < n; q++) {
        epr.safe_append_u("H", {q});
    }
    for (uint32_t q = 0; q < n; q++) {
        epr.safe_append_u("CNOT", {q, q + n});
    }

    Circuit circuit1 = epr + original;
    Circuit circuit2 = epr + Circuit(decomposition);

    // Reset gates make the ancillary qubits irrelevant because the final value is unrelated to the initial value.
    // So, for reset gates, discard the ancillary qubits.
    // CAUTION: this could give false positives if "partial reset" gates are added in the future.
    //          (E.g. a two qubit gate that resets only one of the qubits.)
    if ((gate.flags & GATE_IS_RESET) && !(gate.flags & GATE_PRODUCES_RESULTS)) {
        for (uint32_t q = 0; q < n; q++) {
            circuit1.safe_append_u("R", {q + n});
            circuit2.safe_append_u("R", {q + n});
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
        if (g.h_s_cx_m_r_decomposition != nullptr) {
            EXPECT_TRUE(is_decomposition_correct<W>(g)) << g.name;
        }
    }
})

TEST_EACH_WORD_SIZE_W(gate_data, unitary_inverses_are_correct, {
    for (const auto &g : GATE_DATA.items) {
        if (g.has_known_unitary_matrix()) {
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
        std::vector<GateTarget> targets = gate_decomposition_help_targets_for_gate_type(g.id);

        Circuit c;
        c.safe_append(CircuitInstruction(g.id, {}, targets, ""));
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
        std::vector<GateTarget> targets = gate_decomposition_help_targets_for_gate_type(g.id);

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

TEST(gate_data, is_symmetric_vs_flow_generators_of_two_qubit_gates) {
    for (const auto &g : GATE_DATA.items) {
        if ((g.flags & stim::GATE_IS_NOISY) && !(g.flags & stim::GATE_PRODUCES_RESULTS)) {
            continue;
        }
        if (g.flags & GATE_TARGETS_PAIRS) {
            Circuit c1;
            Circuit c2;
            c1.safe_append_u(g.name, {0, 1}, {});
            c2.safe_append_u(g.name, {1, 0}, {});
            auto f1 = circuit_flow_generators<64>(c1);
            auto f2 = circuit_flow_generators<64>(c2);
            EXPECT_EQ(g.is_symmetric(), f1 == f2) << g.name;
        }
    }
}

TEST(gate_data, hadamard_conjugated_vs_flow_generators_of_two_qubit_gates) {
    auto flow_key = [](const Circuit &circuit, bool ignore_sign) {
        auto f = circuit_flow_generators<64>(circuit);
        if (ignore_sign) {
            for (auto &e : f) {
                e.input.sign = false;
                e.output.sign = false;
            }
        }
        std::stringstream ss;
        ss << comma_sep(f);
        return ss.str();
    };
    std::map<std::string, GateType> known_flows_s;
    std::map<std::string, std::vector<GateType>> known_flows_u;

    for (const auto &g : GATE_DATA.items) {
        if (g.id == GateType::II) {
            ASSERT_EQ(g.hadamard_conjugated(false), g.id);
            ASSERT_EQ(g.hadamard_conjugated(true), g.id);
            continue;
        }
        if (g.arg_count != 0 && g.arg_count != ARG_COUNT_SYGIL_ZERO_OR_ONE && g.arg_count != ARG_COUNT_SYGIL_ANY) {
            continue;
        }
        if ((g.flags & GATE_TARGETS_PAIRS) || (g.flags & GATE_IS_SINGLE_QUBIT_GATE)) {
            Circuit c;
            c.safe_append_u(g.name, {0, 1}, {});
            auto key_s = flow_key(c, false);
            auto key_u = flow_key(c, true);
            ASSERT_EQ(known_flows_s.find(key_s), known_flows_s.end()) << "collision between " << g.name << " and " << GATE_DATA[known_flows_s[key_s]].name;
            known_flows_s[key_s] = g.id;
            known_flows_u[key_u].push_back(g.id);
        }
    }
    for (const auto &g : GATE_DATA.items) {
        if (g.id == GateType::II) {
            continue;
        }
        if (g.arg_count != 0 && g.arg_count != ARG_COUNT_SYGIL_ZERO_OR_ONE && g.arg_count != ARG_COUNT_SYGIL_ANY) {
            continue;
        }
        if ((g.flags & GATE_TARGETS_PAIRS) || (g.flags & GATE_IS_SINGLE_QUBIT_GATE)) {
            Circuit c;
            c.safe_append_u("H", {0, 1}, {});
            c.safe_append_u(g.name, {0, 1}, {});
            c.safe_append_u("H", {0, 1}, {});
            auto key_s = flow_key(c, false);
            auto key_u = flow_key(c, true);
            auto other_s = known_flows_s.find(key_s);
            auto &other_us = known_flows_u[key_u];
            if (other_us.empty()) {
                other_us.push_back(GateType::NOT_A_GATE);
            }

            GateType expected_s = other_s == known_flows_s.end() ? GateType::NOT_A_GATE : other_s->second;
            GateType actual_s = g.hadamard_conjugated(false);
            GateType actual_u = g.hadamard_conjugated(true);
            bool found = std::find(other_us.begin(), other_us.end(), actual_u) != other_us.end();
            EXPECT_EQ(actual_s, expected_s)
                << "signed " << g.name << " -> " << GATE_DATA[actual_s].name << " != " << GATE_DATA[expected_s].name;
            EXPECT_TRUE(found) << "unsigned " << g.name << " -> " << GATE_DATA[actual_u].name << " not in "
                               << GATE_DATA[other_us[0]].name;
        }
    }
}
