#include "stim/diagram/timeline_ascii/diagram_timeline_ascii_drawer.h"
#include "stim/diagram/diagram_util.h"

using namespace stim;
using namespace stim_draw_internal;

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

void DiagramTimelineAsciiDrawer::draw_feedback(const std::string &gate, const GateTarget &qubit_target, const GateTarget &feedback_target) {
    size_t q = qubit_target.qubit_value();
    if (cur_moment_used_flags[q]) {
        start_next_moment();
    }
    cur_moment_used_flags[q] = true;

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
            0.5,
            0.5,
        },
        ss.str(),
    });
    cur_moment_num_used++;
}

void DiagramTimelineAsciiDrawer::draw_two_qubit_gate(const Operation &op, const GateTarget &target1, const GateTarget &target2) {
    auto ends = two_qubit_gate_pieces(op.gate->name, true);
    if (target1.is_measurement_record_target() || target1.is_sweep_bit_target()) {
        draw_feedback(ends.second, target2, target1);
        return;
    }
    if (target2.is_measurement_record_target() || target2.is_sweep_bit_target()) {
        draw_feedback(ends.first, target1, target2);
        return;
    }
    std::stringstream first;
    std::stringstream second;
    first << (ends.first == "Z" ? "@" : ends.first);
    second << (ends.second == "Z" ? "@" : ends.second);

    size_t q1 = target1.qubit_value();
    size_t q2 = target2.qubit_value();
    if (q1 > q2) {
        std::swap(q1, q2);
    }
    for (size_t q = q1; q <= q2; q++) {
        if (cur_moment_used_flags[q]) {
            start_next_moment();
            break;
        }
    }
    for (size_t q = q1; q <= q2; q++) {
        cur_moment_used_flags[q] = true;
    }

    DiagramTimelineAsciiAlignedPos pos1{
        m2x(cur_moment),
        q2y(target1.qubit_value()),
        0.5,
        0.5,
    };
    DiagramTimelineAsciiAlignedPos pos2{
        m2x(cur_moment),
        q2y(target2.qubit_value()),
        0.5,
        0.5,
    };
    if (!op.target_data.args.empty()) {
        if (op.gate->id == gate_name_to_id("PAULI_CHANNEL_2")) {
            first << "[0]";
            second << "[1]";
        }
        first << "(" << comma_sep(op.target_data.args, ",") << ")";
        second << "(" << comma_sep(op.target_data.args, ",") << ")";
    }
    add_cell(DiagramTimelineAsciiCellContents{
        pos1,
        first.str(),
    });
    add_cell(DiagramTimelineAsciiCellContents{
        pos2,
        second.str(),
    });
    lines.push_back({pos1, pos2});
    cur_moment_num_used++;
}

void DiagramTimelineAsciiDrawer::start_next_moment() {
    cur_moment++;
    cur_moment_num_used = 0;
    cur_moment_used_flags.clear();
    cur_moment_used_flags.resize(num_qubits);
}

void DiagramTimelineAsciiDrawer::draw_tick() {
    if (num_ticks > 0 && cur_moment > tick_start_moment) {
        size_t x1 = m2x(tick_start_moment);
        size_t x2 = m2x(cur_moment);
        size_t y1 = 0;
        size_t y2 = q2y(num_qubits - 1) + 1;

        add_cell(DiagramTimelineAsciiCellContents{
            {x1, y1, 0, 0.5},
            "/",
        });
        add_cell(DiagramTimelineAsciiCellContents{
            {x2, y1, 1, 0.5},
            "\\",
        });
        add_cell(DiagramTimelineAsciiCellContents{
            {x1, y2, 0, 0.5},
            "\\",
        });
        add_cell(DiagramTimelineAsciiCellContents{
            {x2, y2, 1, 0.5},
            "/",
        });

        lines.push_back({{x1, y1, 0.0, 0.0}, {x2, y1, 1.0, 0.0}});
        lines.push_back({{x1, y1, 0.0, 0.0}, {x1, y1, 0.0, 1.0}});
        lines.push_back({{x2, y1, 1.0, 0.0}, {x2, y1, 1.0, 1.0}});

        lines.push_back({{x1, y2, 0.0, 0.0}, {x2, y2, 1.0, 0.0}});
        lines.push_back({{x1, y2, 0.0, 0.0}, {x1, y2, 0.0, 1.0}});
        lines.push_back({{x2, y2, 1.0, 0.0}, {x2, y2, 1.0, 1.0}});
    }

    start_next_moment();
    tick_start_moment = cur_moment;
}

void DiagramTimelineAsciiDrawer::draw_single_qubit_gate(const Operation &op, const GateTarget &target) {
    size_t q = target.qubit_value();
    if (cur_moment_used_flags[q]) {
        start_next_moment();
    }
    cur_moment_used_flags[q] = true;
    std::stringstream ss;
    ss << op.gate->name;
    if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
        ss << '[';
        ss << measure_offset;
        ss << ']';
        measure_offset++;
    }
    if (!op.target_data.args.empty()) {
        ss << "(" << comma_sep(op.target_data.args, ",") << ")";
    }
    add_cell(DiagramTimelineAsciiCellContents{
        {
            m2x(cur_moment),
            q2y(target.qubit_value()),
            0.5,
            0.5,
        },
        ss.str(),
    });
    cur_moment_num_used++;
}

