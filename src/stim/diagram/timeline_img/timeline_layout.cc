#include "stim/diagram/timeline_img/timeline_layout.h"
#include "stim/diagram/diagram_util.h"

using namespace stim;
using namespace stim_draw_internal;

size_t stim_draw_internal::utf8_char_count(const std::string &s) {
    size_t t = 0;
    for (uint8_t c : s) {
        // Continuation bytes start with "10" in binary.
        if ((c & 0xC0) != 0x80) {
            t++;
        }
    }
    return t;
}

CellDiagramAlignedPos::CellDiagramAlignedPos(size_t x, size_t y, float align_x, float align_y) :
    x(x),
    y(y),
    align_x(align_x),
    align_y(align_y) {
}

bool CellDiagramAlignedPos::operator==(const CellDiagramAlignedPos &other) const {
    return x == other.x && y == other.y;
}

bool CellDiagramAlignedPos::operator<(const CellDiagramAlignedPos &other) const {
    if (x != other.x) {
        return x < other.x;
    }
    return y < other.y;
}

CellDiagramCellContents::CellDiagramCellContents(CellDiagramAlignedPos center, std::string label, const char *stroke) :
    center(center),
    label(label),
    stroke(stroke) {
}

void CellDiagram::add_cell(CellDiagramCellContents cell) {
    cells.insert({cell.center, cell});
}

void CellDiagram::for_each_pos(const std::function<void(CellDiagramAlignedPos pos)> &callback) {
    for (const auto &item : cells) {
        callback(item.first);
    }
    for (const auto &item : lines) {
        callback(item.first);
        callback(item.second);
    }
}

void CellDiagram::compactify() {
    for (auto &item : cells) {
        auto &label = item.second.label;
        if (label.find("SQRT_") == 0) {
            label = "√" + label.substr(5);
        }
        if (label.find("_DAG") == label.size() - 4) {
            label = label.substr(0, label.size() - 4) + "†"; //"⁻¹";
        }
        while (!label.empty() && label.back() == ' ') {
            label.pop_back();
        }
    }
}

