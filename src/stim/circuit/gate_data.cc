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

#include <complex>

#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

GateDataMap::GateDataMap() {
    bool failed = false;
    add_gate_data_annotations(failed);
    add_gate_data_blocks(failed);
    add_gate_data_collapsing(failed);
    add_gate_data_controlled(failed);
    add_gate_data_hada(failed);
    add_gate_data_noisy(failed);
    add_gate_data_pauli(failed);
    add_gate_data_period_3(failed);
    add_gate_data_period_4(failed);
    add_gate_data_pp(failed);
    add_gate_data_swaps(failed);
    if (failed) {
        throw std::out_of_range("Failed to initialize gate data.");
    }
}

std::vector<std::vector<std::complex<float>>> Gate::unitary() const {
    const auto &unitary_data = extra_data_func().unitary_data;
    if (unitary_data.size() != 2 && unitary_data.size() != 4) {
        throw std::out_of_range(std::string(name) + " doesn't have 1q or 2q unitary data.");
    }
    std::vector<std::vector<std::complex<float>>> result;
    for (size_t k = 0; k < unitary_data.size(); k++) {
        const auto &d = unitary_data[k];
        result.emplace_back();
        for (size_t j = 0; j < d.size(); j++) {
            result.back().push_back(d[j]);
        }
    }
    return result;
}

const Gate &Gate::inverse() const {
    std::string inv_name = name;
    if ((flags & GATE_IS_UNITARY) || id == GateType::TICK) {
        return GATE_DATA.items[static_cast<uint8_t>(best_candidate_inverse_id)];
    }
    throw std::out_of_range(inv_name + " has no inverse.");
}

Gate::Gate() : name(nullptr) {
}

Gate::Gate(
    const char *name,
    GateType gate_id,
    GateType best_inverse_gate,
    uint8_t arg_count,
    GateFlags flags,
    ExtraGateData (*extra_data_func)(void))
    : name(name),
      extra_data_func(extra_data_func),
      flags(flags),
      arg_count(arg_count),
      name_len((uint8_t)strlen(name)),
      id(gate_id),
      best_candidate_inverse_id(best_inverse_gate) {
}

void GateDataMap::add_gate(bool &failed, const Gate &gate) {
    const char *c = gate.name;
    uint8_t h = gate_name_to_hash(c);
    Gate &loc = items[h];
    if (loc.name != nullptr) {
        std::cerr << "GATE COLLISION " << gate.name << " vs " << loc.name << "\n";
        failed = true;
        return;
    }
    loc = gate;
}

void GateDataMap::add_gate_alias(bool &failed, const char *alt_name, const char *canon_name) {
    uint8_t h_alt = gate_name_to_hash(alt_name);
    Gate &g_alt = items[h_alt];
    if (g_alt.name != nullptr) {
        std::cerr << "GATE COLLISION " << alt_name << " vs " << g_alt.name << "\n";
        failed = true;
        return;
    }

    uint8_t h_canon = gate_name_to_hash(canon_name);
    Gate &g_canon = items[h_canon];
    if (g_canon.name == nullptr || static_cast<uint8_t>(g_canon.id) != h_canon) {
        std::cerr << "MISSING CANONICAL GATE " << canon_name << "\n";
        failed = true;
        return;
    }
    g_alt.name = alt_name;
    g_alt.name_len = (uint8_t)strlen(alt_name);
    g_alt.id = g_canon.id;
    g_alt.flags = g_canon.flags;
}

std::vector<Gate> GateDataMap::gates(bool include_aliases) const {
    std::vector<Gate> result;
    for (size_t k = 0; k < items.size(); k++) {
        if (items[k].name != nullptr && (include_aliases || items[k].id == k)) {
            result.push_back(items[k]);
        }
    }
    return result;
}

ExtraGateData::ExtraGateData(
    const char *category,
    const char *help,
    FixedCapVector<FixedCapVector<std::complex<float>, 4>, 4> unitary_data,
    FixedCapVector<const char *, 4> tableau_data,
    const char *h_s_cx_m_r_decomposition)
    : category(category),
      help(help),
      unitary_data(unitary_data),
      tableau_data(tableau_data),
      h_s_cx_m_r_decomposition(h_s_cx_m_r_decomposition) {
}

extern const GateDataMap stim::GATE_DATA = GateDataMap();
