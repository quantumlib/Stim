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
constexpr void GateVTable<SIMULATOR>::add_simulator_function_annotations() {
    if constexpr (std::is_same<SIMULATOR, TableauSimulator>::value ||
                  std::is_same<SIMULATOR, FrameSimulator>::value) {
        add_simulator_function(GateType::DETECTOR, &SIMULATOR::I);
        add_simulator_function(GateType::OBSERVABLE_INCLUDE, &SIMULATOR::I);
        add_simulator_function(GateType::TICK, &SIMULATOR::I);
        add_simulator_function(GateType::QUBIT_COORDS, &SIMULATOR::I);
        add_simulator_function(GateType::SHIFT_COORDS, &SIMULATOR::I);
    } else if constexpr (std::is_same<SIMULATOR, ErrorAnalyzer>::value) {
        add_simulator_function(GateType::DETECTOR, &SIMULATOR::DETECTOR);
        add_simulator_function(GateType::OBSERVABLE_INCLUDE, &SIMULATOR::OBSERVABLE_INCLUDE);
        add_simulator_function(GateType::TICK, &SIMULATOR::TICK);
        add_simulator_function(GateType::QUBIT_COORDS, &SIMULATOR::I);
        add_simulator_function(GateType::SHIFT_COORDS, &SIMULATOR::SHIFT_COORDS);
    } else if constexpr (std::is_same<SIMULATOR, SparseUnsignedRevFrameTracker>::value) {
        add_simulator_function(GateType::DETECTOR, &SIMULATOR::undo_DETECTOR);
        add_simulator_function(GateType::OBSERVABLE_INCLUDE, &SIMULATOR::undo_OBSERVABLE_INCLUDE);
        add_simulator_function(GateType::TICK, &SIMULATOR::undo_I);
        add_simulator_function(GateType::QUBIT_COORDS, &SIMULATOR::undo_I);
        add_simulator_function(GateType::SHIFT_COORDS, &SIMULATOR::undo_I);
    }
}

template <typename SIMULATOR>
constexpr void GateVTable<SIMULATOR>::add_simulator_function_blocks() {
    if constexpr (std::is_same<SIMULATOR, TableauSimulator>::value ||
                  std::is_same<SIMULATOR, FrameSimulator>::value ||
                  std::is_same<SIMULATOR, ErrorAnalyzer>::value) {
        add_simulator_function(GateType::REPEAT, &SIMULATOR::I);
    } else if constexpr (std::is_same<SIMULATOR, SparseUnsignedRevFrameTracker>::value) {
        add_simulator_function(GateType::REPEAT, &SIMULATOR::undo_I);
    }
}

template <typename SIMULATOR>
constexpr void GateVTable<SIMULATOR>::add_simulator_function_collapsing() {
    if constexpr (std::is_same<SIMULATOR, TableauSimulator>::value ||
                  std::is_same<SIMULATOR, FrameSimulator>::value) {
        add_simulator_function(GateType::MX, &SIMULATOR::measure_x);
        add_simulator_function(GateType::MY, &SIMULATOR::measure_y);
        add_simulator_function(GateType::M, &SIMULATOR::measure_z);
        add_simulator_function(GateType::MRX, &SIMULATOR::measure_reset_x);
        add_simulator_function(GateType::MRY, &SIMULATOR::measure_reset_y);
        add_simulator_function(GateType::MR, &SIMULATOR::measure_reset_z);
        add_simulator_function(GateType::RX, &SIMULATOR::reset_x);
        add_simulator_function(GateType::RY, &SIMULATOR::reset_y);
        add_simulator_function(GateType::R, &SIMULATOR::reset_z);
        add_simulator_function(GateType::MPP, &SIMULATOR::MPP);
    } else if constexpr (std::is_same<SIMULATOR, ErrorAnalyzer>::value) {
        add_simulator_function(GateType::MX, &SIMULATOR::MX);
        add_simulator_function(GateType::MY, &SIMULATOR::MY);
        add_simulator_function(GateType::M, &SIMULATOR::MZ);
        add_simulator_function(GateType::MRX, &SIMULATOR::MRX);
        add_simulator_function(GateType::MRY, &SIMULATOR::MRY);
        add_simulator_function(GateType::MR, &SIMULATOR::MRZ);
        add_simulator_function(GateType::RX, &SIMULATOR::RX);
        add_simulator_function(GateType::RY, &SIMULATOR::RY);
        add_simulator_function(GateType::R, &SIMULATOR::RZ);
        add_simulator_function(GateType::MPP, &SIMULATOR::MPP);
    } else if constexpr (std::is_same<SIMULATOR, SparseUnsignedRevFrameTracker>::value) {
        add_simulator_function(GateType::MX, &SIMULATOR::undo_MX);
        add_simulator_function(GateType::MY, &SIMULATOR::undo_MY);
        add_simulator_function(GateType::M, &SIMULATOR::undo_MZ);
        add_simulator_function(GateType::MRX, &SIMULATOR::undo_MRX);
        add_simulator_function(GateType::MRY, &SIMULATOR::undo_MRY);
        add_simulator_function(GateType::MR, &SIMULATOR::undo_MRZ);
        add_simulator_function(GateType::RX, &SIMULATOR::undo_RX);
        add_simulator_function(GateType::RY, &SIMULATOR::undo_RY);
        add_simulator_function(GateType::R, &SIMULATOR::undo_RZ);
        add_simulator_function(GateType::MPP, &SIMULATOR::undo_MPP);
    }
}

