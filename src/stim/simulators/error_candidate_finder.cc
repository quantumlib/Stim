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

#include "stim/simulators/error_candidate_finder.h"

#include <algorithm>
#include <iomanip>
#include <queue>
#include <sstream>

using namespace stim;

ErrorCandidateFinder::ErrorCandidateFinder(
    const Circuit &circuit,
    const DetectorErrorModel &init_filter) :
      error_analyzer(circuit.count_detectors(), circuit.count_qubits(), false, false, true, 1),
      loc(),
      filter(),
      output_map(),
      total_measurements_in_circuit(circuit.count_measurements()),
      total_ticks_in_circuit(circuit.count_ticks()) {

    // Canonicalize the desired error mechanisms and get them into a set for fast searching.
    SparseXorVec<DemTarget> buf;
    init_filter.iter_flatten_error_instructions([&](const DemInstruction &instruction){
        assert(instruction.type == DEM_ERROR);
        buf.clear();
        // Note: quadratic overhead, but typical size is 4 and 100 would be crazy big.
        for (const auto &target : instruction.target_data) {
            if (!target.is_separator()) {
                buf.xor_item(target);
            }
        }
        filter_targets_buf.append_tail(buf.sorted_items);
        filter.insert(filter_targets_buf.commit_tail());
    });
}

void ErrorCandidateFinder::err_atom(
    const Operation &effect,
    const ConstPointerRange<GateTarget> &pauli_terms) {
    assert(error_analyzer.error_class_probabilities.empty());
    (error_analyzer.*effect.gate->reverse_error_analyzer_function)(effect.target_data);
    if (error_analyzer.error_class_probabilities.empty()) {
        /// Maybe there were no detectors or observables nearby?
        return;
    }

    assert(error_analyzer.error_class_probabilities.size() == 1);
    ConstPointerRange<DemTarget> dem_error_terms = error_analyzer.error_class_probabilities.begin()->first;
    if (!dem_error_terms.empty() && (filter.empty() || filter.find(dem_error_terms) != filter.end())) {
        // This copy is needed because in C++11 there's no way to replace keys in a map.
        std::vector<DemTarget> dem_error_terms_copy;
        dem_error_terms_copy.insert(dem_error_terms_copy.begin(),
                                    dem_error_terms.begin(),
                                    dem_error_terms.end());

        auto insertion = output_map.insert({dem_error_terms_copy, loc});
        bool was_inserted = insertion.second;
        auto &entry = insertion.first;
        if (was_inserted || pauli_terms.size() < entry->second.pauli_error_terms.size()) {
            if (!was_inserted) {
                dem_error_terms_copy = std::move(entry->second.dem_error_terms);
            }
            entry->second = loc;
            entry->second.pauli_error_terms.clear();
            entry->second.pauli_error_terms.insert(
                entry->second.pauli_error_terms.end(),
                pauli_terms.begin(),
                pauli_terms.end());
            entry->second.dem_error_terms = std::move(dem_error_terms_copy);
        }
    }

    // Restore the pristine state.
    error_analyzer.mono_buf.clear();
    error_analyzer.error_class_probabilities.clear();
    error_analyzer.flushed_reversed_model.clear();
}

void ErrorCandidateFinder::err_xyz(const Operation &op, uint32_t target_flags) {
    const auto &a = op.target_data.args;
    const auto &t = op.target_data.targets;

    assert(a.size() == 1);
    if (a[0] == 0) {
        return;
    }
    for (size_t k = op.target_data.targets.size(); k--;) {
        loc.instruction_target_start = k;
        loc.instruction_target_end = k + 1;
        GateTarget target = op.target_data.targets[k];
        target.data |= target_flags;
        err_atom({op.gate, {a, &t[k]}}, &target);
    }
}

void ErrorCandidateFinder::err_pauli_channel_1(const Operation &op) {
    const auto &a = op.target_data.args;
    const auto &t = op.target_data.targets;
    err_xyz(Operation{&GATE_DATA.at("X_ERROR"), {&a[0], t}}, TARGET_PAULI_X_BIT);
    err_xyz(Operation{&GATE_DATA.at("Y_ERROR"), {&a[1], t}}, TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT);
    err_xyz(Operation{&GATE_DATA.at("Z_ERROR"), {&a[2], t}}, TARGET_PAULI_Z_BIT);
}

