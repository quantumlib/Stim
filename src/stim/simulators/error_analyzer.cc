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

#include "stim/simulators/error_analyzer.h"

#include <algorithm>
#include <queue>
#include <sstream>

#include "stim/circuit/gate_decomposition.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/util_bot/error_decomp.h"

using namespace stim;

void ErrorAnalyzer::undo_gate(const CircuitInstruction &inst) {
    switch (inst.gate_type) {
        case GateType::DETECTOR:
            undo_DETECTOR(inst);
            break;
        case GateType::OBSERVABLE_INCLUDE:
            undo_OBSERVABLE_INCLUDE(inst);
            break;
        case GateType::TICK:
            undo_TICK(inst);
            break;
        case GateType::QUBIT_COORDS:
            undo_I(inst);
            break;
        case GateType::SHIFT_COORDS:
            undo_SHIFT_COORDS(inst);
            break;
        case GateType::REPEAT:
            undo_I(inst);
            break;
        case GateType::MX:
            undo_MX(inst);
            break;
        case GateType::MY:
            undo_MY(inst);
            break;
        case GateType::M:
            undo_MZ(inst);
            break;
        case GateType::MRX:
            undo_MRX(inst);
            break;
        case GateType::MRY:
            undo_MRY(inst);
            break;
        case GateType::MR:
            undo_MRZ(inst);
            break;
        case GateType::HERALDED_ERASE:
            undo_HERALDED_ERASE(inst);
            break;
        case GateType::HERALDED_PAULI_CHANNEL_1:
            undo_HERALDED_PAULI_CHANNEL_1(inst);
            break;
        case GateType::RX:
            undo_RX(inst);
            break;
        case GateType::RY:
            undo_RY(inst);
            break;
        case GateType::R:
            undo_RZ(inst);
            break;
        case GateType::MPP:
            undo_MPP(inst);
            break;
        case GateType::SPP:
        case GateType::SPP_DAG:
            undo_SPP(inst);
            break;
        case GateType::MPAD:
            undo_MPAD(inst);
            break;
        case GateType::MXX:
            undo_MXX(inst);
            break;
        case GateType::MYY:
            undo_MYY(inst);
            break;
        case GateType::MZZ:
            undo_MZZ(inst);
            break;
        case GateType::XCX:
            undo_XCX(inst);
            break;
        case GateType::XCY:
            undo_XCY(inst);
            break;
        case GateType::XCZ:
            undo_XCZ(inst);
            break;
        case GateType::YCX:
            undo_YCX(inst);
            break;
        case GateType::YCY:
            undo_YCY(inst);
            break;
        case GateType::YCZ:
            undo_YCZ(inst);
            break;
        case GateType::CX:
            undo_ZCX(inst);
            break;
        case GateType::CY:
            undo_ZCY(inst);
            break;
        case GateType::CZ:
            undo_ZCZ(inst);
            break;
        case GateType::DEPOLARIZE1:
            undo_DEPOLARIZE1(inst);
            break;
        case GateType::DEPOLARIZE2:
            undo_DEPOLARIZE2(inst);
            break;
        case GateType::X_ERROR:
            undo_X_ERROR(inst);
            break;
        case GateType::Y_ERROR:
            undo_Y_ERROR(inst);
            break;
        case GateType::Z_ERROR:
            undo_Z_ERROR(inst);
            break;
        case GateType::PAULI_CHANNEL_1:
            undo_PAULI_CHANNEL_1(inst);
            break;
        case GateType::PAULI_CHANNEL_2:
            undo_PAULI_CHANNEL_2(inst);
            break;
        case GateType::E:
            undo_CORRELATED_ERROR(inst);
            break;
        case GateType::ELSE_CORRELATED_ERROR:
            undo_ELSE_CORRELATED_ERROR(inst);
            break;
        case GateType::I:
        case GateType::II:
        case GateType::I_ERROR:
        case GateType::II_ERROR:
        case GateType::X:
        case GateType::Y:
        case GateType::Z:
            undo_I(inst);
            break;
        case GateType::C_XYZ:
        case GateType::C_NXYZ:
        case GateType::C_XNYZ:
        case GateType::C_XYNZ:
            undo_C_XYZ(inst);
            break;
        case GateType::C_ZYX:
        case GateType::C_NZYX:
        case GateType::C_ZNYX:
        case GateType::C_ZYNX:
            undo_C_ZYX(inst);
            break;
        case GateType::H_YZ:
        case GateType::SQRT_X:
        case GateType::SQRT_X_DAG:
        case GateType::H_NYZ:
            undo_H_YZ(inst);
            break;
        case GateType::SQRT_Y:
        case GateType::SQRT_Y_DAG:
        case GateType::H:
        case GateType::H_NXZ:
            undo_H_XZ(inst);
            break;
        case GateType::S:
        case GateType::S_DAG:
        case GateType::H_XY:
        case GateType::H_NXY:
            undo_H_XY(inst);
            break;
        case GateType::SQRT_XX:
        case GateType::SQRT_XX_DAG:
            undo_SQRT_XX(inst);
            break;
        case GateType::SQRT_YY:
        case GateType::SQRT_YY_DAG:
            undo_SQRT_YY(inst);
            break;
        case GateType::SQRT_ZZ:
        case GateType::SQRT_ZZ_DAG:
            undo_SQRT_ZZ(inst);
            break;
        case GateType::SWAP:
            undo_SWAP(inst);
            break;
        case GateType::ISWAP:
        case GateType::ISWAP_DAG:
            undo_ISWAP(inst);
            break;
        case GateType::CXSWAP:
            undo_CXSWAP(inst);
            break;
        case GateType::CZSWAP:
            undo_CZSWAP(inst);
            break;
        case GateType::SWAPCX:
            undo_SWAPCX(inst);
            break;
        default:
            throw std::invalid_argument(
                "Not implemented by ErrorAnalyzer::undo_gate: " + std::string(GATE_DATA[inst.gate_type].name));
    }
}

void ErrorAnalyzer::remove_gauge(SpanRef<const DemTarget> sorted) {
    if (sorted.empty()) {
        return;
    }
    const auto &max = sorted.back();
    // HACK: linear overhead due to not keeping an index of which detectors used where.
    for (auto &x : tracker.xs) {
        if (x.contains(max)) {
            x.xor_sorted_items(sorted);
        }
    }
    for (auto &z : tracker.zs) {
        if (z.contains(max)) {
            z.xor_sorted_items(sorted);
        }
    }
}

void ErrorAnalyzer::undo_RX(const CircuitInstruction &dat) {
    undo_RX_with_context(dat, "an X-basis reset (RX)");
}
void ErrorAnalyzer::undo_RY(const CircuitInstruction &dat) {
    undo_RY_with_context(dat, "an X-basis reset (RY)");
}
void ErrorAnalyzer::undo_RZ(const CircuitInstruction &dat) {
    undo_RZ_with_context(dat, "a Z-basis reset (R)");
}

void ErrorAnalyzer::undo_RX_with_context(const CircuitInstruction &dat, const char *context_op) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        check_for_gauge(tracker.zs[q], context_op, q, dat.tag);
        tracker.xs[q].clear();
        tracker.zs[q].clear();
    }
}

void ErrorAnalyzer::undo_RY_with_context(const CircuitInstruction &inst, const char *context_op) {
    for (size_t k = inst.targets.size(); k-- > 0;) {
        auto q = inst.targets[k].qubit_value();
        check_for_gauge(tracker.xs[q], tracker.zs[q], context_op, q, inst.tag);
        tracker.xs[q].clear();
        tracker.zs[q].clear();
    }
}

void ErrorAnalyzer::undo_RZ_with_context(const CircuitInstruction &inst, const char *context_op) {
    for (size_t k = inst.targets.size(); k-- > 0;) {
        auto q = inst.targets[k].qubit_value();
        check_for_gauge(tracker.xs[q], context_op, q, inst.tag);
        tracker.xs[q].clear();
        tracker.zs[q].clear();
    }
}

