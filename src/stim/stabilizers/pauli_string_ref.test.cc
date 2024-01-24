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
#include "stim/test_util.test.h"

using namespace stim;

template <size_t W>
void check_pauli_string_do_instruction_agrees_with_tableau_sim(Gate gate, TableauSimulator<W> &sim) {
    std::vector<GateTarget> targets{
        GateTarget::qubit(3),
        GateTarget::qubit(5),
        GateTarget::qubit(8),
        GateTarget::qubit(5),
    };
    CircuitInstruction inst{gate.id, {}, targets};

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
        if (gate.flags & GATE_IS_UNITARY) {
            check_pauli_string_do_instruction_agrees_with_tableau_sim<W>(gate, sim);
        }
    }
})
