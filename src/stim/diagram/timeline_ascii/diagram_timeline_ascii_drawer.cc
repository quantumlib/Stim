#include "stim/diagram/timeline_ascii/diagram_timeline_ascii_drawer.h"
#include "stim/diagram/diagram_util.h"

using namespace stim;
using namespace stim_draw_internal;

constexpr double GATE_ALIGNMENT_X = 0;  // Left-justify gates when time moves right.
constexpr double GATE_ALIGNMENT_Y = 0.5; // Center-justify gates when time moves down.

size_t m2x(size_t m) {
    return m * 2 + 2;
}

size_t q2y(size_t q) {
    return q * 2 + 1;
}

DiagramTimelineAsciiAlignedPos::DiagramTimelineAsciiAlignedPos(size_t x, size_t y, float align_x, float align_y) :
    x(x),
    y(y),
    align_x(align_x),
    align_y(align_y) {
}

bool DiagramTimelineAsciiAlignedPos::operator==(const DiagramTimelineAsciiAlignedPos &other) const {
    return x == other.x && y == other.y;
}

bool DiagramTimelineAsciiAlignedPos::operator<(const DiagramTimelineAsciiAlignedPos &other) const {
    if (x != other.x) {
        return x < other.x;
    }
    return y < other.y;
}

DiagramTimelineAsciiCellContents::DiagramTimelineAsciiCellContents(DiagramTimelineAsciiAlignedPos center, std::string label) :
    center(center),
    label(label) {
}

void DiagramTimelineAsciiDrawer::add_cell(DiagramTimelineAsciiCellContents cell) {
    cells.insert({cell.center, cell});
}

void DiagramTimelineAsciiDrawer::for_each_pos(const std::function<void(DiagramTimelineAsciiAlignedPos pos)> &callback) const {
    for (const auto &item : cells) {
        callback(item.first);
    }
    for (const auto &item : lines) {
        callback(item.first);
        callback(item.second);
    }
}

DiagramTimelineAsciiSizing DiagramTimelineAsciiDrawer::compute_sizing() const {
    DiagramTimelineAsciiSizing layout{0, 0, {}, {}, {}, {}};
    for_each_pos([&](DiagramTimelineAsciiAlignedPos pos) {
        layout.num_x = std::max(layout.num_x, pos.x + 1);
        layout.num_y = std::max(layout.num_y, pos.y + 1);
    });
    layout.x_spans.resize(layout.num_x, 1);
    layout.y_spans.resize(layout.num_y, 1);

    for (const auto &item : cells) {
        const auto &box = item.second;
        auto &dx = layout.x_spans[box.center.x];
        auto &dy = layout.y_spans[box.center.y];
        dx = std::max(dx, box.label.size());
        dy = std::max(dy, (size_t)1);
    }

    layout.x_offsets.push_back(0);
    layout.y_offsets.push_back(0);
    for (const auto &e : layout.x_spans) {
        layout.x_offsets.push_back(layout.x_offsets.back() + e);
    }
    for (const auto &e : layout.y_spans) {
        layout.y_offsets.push_back(layout.y_offsets.back() + e);
    }

    return layout;
}

void DiagramTimelineAsciiDrawer::do_feedback(const std::string &gate, const GateTarget &qubit_target, const GateTarget &feedback_target) {
    reserve_drawing_room_for_targets(qubit_target);

    std::stringstream ss;
    ss << gate;
    ss << "^";
    if (feedback_target.is_sweep_bit_target()) {
        ss << "sweep[" << feedback_target.value() << "]";
    } else if (feedback_target.is_measurement_record_target()) {
        ss << "rec[" << (feedback_target.value() + measure_offset) << "]";
    }
    add_cell(DiagramTimelineAsciiCellContents{
        {
            m2x(cur_moment),
            q2y(qubit_target.qubit_value()),
            GATE_ALIGNMENT_X,
            GATE_ALIGNMENT_Y,
        },
        ss.str(),
    });
}

