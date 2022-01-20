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

#include "stim/simulators/error_matcher.h"

#include <algorithm>
#include <iomanip>
#include <queue>
#include <sstream>

using namespace stim;

ErrorMatcher::ErrorMatcher(const Circuit &circuit, const DetectorErrorModel &init_filter)
    : error_analyzer(circuit.count_detectors(), circuit.count_qubits(), false, false, true, 1),
      cur_loc(),
      filter(),
      output_map(),
      dem_coords_map(),
      qubit_coords_map(circuit.get_final_qubit_coords()),
      total_measurements_in_circuit(circuit.count_measurements()),
      total_ticks_in_circuit(circuit.count_ticks()) {
    // Canonicalize the desired error mechanisms and get them into a set for fast searching.
    SparseXorVec<DemTarget> buf;
    init_filter.iter_flatten_error_instructions([&](const DemInstruction &instruction) {
        assert(instruction.type == DEM_ERROR);
        buf.clear();
        // Note: quadratic overhead, but typical size is 4 and 100 would be crazy big.
        for (const auto &target : instruction.target_data) {
            if (!target.is_separator()) {
                buf.xor_item(target);
            }
        }
        dem_targets_buf.append_tail(buf.sorted_items);
        filter.insert(dem_targets_buf.commit_tail());
    });
}

void ErrorMatcher::err_atom(const Operation &effect) {
    assert(error_analyzer.error_class_probabilities.empty());
    (error_analyzer.*effect.gate->reverse_error_analyzer_function)(effect.target_data);
    if (error_analyzer.error_class_probabilities.empty()) {
        /// Maybe there were no detectors or observables nearby? Or the noise probability was zero?
        return;
    }

    assert(error_analyzer.error_class_probabilities.size() == 1);
    ConstPointerRange<DemTarget> dem_error_terms = error_analyzer.error_class_probabilities.begin()->first;
    if (!dem_error_terms.empty() && (filter.empty() || filter.find(dem_error_terms) != filter.end())) {
        // We have a desired match! Record it.
        auto entry = output_map.find(dem_error_terms);
        CircuitErrorLocation new_loc = cur_loc;
        if (cur_op != nullptr) {
            new_loc.instruction_targets.fill_args_and_targets_in_range(cur_op->target_data, qubit_coords_map);
        }
        if (entry == output_map.end()) {
            dem_targets_buf.append_tail(dem_error_terms);
            auto stored_key = dem_targets_buf.commit_tail();
            MatchedError new_match{{}, {new_loc}};
            new_match.fill_in_dem_targets(stored_key, dem_coords_map);
            output_map.insert({stored_key, std::move(new_match)});
        } else {
            entry->second.circuit_error_locations.push_back(std::move(new_loc));
        }
    }

    // Restore the pristine state.
    error_analyzer.mono_buf.clear();
    error_analyzer.error_class_probabilities.clear();
    error_analyzer.flushed_reversed_model.clear();
}

void ErrorMatcher::resolve_paulis_into(ConstPointerRange<GateTarget> targets,
                                       uint32_t target_flags,
                                       std::vector<GateTargetWithCoords> &out) {
    for (const auto &t : targets) {
        auto entry = qubit_coords_map.find(t.qubit_value());
        if (entry != qubit_coords_map.end()) {
            out.push_back({t, entry->second});
        } else {
            out.push_back({t, {}});
        }
        out.back().gate_target.data |= target_flags;
    }
}

void ErrorMatcher::err_xyz(const Operation &op, uint32_t target_flags) {
    const auto &a = op.target_data.args;
    const auto &t = op.target_data.targets;

    assert(a.size() == 1);
    if (a[0] == 0) {
        return;
    }
    for (size_t k = op.target_data.targets.size(); k--;) {
        cur_loc.instruction_targets.target_range_start = k;
        cur_loc.instruction_targets.target_range_end = k + 1;
        resolve_paulis_into(
            &op.target_data.targets[k],
            target_flags,
            cur_loc.flipped_pauli_product);
        err_atom({op.gate, {a, &t[k]}});
        cur_loc.flipped_pauli_product.clear();
    }
}

void ErrorMatcher::err_pauli_channel_1(const Operation &op) {
    const auto &a = op.target_data.args;
    const auto &t = op.target_data.targets;
    err_xyz(Operation{&GATE_DATA.at("X_ERROR"), {&a[0], t}}, TARGET_PAULI_X_BIT);
    err_xyz(Operation{&GATE_DATA.at("Y_ERROR"), {&a[1], t}}, TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT);
    err_xyz(Operation{&GATE_DATA.at("Z_ERROR"), {&a[2], t}}, TARGET_PAULI_Z_BIT);
}

