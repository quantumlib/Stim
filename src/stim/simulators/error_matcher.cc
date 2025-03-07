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

#include <queue>
#include <sstream>

using namespace stim;

ErrorMatcher::ErrorMatcher(
    const Circuit &circuit, const DetectorErrorModel *init_filter, bool reduce_to_one_representative_error)
    : error_analyzer(
          circuit.count_measurements(),
          circuit.count_detectors(),
          circuit.count_qubits(),
          circuit.count_ticks(),
          false,
          false,
          true,
          1,
          false,
          false),
      cur_loc(),
      output_map(),
      allow_adding_new_dem_errors_to_output_map(init_filter == nullptr),
      reduce_to_one_representative_error(reduce_to_one_representative_error),
      dem_coords_map(),
      qubit_coords_map(circuit.get_final_qubit_coords()),
      cur_coord_offset(circuit.final_coord_shift()),
      total_measurements_in_circuit(error_analyzer.tracker.num_measurements_in_past),
      total_ticks_in_circuit(error_analyzer.num_ticks_in_past) {
    // If filtering, get the filter errors into the output map immediately.
    if (!allow_adding_new_dem_errors_to_output_map) {
        SparseXorVec<DemTarget> buf;
        init_filter->iter_flatten_error_instructions([&](const DemInstruction &instruction) {
            assert(instruction.type == DemInstructionType::DEM_ERROR);
            buf.clear();
            // Note: quadratic overhead, but typical size is 4 and 100 would be crazy big.
            for (const auto &target : instruction.target_data) {
                if (!target.is_separator()) {
                    buf.xor_item(target);
                }
            }
            dem_targets_buf.append_tail(buf.sorted_items);
            output_map.insert({dem_targets_buf.commit_tail(), {{}, {}}});
        });
    }
}

void ErrorMatcher::add_dem_error(ErrorEquivalenceClass dem_error) {
    auto entry = output_map.find(dem_error.targets);
    if (!dem_error.targets.empty() && (allow_adding_new_dem_errors_to_output_map || entry != output_map.end())) {
        // We have a desired match! Record it.
        CircuitErrorLocation new_loc = cur_loc;
        new_loc.noise_tag = dem_error.tag;
        if (cur_op != nullptr) {
            new_loc.instruction_targets.fill_args_and_targets_in_range(*cur_op, qubit_coords_map);
        }
        if (entry == output_map.end()) {
            dem_targets_buf.append_tail(dem_error.targets);
            auto stored_key = dem_targets_buf.commit_tail();
            entry = output_map.insert({stored_key, {{}, {}}}).first;
        }
        auto &out = entry->second.circuit_error_locations;
        if (out.empty() || !reduce_to_one_representative_error) {
            out.push_back(std::move(new_loc));
        } else if (new_loc.is_simpler_than(out.front())) {
            out[0] = std::move(new_loc);
        }
    }
}

void ErrorMatcher::err_atom(const CircuitInstruction &effect) {
    assert(error_analyzer.error_class_probabilities.empty());
    error_analyzer.undo_gate(effect);
    if (error_analyzer.error_class_probabilities.empty()) {
        /// Maybe there were no detectors or observables nearby? Or the noise probability was zero?
        return;
    }

    assert(error_analyzer.error_class_probabilities.size() == 1);
    ErrorEquivalenceClass dem_error = error_analyzer.error_class_probabilities.begin()->first;
    add_dem_error(dem_error);

    // Restore the pristine state.
    error_analyzer.mono_buf.clear();
    error_analyzer.error_class_probabilities.clear();
    error_analyzer.flushed_reversed_model.clear();
}

void ErrorMatcher::resolve_paulis_into(
    SpanRef<const GateTarget> targets, uint32_t target_flags, std::vector<GateTargetWithCoords> &out) {
    for (const auto &t : targets) {
        if (t.is_combiner()) {
            continue;
        }
        auto entry = qubit_coords_map.find(t.qubit_value());
        if (entry != qubit_coords_map.end()) {
            out.push_back({t, entry->second});
        } else {
            out.push_back({t, {}});
        }
        out.back().gate_target.data |= target_flags;
    }
}

