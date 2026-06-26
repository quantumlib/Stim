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

using namespace stim;

GateDataMap::GateDataMap() {
    bool failed = false;
    items[0].name = "NOT_A_GATE";

    add_gate_data_collapsing(failed);
    add_gate_data_controlled(failed);
    add_gate_data_hada(failed);
    add_gate_data_heralded(failed);
    add_gate_data_pauli(failed);
    add_gate_data_period_4(failed);
    add_gate_data_pauli_product(failed);
    for (size_t k = 1; k < NUM_DEFINED_GATES; k++) {
        if (items[k].name.empty()) {
            std::cerr << "Uninitialized gate id: " << k << ".\n";
            failed = true;
        }
    }
}

void GateDataMap::add_gate(bool &failed, const Gate &gate) {
    assert((size_t)gate.id < NUM_DEFINED_GATES);
    auto h = gate_name_to_hash(gate.name);
    auto &hash_loc = hashed_name_to_gate_type_table[h];
    if (!hash_loc.expected_name.empty()) {
        std::cerr << "GATE COLLISION " << gate.name << " vs " << items[(size_t)hash_loc.id].name << "\n";
        failed = true;
        return;
    }
    items[(size_t)gate.id] = gate;
    hash_loc.id = gate.id;
    hash_loc.expected_name = gate.name;
}

void GateDataMap::add_gate_alias(bool &failed, const char *alt_name, const char *canon_name) {
    auto h_alt = gate_name_to_hash(alt_name);
    auto &hash_loc = hashed_name_to_gate_type_table[h_alt];
    if (!hash_loc.expected_name.empty()) {
        std::cerr << "GATE COLLISION " << alt_name << " vs " << items[(size_t)hash_loc.id].name << "\n";
        failed = true;
        return;
    }

    auto h_canon = gate_name_to_hash(canon_name);
    if (hashed_name_to_gate_type_table[h_canon].expected_name.empty()) {
        std::cerr << "MISSING CANONICAL GATE " << canon_name << "\n";
        failed = true;
        return;
    }

    hash_loc.id = hashed_name_to_gate_type_table[h_canon].id;
    hash_loc.expected_name = alt_name;
}

extern const GateDataMap stim::GATE_DATA = GateDataMap();