void ErrorMatcher::err_pauli_channel_2(const Operation &op) {
    const auto &t = op.target_data.targets;
    const auto &a = op.target_data.args;

    // Buffers and pointers into them.
    std::array<GateTarget, 2> pair;
    double p;
    Operation pair_effect = {&GATE_DATA.at("E"), {&p, pair}};
    Operation first_effect = {&GATE_DATA.at("E"), {&p, &pair[0]}};
    Operation second_effect = {&GATE_DATA.at("E"), {&p, &pair[1]}};

    for (size_t k = 0; k < t.size(); k += 2) {
        cur_loc.instruction_targets.target_range_start = k;
        cur_loc.instruction_targets.target_range_end = k + 2;
        for (uint8_t p0 = 0; p0 < 4; p0++) {
            for (uint8_t p1 = !p0; p1 < 4; p1++) {
                // Extract data for this term of the error.
                p = a[p0 * 4 + p1 - 1];
                if (p == 0) {
                    continue;
                }
                bool x0 = p0 & 1;
                bool z0 = p0 & 2;
                bool x1 = p1 & 1;
                bool z1 = p1 & 2;
                uint32_t m0 = x0 * TARGET_PAULI_X_BIT + z0 * TARGET_PAULI_Z_BIT;
                uint32_t m1 = x1 * TARGET_PAULI_X_BIT + z1 * TARGET_PAULI_Z_BIT;
                pair[0].data = t[k].data | m0;
                pair[1].data = t[k + 1].data | m1;

                // Handle the error term as if it were an isolated CORRELATED_ERROR.
                if (p0 == 0) {
                    resolve_paulis_into(&pair[1], 0, cur_loc.flipped_pauli_product);
                    err_atom(second_effect);
                } else if (p1 == 0) {
                    resolve_paulis_into(&pair[0], 0, cur_loc.flipped_pauli_product);
                    err_atom(first_effect);
                } else {
                    resolve_paulis_into(pair, 0, cur_loc.flipped_pauli_product);
                    err_atom(pair_effect);
                }
                cur_loc.flipped_pauli_product.clear();
            }
        }
    }
}

void ErrorMatcher::err_m(const Operation &op, uint32_t obs_mask) {
    const auto &t = op.target_data.targets;
    const auto &a = op.target_data.args;

    size_t end = t.size();
    while (end > 0) {
        size_t start = end - 1;
        while (start > 0 && t[start - 1].is_combiner()) {
            start -= std::min(start, size_t{2});
        }

        ConstPointerRange<GateTarget> slice{t.begin() + start, t.begin() + end};

        cur_loc.instruction_targets.target_range_start = start;
        cur_loc.instruction_targets.target_range_end = end;
        cur_loc.flipped_measurement.measurement_record_index =
            total_measurements_in_circuit - error_analyzer.scheduled_measurement_time - 1;
        resolve_paulis_into(
            slice,
            obs_mask,
            cur_loc.flipped_measurement.measured_observable);
        err_atom({op.gate, {a, slice}});
        cur_loc.flipped_measurement.measurement_record_index = UINT64_MAX;
        cur_loc.flipped_measurement.measured_observable.clear();

        end = start;
    }
}

void ErrorMatcher::rev_process_instruction(const Operation &op) {
    cur_loc.instruction_targets.gate = op.gate;
    cur_loc.tick_offset = total_ticks_in_circuit - error_analyzer.ticks_seen;
    cur_op = &op;

    if (op.gate->id == gate_name_to_id("DETECTOR")) {
        error_analyzer.DETECTOR(op.target_data);
        if (!op.target_data.args.empty()) {
            auto id = error_analyzer.total_detectors - error_analyzer.used_detectors;
            auto entry = dem_coords_map.insert({id, {}}).first;
            entry->second.insert(entry->second.begin(), op.target_data.args.begin(), op.target_data.args.end());
        }
    } else if (!(op.gate->flags & (GATE_IS_NOISE | GATE_PRODUCES_NOISY_RESULTS))) {
        (error_analyzer.*op.gate->reverse_error_analyzer_function)(op.target_data);
    } else if (op.gate->id == gate_name_to_id("E")) {
        cur_loc.instruction_targets.target_range_start = 0;
        cur_loc.instruction_targets.target_range_end = op.target_data.targets.size();
        resolve_paulis_into(
            op.target_data.targets,
            0,
            cur_loc.flipped_pauli_product);
        err_atom(op);
        cur_loc.flipped_pauli_product.clear();
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

void ErrorMatcher::rev_process_circuit(uint64_t reps, const Circuit &block) {
    cur_loc.stack_frames.push_back({0, 0});
    cur_loc.flipped_measurement.measurement_record_index = UINT64_MAX;
    for (size_t rep = reps; rep--;) {
        cur_loc.stack_frames.back().iteration_index = rep;
        for (size_t k = block.operations.size(); k--;) {
            cur_loc.stack_frames.back().instruction_offset = k;

            const auto &op = block.operations[k];
            if (op.gate->id == gate_name_to_id("REPEAT")) {
                rev_process_circuit(op_data_rep_count(op.target_data), op_data_block_body(block, op.target_data));
            } else {
                rev_process_instruction(op);
            }
        }
    }
    cur_loc.stack_frames.pop_back();
}

std::vector<MatchedError> ErrorMatcher::match_errors_from_circuit(
    const Circuit &circuit, const DetectorErrorModel &filter) {
    // Find the matches.
    ErrorMatcher finder(circuit, filter);
    finder.rev_process_circuit(1, circuit);

    // And list them out.
    std::vector<MatchedError> result;
    for (auto &e : finder.output_map) {
        result.push_back(std::move(e.second));
    }

    return result;
}