void ErrorAnalyzer::undo_MX_with_context(const CircuitInstruction &inst, const char *context_op) {
    for (size_t k = inst.targets.size(); k-- > 0;) {
        auto q = inst.targets[k].qubit_value();
        tracker.num_measurements_in_past--;

        SparseXorVec<DemTarget> &d = tracker.rec_bits[tracker.num_measurements_in_past];
        xor_sorted_measurement_error(d.range(), inst);
        tracker.xs[q].xor_sorted_items(d.range());
        check_for_gauge(tracker.zs[q], context_op, q, inst.tag);
        tracker.rec_bits.erase(tracker.num_measurements_in_past);
    }
}

void ErrorAnalyzer::undo_MY_with_context(const CircuitInstruction &inst, const char *context_op) {
    for (size_t k = inst.targets.size(); k-- > 0;) {
        auto q = inst.targets[k].qubit_value();
        tracker.num_measurements_in_past--;

        SparseXorVec<DemTarget> &d = tracker.rec_bits[tracker.num_measurements_in_past];
        xor_sorted_measurement_error(d.range(), inst);
        tracker.xs[q].xor_sorted_items(d.range());
        tracker.zs[q].xor_sorted_items(d.range());
        check_for_gauge(tracker.xs[q], tracker.zs[q], context_op, q, inst.tag);
        tracker.rec_bits.erase(tracker.num_measurements_in_past);
    }
}

void ErrorAnalyzer::undo_MZ_with_context(const CircuitInstruction &inst, const char *context_op) {
    for (size_t k = inst.targets.size(); k-- > 0;) {
        auto q = inst.targets[k].qubit_value();
        tracker.num_measurements_in_past--;

        SparseXorVec<DemTarget> &d = tracker.rec_bits[tracker.num_measurements_in_past];
        xor_sorted_measurement_error(d.range(), inst);
        tracker.zs[q].xor_sorted_items(d.range());
        check_for_gauge(tracker.xs[q], context_op, q, inst.tag);
        tracker.rec_bits.erase(tracker.num_measurements_in_past);
    }
}

void ErrorAnalyzer::undo_HERALDED_ERASE(const CircuitInstruction &inst) {
    check_can_approximate_disjoint("HERALDED_ERASE", inst.args, false);
    double p = inst.args[0] * 0.25;
    double i = std::max(0.0, 1.0 - 4 * p);

    for (size_t k = inst.targets.size(); k-- > 0;) {
        auto q = inst.targets[k].qubit_value();
        tracker.num_measurements_in_past--;

        SparseXorVec<DemTarget> &herald_symptoms = tracker.rec_bits[tracker.num_measurements_in_past];
        if (accumulate_errors) {
            add_error_combinations<3>(
                {i, 0, 0, 0, p, p, p, p},
                {tracker.xs[q].range(), tracker.zs[q].range(), herald_symptoms.range()},
                true,
                inst.tag);
        }
        tracker.rec_bits.erase(tracker.num_measurements_in_past);
    }
}

void ErrorAnalyzer::undo_HERALDED_PAULI_CHANNEL_1(const CircuitInstruction &inst) {
    check_can_approximate_disjoint("HERALDED_PAULI_CHANNEL_1", inst.args, true);
    double hi = inst.args[0];
    double hx = inst.args[1];
    double hy = inst.args[2];
    double hz = inst.args[3];
    double i = std::max(0.0, 1.0 - hi - hx - hy - hz);

    for (size_t k = inst.targets.size(); k-- > 0;) {
        auto q = inst.targets[k].qubit_value();
        tracker.num_measurements_in_past--;

        SparseXorVec<DemTarget> &herald_symptoms = tracker.rec_bits[tracker.num_measurements_in_past];
        if (accumulate_errors) {
            add_error_combinations<3>(
                {i, 0, 0, 0, hi, hz, hx, hy},
                {tracker.xs[q].range(), tracker.zs[q].range(), herald_symptoms.range()},
                true,
                inst.tag);
        }
        tracker.rec_bits.erase(tracker.num_measurements_in_past);
    }
}

void ErrorAnalyzer::undo_MPAD(const CircuitInstruction &inst) {
    for (size_t k = inst.targets.size(); k-- > 0;) {
        tracker.num_measurements_in_past--;

        SparseXorVec<DemTarget> &d = tracker.rec_bits[tracker.num_measurements_in_past];
        xor_sorted_measurement_error(d.range(), inst);
        tracker.rec_bits.erase(tracker.num_measurements_in_past);
    }
}

void ErrorAnalyzer::check_for_gauge(
    SparseXorVec<DemTarget> &potential_gauge_summand_1,
    const SparseXorVec<DemTarget> &potential_gauge_summand_2,
    const char *context_op,
    uint64_t context_qubit,
    std::string_view tag) {
    if (potential_gauge_summand_1 == potential_gauge_summand_2) {
        return;
    }
    potential_gauge_summand_1 ^= potential_gauge_summand_2;
    check_for_gauge(potential_gauge_summand_1, context_op, context_qubit, tag);
    potential_gauge_summand_1 ^= potential_gauge_summand_2;
}

// This is redundant with comma_sep from str_util.h, but for reasons I can't figure out
// (something to do with a dependency cycle involving templates) the compilation fails
// if I use that one.
template <typename TIter>
std::string comma_sep_workaround(const TIter &iterable) {
    std::stringstream out;
    bool first = true;
    for (const auto &t : iterable) {
        if (first) {
            first = false;
        } else {
            out << ", ";
        }
        out << t;
    }
    return out.str();
}

void ErrorAnalyzer::check_for_gauge(
    const SparseXorVec<DemTarget> &potential_gauge, const char *context_op, uint64_t context_qubit, std::string_view tag) {
    if (potential_gauge.empty()) {
        return;
    }

    bool has_observables = false;
    bool has_detectors = false;
    for (const auto &t : potential_gauge) {
        has_observables |= t.is_observable_id();
        has_detectors |= t.is_relative_detector_id();
    }
    if (allow_gauge_detectors && !has_observables) {
        remove_gauge(add_error(0.5, potential_gauge.range(), tag).targets);
        return;
    }

    // We are now in an error condition, and it's a bit hard to debug for the user.
    // The goal is to collect a *lot* of information that might be useful to them.

    std::stringstream error_msg;
    has_detectors &= !allow_gauge_detectors;
    if (has_observables) {
        error_msg << "The circuit contains non-deterministic observables.\n";
    }
    if (has_detectors) {
        error_msg << "The circuit contains non-deterministic detectors.\n";
    }
    size_t range_start = num_ticks_in_past - std::min((size_t)num_ticks_in_past, size_t{5});
    size_t range_end = num_ticks_in_past + 5;
    error_msg << "\nTo make an SVG picture of the problem, you can use the python API like this:\n    ";
    error_msg << "your_circuit.diagram('detslice-with-ops-svg'";
    error_msg << ", tick=range(" << range_start << ", " << range_end << ")";
    error_msg << ", filter_coords=[";
    for (auto d : potential_gauge) {
        error_msg << "'" << d << "', ";
    }
    error_msg << "])";
    error_msg << "\nor the command line API like this:\n    ";
    error_msg << "stim diagram --in your_circuit_file.stim";
    error_msg << " --type detslice-with-ops-svg";
    error_msg << " --tick " << range_start << ":" << range_end;
    error_msg << " --filter_coords ";
    for (size_t k = 0; k < potential_gauge.size(); k++) {
        if (k) {
            error_msg << ':';
        }
        error_msg << potential_gauge.sorted_items[k];
    }
    error_msg << " > output_image.svg\n";

    std::map<uint64_t, std::vector<double>> qubit_coords_map;
    if (current_circuit_being_analyzed != nullptr) {
        qubit_coords_map = current_circuit_being_analyzed->get_final_qubit_coords();
    }
    auto error_msg_qubit_with_coords = [&](uint64_t q, uint8_t p) {
        error_msg << "\n";
        auto qubit_coords = qubit_coords_map[q];
        if (p == 0) {
            error_msg << "    qubit " << q;
        } else if (p == 1) {
            error_msg << "    X" << q;
        } else if (p == 2) {
            error_msg << "    Z" << q;
        } else if (p == 3) {
            error_msg << "    Y" << q;
        }
        if (!qubit_coords.empty()) {
            error_msg << " [coords (" << comma_sep_workaround(qubit_coords) << ")]";
        }
    };

    error_msg << "\n";
    error_msg << "This was discovered while analyzing " << context_op << " on:";
    error_msg_qubit_with_coords(context_qubit, 0);

    error_msg << "\n\n";
    error_msg << "The collapse anti-commuted with these detectors/observables:";
    for (const auto &t : potential_gauge) {
        error_msg << "\n    " << t;

        // Try to find recorded coordinate information for the detector.
        if (t.is_relative_detector_id() && current_circuit_being_analyzed != nullptr) {
            auto coords = current_circuit_being_analyzed->coords_of_detector(t.raw_id());
            if (!coords.empty()) {
                error_msg << " [coords (" << comma_sep_workaround(coords) << ")]";
            }
        }
    }

    for (const auto &t : potential_gauge) {
        if (t.is_relative_detector_id() && allow_gauge_detectors) {
            continue;
        }
        error_msg << "\n\n";
        error_msg << "The backward-propagating error sensitivity for " << t << " was:";
        auto sensitivity = current_error_sensitivity_for(t);
        sensitivity.ref().for_each_active_pauli([&](size_t q) {
            uint8_t p = sensitivity.xs[q] + sensitivity.zs[q] * 2;
            error_msg_qubit_with_coords(q, p);
        });
    }

    throw std::invalid_argument(error_msg.str());
}