template <typename SIMULATOR>
constexpr void GateVTable<SIMULATOR>::add_simulator_function_controlled() {
    if constexpr (std::is_same<SIMULATOR, TableauSimulator>::value ||
                  std::is_same<SIMULATOR, FrameSimulator>::value ||
                  std::is_same<SIMULATOR, ErrorAnalyzer>::value) {
        add_simulator_function(GateType::XCX, &SIMULATOR::XCX);
        add_simulator_function(GateType::XCY, &SIMULATOR::XCY);
        add_simulator_function(GateType::XCZ, &SIMULATOR::XCZ);
        add_simulator_function(GateType::YCX, &SIMULATOR::YCX);
        add_simulator_function(GateType::YCY, &SIMULATOR::YCY);
        add_simulator_function(GateType::YCZ, &SIMULATOR::YCZ);
        add_simulator_function(GateType::CX, &SIMULATOR::ZCX);
        add_simulator_function(GateType::CY, &SIMULATOR::ZCY);
        add_simulator_function(GateType::CZ, &SIMULATOR::ZCZ);
    } else if constexpr (std::is_same<SIMULATOR, SparseUnsignedRevFrameTracker>::value) {
        add_simulator_function(GateType::XCX, &SIMULATOR::undo_XCX);
        add_simulator_function(GateType::XCY, &SIMULATOR::undo_XCY);
        add_simulator_function(GateType::XCZ, &SIMULATOR::undo_XCZ);
        add_simulator_function(GateType::YCX, &SIMULATOR::undo_YCX);
        add_simulator_function(GateType::YCY, &SIMULATOR::undo_YCY);
        add_simulator_function(GateType::YCZ, &SIMULATOR::undo_YCZ);
        add_simulator_function(GateType::CX, &SIMULATOR::undo_ZCX);
        add_simulator_function(GateType::CY, &SIMULATOR::undo_ZCY);
        add_simulator_function(GateType::CZ, &SIMULATOR::undo_ZCZ);
    }
}

template <typename SIMULATOR>
constexpr void GateVTable<SIMULATOR>::add_simulator_function_hada() {
    if constexpr (std::is_same<SIMULATOR, TableauSimulator>::value ||
                  std::is_same<SIMULATOR, FrameSimulator>::value ||
                  std::is_same<SIMULATOR, ErrorAnalyzer>::value) {
        add_simulator_function(GateType::H, &SIMULATOR::H_XZ);
        add_simulator_function(GateType::H_XY, &SIMULATOR::H_XY);
        add_simulator_function(GateType::H_YZ, &SIMULATOR::H_YZ);
    } else if constexpr (std::is_same<SIMULATOR, SparseUnsignedRevFrameTracker>::value) {
        add_simulator_function(GateType::H, &SIMULATOR::undo_H_XZ);
        add_simulator_function(GateType::H_XY, &SIMULATOR::undo_H_XY);
        add_simulator_function(GateType::H_YZ, &SIMULATOR::undo_H_YZ);
    }
}