void DiagramTimelineAsciiDrawer::do_two_qubit_gate_instance(const Operation &op, const GateTarget &target1, const GateTarget &target2) {
    auto ends = two_qubit_gate_pieces(op.gate->name);
    if (target1.is_measurement_record_target() || target1.is_sweep_bit_target()) {
        do_feedback(ends.second, target2, target1);
        return;
    }
    if (target2.is_measurement_record_target() || target2.is_sweep_bit_target()) {
        do_feedback(ends.first, target1, target2);
        return;
    }

    std::stringstream first;
    std::stringstream second;
    first << (ends.first == "Z" ? "@" : ends.first);
    second << (ends.second == "Z" ? "@" : ends.second);
    if (!op.target_data.args.empty()) {
        if (op.gate->id == gate_name_to_id("PAULI_CHANNEL_2")) {
            first << "[0]";
            second << "[1]";
        }
        first << "(" << comma_sep(op.target_data.args, ",") << ")";
        second << "(" << comma_sep(op.target_data.args, ",") << ")";
    }

    reserve_drawing_room_for_targets(target1, target2);
    add_cell(DiagramTimelineAsciiCellContents{
        {
            m2x(cur_moment),
            q2y(target1.qubit_value()),
            GATE_ALIGNMENT_X,
            GATE_ALIGNMENT_Y,
        },
        first.str(),
    });
    add_cell(DiagramTimelineAsciiCellContents{
        {
            m2x(cur_moment),
            q2y(target2.qubit_value()),
            GATE_ALIGNMENT_X,
            GATE_ALIGNMENT_Y,
        },
        second.str(),
    });
}

void DiagramTimelineAsciiDrawer::start_next_moment() {
    cur_moment++;
    cur_moment_is_used = false;
    cur_moment_used_flags.clear();
    cur_moment_used_flags.resize(num_qubits);
}

void DiagramTimelineAsciiDrawer::do_tick() {
    if (num_ticks > 0 && cur_moment > tick_start_moment) {
        size_t x1 = m2x(tick_start_moment);
        size_t x2 = m2x(cur_moment);
        size_t y1 = 0;
        size_t y2 = q2y(num_qubits - 1) + 1;

        add_cell(DiagramTimelineAsciiCellContents{
            {x1, y1, 0, 0},
            "/",
        });
        add_cell(DiagramTimelineAsciiCellContents{
            {x2, y1, 1, 0},
            "\\",
        });
        add_cell(DiagramTimelineAsciiCellContents{
            {x1, y2, 0, 1},
            "\\",
        });
        add_cell(DiagramTimelineAsciiCellContents{
            {x2, y2, 1, 0},
            "/",
        });

        lines.push_back({{x1, y1, 0.0, 0.0}, {x2, y1, 1.0, 0.0}});
        lines.push_back({{x1, y2, 0.0, 0.0}, {x2, y2, 1.0, 0.0}});
    }

    start_next_moment();
    tick_start_moment = cur_moment;
}

void DiagramTimelineAsciiDrawer::do_single_qubit_gate_instance(const Operation &op, const GateTarget &target) {
    reserve_drawing_room_for_targets(target);

    std::stringstream ss;
    ss << op.gate->name;
    if (!op.target_data.args.empty()) {
        ss << "(" << comma_sep(op.target_data.args, ",") << ")";
    }
    if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
        ss << ':';
        write_rec_index(ss);
        m2q.set(measure_offset, cur_loop_measurement_periods, cur_loop_repeat_counts, target.value());
        measure_offset++;
    }
    add_cell(DiagramTimelineAsciiCellContents{
        {
            m2x(cur_moment),
            q2y(target.qubit_value()),
            GATE_ALIGNMENT_X,
            GATE_ALIGNMENT_Y,
        },
        ss.str(),
    });
}