PauliString<MAX_BITWORD_WIDTH> ErrorAnalyzer::current_error_sensitivity_for(DemTarget t) const {
    PauliString<MAX_BITWORD_WIDTH> result(tracker.xs.size());
    for (size_t q = 0; q < tracker.xs.size(); q++) {
        result.xs[q] = std::ranges::find(tracker.xs[q], t) != tracker.xs[q].end();
        result.zs[q] = std::ranges::find(tracker.zs[q], t) != tracker.zs[q].end();
    }
    return result;
}

void ErrorAnalyzer::xor_sorted_measurement_error(SpanRef<const DemTarget> targets, const CircuitInstruction &inst) {
    // Measurement error.
    if (!inst.args.empty() && inst.args[0] > 0) {
        add_error(inst.args[0], targets, inst.tag);
    }
}

void ErrorAnalyzer::undo_MX(const CircuitInstruction &dat) {
    undo_MX_with_context(dat, "an X-basis measurement (MX)");
}
void ErrorAnalyzer::undo_MY(const CircuitInstruction &dat) {
    undo_MY_with_context(dat, "a Y-basis measurement (MY)");
}
void ErrorAnalyzer::undo_MZ(const CircuitInstruction &dat) {
    undo_MZ_with_context(dat, "a Z-basis measurement (M)");
}

void ErrorAnalyzer::undo_MRX(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        undo_RX_with_context({GateType::RX, dat.args, &q, dat.tag}, "an X-basis demolition measurement (MRX)");
        undo_MX_with_context({GateType::MX, dat.args, &q, dat.tag}, "an X-basis demolition measurement (MRX)");
    }
}

void ErrorAnalyzer::undo_MRY(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        undo_RY_with_context({GateType::RY, dat.args, &q, dat.tag}, "a Y-basis demolition measurement (MRY)");
        undo_MY_with_context({GateType::MY, dat.args, &q, dat.tag}, "a Y-basis demolition measurement (MRY)");
    }
}

void ErrorAnalyzer::undo_MRZ(const CircuitInstruction &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        undo_RZ_with_context({GateType::R, dat.args, &q, dat.tag}, "a Z-basis demolition measurement (MR)");
        undo_MZ_with_context({GateType::M, dat.args, &q, dat.tag}, "a Z-basis demolition measurement (MR)");
    }
}

void ErrorAnalyzer::undo_H_XZ(const CircuitInstruction &dat) {
    tracker.undo_H_XZ(dat);
}
void ErrorAnalyzer::undo_H_XY(const CircuitInstruction &dat) {
    tracker.undo_H_XY(dat);
}
void ErrorAnalyzer::undo_H_YZ(const CircuitInstruction &dat) {
    tracker.undo_H_YZ(dat);
}
void ErrorAnalyzer::undo_C_XYZ(const CircuitInstruction &dat) {
    tracker.undo_C_XYZ(dat);
}
void ErrorAnalyzer::undo_C_ZYX(const CircuitInstruction &dat) {
    tracker.undo_C_ZYX(dat);
}
void ErrorAnalyzer::undo_XCX(const CircuitInstruction &dat) {
    tracker.undo_XCX(dat);
}
void ErrorAnalyzer::undo_XCY(const CircuitInstruction &dat) {
    tracker.undo_XCY(dat);
}
void ErrorAnalyzer::undo_YCX(const CircuitInstruction &dat) {
    tracker.undo_YCX(dat);
}
void ErrorAnalyzer::undo_ZCY(const CircuitInstruction &dat) {
    tracker.undo_ZCY(dat);
}
void ErrorAnalyzer::undo_YCZ(const CircuitInstruction &dat) {
    tracker.undo_YCZ(dat);
}
void ErrorAnalyzer::undo_YCY(const CircuitInstruction &dat) {
    tracker.undo_YCY(dat);
}
void ErrorAnalyzer::undo_ZCX(const CircuitInstruction &dat) {
    tracker.undo_ZCX(dat);
}
void ErrorAnalyzer::undo_XCZ(const CircuitInstruction &dat) {
    tracker.undo_XCZ(dat);
}
void ErrorAnalyzer::undo_ZCZ(const CircuitInstruction &dat) {
    tracker.undo_ZCZ(dat);
}
void ErrorAnalyzer::undo_TICK(const CircuitInstruction &dat) {
    num_ticks_in_past--;
}
void ErrorAnalyzer::undo_SQRT_XX(const CircuitInstruction &dat) {
    tracker.undo_SQRT_XX(dat);
}
void ErrorAnalyzer::undo_SQRT_YY(const CircuitInstruction &dat) {
    tracker.undo_SQRT_YY(dat);
}
void ErrorAnalyzer::undo_SQRT_ZZ(const CircuitInstruction &dat) {
    tracker.undo_SQRT_ZZ(dat);
}
void ErrorAnalyzer::undo_I(const CircuitInstruction &dat) {
}
void ErrorAnalyzer::undo_SWAP(const CircuitInstruction &dat) {
    tracker.undo_SWAP(dat);
}
void ErrorAnalyzer::undo_ISWAP(const CircuitInstruction &dat) {
    tracker.undo_ISWAP(dat);
}
void ErrorAnalyzer::undo_CXSWAP(const CircuitInstruction &dat) {
    tracker.undo_CXSWAP(dat);
}
void ErrorAnalyzer::undo_CZSWAP(const CircuitInstruction &dat) {
    tracker.undo_CZSWAP(dat);
}
void ErrorAnalyzer::undo_SWAPCX(const CircuitInstruction &dat) {
    tracker.undo_SWAPCX(dat);
}
void ErrorAnalyzer::undo_DETECTOR(const CircuitInstruction &dat) {
    tracker.undo_DETECTOR(dat);
    auto id = DemTarget::relative_detector_id(tracker.num_detectors_in_past);
    flushed_reversed_model.append_detector_instruction(dat.args, id, dat.tag);
}

void ErrorAnalyzer::undo_OBSERVABLE_INCLUDE(const CircuitInstruction &dat) {
    tracker.undo_OBSERVABLE_INCLUDE(dat);
    auto id = DemTarget::observable_id((int32_t)dat.args[0]);
    flushed_reversed_model.append_logical_observable_instruction(id, dat.tag);
}

ErrorAnalyzer::ErrorAnalyzer(
    uint64_t num_measurements,
    uint64_t num_detectors,
    size_t num_qubits,
    uint64_t num_ticks,
    bool decompose_errors,
    bool fold_loops,
    bool allow_gauge_detectors,
    double approximate_disjoint_errors_threshold,
    bool ignore_decomposition_failures,
    bool block_decomposition_from_introducing_remnant_edges)
    : tracker(num_qubits, num_measurements, num_detectors),
      decompose_errors(decompose_errors),
      accumulate_errors(true),
      fold_loops(fold_loops),
      allow_gauge_detectors(allow_gauge_detectors),
      approximate_disjoint_errors_threshold(approximate_disjoint_errors_threshold),
      ignore_decomposition_failures(ignore_decomposition_failures),
      block_decomposition_from_introducing_remnant_edges(block_decomposition_from_introducing_remnant_edges),
      num_ticks_in_past(num_ticks) {
}

