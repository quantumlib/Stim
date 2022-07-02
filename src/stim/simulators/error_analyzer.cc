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
#include <iomanip>
#include <queue>
#include <sstream>

#include "stim/stabilizers/pauli_string.h"

using namespace stim;

void ErrorAnalyzer::remove_gauge(ConstPointerRange<DemTarget> sorted) {
    if (sorted.empty()) {
        return;
    }
    const auto &max = sorted.back();
    // HACK: linear overhead due to not keeping an index of which detectors used where.
    for (auto &x : xs) {
        if (x.contains(max)) {
            x.xor_sorted_items(sorted);
        }
    }
    for (auto &z : zs) {
        if (z.contains(max)) {
            z.xor_sorted_items(sorted);
        }
    }
}

void ErrorAnalyzer::RX(const OperationData &dat) {
    RX_with_context(dat, "an X-basis reset (RX)");
}
void ErrorAnalyzer::RY(const OperationData &dat) {
    RY_with_context(dat, "an X-basis reset (RY)");
}
void ErrorAnalyzer::RZ(const OperationData &dat) {
    RZ_with_context(dat, "a Z-basis reset (R)");
}

void ErrorAnalyzer::RX_with_context(const OperationData &dat, const char *context_op) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        check_for_gauge(zs[q], context_op, q);
        xs[q].clear();
        zs[q].clear();
    }
}

void ErrorAnalyzer::RY_with_context(const OperationData &dat, const char *context_op) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        check_for_gauge(xs[q], zs[q], context_op, q);
        xs[q].clear();
        zs[q].clear();
    }
}

void ErrorAnalyzer::RZ_with_context(const OperationData &dat, const char *context_op) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        check_for_gauge(xs[q], context_op, q);
        xs[q].clear();
        zs[q].clear();
    }
}

void ErrorAnalyzer::MX_with_context(const OperationData &dat, const char *op) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        scheduled_measurement_time++;

        std::vector<DemTarget> &d = measurement_to_detectors[scheduled_measurement_time];
        xor_sort_measurement_error(d, dat);
        xs[q].xor_sorted_items(d);
        check_for_gauge(zs[q], op, q);
        measurement_to_detectors.erase(scheduled_measurement_time);
    }
}

void ErrorAnalyzer::MY_with_context(const OperationData &dat, const char *op) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        scheduled_measurement_time++;

        std::vector<DemTarget> &d = measurement_to_detectors[scheduled_measurement_time];
        xor_sort_measurement_error(d, dat);
        xs[q].xor_sorted_items(d);
        zs[q].xor_sorted_items(d);
        check_for_gauge(xs[q], zs[q], op, q);
        measurement_to_detectors.erase(scheduled_measurement_time);
    }
}

void ErrorAnalyzer::MZ_with_context(const OperationData &dat, const char *context_op) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].qubit_value();
        scheduled_measurement_time++;

        std::vector<DemTarget> &d = measurement_to_detectors[scheduled_measurement_time];
        xor_sort_measurement_error(d, dat);

        zs[q].xor_sorted_items(d);
        check_for_gauge(xs[q], context_op, q);
        measurement_to_detectors.erase(scheduled_measurement_time);
    }
}

void ErrorAnalyzer::check_for_gauge(
    SparseXorVec<DemTarget> &potential_gauge_summand_1,
    SparseXorVec<DemTarget> &potential_gauge_summand_2,
    const char *context_op,
    uint64_t context_qubit) {
    if (potential_gauge_summand_1 == potential_gauge_summand_2) {
        return;
    }
    potential_gauge_summand_1 ^= potential_gauge_summand_2;
    check_for_gauge(potential_gauge_summand_1, context_op, context_qubit);
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
    const SparseXorVec<DemTarget> &potential_gauge, const char *context_op, uint64_t context_qubit) {
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
        remove_gauge(add_error(0.5, potential_gauge.range()));
        return;
    }

    // We are now in an error condition, and it's a bit hard to debug for the user.
    // The goal is to collect a *lot* of information that might be useful to them.

    std::stringstream error_msg;
    has_detectors &= !allow_gauge_detectors;
    if (has_observables) {
        error_msg << "The circuit contains non-deterministic observables.\n";
        error_msg << "(Error analysis requires deterministic observables.)\n";
    }
    if (has_detectors) {
        error_msg << "The circuit contains non-deterministic detectors.\n";
        error_msg << "(To allow non-deterministic detectors, use the `allow_gauge_detectors` option.)\n";
    }

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
        for (size_t q = 0; q < sensitivity.num_qubits; q++) {
            uint8_t p = sensitivity.xs[q] + sensitivity.zs[q] * 2;
            if (p) {
                error_msg_qubit_with_coords(q, p);
            }
        }
    }

    throw std::invalid_argument(error_msg.str());
}

PauliString ErrorAnalyzer::current_error_sensitivity_for(DemTarget t) const {
    PauliString result(xs.size());
    for (size_t q = 0; q < xs.size(); q++) {
        result.xs[q] = std::find(xs[q].begin(), xs[q].end(), t) != xs[q].end();
        result.zs[q] = std::find(zs[q].begin(), zs[q].end(), t) != zs[q].end();
    }
    return result;
}