void DiagramTimelineAsciiDrawer::write_det_index(std::ostream &out) {
    out.put('D');
    if (!cur_loop_detector_periods.empty()) {
        out.put('[');
    }
    out << detector_offset;
    for (size_t k = 0; k < cur_loop_detector_periods.size(); k++) {
        out << "+iter";
        if (k > 0) {
            out << (k + 1);
        }
        if (cur_loop_detector_periods[k] != 1) {
            out << '*' << cur_loop_detector_periods[k];
        }
    }
    if (!cur_loop_detector_periods.empty()) {
        out.put(']');
    }
}
void DiagramTimelineAsciiDrawer::write_rec_index(std::ostream &out, int64_t lookback_shift) {
    out << "rec[";
    out << measure_offset + (decltype(measure_offset))lookback_shift;
    for (size_t k = 0; k < cur_loop_measurement_periods.size(); k++) {
        if (cur_loop_measurement_periods[k] != 0) {
            out << "+iter";
            if (k > 0) {
                out << (k + 1);
            }
            if (cur_loop_measurement_periods[k] != 1) {
                out << '*' << cur_loop_measurement_periods[k];
            }
        }
    }
    out << ']';
}

void DiagramTimelineAsciiDrawer::write_coords(std::ostream &out, ConstPointerRange<double> relative_coordinates) {
    out.put('(');
    for (size_t k = 0; k < relative_coordinates.size(); k++) {
        if (k) {
            out.put(',');
        }
        write_coord(out, k, relative_coordinates[k]);
    }
    out.put(')');
}

void DiagramTimelineAsciiDrawer::write_coord(std::ostream &out, size_t coord_index, double relative_coordinate) {
    double absolute_coordinate = relative_coordinate;
    if (coord_index < coord_shift.size()) {
        absolute_coordinate += coord_shift[coord_index];
    }
    out << absolute_coordinate;
    for (size_t k = 0; k < cur_loop_shift_periods.size(); k++) {
        const auto &p = cur_loop_shift_periods[k];
        if (coord_index < p.size() && p[coord_index] != 0) {
            out << "+iter";
            if (k > 0) {
                out << (k + 1);
            }
            if (p[coord_index] != 1) {
                out << '*' << p[coord_index];
            }
        }
    }
}

void DiagramTimelineAsciiDrawer::reserve_drawing_room_for_targets(ConstPointerRange<GateTarget> targets) {
    if (targets.empty()) {
        return;
    }

    size_t min_q = targets[0].qubit_value();
    size_t max_q = targets[0].qubit_value();
    for (const auto &t : targets) {
        if (t.is_combiner()) {
            continue;
        }
        size_t q = t.qubit_value();
        min_q = std::min(min_q, q);
        max_q = std::max(max_q, q);
    }

    for (size_t q = min_q; q <= max_q; q++) {
        if (cur_moment_used_flags[q]) {
            start_next_moment();
            break;
        }
    }
    for (size_t q = min_q; q <= max_q; q++) {
        cur_moment_used_flags[q] = true;
    }
    cur_moment_is_used = true;
    if (min_q < max_q) {
        lines.push_back({
            {
                m2x(cur_moment),
                q2y(min_q),
                GATE_ALIGNMENT_X,
                GATE_ALIGNMENT_Y,
            },
            {
                m2x(cur_moment),
                q2y(max_q),
                GATE_ALIGNMENT_X,
                GATE_ALIGNMENT_Y,
            }
        });
    }
}
void DiagramTimelineAsciiDrawer::reserve_drawing_room_for_targets(GateTarget t1, GateTarget t2) {
    std::array<GateTarget, 2> arr{t1, t2};
    reserve_drawing_room_for_targets(arr);
}
void DiagramTimelineAsciiDrawer::reserve_drawing_room_for_targets(GateTarget t) {
    reserve_drawing_room_for_targets({&t});
}