void ErrorAnalyzer::undo_circuit(const Circuit &circuit) {
    std::vector<CircuitInstruction> stacked_else_correlated_errors;
    for (size_t k = circuit.operations.size(); k--;) {
        const auto &op = circuit.operations[k];
        try {
            if (op.gate_type == GateType::ELSE_CORRELATED_ERROR) {
                stacked_else_correlated_errors.push_back(op);
            } else if (op.gate_type == GateType::E) {
                stacked_else_correlated_errors.push_back(op);
                correlated_error_block(stacked_else_correlated_errors);
                stacked_else_correlated_errors.clear();
            } else if (!stacked_else_correlated_errors.empty()) {
                throw std::invalid_argument(
                    "ELSE_CORRELATED_ERROR wasn't preceded by ELSE_CORRELATED_ERROR or CORRELATED_ERROR (E)");
            } else if (op.gate_type == GateType::REPEAT) {
                const auto &loop_body = op.repeat_block_body(circuit);
                uint64_t repeats = op.repeat_block_rep_count();
                run_loop(loop_body, repeats, op.tag);
            } else {
                undo_gate(op);
            }
        } catch (std::invalid_argument &ex) {
            std::stringstream error_msg;
            std::string body = ex.what();
            const char *marker = "\n\nCircuit stack trace:\n    at instruction";
            size_t p = body.find(marker);
            if (p == std::string::npos) {
                error_msg << body;
            } else {
                error_msg << body.substr(0, p);
            }
            error_msg << "\n\nCircuit stack trace:";
            if (&circuit == current_circuit_being_analyzed) {
                auto total_ticks = circuit.count_ticks();
                if (total_ticks) {
                    uint64_t current_tick = num_ticks_in_past;
                    error_msg << "\n    during TICK layer #" << (current_tick + 1) << " of " << (total_ticks + 1);
                }
            }
            error_msg << '\n' << circuit.describe_instruction_location(k);
            if (p != std::string::npos) {
                error_msg << "\n    at block's instruction" << body.substr(p + strlen(marker));
            }
            throw std::invalid_argument(error_msg.str());
        }
    }

    if (!stacked_else_correlated_errors.empty()) {
        throw std::invalid_argument(
            "ELSE_CORRELATED_ERROR wasn't preceded by ELSE_CORRELATED_ERROR or CORRELATED_ERROR (E)");
    }
}

void ErrorAnalyzer::post_check_initialization() {
    for (uint32_t q = 0; q < tracker.xs.size(); q++) {
        check_for_gauge(tracker.xs[q], "qubit initialization into |0> at the start of the circuit", q, "");
    }
}

void ErrorAnalyzer::undo_X_ERROR(const CircuitInstruction &inst) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : inst.targets) {
        add_error(inst.args[0], tracker.zs[q.data].range(), inst.tag);
    }
}

void ErrorAnalyzer::undo_Y_ERROR(const CircuitInstruction &inst) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : inst.targets) {
        add_xored_error(inst.args[0], tracker.xs[q.data].range(), tracker.zs[q.data].range(), inst.tag);
    }
}

void ErrorAnalyzer::undo_Z_ERROR(const CircuitInstruction &inst) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : inst.targets) {
        add_error(inst.args[0], tracker.xs[q.data].range(), inst.tag);
    }
}

template <typename T>
inline void inplace_xor_tail(MonotonicBuffer<T> &dst, const SparseXorVec<T> &src) {
    SpanRef<const T> in1 = dst.tail;
    SpanRef<const T> in2 = src.range();
    xor_merge_sort_temp_buffer_callback(in1, in2, [&](SpanRef<const T> result) {
        dst.discard_tail();
        dst.append_tail(result);
    });
}

void ErrorAnalyzer::add_composite_error(double probability, SpanRef<const GateTarget> targets, std::string_view tag) {
    if (!accumulate_errors) {
        return;
    }
    for (auto qp : targets) {
        auto q = qp.qubit_value();
        if (qp.data & TARGET_PAULI_Z_BIT) {
            inplace_xor_tail(mono_buf, tracker.xs[q]);
        }
        if (qp.data & TARGET_PAULI_X_BIT) {
            inplace_xor_tail(mono_buf, tracker.zs[q]);
        }
    }
    add_error_in_sorted_jagged_tail(probability, tag);
}

void ErrorAnalyzer::correlated_error_block(const std::vector<CircuitInstruction> &dats) {
    assert(!dats.empty());

    if (dats.size() == 1) {
        add_composite_error(dats[0].args[0], dats[0].targets, dats[0].tag);
        return;
    }
    check_can_approximate_disjoint("ELSE_CORRELATED_ERROR", {}, false);

    double remaining_p = 1;
    for (size_t k = dats.size(); k--;) {
        CircuitInstruction dat = dats[k];
        double actual_p = dat.args[0] * remaining_p;
        remaining_p *= 1 - dat.args[0];
        if (actual_p > approximate_disjoint_errors_threshold) {
            throw std::invalid_argument(
                "CORRELATED_ERROR/ELSE_CORRELATED_ERROR block has a component probability '" +
                std::to_string(actual_p) +
                "' larger than the "
                "`approximate_disjoint_errors` threshold of "
                "'" +
                std::to_string(approximate_disjoint_errors_threshold) + "'.");
        }
        add_composite_error(actual_p, dat.targets, dat.tag);
    }
}

void ErrorAnalyzer::undo_CORRELATED_ERROR(const CircuitInstruction &inst) {
    add_composite_error(inst.args[0], inst.targets, inst.tag);
}

void ErrorAnalyzer::undo_DEPOLARIZE1(const CircuitInstruction &inst) {
    if (!accumulate_errors) {
        return;
    }
    if (inst.args[0] > 0.75) {
        throw std::invalid_argument("Can't analyze over-mixing DEPOLARIZE1 errors (probability > 3/4).");
    }
    double p = depolarize1_probability_to_independent_per_channel_probability(inst.args[0]);
    for (auto q : inst.targets) {
        add_error_combinations<2>(
            {0, p, p, p},
            {
                tracker.xs[q.data].range(),
                tracker.zs[q.data].range(),
            },
            false,
            inst.tag);
    }
}

void ErrorAnalyzer::undo_DEPOLARIZE2(const CircuitInstruction &inst) {
    if (!accumulate_errors) {
        return;
    }
    if (inst.args[0] > 15.0 / 16.0) {
        throw std::invalid_argument("Can't analyze over-mixing DEPOLARIZE2 errors (probability > 15/16).");
    }
    double p = depolarize2_probability_to_independent_per_channel_probability(inst.args[0]);
    for (size_t i = 0; i < inst.targets.size(); i += 2) {
        auto a = inst.targets[i];
        auto b = inst.targets[i + 1];
        add_error_combinations<4>(
            {0, p, p, p, p, p, p, p, p, p, p, p, p, p, p, p},
            {
                tracker.xs[a.data].range(),
                tracker.zs[a.data].range(),
                tracker.xs[b.data].range(),
                tracker.zs[b.data].range(),
            },
            false,
            inst.tag);
    }
}

void ErrorAnalyzer::undo_ELSE_CORRELATED_ERROR(const CircuitInstruction &dat) {
    if (accumulate_errors) {
        throw std::invalid_argument("Failed to analyze ELSE_CORRELATED_ERROR: " + dat.str());
    }
}

void ErrorAnalyzer::check_can_approximate_disjoint(
    const char *op_name, SpanRef<const double> probabilities, bool allow_single_component) const {
    if (allow_single_component) {
        size_t num_specified = 0;
        for (double p : probabilities) {
            num_specified += p > 0;
        }
        if (num_specified <= 1) {
            return;
        }
    }

    if (approximate_disjoint_errors_threshold == 0) {
        std::stringstream msg;
        msg << "Encountered the operation " << op_name
            << " during error analysis, but this operation requires the `approximate_disjoint_errors` option to be "
               "enabled.";
        msg << "\nIf you're calling from python, using stim.Circuit.detector_error_model, you need to add the "
               "argument approximate_disjoint_errors=True.\n";
        msg << "\nIf you're calling from the command line, you need to specify --approximate_disjoint_errors.";
        throw std::invalid_argument(msg.str());
    }
    for (double p : probabilities) {
        if (p > approximate_disjoint_errors_threshold) {
            std::stringstream msg;
            msg << op_name;
            msg << " has a probability argument (";
            msg << p;
            msg << ") larger than the `approximate_disjoint_errors` threshold (";
            msg << approximate_disjoint_errors_threshold;
            msg << +").";
            throw std::invalid_argument(msg.str());
        }
    }
}

