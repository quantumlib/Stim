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

#include "stim/simulators/transform_without_feedback.h"

#include <algorithm>
#include <queue>
#include <sstream>

#include "stim/simulators/sparse_rev_frame_tracker.h"

using namespace stim;

struct WithoutFeedbackHelper {
    Circuit reversed_semi_flattened_output;

    SparseUnsignedRevFrameTracker tracker;
    SparseXorVec<DemTarget> tmp_sensitivity_buf;
    std::map<uint64_t, SparseXorVec<GateTarget>> obs_changes;
    std::map<uint64_t, SparseXorVec<uint64_t>> det_changes;

    WithoutFeedbackHelper(const Circuit &circuit)
        : tracker(circuit.count_qubits(),
                  circuit.count_measurements(),
                  circuit.count_detectors()) {
    }

    const SparseXorVec<DemTarget> &anticommuting_sensitivity_at(uint32_t qubit, bool x, bool z) {
        if (x > z) {
            return tracker.zs[qubit];
        } else if (z > x) {
            return tracker.xs[qubit];
        } else {
            tmp_sensitivity_buf.clear();
            tmp_sensitivity_buf ^= tracker.xs[qubit];
            tmp_sensitivity_buf ^= tracker.zs[qubit];
            return tmp_sensitivity_buf;
        }
    }

    void do_single_feedback(GateTarget rec, uint32_t qubit, bool x, bool z) {
        const SparseXorVec<DemTarget> &sensitivity = anticommuting_sensitivity_at(qubit, x, z);
        for (const auto &d : sensitivity) {
            if (d.is_observable_id()) {
                obs_changes[d.raw_id()].xor_item(rec);
            } else {
                det_changes[d.raw_id()].xor_item(tracker.num_measurements_in_past + rec.rec_offset());
            }
        }
    }

    void undo_feedback_capable_operation(const Operation &op) {
        for (size_t k = op.target_data.targets.size(); k > 0;) {
            k -= 2;
            Operation op_piece = {op.gate, {op.target_data.args, {&op.target_data.targets[k], &op.target_data.targets[k + 2]}}};
            auto t1 = op.target_data.targets[k];
            auto t2 = op.target_data.targets[k + 1];
            auto b1 = t1.is_measurement_record_target();
            auto b2 = t2.is_measurement_record_target();
            if (b1 > b2) {
                if (op.gate->id == gate_name_to_id("CX")) {
                    do_single_feedback(t1, t2.qubit_value(), true, false);
                } else if (op.gate->id == gate_name_to_id("CY")) {
                    do_single_feedback(t1, t2.qubit_value(), true, true);
                } else if (op.gate->id == gate_name_to_id("CZ")) {
                    do_single_feedback(t1, t2.qubit_value(), false, true);
                } else {
                    throw std::invalid_argument("Unknown feedback gate.");
                }
            } else if (b2 > b1) {
                if (op.gate->id == gate_name_to_id("CX")) {
                    do_single_feedback(t2, t1.qubit_value(), true, false);
                } else if (op.gate->id == gate_name_to_id("CY")) {
                    do_single_feedback(t2, t1.qubit_value(), true, true);
                } else if (op.gate->id == gate_name_to_id("CZ")) {
                    do_single_feedback(t2, t1.qubit_value(), false, true);
                } else {
                    throw std::invalid_argument("Unknown feedback gate.");
                }
            } else if (!b1 && !b2) {
                reversed_semi_flattened_output.operations.push_back(op_piece);
            }
            tracker.undo_operation(op_piece);
        }

        for (const auto &e : obs_changes) {
            if (!e.second.empty()) {
                reversed_semi_flattened_output.arg_buf.append_tail((double)e.first);
                reversed_semi_flattened_output.operations.push_back(Operation{
                    &GATE_DATA.at("OBSERVABLE_INCLUDE"),
                    {
                        reversed_semi_flattened_output.arg_buf.commit_tail(),
                        reversed_semi_flattened_output.target_buf.take_copy(e.second.range()),
                    },
                });
            }
        }
        obs_changes.clear();
    }