CellDiagramSizing CellDiagram::compute_sizing() {
    CellDiagramSizing layout{0, 0, {}, {}, {}, {}};
    for_each_pos([&](CellDiagramAlignedPos pos) {
        layout.num_x = std::max(layout.num_x, pos.x + 1);
        layout.num_y = std::max(layout.num_y, pos.y + 1);
    });
    layout.x_spans.resize(layout.num_x, 1);
    layout.y_spans.resize(layout.num_y, 1);

    for (const auto &item : cells) {
        const auto &box = item.second;
        auto &dx = layout.x_spans[box.center.x];
        auto &dy = layout.y_spans[box.center.y];
        dx = std::max(dx, utf8_char_count(box.label));
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

CellDiagram CellDiagram::from_circuit(const Circuit &circuit) {
    size_t num_qubits = circuit.count_qubits();
    size_t num_ticks = circuit.count_ticks();

    size_t cur_moment = 0;
    size_t cur_moment_num_used = 0;
    size_t measure_offset = 0;
    size_t tick_start_moment = 0;
    std::vector<bool> cur_moment_used_flags;
    cur_moment_used_flags.resize(num_qubits);

    CellDiagram diagram;

    auto m2x = [](size_t m) { return m * 2 + 2; };
    auto q2y = [](size_t q) { return q * 2 + 1; };
    auto start_next_moment = [&]() {
        cur_moment++;
        cur_moment_num_used = 0;
        cur_moment_used_flags.clear();
        cur_moment_used_flags.resize(num_qubits);
    };
    auto do_tick = [&]() {
        if (num_ticks > 0 && cur_moment > tick_start_moment) {
            size_t x1 = m2x(tick_start_moment);
            size_t x2 = m2x(cur_moment);
            size_t y1 = 0;
            size_t y2 = q2y(num_qubits - 1) + 1;
            diagram.lines.push_back({{x1, y1, 0.0, 0.0}, {x2, y1, 1.0, 0.0}});
            diagram.lines.push_back({{x1, y1, 0.0, 0.0}, {x1, y1, 0.0, 1.0}});
            diagram.lines.push_back({{x2, y1, 1.0, 0.0}, {x2, y1, 1.0, 1.0}});

            diagram.lines.push_back({{x1, y2, 0.0, 0.0}, {x2, y2, 1.0, 0.0}});
            diagram.lines.push_back({{x1, y2, 0.0, 0.0}, {x1, y2, 0.0, 1.0}});
            diagram.lines.push_back({{x2, y2, 1.0, 0.0}, {x2, y2, 1.0, 1.0}});
        }
        start_next_moment();
        tick_start_moment = cur_moment;
    };

    auto drawGate1Q = [&](const Operation &op, const GateTarget &target) {
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
        diagram.add_cell(CellDiagramCellContents{
            {
                m2x(cur_moment),
                q2y(target.qubit_value()),
                0.5,
                0.5,
            },
            ss.str(),
            "black",
        });
        cur_moment_num_used++;
    };

    auto drawFeedback = [&](const std::string &gate, const GateTarget &qubit_target, const GateTarget &feedback_target) {
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
            ss << "m" << (feedback_target.value() + measure_offset);
        }
        diagram.add_cell(CellDiagramCellContents{
            {
                m2x(cur_moment),
                q2y(qubit_target.qubit_value()),
                0.5,
                0.5,
            },
            ss.str(),
            "black",
        });
        cur_moment_num_used++;
    };

    auto drawGate2Q = [&](const Operation &op, const GateTarget &target1, const GateTarget &target2) {
        auto ends = two_qubit_gate_pieces(op.gate->name);
        if (target1.is_measurement_record_target() || target1.is_sweep_bit_target()) {
            drawFeedback(ends.second, target2, target1);
            return;
        }
        if (target2.is_measurement_record_target() || target2.is_sweep_bit_target()) {
            drawFeedback(ends.first, target1, target2);
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

        CellDiagramAlignedPos pos1{
            m2x(cur_moment),
            q2y(target1.qubit_value()),
            0.5,
            0.5,
        };
        CellDiagramAlignedPos pos2{
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
        diagram.add_cell(CellDiagramCellContents{
            pos1,
            first.str(),
            "black",
        });
        diagram.add_cell(CellDiagramCellContents{
            pos2,
            second.str(),
            "black",
        });
        diagram.lines.push_back({pos1, pos2});
        cur_moment_num_used++;
    };

    auto drawGateMQ = [&](const Operation &op, ConstPointerRange<GateTarget> targets) {
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
            diagram.add_cell(CellDiagramCellContents{
                {
                    m2x(cur_moment),
                    q2y(t.qubit_value()),
                    0.5,
                    0.5,
                },
                ss.str(),
                "black",
            });
        }
        if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
            measure_offset++;
        }
        diagram.lines.push_back({
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
    };

    std::function<void(const Circuit &circuit)> process_ops;
    process_ops = [&](const Circuit &circuit) {
        for (const auto &op : circuit.operations) {
            if (op.gate->id == gate_name_to_id("MPP")) {
                size_t start = 0;
                while (start < op.target_data.targets.size()) {
                    size_t end = start + 1;
                    while (end < op.target_data.targets.size() && op.target_data.targets[end].is_combiner()) {
                        end += 2;
                    }
                    drawGateMQ(op, {&op.target_data.targets[start], &op.target_data.targets[end]});
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
                drawGateMQ(op, op.target_data.targets);
            } else if (op.gate->id == gate_name_to_id("ELSE_CORRELATED_ERROR")) {
                if (cur_moment_num_used) {
                    start_next_moment();
                }
                drawGateMQ(op, op.target_data.targets);
            } else if (op.gate->id == gate_name_to_id("TICK")) {
                do_tick();
            } else if (op.gate->id == gate_name_to_id("REPEAT")) {
                if (cur_moment_num_used) {
                    do_tick();
                }

                size_t x1 = m2x(cur_moment);
                size_t y1 = 0;
                size_t y2 = q2y(num_qubits - 1) + 1;
                size_t reps = op_data_rep_count(op.target_data);
                cur_moment++;
                tick_start_moment = cur_moment;
                size_t old_m = measure_offset;
                process_ops(op_data_block_body(circuit, op.target_data));
                measure_offset += (measure_offset - old_m) * (reps - 1);
                if (cur_moment_num_used) {
                    do_tick();
                }
                tick_start_moment = cur_moment;

                size_t x2 = m2x(cur_moment);
                CellDiagramAlignedPos top_left{x1, y1, 0.0, 0.0};
                CellDiagramAlignedPos top_right{x2, y1, 1.0, 0.0};
                CellDiagramAlignedPos bottom_left{x1, y2, 0.0, 1.0};
                CellDiagramAlignedPos bottom_right{x2, y2, 1.0, 1.0};
                diagram.lines.push_back({top_left, top_right});
                diagram.lines.push_back({top_right, bottom_right});
                diagram.lines.push_back({bottom_right, bottom_left});
                diagram.lines.push_back({bottom_left, top_left});
                diagram.add_cell(CellDiagramCellContents{
                    top_left,
                    "REP " + std::to_string(reps),
                    "none",
                });
                start_next_moment();
                tick_start_moment = cur_moment;
            } else if (op.gate->flags & GATE_TARGETS_PAIRS) {
                for (size_t k = 0; k < op.target_data.targets.size(); k += 2) {
                    drawGate2Q(op,
                               op.target_data.targets[k],
                               op.target_data.targets[k + 1]);
                }
            } else {
                for (const auto& t: op.target_data.targets) {
                    drawGate1Q(op, t);
                }
            }
        }
    };

    process_ops(circuit);

    diagram.lines.insert(diagram.lines.begin(), num_qubits, {{0, 0, 0.0, 0.5}, {0, 0, 1.0, 0.5}});
    for (size_t q = 0; q < num_qubits; q++) {
        diagram.lines[q] = {
            {0, q2y(q), 1.0, 0.5},
            {m2x(cur_moment) + 1, q2y(q), 1.0, 0.5},
        };
        diagram.add_cell(CellDiagramCellContents{
            {0, q2y(q), 1.0, 0.5},
            "q" + std::to_string(q) + ": ",
            "none",
        });
    }

    return diagram;
}
