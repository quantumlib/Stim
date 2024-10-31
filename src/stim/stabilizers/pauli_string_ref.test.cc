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

#include "stim/stabilizers/pauli_string_ref.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

template <size_t W>
void check_pauli_string_do_instruction_agrees_with_tableau_sim(Gate gate, TableauSimulator<W> &sim) {
    std::vector<GateTarget> targets{
        GateTarget::qubit(3),
        GateTarget::qubit(5),
        GateTarget::qubit(8),
        GateTarget::qubit(5),
    };
    CircuitInstruction inst{gate.id, {}, targets, ""};

    std::vector<PauliString<W>> before;
    for (size_t k = 0; k < 16; k++) {
        sim.inv_state.zs[k].sign = false;
        before.push_back(sim.inv_state.zs[k]);
    }
    auto tableau = gate.tableau<W>();
    if (gate.flags & GATE_TARGETS_PAIRS) {
        sim.inv_state.inplace_scatter_append(tableau, {3, 5});
        sim.inv_state.inplace_scatter_append(tableau, {8, 5});
    } else {
        sim.inv_state.inplace_scatter_append(tableau, {3});
        sim.inv_state.inplace_scatter_append(tableau, {5});
        sim.inv_state.inplace_scatter_append(tableau, {8});
        sim.inv_state.inplace_scatter_append(tableau, {5});
    }

    PauliString<W> v(0);
    for (size_t k = 0; k < before.size(); k++) {
        v = before[k];
        v.ref().do_instruction(inst);
        if (v != sim.inv_state.zs[k]) {
            EXPECT_EQ(v, sim.inv_state.zs[k]) << "do_" << gate.name << "\nbefore=" << before[k];
            return;
        }

        v.ref().undo_instruction(inst);
        if (v != before[k]) {
            EXPECT_EQ(v, sim.inv_state.zs[k]) << "undo_" << gate.name;
            return;
        }
    }
}

TEST_EACH_WORD_SIZE_W(pauli_string, do_instruction_agrees_with_tableau_sim, {
    TableauSimulator<W> sim(INDEPENDENT_TEST_RNG(), 16);
    sim.inv_state = Tableau<W>::random(sim.inv_state.num_qubits, sim.rng);

    for (const auto &gate : GATE_DATA.items) {
        if (gate.has_known_unitary_matrix()) {
            check_pauli_string_do_instruction_agrees_with_tableau_sim<W>(gate, sim);
        }
    }
})

TEST_EACH_WORD_SIZE_W(pauli_string, intersects, {
    ASSERT_FALSE((PauliString<W>("_").ref().intersects(PauliString<W>("_"))));
    ASSERT_FALSE((PauliString<W>("_").ref().intersects(PauliString<W>("X"))));
    ASSERT_FALSE((PauliString<W>("_").ref().intersects(PauliString<W>("Y"))));
    ASSERT_FALSE((PauliString<W>("_").ref().intersects(PauliString<W>("Z"))));
    ASSERT_FALSE((PauliString<W>("X").ref().intersects(PauliString<W>("_"))));
    ASSERT_TRUE((PauliString<W>("X").ref().intersects(PauliString<W>("X"))));
    ASSERT_TRUE((PauliString<W>("X").ref().intersects(PauliString<W>("Y"))));
    ASSERT_TRUE((PauliString<W>("X").ref().intersects(PauliString<W>("Z"))));
    ASSERT_FALSE((PauliString<W>("Y").ref().intersects(PauliString<W>("_"))));
    ASSERT_TRUE((PauliString<W>("Y").ref().intersects(PauliString<W>("X"))));
    ASSERT_TRUE((PauliString<W>("Y").ref().intersects(PauliString<W>("Y"))));
    ASSERT_TRUE((PauliString<W>("Y").ref().intersects(PauliString<W>("Z"))));
    ASSERT_FALSE((PauliString<W>("Z").ref().intersects(PauliString<W>("_"))));
    ASSERT_TRUE((PauliString<W>("Z").ref().intersects(PauliString<W>("X"))));
    ASSERT_TRUE((PauliString<W>("Z").ref().intersects(PauliString<W>("Y"))));
    ASSERT_TRUE((PauliString<W>("Z").ref().intersects(PauliString<W>("Z"))));

    ASSERT_TRUE((PauliString<W>("_Z").ref().intersects(PauliString<W>("ZZ"))));
    ASSERT_TRUE((PauliString<W>("Z_").ref().intersects(PauliString<W>("ZZ"))));
    ASSERT_TRUE((PauliString<W>("ZZ").ref().intersects(PauliString<W>("ZZ"))));
    ASSERT_FALSE((PauliString<W>("ZZ").ref().intersects(PauliString<W>("__"))));
    ASSERT_FALSE((PauliString<W>("__").ref().intersects(PauliString<W>("XZ"))));
    ASSERT_FALSE((PauliString<W>("________________________________________________")
                      .ref()
                      .intersects(PauliString<W>("________________________________________________"))));
    ASSERT_TRUE((PauliString<W>("_______________________________________X________")
                     .ref()
                     .intersects(PauliString<W>("_______________________________________X________"))));
})

