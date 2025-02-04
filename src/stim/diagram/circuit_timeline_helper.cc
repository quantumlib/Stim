#include "stim/diagram/circuit_timeline_helper.h"

using namespace stim;
using namespace stim_draw_internal;

void CircuitTimelineHelper::skip_loop_iterations(const CircuitTimelineLoopData &loop_data, uint64_t skipped_reps) {
    if (loop_data.num_repetitions > 0) {
        vec_pad_add_mul(cur_coord_shift, loop_data.shift_per_iteration, skipped_reps);
        measure_offset += loop_data.measurements_per_iteration * skipped_reps;
        detector_offset += loop_data.detectors_per_iteration * skipped_reps;
        num_ticks_seen += loop_data.ticks_per_iteration * skipped_reps;
    }
}

void CircuitTimelineHelper::do_repeat_block(const Circuit &circuit, const CircuitInstruction &op) {
    const auto &body = op.repeat_block_body(circuit);
    CircuitTimelineLoopData loop_data{
        op.repeat_block_rep_count(),
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
    GateType gate_type, SpanRef<const double> args, SpanRef<const GateTarget> targets) {
    resolved_op_callback({gate_type, args, targets});
}

void CircuitTimelineHelper::do_operation_with_target_combiners(const CircuitInstruction &op) {
    bool paired = GATE_DATA[op.gate_type].flags & GATE_TARGETS_PAIRS;
    size_t start = 0;
    while (start < op.targets.size()) {
        size_t end = start + 1;
        while (end < op.targets.size() && op.targets[end].is_combiner()) {
            end += 2;
        }
        if (paired) {
            end++;
            while (end < op.targets.size() && op.targets[end].is_combiner()) {
                end += 2;
            }
        }

        if (GATE_DATA[op.gate_type].flags & GATE_PRODUCES_RESULTS) {
            do_record_measure_result(op.targets[start].qubit_value());
        }
        do_atomic_operation(op.gate_type, op.args, {&op.targets[start], &op.targets[end]});
        start = end;
    }
}

void CircuitTimelineHelper::do_multi_qubit_atomic_operation(const CircuitInstruction &op) {
    do_atomic_operation(op.gate_type, op.args, op.targets);
}

void CircuitTimelineHelper::do_two_qubit_gate(const CircuitInstruction &op) {
    for (size_t k = 0; k < op.targets.size(); k += 2) {
        const GateTarget *p = &op.targets[k];
        if (GATE_DATA[op.gate_type].flags & GATE_PRODUCES_RESULTS) {
            do_record_measure_result(p[0].qubit_value());
        }
        do_atomic_operation(op.gate_type, op.args, {p, p + 2});
    }
}

void CircuitTimelineHelper::do_single_qubit_gate(const CircuitInstruction &op) {
    for (const auto &t : op.targets) {
        if (GATE_DATA[op.gate_type].flags & GATE_PRODUCES_RESULTS) {
            do_record_measure_result(t.qubit_value());
        }
        do_atomic_operation(op.gate_type, op.args, {&t});
    }
}

GateTarget CircuitTimelineHelper::rec_to_qubit(const GateTarget &target) {
    return GateTarget::qubit(measure_index_to_qubit.get(measure_offset + (decltype(measure_offset))target.value()));
}

GateTarget CircuitTimelineHelper::pick_pseudo_target_representing_measurements(const CircuitInstruction &op) {
    for (const auto &t : op.targets) {
        if (t.is_qubit_target() || t.is_pauli_target()) {
            return t;
        }
    }
    // First check if coordinates prefix-match a qubit's coordinates.
    if (!op.args.empty()) {
        auto coords = shifted_coordinates_in_workspace(op.args);

        for (size_t q = 0; q < latest_qubit_coords.size(); q++) {
            SpanRef<const double> v = latest_qubit_coords[q];
            if (!v.empty() && v.size() <= coords.size()) {
                SpanRef<const double> prefix = {coords.ptr_start, coords.ptr_start + v.size()};
                if (prefix == v) {
                    return GateTarget::qubit((uint32_t)q);
                }
            }
        }
    }

    // Otherwise fall back to picking the qubit of one of the targeted measurements.
    if (op.targets.empty()) {
        return GateTarget::qubit(0);
    }
    GateTarget pseudo_target = rec_to_qubit(op.targets[0]);
    for (const auto &t : op.targets) {
        GateTarget q = rec_to_qubit(t);
        if (q.value() < pseudo_target.value()) {
            pseudo_target = q;
        }
    }

    return pseudo_target;
}

SpanRef<const double> CircuitTimelineHelper::shifted_coordinates_in_workspace(SpanRef<const double> coords) {
    while (coord_workspace.size() < coords.size()) {
        coord_workspace.push_back(0);
    }
    for (size_t k = 0; k < coords.size(); k++) {
        coord_workspace[k] = coords[k];
        if (k < cur_coord_shift.size()) {
            coord_workspace[k] += cur_coord_shift[k];
        }
    }
    return {coord_workspace.data(), coord_workspace.data() + coords.size()};
}

void CircuitTimelineHelper::do_detector(const CircuitInstruction &op) {
    GateTarget pseudo_target = pick_pseudo_target_representing_measurements(op);
    targets_workspace.clear();
    targets_workspace.push_back(pseudo_target);
    targets_workspace.insert(targets_workspace.end(), op.targets.begin(), op.targets.end());
    do_atomic_operation(op.gate_type, shifted_coordinates_in_workspace(op.args), targets_workspace);
    detector_offset++;
}

void CircuitTimelineHelper::do_observable_include(const CircuitInstruction &op) {
    GateTarget pseudo_target = pick_pseudo_target_representing_measurements(op);
    targets_workspace.clear();
    targets_workspace.push_back(pseudo_target);
    targets_workspace.insert(targets_workspace.end(), op.targets.begin(), op.targets.end());
    do_atomic_operation(op.gate_type, op.args, targets_workspace);
}

void CircuitTimelineHelper::do_qubit_coords(const CircuitInstruction &op) {
    for (const auto &target : op.targets) {
        auto shifted = shifted_coordinates_in_workspace(op.args);

        while (target.qubit_value() >= latest_qubit_coords.size()) {
            latest_qubit_coords.push_back({});
        }
        auto &store = latest_qubit_coords[target.qubit_value()];
        store.clear();
        store.insert(store.begin(), shifted.begin(), shifted.end());

        do_atomic_operation(op.gate_type, shifted, {&target});
    }
}

void CircuitTimelineHelper::do_shift_coords(const CircuitInstruction &op) {
    vec_pad_add_mul(cur_coord_shift, op.args);
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

void CircuitTimelineHelper::do_next_operation(const Circuit &circuit, const CircuitInstruction &op) {
    if (op.gate_type == GateType::REPEAT) {
        do_repeat_block(circuit, op);
    } else if (op.gate_type == GateType::DETECTOR) {
        do_detector(op);
    } else if (op.gate_type == GateType::OBSERVABLE_INCLUDE) {
        do_observable_include(op);
    } else if (op.gate_type == GateType::SHIFT_COORDS) {
        do_shift_coords(op);
    } else if (op.gate_type == GateType::E || op.gate_type == GateType::ELSE_CORRELATED_ERROR) {
        do_multi_qubit_atomic_operation(op);
    } else if (op.gate_type == GateType::QUBIT_COORDS) {
        do_qubit_coords(op);
    } else if (op.gate_type == GateType::TICK) {
        do_atomic_operation(op.gate_type, {}, {});
        num_ticks_seen += 1;
    } else if (GATE_DATA[op.gate_type].flags & GATE_TARGETS_COMBINERS) {
        do_operation_with_target_combiners(op);
    } else if (GATE_DATA[op.gate_type].flags & GATE_TARGETS_PAIRS) {
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