void ErrorCandidateFinder::err_pauli_channel_2(const Operation &op) {
    const auto &t = op.target_data.targets;
    const auto &a = op.target_data.args;

    // Buffers and pointers into them.
    std::array<GateTarget, 2> pair;
    double p;
    Operation pair_effect = {&GATE_DATA.at("E"), {&p, pair}};
    Operation first_effect = {&GATE_DATA.at("E"), {&p, &pair[0]}};
    Operation second_effect = {&GATE_DATA.at("E"), {&p, &pair[1]}};

    for (size_t k = 0; k < t.size(); k += 2) {
        loc.instruction_target_start = k;
        loc.instruction_target_end = k + 2;
        for (uint8_t p1 = 0; p1 < 4; p1++) {
            for (uint8_t p2 = !p1; p2 < 4; p2++) {
                // Extract data for this term of the error.
                p = a[p1*4 + p2 - 1];
                if (p == 0) {
                    continue;
                }
                bool x1 = p1 & 1;
                bool z1 = p1 & 2;
                bool x2 = p2 & 1;
                bool z2 = p2 & 2;
                pair[0].data = t[k].data + x1 * TARGET_PAULI_X_BIT + z1 * TARGET_PAULI_Z_BIT;
                pair[1].data = t[k + 1].data + x2 * TARGET_PAULI_X_BIT + z2 * TARGET_PAULI_Z_BIT;

                // Handle the error term as if it were an isolated CORRELATED_ERROR.
                if (p1 == 0) {
                    err_atom(second_effect, &pair[1]);
                } else if (p2 == 0) {
                    err_atom(first_effect, &pair[0]);
                } else {
                    err_atom(pair_effect, pair);
                }
            }
        }
    }
}

void ErrorCandidateFinder::err_m(const Operation &op, uint32_t obs_mask) {
    const auto &t = op.target_data.targets;
    const auto &a = op.target_data.args;

    size_t end = t.size();
    while (end > 0) {
        size_t start = end - 1;
        while (t[start].is_combiner()) {
            start -= std::min(start, size_t{2});
        }

        ConstPointerRange<GateTarget> slice{t.begin() + start, t.begin() + end};
        std::vector<GateTarget> error_terms;
        for (const auto &term : slice) {
            error_terms.push_back({term.data | obs_mask});
        }
        loc.instruction_target_start = start;
        loc.instruction_target_end = end;
        loc.id_of_flipped_measurement = total_measurements_in_circuit - error_analyzer.scheduled_measurement_time - 1;
        err_atom({op.gate, {a, slice}}, error_terms);
        loc.id_of_flipped_measurement = UINT64_MAX;

        end = start;
    }
}

void ErrorCandidateFinder::rev_process_instruction(const Operation &op) {
    loc.instruction_name = op.gate->name;
    loc.ticks_beforehand = total_ticks_in_circuit - error_analyzer.ticks_seen;

    if (!(op.gate->flags & (GATE_IS_NOISE | GATE_PRODUCES_NOISY_RESULTS))) {
        (error_analyzer.*op.gate->reverse_error_analyzer_function)(op.target_data);
    } else if (op.gate->id == gate_name_to_id("E")) {
        loc.instruction_target_start = 0;
        loc.instruction_target_end = op.target_data.targets.size();
        err_atom(op, op.target_data.targets);
    } else if (op.gate->id == gate_name_to_id("X_ERROR")) {
        err_xyz(op, TARGET_PAULI_X_BIT);
    } else if (op.gate->id == gate_name_to_id("Y_ERROR")) {
        err_xyz(op, TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT);
    } else if (op.gate->id == gate_name_to_id("Z_ERROR")) {
        err_xyz(op, TARGET_PAULI_Z_BIT);
    } else if (op.gate->id == gate_name_to_id("PAULI_CHANNEL_1")) {
        err_pauli_channel_1(op);
    } else if (op.gate->id == gate_name_to_id("DEPOLARIZE1")) {
        float p = op.target_data.args[0];
        std::array<double, 3> spread{p, p, p};
        err_pauli_channel_1({op.gate, {spread, op.target_data.targets}});
    } else if (op.gate->id == gate_name_to_id("PAULI_CHANNEL_2")) {
        err_pauli_channel_2(op);
    } else if (op.gate->id == gate_name_to_id("DEPOLARIZE2")) {
        float p = op.target_data.args[0];
        std::array<double, 15> spread{p, p, p, p, p, p, p, p, p, p, p, p, p, p, p};
        err_pauli_channel_2({op.gate, {spread, op.target_data.targets}});
    } else if (op.gate->id == gate_name_to_id("MPP")) {
        err_m(op, 0);
    } else if (op.gate->id == gate_name_to_id("MX") || op.gate->id == gate_name_to_id("MRX")) {
        err_m(op, TARGET_PAULI_X_BIT);
    } else if (op.gate->id == gate_name_to_id("MY") || op.gate->id == gate_name_to_id("MRY")) {
        err_m(op, TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT);
    } else if (op.gate->id == gate_name_to_id("M") || op.gate->id == gate_name_to_id("MR")) {
        err_m(op, TARGET_PAULI_Z_BIT);
    } else {
        throw std::invalid_argument("Not implemented: " + std::string(op.gate->name));
    }
}