void ErrorMatcher::err_xyz(const CircuitInstruction &op, uint32_t target_flags) {
    const auto &a = op.args;
    const auto &t = op.targets;

    assert(a.size() == 1);
    if (a[0] == 0) {
        return;
    }
    for (size_t k = op.targets.size(); k--;) {
        cur_loc.instruction_targets.target_range_start = k;
        cur_loc.instruction_targets.target_range_end = k + 1;
        resolve_paulis_into(&op.targets[k], target_flags, cur_loc.flipped_pauli_product);
        err_atom(CircuitInstruction{op.gate_type, a, &t[k], op.tag});
        cur_loc.flipped_pauli_product.clear();
    }
}

void ErrorMatcher::err_heralded_pauli_channel_1(const CircuitInstruction &op) {
    assert(op.args.size() == 4);
    for (size_t k = op.targets.size(); k--;) {
        auto q = op.targets[k].qubit_value();
        cur_loc.instruction_targets.target_range_start = k;
        cur_loc.instruction_targets.target_range_end = k + 1;

        cur_loc.flipped_measurement.measurement_record_index = error_analyzer.tracker.num_measurements_in_past - 1;
        SpanRef<const DemTarget> herald_symptoms =
            error_analyzer.tracker.rec_bits[error_analyzer.tracker.num_measurements_in_past - 1].range();
        SpanRef<const DemTarget> x_symptoms = error_analyzer.tracker.zs[q].range();
        SpanRef<const DemTarget> z_symptoms = error_analyzer.tracker.xs[q].range();
        if (op.args[0] != 0) {
            add_dem_error(ErrorEquivalenceClass{herald_symptoms, op.tag});
        }
        if (op.args[1] != 0) {
            error_analyzer.mono_buf.append_tail(herald_symptoms);
            error_analyzer.mono_buf.append_tail(x_symptoms);
            error_analyzer.mono_buf.tail = inplace_xor_sort(error_analyzer.mono_buf.tail);
            resolve_paulis_into(&op.targets[k], TARGET_PAULI_X_BIT, cur_loc.flipped_pauli_product);
            add_dem_error(ErrorEquivalenceClass{error_analyzer.mono_buf.tail, op.tag});
            cur_loc.flipped_pauli_product.clear();
            error_analyzer.mono_buf.discard_tail();
        }
        if (op.args[2] != 0) {
            error_analyzer.mono_buf.append_tail(herald_symptoms);
            error_analyzer.mono_buf.append_tail(x_symptoms);
            error_analyzer.mono_buf.append_tail(z_symptoms);
            error_analyzer.mono_buf.tail = inplace_xor_sort(error_analyzer.mono_buf.tail);
            resolve_paulis_into(&op.targets[k], TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT, cur_loc.flipped_pauli_product);
            add_dem_error(ErrorEquivalenceClass{error_analyzer.mono_buf.tail, op.tag});
            cur_loc.flipped_pauli_product.clear();
            error_analyzer.mono_buf.discard_tail();
        }
        if (op.args[3] != 0) {
            error_analyzer.mono_buf.append_tail(herald_symptoms);
            error_analyzer.mono_buf.append_tail(z_symptoms);
            error_analyzer.mono_buf.tail = inplace_xor_sort(error_analyzer.mono_buf.tail);
            resolve_paulis_into(&op.targets[k], TARGET_PAULI_Z_BIT, cur_loc.flipped_pauli_product);
            add_dem_error(ErrorEquivalenceClass{error_analyzer.mono_buf.tail, op.tag});
            cur_loc.flipped_pauli_product.clear();
            error_analyzer.mono_buf.discard_tail();
        }
        cur_loc.flipped_measurement.measurement_record_index = UINT64_MAX;

        assert(error_analyzer.error_class_probabilities.empty());
        error_analyzer.tracker.undo_gate(op);
        error_analyzer.mono_buf.clear();
        error_analyzer.error_class_probabilities.clear();
        error_analyzer.flushed_reversed_model.clear();
    }
}