template <typename SIMULATOR>
constexpr void GateVTable<SIMULATOR>::add_simulator_function_noisy() {
    if constexpr (std::is_same<SIMULATOR, TableauSimulator>::value ||
                  std::is_same<SIMULATOR, FrameSimulator>::value ||
                  std::is_same<SIMULATOR, ErrorAnalyzer>::value) {
        add_simulator_function(GateType::DEPOLARIZE1, &SIMULATOR::DEPOLARIZE1);
        add_simulator_function(GateType::DEPOLARIZE2, &SIMULATOR::DEPOLARIZE2);
        add_simulator_function(GateType::X_ERROR, &SIMULATOR::X_ERROR);
        add_simulator_function(GateType::Y_ERROR, &SIMULATOR::Y_ERROR);
        add_simulator_function(GateType::Z_ERROR, &SIMULATOR::Z_ERROR);
        add_simulator_function(GateType::PAULI_CHANNEL_1, &SIMULATOR::PAULI_CHANNEL_1);
        add_simulator_function(GateType::PAULI_CHANNEL_2, &SIMULATOR::PAULI_CHANNEL_2);
        add_simulator_function(GateType::E, &SIMULATOR::CORRELATED_ERROR);
        add_simulator_function(GateType::ELSE_CORRELATED_ERROR, &SIMULATOR::ELSE_CORRELATED_ERROR);
    } else if constexpr (std::is_same<SIMULATOR, SparseUnsignedRevFrameTracker>::value) {
        add_simulator_function(GateType::DEPOLARIZE1, &SIMULATOR::undo_I);
        add_simulator_function(GateType::DEPOLARIZE2, &SIMULATOR::undo_I);
        add_simulator_function(GateType::X_ERROR, &SIMULATOR::undo_I);
        add_simulator_function(GateType::Y_ERROR, &SIMULATOR::undo_I);
        add_simulator_function(GateType::Z_ERROR, &SIMULATOR::undo_I);
        add_simulator_function(GateType::PAULI_CHANNEL_1, &SIMULATOR::undo_I);
        add_simulator_function(GateType::PAULI_CHANNEL_2, &SIMULATOR::undo_I);
        add_simulator_function(GateType::E, &SIMULATOR::undo_I);
        add_simulator_function(GateType::ELSE_CORRELATED_ERROR, &SIMULATOR::undo_I);
    }
}

template <typename SIMULATOR>
constexpr void GateVTable<SIMULATOR>::add_simulator_function_pauli() {
    if constexpr (std::is_same<SIMULATOR, TableauSimulator>::value) {
        add_simulator_function(GateType::I, &SIMULATOR::I);
        add_simulator_function(GateType::X, &SIMULATOR::X);
        add_simulator_function(GateType::Y, &SIMULATOR::Y);
        add_simulator_function(GateType::Z, &SIMULATOR::Z);
    } else if constexpr (std::is_same<SIMULATOR, FrameSimulator>::value ||
                         std::is_same<SIMULATOR, ErrorAnalyzer>::value) {
        add_simulator_function(GateType::I, &SIMULATOR::I);
        add_simulator_function(GateType::X, &SIMULATOR::I);
        add_simulator_function(GateType::Y, &SIMULATOR::I);
        add_simulator_function(GateType::Z, &SIMULATOR::I);
    } else if constexpr (std::is_same<SIMULATOR, SparseUnsignedRevFrameTracker>::value) {
        add_simulator_function(GateType::I, &SIMULATOR::undo_I);
        add_simulator_function(GateType::X, &SIMULATOR::undo_I);
        add_simulator_function(GateType::Y, &SIMULATOR::undo_I);
        add_simulator_function(GateType::Z, &SIMULATOR::undo_I);
    }
}