void ErrorAnalyzer::xor_sort_measurement_error(std::vector<DemTarget> &d, const OperationData &dat) {
    std::sort(d.begin(), d.end());

    // Cancel duplicate pairs.
    size_t skip = 0;
    for (size_t k2 = 0; k2 < d.size(); k2++) {
        if (k2 + 1 < d.size() && d[k2] == d[k2 + 1]) {
            skip += 2;
            k2 += 1;
        } else if (skip) {
            d[k2 - skip] = d[k2];
        }
    }
    d.resize(d.size() - skip);

    // Measurement error.
    if (!dat.args.empty() && dat.args[0] > 0) {
        add_error(dat.args[0], d);
    }
}

void ErrorAnalyzer::MX(const OperationData &dat) {
    MX_with_context(dat, "an X-basis measurement (MX)");
}
void ErrorAnalyzer::MY(const OperationData &dat) {
    MY_with_context(dat, "a Y-basis measurement (MY)");
}
void ErrorAnalyzer::MZ(const OperationData &dat) {
    MZ_with_context(dat, "a Z-basis measurement (M)");
}

void ErrorAnalyzer::MRX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        OperationData d{dat.args, {&q}};
        RX_with_context(d, "an X-basis demolition measurement (MRX)");
        MX_with_context(d, "an X-basis demolition measurement (MRX)");
    }
}

void ErrorAnalyzer::MRY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        OperationData d{dat.args, {&q}};
        RY_with_context(d, "a Y-basis demolition measurement (MRY)");
        MY_with_context(d, "a Y-basis demolition measurement (MRY)");
    }
}

void ErrorAnalyzer::MRZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k];
        OperationData d{dat.args, {&q}};
        RZ_with_context(d, "a Z-basis demolition measurement (MR)");
        MZ_with_context(d, "a Z-basis demolition measurement (MR)");
    }
}

void ErrorAnalyzer::H_XZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        std::swap(xs[q], zs[q]);
    }
}

void ErrorAnalyzer::H_XY(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        zs[q] ^= xs[q];
    }
}

void ErrorAnalyzer::H_YZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        xs[q] ^= zs[q];
    }
}

void ErrorAnalyzer::C_XYZ(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        zs[q] ^= xs[q];
        xs[q] ^= zs[q];
    }
}

void ErrorAnalyzer::C_ZYX(const OperationData &dat) {
    for (size_t k = dat.targets.size(); k-- > 0;) {
        auto q = dat.targets[k].data;
        xs[q] ^= zs[q];
        zs[q] ^= xs[q];
    }
}

void ErrorAnalyzer::XCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto q1 = dat.targets[k].data;
        auto q2 = dat.targets[k + 1].data;
        xs[q1] ^= zs[q2];
        xs[q2] ^= zs[q1];
    }
}

void ErrorAnalyzer::XCY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto tx = dat.targets[k].data;
        auto ty = dat.targets[k + 1].data;
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void ErrorAnalyzer::YCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto tx = dat.targets[k + 1].data;
        auto ty = dat.targets[k].data;
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void ErrorAnalyzer::TICK(const OperationData &dat) {
    ticks_seen += 1;
}

void ErrorAnalyzer::ZCY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto c = dat.targets[k].data;
        auto t = dat.targets[k + 1].data;
        single_cy(c, t);
    }
}

void ErrorAnalyzer::YCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto t = dat.targets[k].data;
        auto c = dat.targets[k + 1].data;
        single_cy(c, t);
    }
}

void ErrorAnalyzer::YCY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[a] ^= xs[b];
        zs[a] ^= zs[b];
        xs[a] ^= xs[b];
        xs[a] ^= zs[b];

        zs[b] ^= xs[a];
        zs[b] ^= zs[a];
        xs[b] ^= xs[a];
        xs[b] ^= zs[a];
    }
}

void ErrorAnalyzer::ZCX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto c = dat.targets[k].data;
        auto t = dat.targets[k + 1].data;
        single_cx(c, t);
    }
}

void ErrorAnalyzer::SQRT_XX(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        xs[a] ^= zs[a];
        xs[a] ^= zs[b];
        xs[b] ^= zs[a];
        xs[b] ^= zs[b];
    }
}

void ErrorAnalyzer::SQRT_YY(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[a] ^= xs[a];
        zs[b] ^= xs[b];
        xs[a] ^= zs[a];
        xs[a] ^= zs[b];
        xs[b] ^= zs[a];
        xs[b] ^= zs[b];
        zs[a] ^= xs[a];
        zs[b] ^= xs[b];
    }
}

void ErrorAnalyzer::SQRT_ZZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[a] ^= xs[a];
        zs[a] ^= xs[b];
        zs[b] ^= xs[a];
        zs[b] ^= xs[b];
    }
}