void ErrorAnalyzer::undo_PAULI_CHANNEL_1(const CircuitInstruction &inst) {
    double dx = inst.args[0];
    double dy = inst.args[1];
    double dz = inst.args[2];
    double ix;
    double iy;
    double iz;
    bool is_independent = try_disjoint_to_independent_xyz_errors_approx(dx, dy, dz, &ix, &iy, &iz);
    if (!is_independent) {
        check_can_approximate_disjoint("PAULI_CHANNEL_1", inst.args, true);
        ix = dx;
        iy = dy;
        iz = dz;
    }

    if (!accumulate_errors) {
        return;
    }
    for (auto q : inst.targets) {
        add_error_combinations<2>(
            {0, ix, iz, iy},
            {
                tracker.zs[q.data].range(),
                tracker.xs[q.data].range(),
            },
            !is_independent,
            inst.tag);
    }
}

void ErrorAnalyzer::undo_PAULI_CHANNEL_2(const CircuitInstruction &inst) {
    check_can_approximate_disjoint("PAULI_CHANNEL_2", inst.args, true);

    std::array<double, 16> probabilities;
    for (size_t k = 0; k < 15; k++) {
        size_t k2 = pauli_xyz_to_xz((k + 1) & 3) | (pauli_xyz_to_xz(((k + 1) >> 2) & 3) << 2);
        probabilities[k2] = inst.args[k];
    }
    if (!accumulate_errors) {
        return;
    }
    for (size_t i = 0; i < inst.targets.size(); i += 2) {
        auto a = inst.targets[i];
        auto b = inst.targets[i + 1];
        add_error_combinations<4>(
            probabilities,
            {
                tracker.zs[b.data].range(),
                tracker.xs[b.data].range(),
                tracker.zs[a.data].range(),
                tracker.xs[a.data].range(),
            },
            true,
            inst.tag);
    }
}

DetectorErrorModel unreversed(const DetectorErrorModel &rev, uint64_t &base_detector_id, std::set<DemTarget> &seen) {
    DetectorErrorModel out;
    auto conv_append = [&](const DemInstruction &e) {
        auto stored_targets = out.target_buf.take_copy(e.target_data);
        auto stored_args = out.arg_buf.take_copy(e.arg_data);
        auto stored_tag = out.tag_buf.take_copy(e.tag);
        for (auto &t : stored_targets) {
            t.shift_if_detector_id(-(int64_t)base_detector_id);
        }
        out.instructions.push_back(DemInstruction{
            .arg_data=stored_args,
            .target_data=stored_targets,
            .tag=stored_tag,
            .type=e.type,
        });
    };

    for (auto p = rev.instructions.crbegin(); p != rev.instructions.crend(); p++) {
        const auto &e = *p;
        switch (e.type) {
            case DemInstructionType::DEM_SHIFT_DETECTORS:
                base_detector_id += e.target_data[0].data;
                out.append_shift_detectors_instruction(e.arg_data, e.target_data[0].data, e.tag);
                break;
            case DemInstructionType::DEM_ERROR:
                for (auto &t : e.target_data) {
                    seen.insert(t);
                }
                conv_append(e);
                break;
            case DemInstructionType::DEM_DETECTOR:
            case DemInstructionType::DEM_LOGICAL_OBSERVABLE:
                if (!e.arg_data.empty() || !e.tag.empty() || !seen.contains(e.target_data[0])) {
                    conv_append(e);
                }
                break;
            case DemInstructionType::DEM_REPEAT_BLOCK: {
                uint64_t repetitions = e.repeat_block_rep_count();
                if (repetitions) {
                    uint64_t old_base_detector_id = base_detector_id;
                    out.append_repeat_block(
                        e.repeat_block_rep_count(),
                        unreversed(e.repeat_block_body(rev), base_detector_id, seen),
                        e.tag);
                    uint64_t loop_shift = base_detector_id - old_base_detector_id;
                    base_detector_id += loop_shift * (repetitions - 1);
                }
            } break;
            default:
                throw std::invalid_argument("Unknown instruction type in 'unreversed'.");
        }
    }
    return out;
}

DetectorErrorModel ErrorAnalyzer::circuit_to_detector_error_model(
    const Circuit &circuit,
    bool decompose_errors,
    bool fold_loops,
    bool allow_gauge_detectors,
    double approximate_disjoint_errors_threshold,
    bool ignore_decomposition_failures,
    bool block_decomposition_from_introducing_remnant_edges) {
    ErrorAnalyzer analyzer(
        circuit.count_measurements(),
        circuit.count_detectors(),
        circuit.count_qubits(),
        circuit.count_ticks(),
        decompose_errors,
        fold_loops,
        allow_gauge_detectors,
        approximate_disjoint_errors_threshold,
        ignore_decomposition_failures,
        block_decomposition_from_introducing_remnant_edges);
    analyzer.current_circuit_being_analyzed = &circuit;
    analyzer.undo_circuit(circuit);
    analyzer.post_check_initialization();
    analyzer.flush();
    uint64_t t = 0;
    std::set<DemTarget> seen;
    return unreversed(analyzer.flushed_reversed_model, t, seen);
}

void ErrorAnalyzer::flush() {
    do_global_error_decomposition_pass();
    for (auto kv = error_class_probabilities.crbegin(); kv != error_class_probabilities.crend(); kv++) {
        const ErrorEquivalenceClass &key = kv->first;
        const double &probability = kv->second;
        if (key.targets.empty() || probability == 0) {
            continue;
        }
        flushed_reversed_model.append_error_instruction(probability, key.targets, key.tag);
    }
    error_class_probabilities.clear();
}

ErrorEquivalenceClass ErrorAnalyzer::add_xored_error(
    double probability, SpanRef<const DemTarget> flipped1, SpanRef<const DemTarget> flipped2, std::string_view tag) {
    mono_buf.ensure_available(flipped1.size() + flipped2.size());
    mono_buf.tail.ptr_end = xor_merge_sort(flipped1, flipped2, mono_buf.tail.ptr_end);
    return add_error_in_sorted_jagged_tail(probability, tag);
}

ErrorEquivalenceClass ErrorAnalyzer::mono_dedupe_store_tail(std::string_view tag) {
    auto v = error_class_probabilities.find(ErrorEquivalenceClass{mono_buf.tail, tag});
    if (v != error_class_probabilities.end()) {
        mono_buf.discard_tail();
        return v->first;
    }
    auto result = ErrorEquivalenceClass{mono_buf.commit_tail(), tag};
    error_class_probabilities.insert({result, 0});
    return result;
}

ErrorEquivalenceClass ErrorAnalyzer::mono_dedupe_store(ErrorEquivalenceClass sorted) {
    auto v = error_class_probabilities.find(sorted);
    if (v != error_class_probabilities.end()) {
        return v->first;
    }
    mono_buf.append_tail(sorted.targets);
    auto result = ErrorEquivalenceClass{mono_buf.commit_tail(), sorted.tag};
    error_class_probabilities.insert({result, 0});
    return result;
}

ErrorEquivalenceClass ErrorAnalyzer::add_error(double probability, SpanRef<const DemTarget> flipped_sorted, std::string_view tag) {
    auto key = mono_dedupe_store(ErrorEquivalenceClass{flipped_sorted, tag});
    auto &old_p = error_class_probabilities[key];
    old_p = old_p * (1 - probability) + (1 - old_p) * probability;
    return key;
}

ErrorEquivalenceClass ErrorAnalyzer::add_error_in_sorted_jagged_tail(double probability, std::string_view tag) {
    auto key = mono_dedupe_store_tail(tag);
    auto &old_p = error_class_probabilities[key];
    old_p = old_p * (1 - probability) + (1 - old_p) * probability;
    return key;
}