template <typename SIMULATOR>
constexpr void GateVTable<SIMULATOR>::add_simulator_function_period_3() {
    if constexpr (std::is_same<SIMULATOR, TableauSimulator>::value ||
                  std::is_same<SIMULATOR, FrameSimulator>::value ||
                  std::is_same<SIMULATOR, ErrorAnalyzer>::value) {
        add_simulator_function(GateType::C_XYZ, &SIMULATOR::C_XYZ);
        add_simulator_function(GateType::C_ZYX, &SIMULATOR::C_ZYX);
    } else if constexpr (std::is_same<SIMULATOR, SparseUnsignedRevFrameTracker>::value) {
        add_simulator_function(GateType::C_XYZ, &SIMULATOR::undo_C_XYZ);
        add_simulator_function(GateType::C_ZYX, &SIMULATOR::undo_C_ZYX);
    }
}

template <typename SIMULATOR>
constexpr void GateVTable<SIMULATOR>::add_simulator_function_period_4() {
    if constexpr (std::is_same<SIMULATOR, TableauSimulator>::value) {
        add_simulator_function(GateType::SQRT_X, &SIMULATOR::SQRT_X);
        add_simulator_function(GateType::SQRT_X_DAG, &SIMULATOR::SQRT_X_DAG);
        add_simulator_function(GateType::SQRT_Y, &SIMULATOR::SQRT_Y);
        add_simulator_function(GateType::SQRT_Y_DAG, &SIMULATOR::SQRT_Y_DAG);
        add_simulator_function(GateType::S, &SIMULATOR::SQRT_Z);
        add_simulator_function(GateType::S_DAG, &SIMULATOR::SQRT_Z_DAG);
    } else if constexpr (std::is_same<SIMULATOR, FrameSimulator>::value ||
                         std::is_same<SIMULATOR, ErrorAnalyzer>::value) {
        add_simulator_function(GateType::SQRT_X, &SIMULATOR::H_YZ);
        add_simulator_function(GateType::SQRT_X_DAG, &SIMULATOR::H_YZ);
        add_simulator_function(GateType::SQRT_Y, &SIMULATOR::H_XZ);
        add_simulator_function(GateType::SQRT_Y_DAG, &SIMULATOR::H_XZ);
        add_simulator_function(GateType::S, &SIMULATOR::H_XY);
        add_simulator_function(GateType::S_DAG, &SIMULATOR::H_XY);
    } else if constexpr (std::is_same<SIMULATOR, SparseUnsignedRevFrameTracker>::value) {
        add_simulator_function(GateType::SQRT_X, &SIMULATOR::undo_H_YZ);
        add_simulator_function(GateType::SQRT_X_DAG, &SIMULATOR::undo_H_YZ);
        add_simulator_function(GateType::SQRT_Y, &SIMULATOR::undo_H_XZ);
        add_simulator_function(GateType::SQRT_Y_DAG, &SIMULATOR::undo_H_XZ);
        add_simulator_function(GateType::S, &SIMULATOR::undo_H_XY);
        add_simulator_function(GateType::S_DAG, &SIMULATOR::undo_H_XY);
    }
}

template <typename SIMULATOR>
constexpr void GateVTable<SIMULATOR>::add_simulator_function_pp() {
    if constexpr (std::is_same<SIMULATOR, TableauSimulator>::value) {
        add_simulator_function(GateType::SQRT_XX, &SIMULATOR::SQRT_XX);
        add_simulator_function(GateType::SQRT_XX_DAG, &SIMULATOR::SQRT_XX_DAG);
        add_simulator_function(GateType::SQRT_YY, &SIMULATOR::SQRT_YY);
        add_simulator_function(GateType::SQRT_YY_DAG, &SIMULATOR::SQRT_YY_DAG);
        add_simulator_function(GateType::SQRT_ZZ, &SIMULATOR::SQRT_ZZ);
        add_simulator_function(GateType::SQRT_ZZ_DAG, &SIMULATOR::SQRT_ZZ_DAG);
    } else if constexpr (std::is_same<SIMULATOR, FrameSimulator>::value ||
                         std::is_same<SIMULATOR, ErrorAnalyzer>::value) {
        add_simulator_function(GateType::SQRT_XX, &SIMULATOR::SQRT_XX);
        add_simulator_function(GateType::SQRT_XX_DAG, &SIMULATOR::SQRT_XX);
        add_simulator_function(GateType::SQRT_YY, &SIMULATOR::SQRT_YY);
        add_simulator_function(GateType::SQRT_YY_DAG, &SIMULATOR::SQRT_YY);
        add_simulator_function(GateType::SQRT_ZZ, &SIMULATOR::SQRT_ZZ);
        add_simulator_function(GateType::SQRT_ZZ_DAG, &SIMULATOR::SQRT_ZZ);
    } else if constexpr (std::is_same<SIMULATOR, SparseUnsignedRevFrameTracker>::value) {
        add_simulator_function(GateType::SQRT_XX, &SIMULATOR::undo_SQRT_XX);
        add_simulator_function(GateType::SQRT_XX_DAG, &SIMULATOR::undo_SQRT_XX);
        add_simulator_function(GateType::SQRT_YY, &SIMULATOR::undo_SQRT_YY);
        add_simulator_function(GateType::SQRT_YY_DAG, &SIMULATOR::undo_SQRT_YY);
        add_simulator_function(GateType::SQRT_ZZ, &SIMULATOR::undo_SQRT_ZZ);
        add_simulator_function(GateType::SQRT_ZZ_DAG, &SIMULATOR::undo_SQRT_ZZ);
    }
}