    void undo_repeat_block(const Circuit &circuit, const Operation &op) {
        const Circuit &loop = op_data_block_body(circuit, op.target_data);
        uint64_t reps = op_data_rep_count(op.target_data);

        Circuit tmp = std::move(reversed_semi_flattened_output);
        for (size_t rep = 0; rep < reps; rep++) {
            reversed_semi_flattened_output.clear();
            undo_circuit(loop);
            tmp.append_repeat_block(1, std::move(reversed_semi_flattened_output));
        }
        reversed_semi_flattened_output = std::move(tmp);
    }

    void undo_circuit(const Circuit &circuit) {
        for (size_t k = circuit.operations.size(); k--;) {
            const auto &op = circuit.operations[k];
            if (op.gate->id == gate_name_to_id("REPEAT")) {
                undo_repeat_block(circuit, op);
            } else if (op.gate->flags & GATE_CAN_TARGET_BITS) {
                undo_feedback_capable_operation(op);
            } else {
                reversed_semi_flattened_output.operations.push_back(op);
                tracker.undo_operation(op);
            }
        }
    }

    Circuit build_output(const Circuit &reversed) {
        Circuit result;

        for (size_t k = reversed.operations.size(); k--;) {
            const auto &op = reversed.operations[k];
            tracker.num_measurements_in_past += op.count_measurement_results();

            if (op.gate->id == gate_name_to_id("REPEAT")) {
                result.append_repeat_block(
                    op_data_rep_count(op.target_data),
                    build_output(op_data_block_body(reversed, op.target_data)));
                continue;
            }

            if (op.gate->id == gate_name_to_id("DETECTOR")) {
                auto p = det_changes.find(tracker.num_detectors_in_past);
                tracker.num_detectors_in_past++;
                if (p != det_changes.end()) {
                    auto &changes = p->second;
                    for (const auto &t : op.target_data.targets) {
                        changes.xor_item(tracker.num_measurements_in_past + t.rec_offset());
                    }

                    // Build new targets at tail of reversed_semi_flattened_output.
                    for (const auto &m : changes) {
                        reversed_semi_flattened_output.target_buf.append_tail(GateTarget::rec((int64_t)m - (int64_t)tracker.num_measurements_in_past));
                    }
                    result.safe_append(*op.gate, reversed_semi_flattened_output.target_buf.tail, op.target_data.args);
                    reversed_semi_flattened_output.target_buf.discard_tail();

                    continue;
                }
            }

            result.safe_append(op);
        }
        return result;
    }
};

Circuit circuit_with_identical_adjacent_loops_fused(const Circuit &circuit) {
    Circuit result;
    Circuit growing_loop;
    uint64_t loop_reps = 0;

    auto flush_loop = [&]() {
        if (loop_reps > 0) {
            growing_loop = circuit_with_identical_adjacent_loops_fused(growing_loop);
            if (loop_reps > 1) {
                result.append_repeat_block(loop_reps, std::move(growing_loop));
            } else if (loop_reps == 1) {
                result += growing_loop;
            }
        }
        loop_reps = 0;
    };
    for (const auto &op : circuit.operations) {
        bool is_loop = op.gate->id == gate_name_to_id("REPEAT");

        // Grow the growing loop or flush it if needed.
        if (loop_reps > 0) {
            if (is_loop && growing_loop == op_data_block_body(circuit, op.target_data)) {
                loop_reps += op_data_rep_count(op.target_data);
                continue;
            }
            flush_loop();
        }

        // Start a new growing loop if needed.
        assert(loop_reps == 0);
        if (is_loop) {
            growing_loop = op_data_block_body(circuit, op.target_data);
            loop_reps = op_data_rep_count(op.target_data);
            continue;
        }

        assert(loop_reps == 0);
        result.safe_append(op);
    }
    flush_loop();

    return result;
}

Circuit stim::circuit_with_inlined_feedback(const Circuit &circuit) {
    WithoutFeedbackHelper helper(circuit);
    helper.undo_circuit(circuit);
    assert(helper.tracker.num_measurements_in_past == 0);
    assert(helper.tracker.num_detectors_in_past == 0);
    return circuit_with_identical_adjacent_loops_fused(helper.build_output(helper.reversed_semi_flattened_output));
}