void ErrorAnalyzer::run_loop(const Circuit &loop, uint64_t iterations, std::string_view loop_tag) {
    if (!fold_loops) {
        // If loop folding is disabled, just manually run each iteration.
        for (size_t k = 0; k < iterations; k++) {
            undo_circuit(loop);
        }
        return;
    }

    uint64_t hare_iter = 0;
    uint64_t tortoise_iter = 0;
    ErrorAnalyzer hare(
        tracker.num_measurements_in_past,
        tracker.num_detectors_in_past,
        tracker.xs.size(),
        num_ticks_in_past,
        false,
        true,
        allow_gauge_detectors,
        approximate_disjoint_errors_threshold,
        false,
        false);
    hare.tracker = tracker;
    hare.accumulate_errors = false;

    // Perform tortoise-and-hare cycle finding.
    while (hare_iter < iterations) {
        try {
            hare.undo_circuit(loop);
        } catch (const std::invalid_argument &) {
            // Encountered an error. Abort loop folding so it can be re-triggered in a normal way.
            hare_iter = iterations;
            break;
        }
        hare_iter++;
        if (hare.tracker.is_shifted_copy(tracker)) {
            break;
        }

        if (hare_iter % 2 == 0) {
            undo_circuit(loop);
            tortoise_iter++;
            if (hare.tracker.is_shifted_copy(tracker)) {
                break;
            }
        }
    }

    if (hare_iter < iterations) {
        // Don't bother folding a single iteration into a repeated block.
        uint64_t period = hare_iter - tortoise_iter;
        uint64_t period_iterations = (iterations - tortoise_iter) / period;
        uint64_t ticks_per_period = num_ticks_in_past - hare.num_ticks_in_past;
        uint64_t detectors_per_period = tracker.num_detectors_in_past - hare.tracker.num_detectors_in_past;
        uint64_t measurements_per_period = tracker.num_measurements_in_past - hare.tracker.num_measurements_in_past;
        if (period_iterations > 1) {
            // Stash error model build up so far.
            flush();
            DetectorErrorModel tmp = std::move(flushed_reversed_model);

            // Rewrite state to look like it would if loop had executed all but the last iteration.
            uint64_t skipped_periods = period_iterations - 1;
            tracker.shift(
                -(int64_t)(skipped_periods * measurements_per_period),
                -(int64_t)(skipped_periods * detectors_per_period));
            num_ticks_in_past -= skipped_periods * ticks_per_period;
            tortoise_iter += skipped_periods * period;

            // Compute the loop's error model.
            for (size_t k = 0; k < period; k++) {
                undo_circuit(loop);
                tortoise_iter++;
            }
            flush();
            DetectorErrorModel body = std::move(flushed_reversed_model);

            // The loop ends (well, starts because everything is reversed) by shifting the detector coordinates.
            uint64_t lower_level_shifts = body.total_detector_shift();
            DemTarget remaining_shift = {detectors_per_period - lower_level_shifts};
            if (remaining_shift.data > 0) {
                if (body.instructions.empty() ||
                    body.instructions.front().type != DemInstructionType::DEM_SHIFT_DETECTORS) {
                    auto shift_targets = body.target_buf.take_copy(SpanRef<const DemTarget>(&remaining_shift));
                    body.instructions.insert(
                        body.instructions.begin(),
                        DemInstruction{
                            .arg_data={},
                            .target_data=shift_targets,
                            .tag="",
                            .type=DemInstructionType::DEM_SHIFT_DETECTORS,
                        });
                } else {
                    remaining_shift.data += body.instructions[0].target_data[0].data;
                    auto shift_targets = body.target_buf.take_copy(SpanRef<const DemTarget>(&remaining_shift));
                    body.instructions[0].target_data = shift_targets;
                }
            }

            // Append the loop to the growing error model and put the error model back in its proper place.
            tmp.append_repeat_block(period_iterations, std::move(body), loop_tag);
            flushed_reversed_model = std::move(tmp);
        }
    }

    // Perform remaining loop iterations leftover after jumping forward by multiples of the recurrence period.
    while (tortoise_iter < iterations) {
        undo_circuit(loop);
        tortoise_iter++;
    }
}

void ErrorAnalyzer::undo_SHIFT_COORDS(const CircuitInstruction &inst) {
    flushed_reversed_model.append_shift_detectors_instruction(inst.args, 0, inst.tag);
}

template <size_t s>
void ErrorAnalyzer::decompose_helper_add_error_combinations(
    const std::array<uint64_t, 1 << s> &detector_masks, std::array<SpanRef<const DemTarget>, 1 << s> &stored_ids, std::string_view tag) {
    // Count number of detectors affected by each error.
    std::array<uint8_t, 1 << s> detector_counts{};
    for (size_t k = 1; k < 1 << s; k++) {
        detector_counts[k] = std::popcount(detector_masks[k]);
    }

    // Find single-detector errors (and empty errors).
    uint64_t solved = 0;
    uint64_t single_detectors_union = 0;
    for (size_t k = 1; k < 1 << s; k++) {
        if (detector_counts[k] == 1) {
            single_detectors_union |= detector_masks[k];
            solved |= 1 << k;
        }
    }

    // Find irreducible double-detector errors.
    FixedCapVector<uint8_t, 1 << s> irreducible_pairs{};
    for (size_t k = 1; k < 1 << s; k++) {
        if (detector_counts[k] == 2 && (detector_masks[k] & ~single_detectors_union)) {
            irreducible_pairs.push_back(k);
            solved |= 1 << k;
        }
    }

    auto append_involved_pairs_to_jag_tail = [&](size_t goal_k) -> uint64_t {
        uint64_t goal = detector_masks[goal_k];

        // If single-detector excitations are sufficient, just use those.
        if ((goal & ~single_detectors_union) == 0) {
            return goal;
        }

        // Check if one double-detector excitation can get us into the single-detector region.
        for (auto k : irreducible_pairs) {
            auto m = detector_masks[k];
            if ((goal & m) == m && (goal & ~(single_detectors_union | m)) == 0) {
                mono_buf.append_tail(stored_ids[k]);
                mono_buf.append_tail(DemTarget::separator());
                return goal & ~m;
            }
        }

        // Check if two double-detector excitations can get us into the single-detector region.
        for (size_t i1 = 0; i1 < irreducible_pairs.size(); i1++) {
            auto k1 = irreducible_pairs[i1];
            auto m1 = detector_masks[k1];
            for (size_t i2 = i1 + 1; i2 < irreducible_pairs.size(); i2++) {
                auto k2 = irreducible_pairs[i2];
                auto m2 = detector_masks[k2];
                if ((m1 & m2) == 0 && (goal & ~(single_detectors_union | m1 | m2)) == 0) {
                    if (stored_ids[k2] < stored_ids[k1]) {
                        std::swap(k1, k2);
                    }
                    mono_buf.append_tail(stored_ids[k1]);
                    mono_buf.append_tail(DemTarget::separator());
                    mono_buf.append_tail(stored_ids[k2]);
                    mono_buf.append_tail(DemTarget::separator());
                    return goal & ~(m1 | m2);
                }
            }
        }

        // Failed to decompose into other components of the same composite Pauli channel.
        // Put it into the result undecomposed, to be worked on more later.
        mono_buf.append_tail(stored_ids[goal_k]);
        mono_buf.append_tail(DemTarget::separator());
        return 0;
    };

    // Solve the decomposition of each composite case.
    for (size_t k = 1; k < 1 << s; k++) {
        if (detector_counts[k] && ((solved >> k) & 1) == 0) {
            auto remnants = append_involved_pairs_to_jag_tail(k);

            // Finish off the solution using single-detector components.
            for (size_t k2 = 0; remnants && k2 < 1 << s; k2++) {
                if (detector_counts[k2] == 1 && (detector_masks[k2] & ~remnants) == 0) {
                    remnants &= ~detector_masks[k2];
                    mono_buf.append_tail(stored_ids[k2]);
                    mono_buf.append_tail(DemTarget::separator());
                }
            }
            if (!mono_buf.tail.empty()) {
                mono_buf.tail.ptr_end -= 1;
            }
            stored_ids[k] = mono_dedupe_store_tail(tag).targets;
        }
    }
}

bool stim::is_graphlike(const SpanRef<const DemTarget> &components) {
    size_t symptom_count = 0;
    for (const auto &t : components) {
        if (t.is_separator()) {
            symptom_count = 0;
        } else if (t.is_relative_detector_id()) {
            symptom_count++;
            if (symptom_count > 2) {
                return false;
            }
        }
    }
    return true;
}