template <typename SIMULATOR>
constexpr void GateVTable<SIMULATOR>::add_simulator_function_swaps() {
    if constexpr (std::is_same<SIMULATOR, TableauSimulator>::value) {
        add_simulator_function(GateType::SWAP, &SIMULATOR::SWAP);
        add_simulator_function(GateType::ISWAP, &SIMULATOR::ISWAP);
        add_simulator_function(GateType::ISWAP_DAG, &SIMULATOR::ISWAP_DAG);
        add_simulator_function(GateType::CXSWAP, &SIMULATOR::CXSWAP);
        add_simulator_function(GateType::SWAPCX, &SIMULATOR::SWAPCX);
    } else if constexpr (std::is_same<SIMULATOR, FrameSimulator>::value ||
                         std::is_same<SIMULATOR, ErrorAnalyzer>::value) {
        add_simulator_function(GateType::SWAP, &SIMULATOR::SWAP);
        add_simulator_function(GateType::ISWAP, &SIMULATOR::ISWAP);
        add_simulator_function(GateType::ISWAP_DAG, &SIMULATOR::ISWAP);
        add_simulator_function(GateType::CXSWAP, &SIMULATOR::CXSWAP);
        add_simulator_function(GateType::SWAPCX, &SIMULATOR::SWAPCX);
    } else if constexpr (std::is_same<SIMULATOR, SparseUnsignedRevFrameTracker>::value) {
        add_simulator_function(GateType::SWAP, &SIMULATOR::undo_SWAP);
        add_simulator_function(GateType::ISWAP, &SIMULATOR::undo_ISWAP);
        add_simulator_function(GateType::ISWAP_DAG, &SIMULATOR::undo_ISWAP);
        add_simulator_function(GateType::CXSWAP, &SIMULATOR::undo_CXSWAP);
        add_simulator_function(GateType::SWAPCX, &SIMULATOR::undo_SWAPCX);
    }
}

template <typename SIMULATOR>
constexpr GateVTable<SIMULATOR>::GateVTable() {
    add_simulator_function_annotations();
    add_simulator_function_blocks();
    add_simulator_function_collapsing();
    add_simulator_function_controlled();
    add_simulator_function_hada();
    add_simulator_function_noisy();
    add_simulator_function_pauli();
    add_simulator_function_period_3();
    add_simulator_function_period_4();
    add_simulator_function_pp();
    add_simulator_function_swaps();
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
extern const GateVTable<FrameSimulator> stim::FRAME_SIM_VTABLE = GateVTable<FrameSimulator>();
extern const GateVTable<TableauSimulator> stim::TABLEAU_SIM_VTABLE = GateVTable<TableauSimulator>();
extern const GateVTable<ErrorAnalyzer> stim::ERROR_ANALYZER_VTABLE = GateVTable<ErrorAnalyzer>();
extern const GateVTable<SparseUnsignedRevFrameTracker>
stim::SPARSE_UNSIGNED_REV_FRAME_TRACKER_VTABLE = GateVTable<SparseUnsignedRevFrameTracker>();