void DiagramTimelineAsciiDrawer::do_multi_qubit_gate_with_pauli_targets(const Operation &op, ConstPointerRange<GateTarget> targets) {
    reserve_drawing_room_for_targets(targets);

    const char *override_gate_name = nullptr;
    if (op.gate->id == gate_name_to_id("MPP") && targets.size() == 3) {
        if (targets[0].is_x_target() && targets[2].is_x_target()) {
            override_gate_name = "MXX";
        }
        if (targets[0].is_y_target() && targets[2].is_y_target()) {
            override_gate_name = "MYY";
        }
        if (targets[0].is_z_target() && targets[2].is_z_target()) {
            override_gate_name = "MZZ";
        }
    }
    for (const auto &t : targets) {
        if (t.is_combiner()) {
            continue;
        }
        std::stringstream ss;
        if (override_gate_name == nullptr) {
            ss << op.gate->name;
            if (t.is_x_target()) {
                ss << "[X]";
            } else if (t.is_y_target()) {
                ss << "[Y]";
            } else if (t.is_z_target()) {
                ss << "[Z]";
            }
        } else {
            ss << override_gate_name;
        }
        if (!op.target_data.args.empty()) {
            ss << "(" << comma_sep(op.target_data.args, ",") << ")";
        }
        if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
            ss << ':';
            write_rec_index(ss);
        }
        add_cell(DiagramTimelineAsciiCellContents{
            {
                m2x(cur_moment),
                q2y(t.qubit_value()),
                GATE_ALIGNMENT_X,
                GATE_ALIGNMENT_Y,
            },
            ss.str(),
        });
    }
    if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
        if (!targets.empty()) {
            m2q.set(measure_offset, cur_loop_measurement_periods, cur_loop_repeat_counts, targets[0].value());
        }
        measure_offset++;
    }
}

void DiagramTimelineAsciiDrawer::do_repeat_block(const Circuit &circuit, const Operation &op) {
    if (cur_moment_is_used) {
        do_tick();
    }

    size_t x1 = m2x(cur_moment);
    size_t y1 = 0;
    size_t y2 = q2y(num_qubits - 1) + 1;
    size_t reps = op_data_rep_count(op.target_data);

    cur_moment++;
    tick_start_moment = cur_moment;
    const auto &body = op_data_block_body(circuit, op.target_data);
    size_t measurements_per_iteration = body.count_measurements();
    size_t detectors_per_iteration = body.count_detectors();
    auto shift_per_iteration = body.final_coord_shift();
    cur_loop_repeat_counts.push_back(reps);
    cur_loop_measurement_periods.push_back(measurements_per_iteration);
    cur_loop_detector_periods.push_back(detectors_per_iteration);
    cur_loop_shift_periods.push_back(shift_per_iteration);

    do_circuit(body);

    cur_loop_measurement_periods.pop_back();
    cur_loop_detector_periods.pop_back();
    cur_loop_shift_periods.pop_back();
    cur_loop_repeat_counts.pop_back();
    vec_pad_add_mul(coord_shift, shift_per_iteration, reps - 1);
    measure_offset += measurements_per_iteration * (reps - 1);
    detector_offset += detectors_per_iteration * (reps - 1);
    if (cur_moment_is_used) {
        do_tick();
    }
    tick_start_moment = cur_moment;

    size_t x2 = m2x(cur_moment);
    DiagramTimelineAsciiAlignedPos top_left{x1, y1, 0.0, 0.0};
    DiagramTimelineAsciiAlignedPos top_right{x2, y1, 1.0, 0.0};
    DiagramTimelineAsciiAlignedPos bottom_left{x1, y2, 0.0, 1.0};
    DiagramTimelineAsciiAlignedPos bottom_right{x2, y2, 1.0, 1.0};
    lines.push_back({top_right, bottom_right});
    lines.push_back({bottom_left, top_left});

    add_cell(DiagramTimelineAsciiCellContents{
        top_left,
        "/REP " + std::to_string(reps),
    });
    add_cell(DiagramTimelineAsciiCellContents{
        top_right,
        "\\",
    });
    add_cell(DiagramTimelineAsciiCellContents{
        bottom_left,
        "\\",
    });
    add_cell(DiagramTimelineAsciiCellContents{
        bottom_right,
        "/",
    });

    start_next_moment();
    tick_start_moment = cur_moment;
}

DiagramTimelineAsciiAlignedPos DiagramTimelineAsciiAlignedPos::transposed() const {
    return {y, x, align_y, align_x};
}
DiagramTimelineAsciiCellContents DiagramTimelineAsciiCellContents::transposed() const {
    return {center.transposed(), label};
}

