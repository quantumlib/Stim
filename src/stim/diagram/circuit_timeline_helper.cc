#include "stim/diagram/circuit_timeline_helper.h"

#include "stim/diagram/diagram_util.h"

using namespace stim;
using namespace stim_draw_internal;

void CircuitTimelineHelper::skip_loop_iterations(CircuitTimelineLoopData loop_data, uint64_t skipped_reps) {
    if (loop_data.num_repetitions > 0) {
        vec_pad_add_mul(cur_coord_shift, loop_data.shift_per_iteration, skipped_reps);
        measure_offset += loop_data.measurements_per_iteration * skipped_reps;
        detector_offset += loop_data.detectors_per_iteration * skipped_reps;
        num_ticks_seen += loop_data.ticks_per_iteration * skipped_reps;
    }
}

void CircuitTimelineHelper::do_repeat_block(const Circuit &circuit, const Operation &op) {
    const auto &body = op_data_block_body(circuit, op.target_data);
    CircuitTimelineLoopData loop_data{
        op_data_rep_count(op.target_data),
        body.count_measurements(),
        body.count_detectors(),
        body.count_ticks(),
        body.final_coord_shift(),
    };
    cur_loop_nesting.push_back(loop_data);

    if (unroll_loops) {
        for (size_t k = 0; k < loop_data.num_repetitions; k++) {
            do_circuit(body);
        }
    } else {
        start_repeat_callback(loop_data);
        do_circuit(body);
        end_repeat_callback(loop_data);
        skip_loop_iterations(loop_data, loop_data.num_repetitions - 1);
    }

    cur_loop_nesting.pop_back();
}

void CircuitTimelineHelper::do_atomic_operation(
    const Gate *gate, ConstPointerRange<double> args, ConstPointerRange<GateTarget> targets) {
    resolved_op_callback({gate, args, targets});
}

void CircuitTimelineHelper::do_operation_with_target_combiners(const Operation &op) {
    size_t start = 0;
    while (start < op.target_data.targets.size()) {
        size_t end = start + 1;
        while (end < op.target_data.targets.size() && op.target_data.targets[end].is_combiner()) {
            end += 2;
        }
        if (op.gate->flags & stim::GATE_PRODUCES_NOISY_RESULTS) {
            do_record_measure_result(op.target_data.targets[start].qubit_value());
        }
        do_atomic_operation(
            op.gate, op.target_data.args, {&op.target_data.targets[start], &op.target_data.targets[end]});
        start = end;
    }
}

void CircuitTimelineHelper::do_multi_qubit_atomic_operation(const Operation &op) {
    do_atomic_operation(op.gate, op.target_data.args, op.target_data.targets);
}

void CircuitTimelineHelper::do_two_qubit_gate(const Operation &op) {
    for (size_t k = 0; k < op.target_data.targets.size(); k += 2) {
        const GateTarget *p = &op.target_data.targets[k];
        do_atomic_operation(op.gate, op.target_data.args, {p, p + 2});
    }
}

void CircuitTimelineHelper::do_single_qubit_gate(const Operation &op) {
    for (const auto &t : op.target_data.targets) {
        if (op.gate->flags & stim::GATE_PRODUCES_NOISY_RESULTS) {
            do_record_measure_result(t.qubit_value());
        }
        do_atomic_operation(op.gate, op.target_data.args, {&t});
    }
}

GateTarget CircuitTimelineHelper::rec_to_qubit(const GateTarget &target) {
    return GateTarget::qubit(measure_index_to_qubit.get(measure_offset + (decltype(measure_offset))target.value()));
}

GateTarget CircuitTimelineHelper::pick_pseudo_target_representing_measurements(const Operation &op) {
    // First check if coordinates prefix-match a qubit's coordinates.
    if (!op.target_data.args.empty()) {
        auto coords = shifted_coordinates_in_workspace(op.target_data.args);

        for (size_t q = 0; q < latest_qubit_coords.size(); q++) {
            ConstPointerRange<double> v = latest_qubit_coords[q];
            if (!v.empty() && v.size() <= coords.size()) {
                ConstPointerRange<double> prefix = {coords.ptr_start, coords.ptr_start + v.size()};
                if (prefix == v) {
                    return GateTarget::qubit(q);
                }
            }
        }
    }

    // Otherwise fall back to picking the qubit of one of the targeted measurements.
    if (op.target_data.targets.empty()) {
        return GateTarget::qubit(0);
    }
    GateTarget pseudo_target = rec_to_qubit(op.target_data.targets[0]);
    for (const auto &t : op.target_data.targets) {
        GateTarget q = rec_to_qubit(t);
        if (q.value() < pseudo_target.value()) {
            pseudo_target = q;
        }
    }

    return pseudo_target;
}