void DiagramTimelineAsciiDrawer::draw_measurement_gate(const Operation &op, ConstPointerRange<GateTarget> targets) {
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
        if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
            ss << '[';
            ss << measure_offset;
            ss << ']';
        }
        if (!op.target_data.args.empty()) {
            ss << "(" << comma_sep(op.target_data.args, ",") << ")";
        }
        add_cell(DiagramTimelineAsciiCellContents{
            {
                m2x(cur_moment),
                q2y(t.qubit_value()),
                0.5,
                0.5,
            },
            ss.str(),
        });
    }
    if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
        measure_offset++;
    }
    lines.push_back({
        {
            m2x(cur_moment),
            q2y(min_q),
            0.5,
            0.5,
        },
        {
            m2x(cur_moment),
            q2y(max_q),
            0.5,
            0.5,
        }});
    cur_moment_num_used++;
}

void DiagramTimelineAsciiDrawer::draw_repeat_block(const Circuit &circuit, const Operation &op) {
    if (cur_moment_num_used) {
        draw_tick();
    }

    size_t x1 = m2x(cur_moment);
    size_t y1 = 0;
    size_t y2 = q2y(num_qubits - 1) + 1;
    size_t reps = op_data_rep_count(op.target_data);
    cur_moment++;
    tick_start_moment = cur_moment;
    size_t old_m = measure_offset;
    draw_circuit(op_data_block_body(circuit, op.target_data));
    measure_offset += (measure_offset - old_m) * (reps - 1);
    if (cur_moment_num_used) {
        draw_tick();
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

void DiagramTimelineAsciiDrawer::draw_next_operation(const Circuit &circuit, const Operation &op) {
    if (op.gate->id == gate_name_to_id("MPP")) {
        size_t start = 0;
        while (start < op.target_data.targets.size()) {
            size_t end = start + 1;
            while (end < op.target_data.targets.size() && op.target_data.targets[end].is_combiner()) {
                end += 2;
            }
            draw_measurement_gate(op, {&op.target_data.targets[start], &op.target_data.targets[end]});
            start = end;
        }
    } else if (op.gate->id == gate_name_to_id("DETECTOR")) {
        // TODO.
    } else if (op.gate->id == gate_name_to_id("OBSERVABLE_INCLUDE")) {
        // TODO.
    } else if (op.gate->id == gate_name_to_id("SHIFT_COORDS")) {
        // TODO.
    } else if (op.gate->id == gate_name_to_id("E")) {
        if (cur_moment_num_used) {
            start_next_moment();
        }
        draw_measurement_gate(op, op.target_data.targets);
    } else if (op.gate->id == gate_name_to_id("ELSE_CORRELATED_ERROR")) {
        if (cur_moment_num_used) {
            start_next_moment();
        }
        draw_measurement_gate(op, op.target_data.targets);
    } else if (op.gate->id == gate_name_to_id("TICK")) {
        draw_tick();
    } else if (op.gate->id == gate_name_to_id("REPEAT")) {
        draw_repeat_block(circuit, op);
    } else if (op.gate->flags & GATE_TARGETS_PAIRS) {
        for (size_t k = 0; k < op.target_data.targets.size(); k += 2) {
            draw_two_qubit_gate(op, op.target_data.targets[k], op.target_data.targets[k + 1]);
        }
    } else {
        for (const auto& t: op.target_data.targets) {
            draw_single_qubit_gate(op, t);
        }
    }
}

void DiagramTimelineAsciiDrawer::draw_circuit(const Circuit &circuit) {
    for (const auto &op : circuit.operations) {
        draw_next_operation(circuit, op);
    }
}

DiagramTimelineAsciiDrawer DiagramTimelineAsciiDrawer::from_circuit(const Circuit &circuit) {
    DiagramTimelineAsciiDrawer diagram;
    diagram.num_qubits = circuit.count_qubits();
    diagram.num_ticks = circuit.count_ticks();
    diagram.cur_moment_used_flags.resize(diagram.num_qubits);
    diagram.draw_circuit(circuit);

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

    // Strip spacing at end of lines and end of diagram.
    for (auto &line : out_lines) {
        while (!line.empty() && line.back() == ' ') {
            line.pop_back();
        }
    }
    while (!out_lines.empty() && out_lines.back().empty()) {
        out_lines.pop_back();
    }

    // Output while stripping empty lines at start of diagram.
    bool has_first = false;
    for (const auto &line : out_lines) {
        if (has_first || !line.empty()) {
            has_first = true;
            out << line << '\n';
        }
    }
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