DiagramTimelineAsciiDrawer DiagramTimelineAsciiDrawer::transposed() const {
    DiagramTimelineAsciiDrawer result;
    for (const auto &e : cells) {
        result.cells.insert({e.first.transposed(), e.second.transposed()});
    }
    result.lines.reserve(lines.size());
    for (const auto &e : lines) {
        result.lines.push_back({e.first.transposed(), e.second.transposed()});
    }
    return result;
}
void DiagramTimelineAsciiDrawer::do_mpp(const Operation &op) {
    size_t start = 0;
    while (start < op.target_data.targets.size()) {
        size_t end = start + 1;
        while (end < op.target_data.targets.size() && op.target_data.targets[end].is_combiner()) {
            end += 2;
        }
        do_multi_qubit_gate_with_pauli_targets(op, {&op.target_data.targets[start], &op.target_data.targets[end]});
        start = end;
    }
}

void DiagramTimelineAsciiDrawer::do_correlated_error(const Operation &op) {
    if (cur_moment_is_used) {
        start_next_moment();
    }
    do_multi_qubit_gate_with_pauli_targets(op, op.target_data.targets);
}

void DiagramTimelineAsciiDrawer::do_else_correlated_error(const Operation &op) {
    do_correlated_error(op);
}

void LoopingIndexMap::set(uint64_t index, stim::ConstPointerRange<uint64_t> offsets_per_iteration, stim::ConstPointerRange<uint64_t> iteration_counts, uint32_t value) {
    if (offsets_per_iteration.empty()) {
        if (index >= brute_force_data.size()) {
            brute_force_data.resize(2 * index + 10);
        }
        brute_force_data[index] = value;
        return;
    }

    for (uint64_t k = 0; k < iteration_counts[0]; k++) {
        set(
            index + k * offsets_per_iteration[0],
            {offsets_per_iteration.ptr_start + 1, offsets_per_iteration.ptr_end},
            {iteration_counts.ptr_start + 1, iteration_counts.ptr_end},
            value);
    }
}
uint32_t LoopingIndexMap::get(uint64_t index) {
    if (index >= brute_force_data.size()) {
        return 0;
    }
    return brute_force_data[index];
}

void DiagramTimelineAsciiDrawer::do_two_qubit_gate(const Operation &op) {
    for (size_t k = 0; k < op.target_data.targets.size(); k += 2) {
        do_two_qubit_gate_instance(op, op.target_data.targets[k], op.target_data.targets[k + 1]);
    }
}

void DiagramTimelineAsciiDrawer::do_single_qubit_gate(const Operation &op) {
    for (const auto& t: op.target_data.targets) {
        do_single_qubit_gate_instance(op, t);
    }
}

GateTarget DiagramTimelineAsciiDrawer::rec_to_qubit(const GateTarget &target) {
    return GateTarget::qubit(m2q.get(measure_offset + (decltype(measure_offset))target.value()));
}