void ErrorAnalyzer::feedback(uint32_t record_control, size_t target, bool x, bool z) {
    if (record_control & TARGET_SWEEP_BIT) {
        // Sweep bits have no effect on error propagation.
        return;
    }
    assert(record_control & TARGET_RECORD_BIT);

    uint64_t time = scheduled_measurement_time + (record_control & ~TARGET_RECORD_BIT);
    std::vector<DemTarget> &dst = measurement_to_detectors[time];

    // Temporarily move map's vector data into a SparseXorVec for manipulation.
    std::sort(dst.begin(), dst.end());
    SparseXorVec<DemTarget> tmp(std::move(dst));

    if (x) {
        tmp ^= xs[target];
    }
    if (z) {
        tmp ^= zs[target];
    }

    // Move data back into the map.
    dst = std::move(tmp.sorted_items);
}

void ErrorAnalyzer::single_cx(uint32_t c, uint32_t t) {
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        zs[c] ^= zs[t];
        xs[t] ^= xs[c];
    } else if (t & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT)) {
        throw std::invalid_argument(
            "Controlled X had a bit (" + GateTarget{t}.str() + ") as its target, instead of its control.");
    } else {
        feedback(c, t, false, true);
    }
}

void ErrorAnalyzer::single_cy(uint32_t c, uint32_t t) {
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        zs[c] ^= zs[t];
        zs[c] ^= xs[t];
        xs[t] ^= xs[c];
        zs[t] ^= xs[c];
    } else if (t & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT)) {
        throw std::invalid_argument(
            "Controlled Y had a bit (" + GateTarget{t}.str() + ") as its target, instead of its control.");
    } else {
        feedback(c, t, true, true);
    }
}

void ErrorAnalyzer::single_cz(uint32_t c, uint32_t t) {
    if (!((c | t) & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        zs[c] ^= xs[t];
        zs[t] ^= xs[c];
    } else if (!(t & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        feedback(c, t, true, false);
    } else if (!(c & (TARGET_RECORD_BIT | TARGET_SWEEP_BIT))) {
        feedback(t, c, true, false);
    } else {
        // Both targets are classical. No effect.
    }
}

void ErrorAnalyzer::XCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto t = dat.targets[k].data;
        auto c = dat.targets[k + 1].data;
        single_cx(c, t);
    }
}

void ErrorAnalyzer::ZCZ(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto q1 = dat.targets[k].data;
        auto q2 = dat.targets[k + 1].data;
        single_cz(q1, q2);
    }
}

void ErrorAnalyzer::I(const OperationData &dat) {
}

void ErrorAnalyzer::SWAP(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        std::swap(xs[a], xs[b]);
        std::swap(zs[a], zs[b]);
    }
}

void ErrorAnalyzer::ISWAP(const OperationData &dat) {
    for (size_t k = dat.targets.size() - 2; k + 2 != 0; k -= 2) {
        auto a = dat.targets[k].data;
        auto b = dat.targets[k + 1].data;
        zs[a] ^= xs[a];
        zs[a] ^= xs[b];
        zs[b] ^= xs[a];
        zs[b] ^= xs[b];
        std::swap(xs[a], xs[b]);
        std::swap(zs[a], zs[b]);
    }
}

void ErrorAnalyzer::DETECTOR(const OperationData &dat) {
    used_detectors++;
    auto id = DemTarget::relative_detector_id(total_detectors - used_detectors);
    for (auto t : dat.targets) {
        auto delay = t.qubit_value();
        measurement_to_detectors[scheduled_measurement_time + delay].push_back(id);
    }
    flushed_reversed_model.append_detector_instruction(dat.args, id);
}

void ErrorAnalyzer::OBSERVABLE_INCLUDE(const OperationData &dat) {
    auto id = DemTarget::observable_id((int32_t)dat.args[0]);
    for (auto t : dat.targets) {
        auto delay = t.qubit_value();
        measurement_to_detectors[scheduled_measurement_time + delay].push_back(id);
    }
    flushed_reversed_model.append_logical_observable_instruction(id);
}

ErrorAnalyzer::ErrorAnalyzer(
    uint64_t num_detectors,
    size_t num_qubits,
    bool decompose_errors,
    bool fold_loops,
    bool allow_gauge_detectors,
    double approximate_disjoint_errors_threshold,
    bool ignore_decomposition_failures,
    bool block_decomposition_from_introducing_remnant_edges)
    : total_detectors(num_detectors),
      used_detectors(0),
      xs(num_qubits),
      zs(num_qubits),
      scheduled_measurement_time(0),
      decompose_errors(decompose_errors),
      accumulate_errors(true),
      fold_loops(fold_loops),
      allow_gauge_detectors(allow_gauge_detectors),
      approximate_disjoint_errors_threshold(approximate_disjoint_errors_threshold),
      ignore_decomposition_failures(ignore_decomposition_failures),
      block_decomposition_from_introducing_remnant_edges(block_decomposition_from_introducing_remnant_edges) {
}