bool ErrorAnalyzer::has_unflushed_ungraphlike_errors() const {
    for (const auto &kv : error_class_probabilities) {
        const auto &component = kv.first;
        if (kv.second != 0 && !is_graphlike(component.targets)) {
            return true;
        }
    }
    return false;
}

bool ErrorAnalyzer::decompose_and_append_component_to_tail(
    SpanRef<const DemTarget> component,
    const std::map<FixedCapVector<DemTarget, 2>, SpanRef<const DemTarget>> &known_symptoms) {
    std::vector<bool> done(component.size(), false);

    size_t num_component_detectors = 0;
    for (size_t k = 0; k < component.size(); k++) {
        if (component[k].is_relative_detector_id()) {
            num_component_detectors++;
        } else {
            done[k] = true;
        }
    }
    if (num_component_detectors <= 2) {
        mono_buf.append_tail(component);
        mono_buf.append_tail(DemTarget::separator());
        return true;
    }

    SparseXorVec<DemTarget> sparse;
    sparse.xor_sorted_items(component);

    for (size_t k = 0; k < component.size(); k++) {
        if (!done[k]) {
            for (size_t k2 = k + 1; k2 < component.size(); k2++) {
                if (!done[k2]) {
                    auto p = known_symptoms.find({component[k], component[k2]});
                    if (p != known_symptoms.end()) {
                        done[k] = true;
                        done[k2] = true;
                        mono_buf.append_tail(p->second);
                        mono_buf.append_tail(DemTarget::separator());
                        sparse.xor_sorted_items(p->second);
                        break;
                    }
                }
            }
        }
    }

    size_t missed = 0;
    for (size_t k = 0; k < component.size(); k++) {
        if (!done[k]) {
            auto p = known_symptoms.find({component[k]});
            if (p != known_symptoms.end()) {
                done[k] = true;
                mono_buf.append_tail(p->second);
                mono_buf.append_tail(DemTarget::separator());
                sparse.xor_sorted_items(p->second);
            }
        }
        missed += !done[k];
    }

    if (missed <= 2) {
        if (!sparse.empty()) {
            mono_buf.append_tail({sparse.begin(), sparse.end()});
            mono_buf.append_tail(DemTarget::separator());
        }
        return true;
    }

    mono_buf.discard_tail();
    return false;
}

std::pair<uint64_t, uint64_t> obs_mask_of_targets(SpanRef<const DemTarget> targets) {
    uint64_t obs_mask = 0;
    uint64_t used_mask = 0;
    for (size_t k = 0; k < targets.size(); k++) {
        const auto &t = targets[k];
        if (t.is_observable_id()) {
            if (t.val() >= 64) {
                throw std::invalid_argument("Not implemented: decomposing errors observable ids larger than 63.");
            }
            obs_mask |= uint64_t{1} << t.val();
            used_mask |= uint64_t{1} << k;
        }
    }
    return {obs_mask, used_mask};
}

bool brute_force_decomp_helper(
    size_t start,
    uint64_t used_term_mask,
    uint64_t remaining_obs_mask,
    SpanRef<const DemTarget> problem,
    const std::map<FixedCapVector<DemTarget, 2>, SpanRef<const DemTarget>> &known_symptoms,
    std::vector<SpanRef<const DemTarget>> &out_result) {
    while (true) {
        if (start >= problem.size()) {
            return remaining_obs_mask == 0;
        }
        if (((used_term_mask >> start) & 1) == 0) {
            break;
        }
        start++;
    }
    used_term_mask |= 1 << start;

    FixedCapVector<DemTarget, 2> key;
    key.push_back(problem[start]);
    for (size_t k = start + 1; k <= problem.size(); k++) {
        if (k < problem.size()) {
            if ((used_term_mask >> k) & 1) {
                continue;
            }
            key.push_back(problem[k]);
            used_term_mask ^= 1 << k;
        }
        auto match = known_symptoms.find(key);
        if (match != known_symptoms.end()) {
            uint64_t obs_change = obs_mask_of_targets(match->second).first;
            if (brute_force_decomp_helper(
                    start + 1, used_term_mask, remaining_obs_mask ^ obs_change, problem, known_symptoms, out_result)) {
                out_result.push_back(match->second);
                return true;
            }
        }
        if (k < problem.size()) {
            key.pop_back();
            used_term_mask ^= 1 << k;
        }
    }

    return false;
}

bool stim::brute_force_decomposition_into_known_graphlike_errors(
    SpanRef<const DemTarget> problem,
    const std::map<FixedCapVector<DemTarget, 2>, SpanRef<const DemTarget>> &known_graphlike_errors,
    MonotonicBuffer<DemTarget> &output) {
    if (problem.size() >= 64) {
        throw std::invalid_argument("Not implemented: decomposing errors with more than 64 terms.");
    }

    std::vector<SpanRef<const DemTarget>> out;
    out.reserve(problem.size());
    auto prob_masks = obs_mask_of_targets(problem);

    bool result =
        brute_force_decomp_helper(0, prob_masks.second, prob_masks.first, problem, known_graphlike_errors, out);
    if (result) {
        for (auto r = out.crbegin(); r != out.crend(); r++) {
            output.append_tail(*r);
            output.append_tail(DemTarget::separator());
        }
    }
    return result;
}

void ErrorAnalyzer::do_global_error_decomposition_pass() {
    if (!decompose_errors || !has_unflushed_ungraphlike_errors()) {
        return;
    }

    std::vector<DemTarget> component_symptoms;

    // Make a map from all known symptoms singlets and pairs to actual components including frame changes.
    std::map<FixedCapVector<DemTarget, 2>, SpanRef<const DemTarget>> known_symptoms;
    for (const auto &kv : error_class_probabilities) {
        if (kv.second == 0 || kv.first.targets.empty()) {
            continue;
        }
        const auto &targets = kv.first.targets;
        size_t start = 0;
        for (size_t k = 0; k <= targets.size(); k++) {
            if (k == targets.size() || targets[k].is_separator()) {
                if (component_symptoms.size() == 1) {
                    known_symptoms[{component_symptoms[0]}] = {&targets[start], &targets[k]};
                } else if (component_symptoms.size() == 2) {
                    known_symptoms[{component_symptoms[0], component_symptoms[1]}] = {&targets[start], &targets[k]};
                }
                component_symptoms.clear();
                start = k + 1;
            } else if (targets[k].is_relative_detector_id()) {
                component_symptoms.push_back(targets[k]);
            }
        }
    }

    // Find how to rewrite hyper errors into graphlike errors.
    std::vector<std::pair<ErrorEquivalenceClass, ErrorEquivalenceClass>> rewrites;
    for (const auto &kv : error_class_probabilities) {
        if (kv.second == 0 || kv.first.targets.empty()) {
            continue;
        }

        const auto &targets = kv.first.targets;
        if (is_graphlike(targets)) {
            continue;
        }

        size_t start = 0;
        for (size_t k = 0; k <= targets.size(); k++) {
            if (k == targets.size() || targets[k].is_separator()) {
                SpanRef<const DemTarget> problem{&targets[start], &targets[k]};
                if (brute_force_decomposition_into_known_graphlike_errors(problem, known_symptoms, mono_buf)) {
                    // Solved using only existing edges.
                } else if (
                    !block_decomposition_from_introducing_remnant_edges &&
                    // We are now *really* desperate.
                    // We need to start considering decomposing into errors that
                    // don't exist, as long as they can be formed by xoring
                    // together errors that do exist. This might impact the
                    // graphlike code distance.
                    decompose_and_append_component_to_tail({&targets[start], &targets[k]}, known_symptoms)) {
                    // Solved using a remnant edge.
                } else if (ignore_decomposition_failures) {
                    mono_buf.append_tail(problem);
                    mono_buf.append_tail(DemTarget::separator());
                } else {
                    std::stringstream ss;
                    ss << "Failed to decompose errors into graphlike components with at most two symptoms.\n";
                    ss << "The error component that failed to decompose is '" << comma_sep_workaround(problem)
                       << "'.\n";
                    ss << "\n";
                    ss << "In Python, you can ignore this error by passing `ignore_decomposition_failures=True` to "
                          "`stim.Circuit.detector_error_model(...)`.\n";
                    ss << "From the command line, you can ignore this error by passing the flag "
                          "`--ignore_decomposition_failures` to `stim analyze_errors`.";
                    if (block_decomposition_from_introducing_remnant_edges) {
                        ss << "\n\nNote: `block_decomposition_from_introducing_remnant_edges` is ON.\n";
                        ss << "Turning it off may prevent this error.";
                    }
                    throw std::invalid_argument(ss.str());
                }
                start = k + 1;
            }
        }

        if (!mono_buf.tail.empty()) {
            // Drop final separator.
            mono_buf.tail.ptr_end -= 1;
        }

        rewrites.push_back({kv.first, ErrorEquivalenceClass{mono_buf.commit_tail(), kv.first.tag}});
    }

    for (const auto &rewrite : rewrites) {
        double p = error_class_probabilities[rewrite.first];
        error_class_probabilities.erase(rewrite.first);
        add_error(p, rewrite.second.targets, rewrite.second.tag);
    }
}