GateTarget DiagramTimelineAsciiDrawer::pick_pseudo_target_representing_measurements(const Operation &op) {
    // First check if coordinates prefix-match a qubit's coordinates.
    if (!op.target_data.args.empty()) {
        std::vector<double> coords = coord_shift;
        vec_pad_add_mul(coords, op.target_data.args);
        while (coords.size() > op.target_data.args.size()) {
            coords.pop_back();
        }
        const double *p = coords.data();
        for (size_t q = 0; q < latest_qubit_coords.size(); q++) {
            ConstPointerRange<double> v = latest_qubit_coords[q];
            if (!v.empty() && v.size() <= coords.size()) {
                ConstPointerRange<double> prefix = {p, p + v.size()};
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

void DiagramTimelineAsciiDrawer::do_detector(const Operation &op) {
    GateTarget pseudo_target = pick_pseudo_target_representing_measurements(op);
    reserve_drawing_room_for_targets(pseudo_target);

    std::stringstream ss;
    ss << "DETECTOR";
    if (!op.target_data.args.empty()) {
        write_coords(ss, op.target_data.args);
    }
    ss.put(':');
    write_det_index(ss);
    ss.put('=');
    for (size_t k = 0; k < op.target_data.targets.size(); k++) {
        if (k) {
            ss << "*";
        }
        write_rec_index(ss, op.target_data.targets[k].value());
    }
    if (op.target_data.targets.empty()) {
        ss.put('1');
    }
    add_cell(DiagramTimelineAsciiCellContents{
        {
            m2x(cur_moment),
            q2y(pseudo_target.qubit_value()),
            GATE_ALIGNMENT_X,
            GATE_ALIGNMENT_Y,
        },
        ss.str(),
    });

    detector_offset++;
}

void DiagramTimelineAsciiDrawer::do_observable_include(const Operation &op) {
    GateTarget pseudo_target = pick_pseudo_target_representing_measurements(op);
    reserve_drawing_room_for_targets(pseudo_target);

    assert(op.target_data.args.size() == 1);
    std::stringstream ss;
    ss << "OBSERVABLE_INCLUDE:L" << op.target_data.args[0] << "*=";
    for (size_t k = 0; k < op.target_data.targets.size(); k++) {
        if (k) {
            ss << "*";
        }
        write_rec_index(ss, op.target_data.targets[k].value());
    }
    if (op.target_data.targets.empty()) {
        ss.put('1');
    }
    add_cell(DiagramTimelineAsciiCellContents{
        {
            m2x(cur_moment),
            q2y(pseudo_target.qubit_value()),
            GATE_ALIGNMENT_X,
            GATE_ALIGNMENT_Y,
        },
        ss.str(),
    });
}

void DiagramTimelineAsciiDrawer::do_qubit_coords(const Operation &op) {
    for (const auto &target : op.target_data.targets) {
        reserve_drawing_room_for_targets(target);

        while (latest_qubit_coords.size() <= target.qubit_value()) {
            latest_qubit_coords.push_back({});
        }
        auto &stored = latest_qubit_coords[target.qubit_value()];
        vec_pad_add_mul(stored, op.target_data.args);
        vec_pad_add_mul(stored, coord_shift);
        while (stored.size() > op.target_data.args.size()) {
            stored.pop_back();
        }

        std::stringstream ss;
        ss << op.gate->name;
        write_coords(ss, op.target_data.args);
        add_cell(DiagramTimelineAsciiCellContents{
            {
                m2x(cur_moment),
                q2y(target.qubit_value()),
                GATE_ALIGNMENT_X,
                GATE_ALIGNMENT_Y,
            },
            ss.str(),
        });
    }
}

void DiagramTimelineAsciiDrawer::do_shift_coords(const Operation &op) {
    vec_pad_add_mul(coord_shift, op.target_data.args);
}

void DiagramTimelineAsciiDrawer::do_next_operation(const Circuit &circuit, const Operation &op) {
    if (op.gate->id == gate_name_to_id("REPEAT")) {
        do_repeat_block(circuit, op);
    } else if (op.gate->id == gate_name_to_id("MPP")) {
        do_mpp(op);
    } else if (op.gate->id == gate_name_to_id("DETECTOR")) {
        do_detector(op);
    } else if (op.gate->id == gate_name_to_id("OBSERVABLE_INCLUDE")) {
        do_observable_include(op);
    } else if (op.gate->id == gate_name_to_id("SHIFT_COORDS")) {
        do_shift_coords(op);
    } else if (op.gate->id == gate_name_to_id("E")) {
        do_correlated_error(op);
    } else if (op.gate->id == gate_name_to_id("ELSE_CORRELATED_ERROR")) {
        do_else_correlated_error(op);
    } else if (op.gate->id == gate_name_to_id("TICK")) {
        do_tick();
    } else if (op.gate->id == gate_name_to_id("QUBIT_COORDS")) {
        do_qubit_coords(op);
    } else if (op.gate->flags & GATE_TARGETS_PAIRS) {
        do_two_qubit_gate(op);
    } else {
        do_single_qubit_gate(op);
    }
}

void DiagramTimelineAsciiDrawer::do_circuit(const Circuit &circuit) {
    for (const auto &op : circuit.operations) {
        do_next_operation(circuit, op);
    }
}

DiagramTimelineAsciiDrawer DiagramTimelineAsciiDrawer::from_circuit(const Circuit &circuit) {
    DiagramTimelineAsciiDrawer diagram;
    diagram.num_qubits = circuit.count_qubits();
    diagram.num_ticks = circuit.count_ticks();
    diagram.cur_moment_used_flags.resize(diagram.num_qubits);
    diagram.do_circuit(circuit);

    diagram.lines.insert(diagram.lines.begin(), diagram.num_qubits, {{0, 0, 0.0, 0.5}, {0, 0, 1.0, 0.5}});
    for (size_t q = 0; q < diagram.num_qubits; q++) {
        diagram.lines[q] = {
            {0, q2y(q), 1.0, 0.5},
            {m2x(diagram.cur_moment) + 1, q2y(q), 1.0, 0.5},
        };
        diagram.add_cell(DiagramTimelineAsciiCellContents{
            {0, q2y(q), 1.0, 0.5},
            "q" + std::to_string(q) + ": ",
        });
    }

    return diagram;
}

void strip_padding_from_lines_and_write_to(PointerRange<std::string> out_lines, std::ostream &out) {
    // Strip spacing at end of lines and end of diagram.
    for (auto &line : out_lines) {
        while (!line.empty() && line.back() == ' ') {
            line.pop_back();
        }
    }

    // Strip empty lines at start and end.
    while (!out_lines.empty() && out_lines.back().empty()) {
        out_lines.ptr_end--;
    }
    while (!out_lines.empty() && out_lines.front().empty()) {
        out_lines.ptr_start++;
    }

    // Find indentation.
    size_t indentation = SIZE_MAX;
    for (const auto &line : out_lines) {
        size_t k = 0;
        while (k < line.length() && line[k] == ' ') {
            k++;
        }
        indentation = std::min(indentation, k);
    }

    // Output while stripping empty lines at start of diagram.
    for (const auto &line : out_lines) {
        out.write(line.data() + indentation, line.size() - indentation);
        out.put('\n');
    }
}

void DiagramTimelineAsciiDrawer::render(std::ostream &out) const {
    DiagramTimelineAsciiSizing layout = compute_sizing();

    std::vector<std::string> out_lines;
    out_lines.resize(layout.y_offsets.back());
    for (auto &line : out_lines) {
        line.resize(layout.x_offsets.back(), ' ');
    }

    for (const auto &line : lines) {
        auto &p1 = line.first;
        auto &p2 = line.second;
        auto x = layout.x_offsets[p1.x];
        auto y = layout.y_offsets[p1.y];
        auto x2 = layout.x_offsets[p2.x];
        auto y2 = layout.y_offsets[p2.y];
        x += (int)floor(p1.align_x * (layout.x_spans[p1.x] - 1));
        y += (int)floor(p1.align_y * (layout.y_spans[p1.y] - 1));
        x2 += (int)floor(p2.align_x * (layout.x_spans[p2.x] - 1));
        y2 += (int)floor(p2.align_y * (layout.y_spans[p2.y] - 1));
        while (x != x2) {
            out_lines[y][x] = '-';
            x += x < x2 ? 1 : -1;
        }
        if (p1.x != p2.x && p1.y != p2.y) {
            out_lines[y][x] = '.';
        } else if (p1.x != p2.x) {
            out_lines[y][x] = '-';
        } else if (p1.y != p2.y) {
            out_lines[y][x] = '|';
        } else {
            out_lines[y][x] = '.';
        }
        while (y != y2) {
            y += y < y2 ? 1 : -1;
            out_lines[y][x] = '|';
        }
    }

    for (const auto &item : cells) {
        const auto &box = item.second;
        auto x = layout.x_offsets[box.center.x];
        auto y = layout.y_offsets[box.center.y];
        x += (int)floor(box.center.align_x * (layout.x_spans[box.center.x] - box.label.size()));
        y += (int)floor(box.center.align_y * (layout.y_spans[box.center.y] - 1));
        for (size_t k = 0; k < box.label.size(); k++) {
            out_lines[y][x + k] = box.label[k];
        }
    }

    strip_padding_from_lines_and_write_to(out_lines, out);
}

std::string DiagramTimelineAsciiDrawer::str() const {
    std::stringstream ss;
    render(ss);
    return ss.str();
}

std::ostream &stim_draw_internal::operator<<(std::ostream &out, const DiagramTimelineAsciiDrawer &drawer) {
    drawer.render(out);
    return out;
}
