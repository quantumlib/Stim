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

Tableau Gate::tableau() const {
    const auto &tableau_data = extra_data_func().tableau_data;
    const auto &d = tableau_data;
    if (tableau_data.size() == 2) {
        return Tableau::gate1(d[0], d[1]);
    }
    if (tableau_data.size() == 4) {
        return Tableau::gate2(d[0], d[1], d[2], d[3]);
    }
    throw std::out_of_range(std::string(name) + " doesn't have 1q or 2q tableau data.");
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

template <typename SIMULATOR>
constexpr GateVTable<SIMULATOR>::GateVTable(
    std::vector<std::pair<GateType, sim_func_ptr_t>> simulator_functions) {
    for (const auto [gate_type, sim_func] : simulator_functions) {
        add_simulator_function(gate_type, sim_func);
    }
}

template <typename SIMULATOR>
constexpr void GateVTable<SIMULATOR>::add_simulator_function(GateType gate_id, sim_func_ptr_t simulator_function) {
    funcs[static_cast<uint8_t>(gate_id)] = simulator_function;
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
}

std::vector<Gate> GateDataMap::gates() const {
    std::vector<Gate> result;
    for (const auto &item : items) {
        if (item.name != nullptr) {
            result.push_back(item);
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
extern const GateVTable<FrameSimulator> stim::FRAME_SIM_VTABLE = GateVTable<FrameSimulator>({
    {GateType::DETECTOR, &FrameSimulator::I},
    {GateType::OBSERVABLE_INCLUDE, &FrameSimulator::I},
    {GateType::TICK, &FrameSimulator::I},
    {GateType::QUBIT_COORDS, &FrameSimulator::I},
    {GateType::SHIFT_COORDS, &FrameSimulator::I},
    {GateType::REPEAT, &FrameSimulator::I},
    {GateType::MX, &FrameSimulator::measure_x},
    {GateType::MY, &FrameSimulator::measure_y},
    {GateType::M, &FrameSimulator::measure_z},
    {GateType::MRX, &FrameSimulator::measure_reset_x},
    {GateType::MRY, &FrameSimulator::measure_reset_y},
    {GateType::MR, &FrameSimulator::measure_reset_z},
    {GateType::RX, &FrameSimulator::reset_x},
    {GateType::RY, &FrameSimulator::reset_y},
    {GateType::R, &FrameSimulator::reset_z},
    {GateType::MPP, &FrameSimulator::MPP},
    {GateType::XCX, &FrameSimulator::XCX},
    {GateType::XCY, &FrameSimulator::XCY},
    {GateType::XCZ, &FrameSimulator::XCZ},
    {GateType::YCX, &FrameSimulator::YCX},
    {GateType::YCY, &FrameSimulator::YCY},
    {GateType::YCZ, &FrameSimulator::YCZ},
    {GateType::CX, &FrameSimulator::ZCX},
    {GateType::CY, &FrameSimulator::ZCY},
    {GateType::CZ, &FrameSimulator::ZCZ},
    {GateType::H, &FrameSimulator::H_XZ},
    {GateType::H_XY, &FrameSimulator::H_XY},
    {GateType::H_YZ, &FrameSimulator::H_YZ},
    {GateType::DEPOLARIZE1, &FrameSimulator::DEPOLARIZE1},
    {GateType::DEPOLARIZE2, &FrameSimulator::DEPOLARIZE2},
    {GateType::X_ERROR, &FrameSimulator::X_ERROR},
    {GateType::Y_ERROR, &FrameSimulator::Y_ERROR},
    {GateType::Z_ERROR, &FrameSimulator::Z_ERROR},
    {GateType::PAULI_CHANNEL_1, &FrameSimulator::PAULI_CHANNEL_1},
    {GateType::PAULI_CHANNEL_2, &FrameSimulator::PAULI_CHANNEL_2},
    {GateType::E, &FrameSimulator::CORRELATED_ERROR},
    {GateType::ELSE_CORRELATED_ERROR, &FrameSimulator::ELSE_CORRELATED_ERROR},
    {GateType::I, &FrameSimulator::I},
    {GateType::X, &FrameSimulator::I},
    {GateType::Y, &FrameSimulator::I},
    {GateType::Z, &FrameSimulator::I},
    {GateType::C_XYZ, &FrameSimulator::C_XYZ},
    {GateType::C_ZYX, &FrameSimulator::C_ZYX},
    {GateType::SQRT_X, &FrameSimulator::H_YZ},
    {GateType::SQRT_X_DAG, &FrameSimulator::H_YZ},
    {GateType::SQRT_Y, &FrameSimulator::H_XZ},
    {GateType::SQRT_Y_DAG, &FrameSimulator::H_XZ},
    {GateType::S, &FrameSimulator::H_XY},
    {GateType::S_DAG, &FrameSimulator::H_XY},
    {GateType::SQRT_XX, &FrameSimulator::SQRT_XX},
    {GateType::SQRT_XX_DAG, &FrameSimulator::SQRT_XX},
    {GateType::SQRT_YY, &FrameSimulator::SQRT_YY},
    {GateType::SQRT_YY_DAG, &FrameSimulator::SQRT_YY},
    {GateType::SQRT_ZZ, &FrameSimulator::SQRT_ZZ},
    {GateType::SQRT_ZZ_DAG, &FrameSimulator::SQRT_ZZ},
    {GateType::SWAP, &FrameSimulator::SWAP},
    {GateType::ISWAP, &FrameSimulator::ISWAP},
    {GateType::ISWAP_DAG, &FrameSimulator::ISWAP},
    {GateType::CXSWAP, &FrameSimulator::CXSWAP},
    {GateType::SWAPCX, &FrameSimulator::SWAPCX},

});

extern const GateVTable<TableauSimulator> stim::TABLEAU_SIM_VTABLE = GateVTable<TableauSimulator>({
    {GateType::DETECTOR, &TableauSimulator::I},
    {GateType::OBSERVABLE_INCLUDE, &TableauSimulator::I},
    {GateType::TICK, &TableauSimulator::I},
    {GateType::QUBIT_COORDS, &TableauSimulator::I},
    {GateType::SHIFT_COORDS, &TableauSimulator::I},
    {GateType::REPEAT, &TableauSimulator::I},
    {GateType::MX, &TableauSimulator::measure_x},
    {GateType::MY, &TableauSimulator::measure_y},
    {GateType::M, &TableauSimulator::measure_z},
    {GateType::MRX, &TableauSimulator::measure_reset_x},
    {GateType::MRY, &TableauSimulator::measure_reset_y},
    {GateType::MR, &TableauSimulator::measure_reset_z},
    {GateType::RX, &TableauSimulator::reset_x},
    {GateType::RY, &TableauSimulator::reset_y},
    {GateType::R, &TableauSimulator::reset_z},
    {GateType::MPP, &TableauSimulator::MPP},
    {GateType::XCX, &TableauSimulator::XCX},
    {GateType::XCY, &TableauSimulator::XCY},
    {GateType::XCZ, &TableauSimulator::XCZ},
    {GateType::YCX, &TableauSimulator::YCX},
    {GateType::YCY, &TableauSimulator::YCY},
    {GateType::YCZ, &TableauSimulator::YCZ},
    {GateType::CX, &TableauSimulator::ZCX},
    {GateType::CY, &TableauSimulator::ZCY},
    {GateType::CZ, &TableauSimulator::ZCZ},
    {GateType::H, &TableauSimulator::H_XZ},
    {GateType::H_XY, &TableauSimulator::H_XY},
    {GateType::H_YZ, &TableauSimulator::H_YZ},
    {GateType::DEPOLARIZE1, &TableauSimulator::DEPOLARIZE1},
    {GateType::DEPOLARIZE2, &TableauSimulator::DEPOLARIZE2},
    {GateType::X_ERROR, &TableauSimulator::X_ERROR},
    {GateType::Y_ERROR, &TableauSimulator::Y_ERROR},
    {GateType::Z_ERROR, &TableauSimulator::Z_ERROR},
    {GateType::PAULI_CHANNEL_1, &TableauSimulator::PAULI_CHANNEL_1},
    {GateType::PAULI_CHANNEL_2, &TableauSimulator::PAULI_CHANNEL_2},
    {GateType::E, &TableauSimulator::CORRELATED_ERROR},
    {GateType::ELSE_CORRELATED_ERROR, &TableauSimulator::ELSE_CORRELATED_ERROR},
    {GateType::I, &TableauSimulator::I},
    {GateType::X, &TableauSimulator::X},
    {GateType::Y, &TableauSimulator::Y},
    {GateType::Z, &TableauSimulator::Z},
    {GateType::C_XYZ, &TableauSimulator::C_XYZ},
    {GateType::C_ZYX, &TableauSimulator::C_ZYX},
    {GateType::SQRT_X, &TableauSimulator::SQRT_X},
    {GateType::SQRT_X_DAG, &TableauSimulator::SQRT_X_DAG},
    {GateType::SQRT_Y, &TableauSimulator::SQRT_Y},
    {GateType::SQRT_Y_DAG, &TableauSimulator::SQRT_Y_DAG},
    {GateType::S, &TableauSimulator::SQRT_Z},
    {GateType::S_DAG, &TableauSimulator::SQRT_Z_DAG},
    {GateType::SQRT_XX, &TableauSimulator::SQRT_XX},
    {GateType::SQRT_XX_DAG, &TableauSimulator::SQRT_XX_DAG},
    {GateType::SQRT_YY, &TableauSimulator::SQRT_YY},
    {GateType::SQRT_YY_DAG, &TableauSimulator::SQRT_YY_DAG},
    {GateType::SQRT_ZZ, &TableauSimulator::SQRT_ZZ},
    {GateType::SQRT_ZZ_DAG, &TableauSimulator::SQRT_ZZ_DAG},
    {GateType::SWAP, &TableauSimulator::SWAP},
    {GateType::ISWAP, &TableauSimulator::ISWAP},
    {GateType::ISWAP_DAG, &TableauSimulator::ISWAP_DAG},
    {GateType::CXSWAP, &TableauSimulator::CXSWAP},
    {GateType::SWAPCX, &TableauSimulator::SWAPCX},




});

extern const GateVTable<ErrorAnalyzer> stim::ERROR_ANALYZER_VTABLE = GateVTable<ErrorAnalyzer>({
    {GateType::DETECTOR, &ErrorAnalyzer::DETECTOR},
    {GateType::OBSERVABLE_INCLUDE, &ErrorAnalyzer::OBSERVABLE_INCLUDE},
    {GateType::TICK, &ErrorAnalyzer::TICK},
    {GateType::QUBIT_COORDS, &ErrorAnalyzer::I},
    {GateType::SHIFT_COORDS, &ErrorAnalyzer::SHIFT_COORDS},
    {GateType::REPEAT, &ErrorAnalyzer::I},
    {GateType::MX, &ErrorAnalyzer::MX},
    {GateType::MY, &ErrorAnalyzer::MY},
    {GateType::M, &ErrorAnalyzer::MZ},
    {GateType::MRX, &ErrorAnalyzer::MRX},
    {GateType::MRY, &ErrorAnalyzer::MRY},
    {GateType::MR, &ErrorAnalyzer::MRZ},
    {GateType::RX, &ErrorAnalyzer::RX},
    {GateType::RY, &ErrorAnalyzer::RY},
    {GateType::R, &ErrorAnalyzer::RZ},
    {GateType::MPP, &ErrorAnalyzer::MPP},
    {GateType::XCX, &ErrorAnalyzer::XCX},
    {GateType::XCY, &ErrorAnalyzer::XCY},
    {GateType::XCZ, &ErrorAnalyzer::XCZ},
    {GateType::YCX, &ErrorAnalyzer::YCX},
    {GateType::YCY, &ErrorAnalyzer::YCY},
    {GateType::YCZ, &ErrorAnalyzer::YCZ},
    {GateType::CX, &ErrorAnalyzer::ZCX},
    {GateType::CY, &ErrorAnalyzer::ZCY},
    {GateType::CZ, &ErrorAnalyzer::ZCZ},
    {GateType::H, &ErrorAnalyzer::H_XZ},
    {GateType::H_XY, &ErrorAnalyzer::H_XY},
    {GateType::H_YZ, &ErrorAnalyzer::H_YZ},
    {GateType::DEPOLARIZE1, &ErrorAnalyzer::DEPOLARIZE1},
    {GateType::DEPOLARIZE2, &ErrorAnalyzer::DEPOLARIZE2},
    {GateType::X_ERROR, &ErrorAnalyzer::X_ERROR},
    {GateType::Y_ERROR, &ErrorAnalyzer::Y_ERROR},
    {GateType::Z_ERROR, &ErrorAnalyzer::Z_ERROR},
    {GateType::PAULI_CHANNEL_1, &ErrorAnalyzer::PAULI_CHANNEL_1},
    {GateType::PAULI_CHANNEL_2, &ErrorAnalyzer::PAULI_CHANNEL_2},
    {GateType::E, &ErrorAnalyzer::CORRELATED_ERROR},
    {GateType::ELSE_CORRELATED_ERROR, &ErrorAnalyzer::ELSE_CORRELATED_ERROR},
    {GateType::I, &ErrorAnalyzer::I},
    {GateType::X, &ErrorAnalyzer::I},
    {GateType::Y, &ErrorAnalyzer::I},
    {GateType::Z, &ErrorAnalyzer::I},
    {GateType::C_XYZ, &ErrorAnalyzer::C_XYZ},
    {GateType::C_ZYX, &ErrorAnalyzer::C_ZYX},
    {GateType::SQRT_X, &ErrorAnalyzer::H_YZ},
    {GateType::SQRT_X_DAG, &ErrorAnalyzer::H_YZ},
    {GateType::SQRT_Y, &ErrorAnalyzer::H_XZ},
    {GateType::SQRT_Y_DAG, &ErrorAnalyzer::H_XZ},
    {GateType::S, &ErrorAnalyzer::H_XY},
    {GateType::S_DAG, &ErrorAnalyzer::H_XY},
    {GateType::SQRT_XX, &ErrorAnalyzer::SQRT_XX},
    {GateType::SQRT_XX_DAG, &ErrorAnalyzer::SQRT_XX},
    {GateType::SQRT_YY, &ErrorAnalyzer::SQRT_YY},
    {GateType::SQRT_YY_DAG, &ErrorAnalyzer::SQRT_YY},
    {GateType::SQRT_ZZ, &ErrorAnalyzer::SQRT_ZZ},
    {GateType::SQRT_ZZ_DAG, &ErrorAnalyzer::SQRT_ZZ},
    {GateType::SWAP, &ErrorAnalyzer::SWAP},
    {GateType::ISWAP, &ErrorAnalyzer::ISWAP},
    {GateType::ISWAP_DAG, &ErrorAnalyzer::ISWAP},
    {GateType::CXSWAP, &ErrorAnalyzer::CXSWAP},
    {GateType::SWAPCX, &ErrorAnalyzer::SWAPCX},
});

extern const GateVTable<SparseUnsignedRevFrameTracker>
stim::SPARSE_UNSIGNED_REV_FRAME_TRACKER_VTABLE = GateVTable<SparseUnsignedRevFrameTracker>({
    {GateType::DETECTOR, &SparseUnsignedRevFrameTracker::undo_DETECTOR},
    {GateType::OBSERVABLE_INCLUDE, &SparseUnsignedRevFrameTracker::undo_OBSERVABLE_INCLUDE},
    {GateType::TICK, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::QUBIT_COORDS, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::SHIFT_COORDS, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::REPEAT, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::MX, &SparseUnsignedRevFrameTracker::undo_MX},
    {GateType::MY, &SparseUnsignedRevFrameTracker::undo_MY},
    {GateType::M, &SparseUnsignedRevFrameTracker::undo_MZ},
    {GateType::MRX, &SparseUnsignedRevFrameTracker::undo_MRX},
    {GateType::MRY, &SparseUnsignedRevFrameTracker::undo_MRY},
    {GateType::MR, &SparseUnsignedRevFrameTracker::undo_MRZ},
    {GateType::RX, &SparseUnsignedRevFrameTracker::undo_RX},
    {GateType::RY, &SparseUnsignedRevFrameTracker::undo_RY},
    {GateType::R, &SparseUnsignedRevFrameTracker::undo_RZ},
    {GateType::MPP, &SparseUnsignedRevFrameTracker::undo_MPP},
    {GateType::XCX, &SparseUnsignedRevFrameTracker::undo_XCX},
    {GateType::XCY, &SparseUnsignedRevFrameTracker::undo_XCY},
    {GateType::XCZ, &SparseUnsignedRevFrameTracker::undo_XCZ},
    {GateType::YCX, &SparseUnsignedRevFrameTracker::undo_YCX},
    {GateType::YCY, &SparseUnsignedRevFrameTracker::undo_YCY},
    {GateType::YCZ, &SparseUnsignedRevFrameTracker::undo_YCZ},
    {GateType::CX, &SparseUnsignedRevFrameTracker::undo_ZCX},
    {GateType::CY, &SparseUnsignedRevFrameTracker::undo_ZCY},
    {GateType::CZ, &SparseUnsignedRevFrameTracker::undo_ZCZ},
    {GateType::H, &SparseUnsignedRevFrameTracker::undo_H_XZ},
    {GateType::H_XY, &SparseUnsignedRevFrameTracker::undo_H_XY},
    {GateType::H_YZ, &SparseUnsignedRevFrameTracker::undo_H_YZ},
    {GateType::DEPOLARIZE1, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::DEPOLARIZE2, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::X_ERROR, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::Y_ERROR, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::Z_ERROR, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::PAULI_CHANNEL_1, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::PAULI_CHANNEL_2, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::E, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::ELSE_CORRELATED_ERROR, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::I, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::X, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::Y, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::Z, &SparseUnsignedRevFrameTracker::undo_I},
    {GateType::C_XYZ, &SparseUnsignedRevFrameTracker::undo_C_XYZ},
    {GateType::C_ZYX, &SparseUnsignedRevFrameTracker::undo_C_ZYX},
    {GateType::SQRT_X, &SparseUnsignedRevFrameTracker::undo_H_YZ},
    {GateType::SQRT_X_DAG, &SparseUnsignedRevFrameTracker::undo_H_YZ},
    {GateType::SQRT_Y, &SparseUnsignedRevFrameTracker::undo_H_XZ},
    {GateType::SQRT_Y_DAG, &SparseUnsignedRevFrameTracker::undo_H_XZ},
    {GateType::S, &SparseUnsignedRevFrameTracker::undo_H_XY},
    {GateType::S_DAG, &SparseUnsignedRevFrameTracker::undo_H_XY},
    {GateType::SQRT_XX, &SparseUnsignedRevFrameTracker::undo_SQRT_XX},
    {GateType::SQRT_XX_DAG, &SparseUnsignedRevFrameTracker::undo_SQRT_XX},
    {GateType::SQRT_YY, &SparseUnsignedRevFrameTracker::undo_SQRT_YY},
    {GateType::SQRT_YY_DAG, &SparseUnsignedRevFrameTracker::undo_SQRT_YY},
    {GateType::SQRT_ZZ, &SparseUnsignedRevFrameTracker::undo_SQRT_ZZ},
    {GateType::SQRT_ZZ_DAG, &SparseUnsignedRevFrameTracker::undo_SQRT_ZZ},
    {GateType::SWAP, &SparseUnsignedRevFrameTracker::undo_SWAP},
    {GateType::ISWAP, &SparseUnsignedRevFrameTracker::undo_ISWAP},
    {GateType::ISWAP_DAG, &SparseUnsignedRevFrameTracker::undo_ISWAP},
    {GateType::CXSWAP, &SparseUnsignedRevFrameTracker::undo_CXSWAP},
    {GateType::SWAPCX, &SparseUnsignedRevFrameTracker::undo_SWAPCX},
});
