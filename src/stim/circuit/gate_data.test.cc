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
    ASSERT_EQ(GATE_DATA.items[0].id, 0);
    ASSERT_EQ(GATE_DATA.items[0].flags, GateFlags::NO_GATE_FLAG);
    for (size_t k = 0; k < GATE_DATA.items.size(); k++) {
        const auto &g = GATE_DATA.items[k];
        if (g.id != 0) {
            EXPECT_NE(g.flags, GateFlags::NO_GATE_FLAG) << g.name;
        }
    }
}

TEST(gate_data, one_step_to_canonical_gate) {
    for (size_t k = 0; k < GATE_DATA.items.size(); k++) {
        const auto &g = GATE_DATA.items[k];
        if (g.id != 0) {
            EXPECT_TRUE(g.id == k || GATE_DATA.items[g.id].id == g.id) << g.name;
        }
    }
}

TEST(gate_data, hash_matches_storage_location) {
    ASSERT_EQ(GATE_DATA.items[0].id, 0);
    ASSERT_EQ(GATE_DATA.items[0].flags, GateFlags::NO_GATE_FLAG);
    for (size_t k = 0; k < GATE_DATA.items.size(); k++) {
        const auto &g = GATE_DATA.items[k];
        if (g.id != 0) {
            EXPECT_EQ(gate_name_to_hash(g.name), k) << g.name;
        }
    }
}

std::pair<std::vector<PauliString>, std::vector<PauliString>> circuit_output_eq_val(const Circuit &circuit) {
    if (circuit.count_measurements() > 1) {
        throw std::invalid_argument("count_measurements > 1");
    }
    TableauSimulator sim1(SHARED_TEST_RNG(), circuit.count_qubits(), -1);
    TableauSimulator sim2(SHARED_TEST_RNG(), circuit.count_qubits(), +1);
    sim1.expand_do_circuit(circuit);
    sim2.expand_do_circuit(circuit);
    return {sim1.canonical_stabilizers(), sim2.canonical_stabilizers()};
}

bool is_decomposition_correct(const Gate &gate) {
    const char *decomposition = gate.extra_data_func().h_s_cx_m_r_decomposition;
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
    auto v1 = circuit_output_eq_val(circuit1);

    Circuit circuit2 = epr + Circuit(decomposition);
    auto v2 = circuit_output_eq_val(circuit2);
    for (const auto &op : circuit2.operations) {
        if (op.gate_type != GateType::CX && op.gate_type != GateType::H && op.gate_type != GateType::S &&
            op.gate_type != GateType::M && op.gate_type != GateType::R) {
            return false;
        }
    }

    return v1 == v2;
}

TEST(gate_data, decompositions_are_correct) {
    for (const auto &g : GATE_DATA.gates()) {
        if (g.extra_data_func != nullptr) {
            auto data = g.extra_data_func();
            if (g.flags & GATE_IS_UNITARY) {
                EXPECT_TRUE(data.h_s_cx_m_r_decomposition != nullptr) << g.name;
            }
            if (data.h_s_cx_m_r_decomposition != nullptr) {
                EXPECT_TRUE(is_decomposition_correct(g)) << g.name;
            }
        }
    }
}

TEST(gate_data, unitary_inverses_are_correct) {
    for (const auto &g : GATE_DATA.gates()) {
        if (g.flags & GATE_IS_UNITARY) {
            auto g_t_inv = g.tableau().inverse(false);
            auto g_inv_t = GATE_DATA.items[static_cast<uint8_t>(g.best_candidate_inverse_id)].tableau();
            EXPECT_EQ(g_t_inv, g_inv_t) << g.name;
        }
    }
}