template <size_t s>
void ErrorAnalyzer::add_error_combinations(
    std::array<double, 1 << s> probabilities,
    std::array<SpanRef<const DemTarget>, s> basis_errors,
    bool probabilities_are_disjoint,
    std::string_view tag) {
    std::array<uint64_t, 1 << s> detector_masks{};
    FixedCapVector<DemTarget, 16> involved_detectors{};
    std::array<SpanRef<const DemTarget>, 1 << s> stored_ids;

    for (size_t k = 0; k < s; k++) {
        stored_ids[1 << k] = mono_dedupe_store(ErrorEquivalenceClass{basis_errors[k], tag}).targets;

        if (decompose_errors) {
            for (const auto &id : basis_errors[k]) {
                if (id.is_relative_detector_id()) {
                    auto r = involved_detectors.find(id);
                    if (r == involved_detectors.end()) {
                        try {
                            involved_detectors.push_back(id);
                        } catch (const std::out_of_range &) {
                            std::stringstream message;
                            message
                                << "An error case in a composite error exceeded the max supported number of symptoms "
                                   "(<=15).";
                            message << "\nThe " << std::to_string(s)
                                    << " basis error cases (e.g. X, Z) used to form the combined ";
                            message << "error cases (e.g. Y = X*Z) are:\n";
                            for (size_t k2 = 0; k2 < s; k2++) {
                                message << std::to_string(k2) << ":";
                                if (!basis_errors[k2].empty()) {
                                    message << ' ';
                                }
                                message << comma_sep_workaround(basis_errors[k2]) << "\n";
                            }
                            throw std::invalid_argument(message.str());
                        }
                    }
                    detector_masks[1 << k] ^= 1 << (r - involved_detectors.begin());
                }
            }
        }
    }

    // Fill in all 2**s - 1 possible combinations from the initial basis values.
    for (size_t k = 3; k < 1 << s; k++) {
        auto c1 = k & (k - 1);
        auto c2 = k ^ c1;
        if (c1) {
            mono_buf.ensure_available(stored_ids[c1].size() + stored_ids[c2].size());
            mono_buf.tail.ptr_end = xor_merge_sort(stored_ids[c1], stored_ids[c2], mono_buf.tail.ptr_end);
            stored_ids[k] = mono_dedupe_store_tail(tag).targets;
            detector_masks[k] = detector_masks[c1] ^ detector_masks[c2];
        }
    }

    // Determine involved detectors while creating basis masks and storing added data.
    if (decompose_errors) {
        decompose_helper_add_error_combinations<s>(detector_masks, stored_ids, tag);
    }
    if (probabilities_are_disjoint) {
        // Merge indistinguishable cases.
        for (size_t k = 1; k < 1 << s; k++) {
            if (stored_ids[k].empty()) {
                // Since symptom k is empty, merge pairs A, B such that A^B = k.
                for (size_t k_dst = 0; k_dst < 1 << s; k_dst++) {
                    size_t k_src = k_dst ^ k;
                    if (k_src > k_dst) {
                        probabilities[k_dst] += probabilities[k_src];
                        probabilities[k_src] = 0;
                    }
                }
            }
        }
    }

    // Include errors in the record.
    for (size_t k = 1; k < 1 << s; k++) {
        add_error(probabilities[k], stored_ids[k], tag);
    }
}

void ErrorAnalyzer::undo_MPP(const CircuitInstruction &inst) {
    size_t n = inst.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = inst.targets[n - k - 1];
    }
    decompose_mpp_operation(
        CircuitInstruction{GateType::MPP, inst.args, reversed_targets, inst.tag},
        tracker.xs.size(),
        [&](const CircuitInstruction &sub_inst) {
            if (sub_inst.gate_type == GateType::M) {
                reversed_measure_targets.clear();
                for (size_t k = sub_inst.targets.size(); k--;) {
                    reversed_measure_targets.push_back(sub_inst.targets[k]);
                }
                undo_MZ_with_context(
                    CircuitInstruction{GateType::M, sub_inst.args, reversed_measure_targets, sub_inst.tag},
                    "a Pauli product measurement (MPP)");
            } else {
                undo_gate(sub_inst);
            }
        });
}

void ErrorAnalyzer::undo_SPP(const CircuitInstruction &inst) {
    size_t n = inst.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = inst.targets[n - k - 1];
    }
    decompose_spp_or_spp_dag_operation(
        CircuitInstruction{GateType::SPP, inst.args, reversed_targets, inst.tag},
        tracker.xs.size(),
        false,
        [&](const CircuitInstruction &sub_inst) {
            undo_gate(sub_inst);
        });
}

void ErrorAnalyzer::undo_MXX_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    undo_ZCX(CircuitInstruction{GateType::CX, {}, inst.targets, inst.tag});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        undo_MX_with_context(
            CircuitInstruction{GateType::MX, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}, inst.tag},
            "an X-basis pair measurement (MXX)");
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    undo_ZCX(CircuitInstruction{GateType::CX, {}, inst.targets, inst.tag});
}

void ErrorAnalyzer::undo_MYY_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    undo_ZCY(CircuitInstruction{GateType::CY, {}, inst.targets, inst.tag});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        undo_MY_with_context(
            CircuitInstruction{GateType::MY, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}, inst.tag},
            "a Y-basis pair measurement (MYY)");
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    undo_ZCY(CircuitInstruction{GateType::CY, {}, inst.targets, inst.tag});
}

void ErrorAnalyzer::undo_MZZ_disjoint_controls_segment(const CircuitInstruction &inst) {
    // Transform from 2 qubit measurements to single qubit measurements.
    undo_XCZ(CircuitInstruction{GateType::XCZ, {}, inst.targets, inst.tag});

    // Record measurement results.
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        undo_MZ_with_context(
            CircuitInstruction{GateType::M, inst.args, SpanRef<const GateTarget>{&inst.targets[k]}, inst.tag},
            "a Z-basis pair measurement (MZ)");
    }

    // Untransform from single qubit measurements back to 2 qubit measurements.
    undo_XCZ(CircuitInstruction{GateType::XCZ, {}, inst.targets, inst.tag});
}

void ErrorAnalyzer::undo_MXX(const CircuitInstruction &inst) {
    size_t n = inst.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = inst.targets[n - k - 1];
    }

    decompose_pair_instruction_into_disjoint_segments(
        CircuitInstruction{inst.gate_type, inst.args, reversed_targets, inst.tag},
        tracker.xs.size(),
        [&](CircuitInstruction segment) {
            undo_MXX_disjoint_controls_segment(segment);
        });
}

void ErrorAnalyzer::undo_MYY(const CircuitInstruction &inst) {
    size_t n = inst.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = inst.targets[n - k - 1];
    }

    decompose_pair_instruction_into_disjoint_segments(
        CircuitInstruction{inst.gate_type, inst.args, reversed_targets, inst.tag},
        tracker.xs.size(),
        [&](CircuitInstruction segment) {
            undo_MYY_disjoint_controls_segment(segment);
        });
}

void ErrorAnalyzer::undo_MZZ(const CircuitInstruction &inst) {
    size_t n = inst.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = inst.targets[n - k - 1];
    }

    decompose_pair_instruction_into_disjoint_segments(
        CircuitInstruction{inst.gate_type, inst.args, reversed_targets, inst.tag},
        tracker.xs.size(),
        [&](CircuitInstruction segment) {
            undo_MZZ_disjoint_controls_segment(segment);
        });
}