void ErrorAnalyzer::run_circuit(const Circuit &circuit) {
    std::vector<OperationData> stacked_else_correlated_errors;
    for (size_t k = circuit.operations.size(); k--;) {
        const auto &op = circuit.operations[k];
        assert(op.gate != nullptr);
        try {
            if (op.gate->id == gate_name_to_id("ELSE_CORRELATED_ERROR")) {
                stacked_else_correlated_errors.push_back(op.target_data);
            } else if (op.gate->id == gate_name_to_id("E")) {
                stacked_else_correlated_errors.push_back(op.target_data);
                correlated_error_block(stacked_else_correlated_errors);
                stacked_else_correlated_errors.clear();
            } else if (!stacked_else_correlated_errors.empty()) {
                throw std::invalid_argument(
                    "ELSE_CORRELATED_ERROR wasn't preceded by ELSE_CORRELATED_ERROR or CORRELATED_ERROR (E)");
            } else if (op.gate->id == gate_name_to_id("REPEAT")) {
                assert(op.target_data.targets.size() == 3);
                auto b = op.target_data.targets[0].data;
                assert(op.target_data.targets[0].data < circuit.blocks.size());
                uint64_t repeats = op_data_rep_count(op.target_data);
                const auto &block = circuit.blocks[b];
                run_loop(block, repeats);
            } else {
                (this->*op.gate->reverse_error_analyzer_function)(op.target_data);
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
                    uint64_t current_tick = total_ticks - ticks_seen;
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
    for (uint32_t q = 0; q < xs.size(); q++) {
        check_for_gauge(xs[q], "qubit initialization into |0> at the start of the circuit", q);
    }
}

void ErrorAnalyzer::X_ERROR(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : dat.targets) {
        add_error(dat.args[0], zs[q.data].range());
    }
}

void ErrorAnalyzer::Y_ERROR(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : dat.targets) {
        add_xored_error(dat.args[0], xs[q.data].range(), zs[q.data].range());
    }
}

void ErrorAnalyzer::Z_ERROR(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    for (auto q : dat.targets) {
        add_error(dat.args[0], xs[q.data].range());
    }
}

template <typename T>
inline void inplace_xor_tail(MonotonicBuffer<T> &dst, const SparseXorVec<T> &src) {
    ConstPointerRange<T> in1 = dst.tail;
    ConstPointerRange<T> in2 = src.range();
    xor_merge_sort_temp_buffer_callback(in1, in2, [&](ConstPointerRange<T> result) {
        dst.discard_tail();
        dst.append_tail(result);
    });
}

void ErrorAnalyzer::add_composite_error(double probability, ConstPointerRange<GateTarget> targets) {
    if (!accumulate_errors) {
        return;
    }
    for (auto qp : targets) {
        auto q = qp.qubit_value();
        if (qp.data & TARGET_PAULI_Z_BIT) {
            inplace_xor_tail(mono_buf, xs[q]);
        }
        if (qp.data & TARGET_PAULI_X_BIT) {
            inplace_xor_tail(mono_buf, zs[q]);
        }
    }
    add_error_in_sorted_jagged_tail(probability);
}

void ErrorAnalyzer::correlated_error_block(const std::vector<OperationData> &dats) {
    assert(!dats.empty());

    if (dats.size() == 1) {
        add_composite_error(dats[0].args[0], dats[0].targets);
        return;
    }
    check_can_approximate_disjoint("ELSE_CORRELATED_ERROR");

    double remaining_p = 1;
    for (size_t k = dats.size(); k--;) {
        OperationData dat = dats[k];
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
        add_composite_error(actual_p, dat.targets);
    }
}

void ErrorAnalyzer::CORRELATED_ERROR(const OperationData &dat) {
    add_composite_error(dat.args[0], dat.targets);
}

void ErrorAnalyzer::DEPOLARIZE1(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    if (dat.args[0] >= 3.0 / 4.0) {
        throw std::invalid_argument("Can't analyze over-mixing DEPOLARIZE1 errors (probability >= 3/4).");
    }
    double p = 0.5 - 0.5 * sqrt(1 - (4 * dat.args[0]) / 3);
    for (auto q : dat.targets) {
        add_error_combinations<2>(
            {0, p, p, p},
            {
                xs[q.data].range(),
                zs[q.data].range(),
            });
    }
}

void ErrorAnalyzer::DEPOLARIZE2(const OperationData &dat) {
    if (!accumulate_errors) {
        return;
    }
    if (dat.args[0] >= 15.0 / 16.0) {
        throw std::invalid_argument("Can't analyze over-mixing DEPOLARIZE2 errors (probability >= 15/16).");
    }
    double p = 0.5 - 0.5 * pow(1 - (16 * dat.args[0]) / 15, 0.125);
    for (size_t i = 0; i < dat.targets.size(); i += 2) {
        auto a = dat.targets[i];
        auto b = dat.targets[i + 1];
        add_error_combinations<4>(
            {0, p, p, p, p, p, p, p, p, p, p, p, p, p, p, p},
            {
                xs[a.data].range(),
                zs[a.data].range(),
                xs[b.data].range(),
                zs[b.data].range(),
            });
    }
}

void ErrorAnalyzer::ELSE_CORRELATED_ERROR(const OperationData &dat) {
    throw std::invalid_argument("Failed to analyze ELSE_CORRELATED_ERROR" + dat.str());
}

void ErrorAnalyzer::check_can_approximate_disjoint(const char *op_name) {
    if (approximate_disjoint_errors_threshold == 0) {
        std::stringstream msg;
        msg << "Encountered the operation " << op_name
            << " during error analysis, but this operation requires the `approximate_disjoint_errors` option to be "
               "enabled.";
        msg << "\nIf you're' calling from python, using stim.Circuit.detector_error_model, you need to add the "
               "argument approximate_disjoint_errors=True.\n";
        msg << "\nIf you're' calling from the command line, you need to specify --approximate_disjoint_errors.";
        throw std::invalid_argument(msg.str());
    }
}
void ErrorAnalyzer::PAULI_CHANNEL_1(const OperationData &dat) {
    check_can_approximate_disjoint("PAULI_CHANNEL_1");
    ConstPointerRange<double> args = dat.args;
    std::array<double, 4> probabilities;
    for (size_t k = 0; k < 3; k++) {
        if (args[k] > approximate_disjoint_errors_threshold) {
            throw std::invalid_argument(
                "PAULI_CHANNEL_1 has a component probability '" + std::to_string(args[k]) +
                "' larger than the "
                "`approximate_disjoint_errors` threshold of "
                "'" +
                std::to_string(approximate_disjoint_errors_threshold) + "'.");
        }
        probabilities[pauli_xyz_to_xz(k + 1)] = args[k];
    }
    if (!accumulate_errors) {
        return;
    }
    for (auto q : dat.targets) {
        add_error_combinations<2>(
            probabilities,
            {
                zs[q.data].range(),
                xs[q.data].range(),
            });
    }
}

void ErrorAnalyzer::PAULI_CHANNEL_2(const OperationData &dat) {
    check_can_approximate_disjoint("PAULI_CHANNEL_2");
    ConstPointerRange<double> args = dat.args;
    std::array<double, 16> probabilities;
    for (size_t k = 0; k < 15; k++) {
        if (args[k] > approximate_disjoint_errors_threshold) {
            throw std::invalid_argument(
                "PAULI_CHANNEL_2 has a component probability '" + std::to_string(args[k]) +
                "' larger than the "
                "`approximate_disjoint_errors` threshold of "
                "'" +
                std::to_string(approximate_disjoint_errors_threshold) + "'.");
        }
        size_t k2 = pauli_xyz_to_xz((k + 1) & 3) | (pauli_xyz_to_xz(((k + 1) >> 2) & 3) << 2);
        probabilities[k2] = args[k];
    }
    if (!accumulate_errors) {
        return;
    }
    for (size_t i = 0; i < dat.targets.size(); i += 2) {
        auto a = dat.targets[i];
        auto b = dat.targets[i + 1];
        add_error_combinations<4>(
            probabilities,
            {
                zs[b.data].range(),
                xs[b.data].range(),
                zs[a.data].range(),
                xs[a.data].range(),
            });
    }
}

DetectorErrorModel unreversed(const DetectorErrorModel &rev, uint64_t &base_detector_id, std::set<DemTarget> &seen) {
    DetectorErrorModel out;
    auto conv_append = [&](const DemInstruction &e) {
        auto stored_targets = out.target_buf.take_copy(e.target_data);
        auto stored_args = out.arg_buf.take_copy(e.arg_data);
        for (auto &t : stored_targets) {
            t.shift_if_detector_id(-(int64_t)base_detector_id);
        }
        out.instructions.push_back(DemInstruction{stored_args, stored_targets, e.type});
    };

    for (auto p = rev.instructions.crbegin(); p != rev.instructions.crend(); p++) {
        const auto &e = *p;
        switch (e.type) {
            case DEM_SHIFT_DETECTORS:
                base_detector_id += e.target_data[0].data;
                out.append_shift_detectors_instruction(e.arg_data, e.target_data[0].data);
                break;
            case DEM_ERROR:
                for (auto &t : e.target_data) {
                    seen.insert(t);
                }
                conv_append(e);
                break;
            case DEM_DETECTOR:
            case DEM_LOGICAL_OBSERVABLE:
                if (!e.arg_data.empty() || seen.find(e.target_data[0]) == seen.end()) {
                    conv_append(e);
                }
                break;
            case DEM_REPEAT_BLOCK: {
                uint64_t repetitions = e.target_data[0].data;
                if (repetitions) {
                    uint64_t old_base_detector_id = base_detector_id;
                    out.append_repeat_block(
                        e.target_data[0].data, unreversed(rev.blocks[e.target_data[1].data], base_detector_id, seen));
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
        circuit.count_detectors(),
        circuit.count_qubits(),
        decompose_errors,
        fold_loops,
        allow_gauge_detectors,
        approximate_disjoint_errors_threshold,
        ignore_decomposition_failures,
        block_decomposition_from_introducing_remnant_edges);
    analyzer.current_circuit_being_analyzed = &circuit;
    analyzer.run_circuit(circuit);
    analyzer.post_check_initialization();
    analyzer.flush();
    uint64_t t = 0;
    std::set<DemTarget> seen;
    return unreversed(analyzer.flushed_reversed_model, t, seen);
}

void ErrorAnalyzer::flush() {
    do_global_error_decomposition_pass();
    for (auto kv = error_class_probabilities.crbegin(); kv != error_class_probabilities.crend(); kv++) {
        if (kv->first.empty() || kv->second == 0) {
            continue;
        }
        flushed_reversed_model.append_error_instruction(kv->second, kv->first);
    }
    error_class_probabilities.clear();
}

ConstPointerRange<DemTarget> ErrorAnalyzer::add_xored_error(
    double probability, ConstPointerRange<DemTarget> flipped1, ConstPointerRange<DemTarget> flipped2) {
    mono_buf.ensure_available(flipped1.size() + flipped2.size());
    mono_buf.tail.ptr_end = xor_merge_sort(flipped1, flipped2, mono_buf.tail.ptr_end);
    return add_error_in_sorted_jagged_tail(probability);
}

ConstPointerRange<DemTarget> ErrorAnalyzer::mono_dedupe_store_tail() {
    auto v = error_class_probabilities.find(mono_buf.tail);
    if (v != error_class_probabilities.end()) {
        mono_buf.discard_tail();
        return v->first;
    }
    auto result = mono_buf.commit_tail();
    error_class_probabilities.insert({result, 0});
    return result;
}

ConstPointerRange<DemTarget> ErrorAnalyzer::mono_dedupe_store(ConstPointerRange<DemTarget> sorted) {
    auto v = error_class_probabilities.find(sorted);
    if (v != error_class_probabilities.end()) {
        return v->first;
    }
    mono_buf.append_tail(sorted);
    auto result = mono_buf.commit_tail();
    error_class_probabilities.insert({result, 0});
    return result;
}

ConstPointerRange<DemTarget> ErrorAnalyzer::add_error(double probability, ConstPointerRange<DemTarget> flipped_sorted) {
    auto key = mono_dedupe_store(flipped_sorted);
    auto &old_p = error_class_probabilities[key];
    old_p = old_p * (1 - probability) + (1 - old_p) * probability;
    return key;
}

ConstPointerRange<DemTarget> ErrorAnalyzer::add_error_in_sorted_jagged_tail(double probability) {
    auto key = mono_dedupe_store_tail();
    auto &old_p = error_class_probabilities[key];
    old_p = old_p * (1 - probability) + (1 - old_p) * probability;
    return key;
}

bool shifted_equals(int64_t shift, ConstPointerRange<DemTarget> unshifted, ConstPointerRange<DemTarget> expected) {
    if (unshifted.size() != expected.size()) {
        return false;
    }
    for (size_t k = 0; k < unshifted.size(); k++) {
        DemTarget a = unshifted[k];
        DemTarget e = expected[k];
        a.shift_if_detector_id(shift);
        if (a != e) {
            return false;
        }
    }
    return true;
}

bool shifted_equals(
    int64_t m_shift,
    int64_t d_shift,
    const std::map<uint64_t, std::vector<DemTarget>> &unshifted,
    const std::map<uint64_t, std::vector<DemTarget>> &expected) {
    if (unshifted.size() != expected.size()) {
        return false;
    }
    for (const auto &unshifted_entry : unshifted) {
        const auto &shifted_entry = expected.find(unshifted_entry.first + m_shift);
        if (shifted_entry == expected.end()) {
            return false;
        }
        if (!shifted_equals(d_shift, unshifted_entry.second, shifted_entry->second)) {
            return false;
        }
    }
    return true;
}

void ErrorAnalyzer::run_loop(const Circuit &loop, uint64_t iterations) {
    if (!fold_loops) {
        // If loop folding is disabled, just manually run each iteration.
        for (size_t k = 0; k < iterations; k++) {
            run_circuit(loop);
        }
        return;
    }

    uint64_t num_loop_detectors = loop.count_detectors();
    uint64_t hare_iter = 0;
    uint64_t tortoise_iter = 0;
    ErrorAnalyzer hare(
        total_detectors - used_detectors,
        xs.size(),
        false,
        true,
        allow_gauge_detectors,
        approximate_disjoint_errors_threshold,
        false,
        false);
    hare.xs = xs;
    hare.zs = zs;
    hare.ticks_seen = ticks_seen;
    hare.measurement_to_detectors = measurement_to_detectors;
    hare.scheduled_measurement_time = scheduled_measurement_time;
    hare.accumulate_errors = false;

    auto hare_is_colliding_with_tortoise = [&]() -> bool {
        // When comparing different loop iterations, shift detector ids to account for
        // detectors being introduced during each iteration.
        int64_t dt = -(int64_t)((hare_iter - tortoise_iter) * num_loop_detectors);
        int64_t mt = hare.scheduled_measurement_time - scheduled_measurement_time;
        for (size_t k = 0; k < hare.xs.size(); k++) {
            if (!shifted_equals(dt, xs[k].range(), hare.xs[k].range())) {
                return false;
            }
            if (!shifted_equals(dt, zs[k].range(), hare.zs[k].range())) {
                return false;
            }
        }
        if (!shifted_equals(mt, dt, measurement_to_detectors, hare.measurement_to_detectors)) {
            return false;
        }
        return true;
    };

    // Perform tortoise-and-hare cycle finding.
    while (hare_iter < iterations) {
        try {
            hare.run_circuit(loop);
        } catch (const std::invalid_argument &ex) {
            // Encountered an error. Abort loop folding so it can be re-triggered in a normal way.
            hare_iter = iterations;
            break;
        }
        hare_iter++;
        if (hare_is_colliding_with_tortoise()) {
            break;
        }

        if (hare_iter % 2 == 0) {
            run_circuit(loop);
            tortoise_iter++;
            if (hare_is_colliding_with_tortoise()) {
                break;
            }
        }
    }

    if (hare_iter < iterations) {
        // Don't bother folding a single iteration into a repeated block.
        uint64_t period = hare_iter - tortoise_iter;
        uint64_t period_iterations = (iterations - tortoise_iter) / period;
        uint64_t ticks_per_circuit_loop_iteration = (hare.ticks_seen - ticks_seen) / period;
        if (period_iterations > 1) {
            // Stash error model build up so far.
            flush();
            DetectorErrorModel tmp = std::move(flushed_reversed_model);

            // Rewrite state to look like it would if loop had executed all but the last iteration.
            uint64_t shift_per_iteration = period * num_loop_detectors;
            int64_t detector_shift = (int64_t)((period_iterations - 1) * shift_per_iteration);
            shift_active_detector_ids(-detector_shift);
            used_detectors += detector_shift;
            ticks_seen += (period_iterations - 1) * period * ticks_per_circuit_loop_iteration;
            tortoise_iter += (period_iterations - 1) * period;

            // Compute the loop's error model.
            for (size_t k = 0; k < period; k++) {
                run_circuit(loop);
                tortoise_iter++;
            }
            flush();
            DetectorErrorModel body = std::move(flushed_reversed_model);

            // The loop ends (well, starts because everything is reversed) by shifting the detector coordinates.
            uint64_t lower_level_shifts = body.total_detector_shift();
            DemTarget remaining_shift = {shift_per_iteration - lower_level_shifts};
            if (remaining_shift.data > 0) {
                if (body.instructions.empty() || body.instructions.front().type != DEM_SHIFT_DETECTORS) {
                    auto shift_targets = body.target_buf.take_copy({&remaining_shift});
                    body.instructions.insert(
                        body.instructions.begin(), DemInstruction{{}, shift_targets, DEM_SHIFT_DETECTORS});
                } else {
                    remaining_shift.data += body.instructions[0].target_data[0].data;
                    auto shift_targets = body.target_buf.take_copy({&remaining_shift});
                    body.instructions[0].target_data = shift_targets;
                }
            }

            // Append the loop to the growing error model and put the error model back in its proper place.
            tmp.append_repeat_block(period_iterations, std::move(body));
            flushed_reversed_model = std::move(tmp);
        }
    }

    // Perform remaining loop iterations leftover after jumping forward by multiples of the recurrence period.
    while (tortoise_iter < iterations) {
        run_circuit(loop);
        tortoise_iter++;
    }
}

void ErrorAnalyzer::shift_active_detector_ids(int64_t shift) {
    for (auto &e : measurement_to_detectors) {
        for (auto &v : e.second) {
            v.shift_if_detector_id(shift);
        }
    }
    for (auto &x : xs) {
        for (auto &v : x) {
            v.shift_if_detector_id(shift);
        }
    }
    for (auto &x : zs) {
        for (auto &v : x) {
            v.shift_if_detector_id(shift);
        }
    }
}

void ErrorAnalyzer::SHIFT_COORDS(const OperationData &dat) {
    flushed_reversed_model.append_shift_detectors_instruction(dat.args, 0);
}

template <size_t s>
void ErrorAnalyzer::decompose_helper_add_error_combinations(
    const std::array<uint64_t, 1 << s> &detector_masks, std::array<ConstPointerRange<DemTarget>, 1 << s> &stored_ids) {
    // Count number of detectors affected by each error.
    std::array<uint8_t, 1 << s> detector_counts{};
    for (size_t k = 1; k < 1 << s; k++) {
        detector_counts[k] = popcnt64(detector_masks[k]);
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
            stored_ids[k] = mono_dedupe_store_tail();
        }
    }
}

bool stim::is_graphlike(const ConstPointerRange<DemTarget> &components) {
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
        if (kv.second != 0 && !is_graphlike(component)) {
            return true;
        }
    }
    return false;
}

bool ErrorAnalyzer::decompose_and_append_component_to_tail(
    ConstPointerRange<DemTarget> component,
    const std::map<FixedCapVector<DemTarget, 2>, ConstPointerRange<DemTarget>> &known_symptoms) {
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

std::pair<uint64_t, uint64_t> obs_mask_of_targets(ConstPointerRange<DemTarget> targets) {
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
    ConstPointerRange<DemTarget> problem,
    const std::map<FixedCapVector<DemTarget, 2>, ConstPointerRange<DemTarget>> &known_symptoms,
    std::vector<ConstPointerRange<DemTarget>> &out_result) {
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
    ConstPointerRange<DemTarget> problem,
    const std::map<FixedCapVector<DemTarget, 2>, ConstPointerRange<DemTarget>> &known_graphlike_errors,
    MonotonicBuffer<DemTarget> &output) {
    if (problem.size() >= 64) {
        throw std::invalid_argument("Not implemented: decomposing errors with more than 64 terms.");
    }

    std::vector<ConstPointerRange<DemTarget>> out;
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
    std::map<FixedCapVector<DemTarget, 2>, ConstPointerRange<DemTarget>> known_symptoms;
    for (const auto &kv : error_class_probabilities) {
        if (kv.second == 0 || kv.first.empty()) {
            continue;
        }
        const auto &targets = kv.first;
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
    std::vector<std::pair<ConstPointerRange<DemTarget>, ConstPointerRange<DemTarget>>> rewrites;
    for (const auto &kv : error_class_probabilities) {
        if (kv.second == 0 || kv.first.empty()) {
            continue;
        }

        const auto &targets = kv.first;
        if (is_graphlike(targets)) {
            continue;
        }

        size_t start = 0;
        for (size_t k = 0; k <= targets.size(); k++) {
            if (k == targets.size() || targets[k].is_separator()) {
                ConstPointerRange<DemTarget> problem{&targets[start], &targets[k]};
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
                        ss << "Turning it off may prevent this error.\n";
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

        rewrites.push_back({kv.first, mono_buf.commit_tail()});
    }

    for (const auto &rewrite : rewrites) {
        double p = error_class_probabilities[rewrite.first];
        error_class_probabilities.erase(rewrite.first);
        add_error(p, rewrite.second);
    }
}

template <size_t s>
void ErrorAnalyzer::add_error_combinations(
    std::array<double, 1 << s> independent_probabilities, std::array<ConstPointerRange<DemTarget>, s> basis_errors) {
    std::array<uint64_t, 1 << s> detector_masks{};
    FixedCapVector<DemTarget, 16> involved_detectors{};
    std::array<ConstPointerRange<DemTarget>, 1 << s> stored_ids;

    for (size_t k = 0; k < s; k++) {
        for (const auto &id : basis_errors[k]) {
            if (id.is_relative_detector_id()) {
                auto r = involved_detectors.find(id);
                if (r == involved_detectors.end()) {
                    try {
                        involved_detectors.push_back(id);
                    } catch (const std::out_of_range &ex) {
                        std::stringstream message;
                        message << "An error case in a composite error exceeded that max supported number of symptoms "
                                   "(<=15). ";
                        message << "\nThe " << std::to_string(s)
                                << " basis error cases (e.g. X, Z) used to form the combined ";
                        message << "error cases (e.g. Y = X*Z) are:\n";
                        for (size_t k2 = 0; k2 < s; k2++) {
                            message << std::to_string(k2) << ": " << comma_sep_workaround(basis_errors[k2]) << "\n";
                        }
                        throw std::invalid_argument(message.str());
                    }
                }
                detector_masks[1 << k] ^= 1 << (r - involved_detectors.begin());
            }
        }
        stored_ids[1 << k] = mono_dedupe_store(basis_errors[k]);
    }

    // Fill in all 2**s - 1 possible combinations from the initial basis values.
    for (size_t k = 3; k < 1 << s; k++) {
        auto c1 = k & (k - 1);
        auto c2 = k ^ c1;
        if (c1) {
            mono_buf.ensure_available(stored_ids[c1].size() + stored_ids[c2].size());
            mono_buf.tail.ptr_end = xor_merge_sort(stored_ids[c1], stored_ids[c2], mono_buf.tail.ptr_end);
            stored_ids[k] = mono_dedupe_store_tail();
            detector_masks[k] = detector_masks[c1] ^ detector_masks[c2];
        }
    }

    // Determine involved detectors while creating basis masks and storing added data.
    if (decompose_errors) {
        decompose_helper_add_error_combinations<s>(detector_masks, stored_ids);
    }

    // Include errors in the record.
    for (size_t k = 1; k < 1 << s; k++) {
        add_error(independent_probabilities[k], stored_ids[k]);
    }
}

void ErrorAnalyzer::MPP(const OperationData &target_data) {
    size_t n = target_data.targets.size();
    std::vector<GateTarget> reversed_targets(n);
    std::vector<GateTarget> reversed_measure_targets;
    for (size_t k = 0; k < n; k++) {
        reversed_targets[k] = target_data.targets[n - k - 1];
    }
    decompose_mpp_operation(
        OperationData{target_data.args, reversed_targets},
        xs.size(),
        [&](const OperationData &h_xz,
            const OperationData &h_yz,
            const OperationData &cnot,
            const OperationData &meas) {
            H_XZ(h_xz);
            H_YZ(h_yz);
            ZCX(cnot);
            reversed_measure_targets.clear();
            for (size_t k = meas.targets.size(); k--;) {
                reversed_measure_targets.push_back(meas.targets[k]);
            }
            MZ_with_context({meas.args, reversed_measure_targets}, "a Pauli product measurement (MPP)");
            ZCX(cnot);
            H_YZ(h_yz);
            H_XZ(h_xz);
        });
}