void ErrorCandidateFinder::rev_process_circuit(uint64_t reps, const Circuit &block) {
    loc.instruction_indices.push_back(0);
    loc.iteration_indices.push_back(0);
    for (size_t rep = reps; rep--;) {
        loc.iteration_indices.back() = rep;
        for (size_t k = block.operations.size(); k--;) {
            loc.instruction_indices.back() = k;

            const auto &op = block.operations[k];
            if (op.gate->id == gate_name_to_id("REPEAT")) {
                rev_process_circuit(
                    op_data_rep_count(op.target_data),
                    op_data_block_body(block, op.target_data));
            } else {
                rev_process_instruction(op);
            }
        }
    }
    loc.instruction_indices.pop_back();
    loc.iteration_indices.pop_back();
}

std::vector<MatchedDetectorCircuitError> ErrorCandidateFinder::candidate_localized_dem_errors_from_circuit(
        const Circuit &circuit,
        const DetectorErrorModel &filter) {

    // Find the matches.
    ErrorCandidateFinder finder(circuit, filter);
    finder.rev_process_circuit(1, circuit);

    // And list them out.
    std::vector<MatchedDetectorCircuitError> result;
    for (const auto &e : finder.output_map) {
        result.push_back(e.second);
    }

    return result;
}

std::string MatchedDetectorCircuitError::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

bool MatchedDetectorCircuitError::operator==(const MatchedDetectorCircuitError &other) const {
    return dem_error_terms == other.dem_error_terms
        && id_of_flipped_measurement == other.id_of_flipped_measurement
        && pauli_error_terms == other.pauli_error_terms
        && ticks_beforehand == other.ticks_beforehand
        && instruction_target_start == other.instruction_target_start
        && strcmp(instruction_name, other.instruction_name) == 0
        && instruction_indices == other.instruction_indices
        && iteration_indices == other.iteration_indices;
}

bool MatchedDetectorCircuitError::operator!=(const MatchedDetectorCircuitError &other) const {
    return !(*this == other);
}

void print_pauli_product(std::ostream &out, const std::vector<GateTarget> &pauli_terms) {
    for (size_t k = 0; k < pauli_terms.size(); k++) {
        if (k) {
            out << "*";
        }
        const auto &p = pauli_terms[k];
        if (p.is_x_target()) {
            out << "X";
        } else if (p.is_y_target()) {
            out << "Y";
        } else if (p.is_z_target()) {
            out << "Z";
        }
        out << p.qubit_value();
    }
}

std::ostream &stim::operator<<(std::ostream &out, const MatchedDetectorCircuitError &e) {
    out << "MatchedDetectorCircuitError {\n";
    out << "    dem_error_terms: " << comma_sep(e.dem_error_terms, " ") << "\n";
    out << "    ticks_beforehand: " << e.ticks_beforehand << "\n";

    if (e.id_of_flipped_measurement == UINT64_MAX) {
        out << "    pauli_error_terms: ";
        print_pauli_product(out, e.pauli_error_terms);
        out << "\n";
    } else {
        out << "    id_of_flipped_measurement: " << e.id_of_flipped_measurement << "\n";
        out << "    observable_of_flipped_measurement: ";
        print_pauli_product(out, e.pauli_error_terms);
        out << "\n";
    }

    out << "    Circuit location stack trace:\n";
    for (size_t k = 0; k < e.instruction_indices.size(); k++) {
        out << "        ";
        if (k < e.iteration_indices.size() && e.iteration_indices[k] > 1) {
            out << "(during iteration #" << (e.iteration_indices[k] + 1) << ") ";
        }
        out << "at instruction #" << (e.instruction_indices[k] + 1);
        if (k < e.instruction_indices.size() - 1) {
            out << " (this instruction is a REPEAT block)";
        } else {
            out << " (this instruction is a " << e.instruction_name << ")";
        }
        out << "\n";
    }
    out << "        during targets #" << (e.instruction_target_start + 1);
    out << " to #" << e.instruction_target_end << " of the instruction\n";

    out << "}";
    return out;
}