void ErrorMatcher::err_pauli_channel_1(const CircuitInstruction &op) {
    const auto &a = op.args;
    const auto &t = op.targets;
    err_xyz(CircuitInstruction{GateType::X_ERROR, &a[0], t, op.tag}, TARGET_PAULI_X_BIT);
    err_xyz(CircuitInstruction{GateType::Y_ERROR, &a[1], t, op.tag}, TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT);
    err_xyz(CircuitInstruction{GateType::Z_ERROR, &a[2], t, op.tag}, TARGET_PAULI_Z_BIT);
}

void ErrorMatcher::err_pauli_channel_2(const CircuitInstruction &op) {
    const auto &t = op.targets;
    const auto &a = op.args;

    // Buffers and pointers into them.
    std::array<GateTarget, 2> pair;
    double p = 0;
    CircuitInstruction pair_effect = {GateType::E, &p, pair, op.tag};
    CircuitInstruction first_effect = {GateType::E, &p, &pair[0], op.tag};
    CircuitInstruction second_effect = {GateType::E, &p, &pair[1], op.tag};

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

void ErrorMatcher::err_m(const CircuitInstruction &op, uint32_t obs_mask) {
    const auto &t = op.targets;
    const auto &a = op.args;

    bool q2 = GATE_DATA[op.gate_type].flags & GATE_TARGETS_PAIRS;
    size_t end = t.size();
    while (end > 0) {
        size_t start = end - 1;
        while (start > 0 && t[start - 1].is_combiner()) {
            start -= std::min(start, size_t{2});
        }
        if (q2) {
            start--;
        }

        SpanRef<const GateTarget> slice{t.begin() + start, t.begin() + end};

        cur_loc.instruction_targets.target_range_start = start;
        cur_loc.instruction_targets.target_range_end = end;
        cur_loc.flipped_measurement.measurement_record_index = error_analyzer.tracker.num_measurements_in_past - 1;
        resolve_paulis_into(slice, obs_mask, cur_loc.flipped_measurement.measured_observable);
        err_atom(CircuitInstruction{op.gate_type, a, slice, op.tag});
        cur_loc.flipped_measurement.measurement_record_index = UINT64_MAX;
        cur_loc.flipped_measurement.measured_observable.clear();

        end = start;
    }
}

void ErrorMatcher::rev_process_instruction(const CircuitInstruction &op) {
    cur_loc.instruction_targets.gate_type = op.gate_type;
    cur_loc.instruction_targets.gate_tag = op.tag;
    auto flags = GATE_DATA[op.gate_type].flags;
    cur_loc.tick_offset = error_analyzer.num_ticks_in_past;
    cur_op = &op;

    if (op.gate_type == GateType::DETECTOR) {
        error_analyzer.undo_DETECTOR(op);
        if (!op.args.empty()) {
            auto id = error_analyzer.tracker.num_detectors_in_past;
            auto entry = dem_coords_map.insert({id, {}}).first;
            for (size_t k = 0; k < op.args.size(); k++) {
                double d = op.args[k];
                if (k < cur_coord_offset.size()) {
                    d += cur_coord_offset[k];
                }
                entry->second.push_back(d);
            }
        }
        return;
    } else if (op.gate_type == GateType::SHIFT_COORDS) {
        error_analyzer.undo_SHIFT_COORDS(op);
        for (size_t k = 0; k < op.args.size(); k++) {
            cur_coord_offset[k] -= op.args[k];
        }
        return;
    } else if (!(flags & (GATE_IS_NOISY | GATE_PRODUCES_RESULTS))) {
        error_analyzer.undo_gate(op);
        return;
    }
    switch (op.gate_type) {
        case GateType::MPAD:
            error_analyzer.undo_gate(op);
            break;
        case GateType::E:
        case GateType::ELSE_CORRELATED_ERROR: {
            cur_loc.instruction_targets.target_range_start = 0;
            cur_loc.instruction_targets.target_range_end = op.targets.size();
            resolve_paulis_into(op.targets, 0, cur_loc.flipped_pauli_product);
            CircuitInstruction op2 = op;
            op2.gate_type = GateType::E;
            err_atom(op2);
            cur_loc.flipped_pauli_product.clear();
            break;
        }
        case GateType::I_ERROR:
        case GateType::II_ERROR:
            // No effect.
            break;
        case GateType::X_ERROR:
            err_xyz(op, TARGET_PAULI_X_BIT);
            break;
        case GateType::Y_ERROR:
            err_xyz(op, TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT);
            break;
        case GateType::Z_ERROR:
            err_xyz(op, TARGET_PAULI_Z_BIT);
            break;
        case GateType::PAULI_CHANNEL_1:
            err_pauli_channel_1(op);
            break;
        case GateType::HERALDED_PAULI_CHANNEL_1:
            err_heralded_pauli_channel_1(op);
            break;
        case GateType::HERALDED_ERASE: {
            float p = op.args[0] / 4;
            std::array<double, 4> spread{p, p, p, p};
            err_heralded_pauli_channel_1(CircuitInstruction{op.gate_type, spread, op.targets, op.tag});
            break;
        }
        case GateType::DEPOLARIZE1: {
            float p = op.args[0];
            std::array<double, 3> spread{p, p, p};
            err_pauli_channel_1(CircuitInstruction{op.gate_type, spread, op.targets, op.tag});
            break;
        }
        case GateType::PAULI_CHANNEL_2:
            err_pauli_channel_2(op);
            break;
        case GateType::DEPOLARIZE2: {
            float p = op.args[0];
            std::array<double, 15> spread{p, p, p, p, p, p, p, p, p, p, p, p, p, p, p};
            err_pauli_channel_2(CircuitInstruction{op.gate_type, spread, op.targets, op.tag});
            break;
        }
        case GateType::MPP:
            err_m(op, 0);
            break;
        case GateType::MX:
        case GateType::MRX:
        case GateType::MXX:
            err_m(op, TARGET_PAULI_X_BIT);
            break;
        case GateType::MY:
        case GateType::MRY:
        case GateType::MYY:
            err_m(op, TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT);
            break;
        case GateType::M:
        case GateType::MR:
        case GateType::MZZ:
            err_m(op, TARGET_PAULI_Z_BIT);
            break;
        default:
            throw std::invalid_argument(
                "Not implemented in ErrorMatcher::rev_process_instruction: " +
                std::string(GATE_DATA[op.gate_type].name));
    }
}

void ErrorMatcher::rev_process_circuit(uint64_t reps, const Circuit &block) {
    cur_loc.stack_frames.push_back({0, 0, 0});
    cur_loc.flipped_measurement.measurement_record_index = UINT64_MAX;
    for (size_t rep = reps; rep--;) {
        cur_loc.stack_frames.back().iteration_index = rep;
        for (size_t k = block.operations.size(); k--;) {
            cur_loc.stack_frames.back().instruction_offset = k;

            const auto &op = block.operations[k];
            if (op.gate_type == GateType::REPEAT) {
                auto rep_count = op.repeat_block_rep_count();
                cur_loc.stack_frames.back().instruction_repetitions_arg = op.repeat_block_rep_count();
                rev_process_circuit(rep_count, op.repeat_block_body(block));
                cur_loc.stack_frames.back().instruction_repetitions_arg = 0;
            } else {
                rev_process_instruction(op);
            }
        }
    }
    cur_loc.stack_frames.pop_back();
}

std::vector<ExplainedError> ErrorMatcher::explain_errors_from_circuit(
    const Circuit &circuit, const DetectorErrorModel *filter, bool reduce_to_one_representative_error) {
    // Find the matches.
    ErrorMatcher finder(circuit, filter, reduce_to_one_representative_error);
    finder.rev_process_circuit(1, circuit);

    // And list them out.
    std::vector<ExplainedError> result;
    for (auto &e : finder.output_map) {
        e.second.fill_in_dem_targets(e.first, finder.dem_coords_map);
        result.push_back(std::move(e.second));
    }

    return result;
}