TEST_EACH_WORD_SIZE_W(pauli_string, weight, {
    ASSERT_EQ(PauliString<W>::from_str("+").ref().weight(), 0);
    ASSERT_EQ(PauliString<W>::from_str("+I").ref().weight(), 0);
    ASSERT_EQ(PauliString<W>::from_str("+X").ref().weight(), 1);
    ASSERT_EQ(PauliString<W>::from_str("+Y").ref().weight(), 1);
    ASSERT_EQ(PauliString<W>::from_str("+Z").ref().weight(), 1);

    ASSERT_EQ(PauliString<W>::from_str("+IX").ref().weight(), 1);
    ASSERT_EQ(PauliString<W>::from_str("+XZ").ref().weight(), 2);
    ASSERT_EQ(PauliString<W>::from_str("+YY").ref().weight(), 2);
    ASSERT_EQ(PauliString<W>::from_str("+XI").ref().weight(), 1);

    PauliString<W> p(1000);
    ASSERT_EQ(p.ref().weight(), 0);
    for (size_t k = 0; k < 1000; k++) {
        p.xs[k] = k % 3 == 1;
        p.zs[k] = k % 5 == 1;
    }
    ASSERT_EQ(p.ref().weight(), 333 + 199 - 66);
    p.sign = true;
    ASSERT_EQ(p.ref().weight(), 333 + 199 - 66);
})

TEST_EACH_WORD_SIZE_W(pauli_string, has_no_pauli_terms, {
    ASSERT_EQ((PauliString<W>::from_str("+").ref().has_no_pauli_terms()), true);
    ASSERT_EQ((PauliString<W>::from_str("+I").ref().has_no_pauli_terms()), true);
    ASSERT_EQ((PauliString<W>::from_str("+X").ref().has_no_pauli_terms()), false);
    ASSERT_EQ((PauliString<W>::from_str("+Y").ref().has_no_pauli_terms()), false);
    ASSERT_EQ((PauliString<W>::from_str("+Z").ref().has_no_pauli_terms()), false);

    ASSERT_EQ((PauliString<W>::from_str("+II").ref().has_no_pauli_terms()), true);
    ASSERT_EQ((PauliString<W>::from_str("+IX").ref().has_no_pauli_terms()), false);
    ASSERT_EQ((PauliString<W>::from_str("+XZ").ref().has_no_pauli_terms()), false);
    ASSERT_EQ((PauliString<W>::from_str("+YY").ref().has_no_pauli_terms()), false);
    ASSERT_EQ((PauliString<W>::from_str("+XI").ref().has_no_pauli_terms()), false);

    PauliString<W> p(1000);
    ASSERT_TRUE(p.ref().has_no_pauli_terms());
    p.xs[700] = true;
    ASSERT_FALSE(p.ref().has_no_pauli_terms());
    p.zs[700] = true;
    ASSERT_FALSE(p.ref().has_no_pauli_terms());
    p.xs[700] = false;
    ASSERT_FALSE(p.ref().has_no_pauli_terms());
})

TEST_EACH_WORD_SIZE_W(pauli_string, for_each_active_pauli, {
    auto v = PauliString<W>(500);
    v.zs[0] = true;
    v.xs[20] = true;
    v.xs[50] = true;
    v.zs[50] = true;
    v.xs[63] = true;
    v.zs[63] = true;
    v.zs[100] = true;
    v.xs[200] = true;
    v.xs[301] = true;
    v.zs[301] = true;
    std::vector<size_t> indices;
    v.ref().for_each_active_pauli([&](size_t index) {
        indices.push_back(index);
    });
    ASSERT_EQ(indices, (std::vector<size_t>{0, 20, 50, 63, 100, 200, 301}));
})