ConstPointerRange<double> CircuitTimelineHelper::shifted_coordinates_in_workspace(ConstPointerRange<double> coords) {
    while (coord_workspace.size() < coords.size()) {
        coord_workspace.push_back(0);
    }
    for (size_t k = 0; k < coords.size(); k++) {
        coord_workspace[k] = coords[k];
        if (k < cur_coord_shift.size()) {
            coord_workspace[k] += cur_coord_shift[k];
        }
    }
    return {&coord_workspace[0], &coord_workspace[coords.size()]};
}

void CircuitTimelineHelper::do_detector(const Operation &op) {
    GateTarget pseudo_target = pick_pseudo_target_representing_measurements(op);
    targets_workspace.clear();
    targets_workspace.push_back(pseudo_target);
    targets_workspace.insert(targets_workspace.end(), op.target_data.targets.begin(), op.target_data.targets.end());
    do_atomic_operation(op.gate, shifted_coordinates_in_workspace(op.target_data.args), targets_workspace);
    detector_offset++;
}

void CircuitTimelineHelper::do_observable_include(const Operation &op) {
    GateTarget pseudo_target = pick_pseudo_target_representing_measurements(op);
    targets_workspace.clear();
    targets_workspace.push_back(pseudo_target);
    targets_workspace.insert(targets_workspace.end(), op.target_data.targets.begin(), op.target_data.targets.end());
    do_atomic_operation(op.gate, op.target_data.args, targets_workspace);
}

void CircuitTimelineHelper::do_qubit_coords(const Operation &op) {
    for (const auto &target : op.target_data.targets) {
        auto shifted = shifted_coordinates_in_workspace(op.target_data.args);

        while (target.qubit_value() >= latest_qubit_coords.size()) {
            latest_qubit_coords.push_back({});
        }
        auto &store = latest_qubit_coords[target.qubit_value()];
        store.clear();
        store.insert(store.begin(), shifted.begin(), shifted.end());

        do_atomic_operation(op.gate, shifted, {&target});
    }
}

void CircuitTimelineHelper::do_shift_coords(const Operation &op) {
    vec_pad_add_mul(cur_coord_shift, op.target_data.args);
}

void CircuitTimelineHelper::do_record_measure_result(uint32_t target_qubit) {
    u64_workspace.clear();
    for (const auto &e : cur_loop_nesting) {
        u64_workspace.push_back(e.measurements_per_iteration);
    }
    for (const auto &e : cur_loop_nesting) {
        u64_workspace.push_back(e.num_repetitions);
    }
    const uint64_t *p = u64_workspace.data();
    auto n = cur_loop_nesting.size();
    measure_index_to_qubit.set(measure_offset, {p, p + n}, {p + n, p + 2 * n}, target_qubit);
    measure_offset++;
}

void CircuitTimelineHelper::do_next_operation(const Circuit &circuit, const Operation &op) {
    if (op.gate->id == gate_name_to_id("REPEAT")) {
        do_repeat_block(circuit, op);
    } else if (op.gate->id == gate_name_to_id("MPP")) {
        do_operation_with_target_combiners(op);
    } else if (op.gate->id == gate_name_to_id("DETECTOR")) {
        do_detector(op);
    } else if (op.gate->id == gate_name_to_id("OBSERVABLE_INCLUDE")) {
        do_observable_include(op);
    } else if (op.gate->id == gate_name_to_id("SHIFT_COORDS")) {
        do_shift_coords(op);
    } else if (op.gate->id == gate_name_to_id("E") || op.gate->id == gate_name_to_id("ELSE_CORRELATED_ERROR")) {
        do_multi_qubit_atomic_operation(op);
    } else if (op.gate->id == gate_name_to_id("QUBIT_COORDS")) {
        do_qubit_coords(op);
    } else if (op.gate->id == gate_name_to_id("TICK")) {
        do_atomic_operation(op.gate, {}, {});
        num_ticks_seen += 1;
    } else if (op.gate->flags & GATE_TARGETS_PAIRS) {
        do_two_qubit_gate(op);
    } else {
        do_single_qubit_gate(op);
    }
}

void CircuitTimelineHelper::do_circuit(const Circuit &circuit) {
    for (const auto &op : circuit.operations) {
        do_next_operation(circuit, op);
    }
}
