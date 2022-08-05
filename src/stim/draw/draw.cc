#include "stim/draw/draw.h"

using namespace stim;

struct DiagramPos {
    size_t x;
    size_t y;
    size_t z;

    bool operator==(const DiagramPos &other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator<(const DiagramPos &other) const {
        if (x != other.x) {
            return x < other.x;
        }
        if (y != other.y) {
            return y < other.y;
        }
        return z < other.z;
    }
};

struct DiagramBox {
    DiagramPos center;
    std::string label;
    std::vector<std::string> annotations;
};

struct DiagramLine {
    DiagramPos p1;
    DiagramPos p2;
};

struct DiagramLayout {
    size_t num_x;
    size_t num_y;
    size_t num_z;
    std::vector<size_t> x_widths;
    std::vector<size_t> y_heights;
    std::vector<size_t> z_depths;
    std::vector<size_t> x_offsets;
    std::vector<size_t> y_offsets;
    std::vector<size_t> z_offsets;
};

struct Diagram {
    std::map<DiagramPos, DiagramBox> boxes;
    std::vector<DiagramLine> lines;

    void add_box(DiagramBox box) {
        boxes[box.center] = box;
    }

    void for_each_pos(const std::function<void(DiagramPos pos)> &callback) {
        for (const auto &item : boxes) {
            callback(item.first);
        }
        for (const auto &item : lines) {
            callback(item.p1);
            callback(item.p2);
        }
    }

    DiagramLayout to_layout() {
        DiagramLayout layout{0, 0, 0, {}, {}, {}, {}, {}, {}};
        for_each_pos([&](DiagramPos pos) {
            layout.num_x = std::max(layout.num_x, pos.x + 1);
            layout.num_y = std::max(layout.num_y, pos.y + 1);
            layout.num_z = std::max(layout.num_z, pos.z + 1);
        });
        layout.x_widths.resize(layout.num_x, 1);
        layout.y_heights.resize(layout.num_y, 1);
        layout.z_depths.resize(layout.num_z, 1);

        for (const auto &item : boxes) {
            const auto &box = item.second;
            auto &dx = layout.x_widths[box.center.x];
            auto &dy = layout.y_heights[box.center.y];
//            auto &dz = layout.z_depths[box.center.z];
            dx = std::max(dx, box.label.size());
            for (const auto &annotation : box.annotations) {
                dx = std::max(dx, annotation.size());
            }
            dy = std::max(dy, box.annotations.size());
        }

        layout.x_offsets.push_back(0);
        layout.y_offsets.push_back(0);
        layout.z_offsets.push_back(0);
        for (const auto &e : layout.x_widths) {
            layout.x_offsets.push_back(layout.x_offsets.back() + e);
        }
        for (const auto &e : layout.y_heights) {
            layout.y_offsets.push_back(layout.y_offsets.back() + e);
        }
        for (const auto &e : layout.z_depths) {
            layout.z_offsets.push_back(layout.z_offsets.back() + e);
        }

        return layout;
    }
};

Diagram to_diagram(const Circuit &circuit) {
    size_t num_qubits = circuit.count_qubits();

    size_t cur_moment = 0;
    size_t cur_moment_used = 0;
    size_t measure_offset = 0;
    std::vector<bool> moment_used;
    moment_used.resize(num_qubits);
    Diagram diagram;

    auto m2x = [](size_t m) { return m * 2 + 2; };
    auto q2y = [](size_t q) { return q * 2 + 1; };
    auto start_next_moment = [&]() {
        cur_moment++;
        cur_moment_used = 0;
        moment_used.clear();
        moment_used.resize(num_qubits);
    };

    auto drawGate1Q = [&](const Operation &op, const GateTarget &target) {
        size_t q = target.qubit_value();
        if (moment_used[q]) {
            start_next_moment();
        }
        moment_used[q] = true;
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
        diagram.add_box(DiagramBox{
            {m2x(cur_moment), q2y(target.qubit_value()), 0},
            ss.str(),
            {},
        });
        cur_moment_used++;
    };

    auto drawGate2Q = [&](const Operation &op, const GateTarget &target1, const GateTarget &target2) {
        size_t q1 = target1.qubit_value();
        size_t q2 = target2.qubit_value();
        if (q1 > q2) {
            std::swap(q1, q2);
        }
        for (size_t q = q1; q <= q2; q++) {
            if (moment_used[q]) {
                start_next_moment();
                break;
            }
        }
        for (size_t q = q1; q <= q2; q++) {
            moment_used[q] = true;
        }
        DiagramPos pos1{m2x(cur_moment), q2y(target1.qubit_value()), 0};
        DiagramPos pos2{m2x(cur_moment), q2y(target2.qubit_value()), 0};
        std::stringstream first;
        std::stringstream second;
        if ((op.gate->name_len == 2 || op.gate->name_len == 3) && op.gate->name[op.gate->name_len - 2] == 'C') {
            char front = op.gate->name[0];
            char back = op.gate->name[op.gate->name_len - 1];
            if (front == 'C' || front == 'Z') {
                front = '@';
            }
            if (back == 'C' || back == 'Z') {
                back = '@';
            }
            first << front;
            second << back;
        } else {
            first << op.gate->name;
            second << op.gate->name;
        }
        if (!op.target_data.args.empty()) {
            if (op.gate->id == gate_name_to_id("PAULI_CHANNEL_2")) {
                first << "[0]";
                second << "[1]";
            }
            first << "(" << comma_sep(op.target_data.args, ",") << ")";
            second << "(" << comma_sep(op.target_data.args, ",") << ")";
        }
        diagram.boxes[pos1] = DiagramBox{
            pos1,
            first.str(),
            {},
        };
        diagram.boxes[pos2] = DiagramBox{
            pos2,
            second.str(),
            {},
        };
        diagram.lines.push_back({pos1, pos2});
        cur_moment_used++;
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
            if (moment_used[q]) {
                start_next_moment();
                break;
            }
        }
        for (size_t q = min_q; q <= max_q; q++) {
            moment_used[q] = true;
        }
        for (const auto &t : targets) {
            if (t.is_combiner()) {
                continue;
            }
            std::stringstream ss;
            ss << op.gate->name;
            if (t.is_x_target()) {
                ss << "[X]";
            } else if (t.is_y_target()) {
                ss << "[Y]";
            } else if (t.is_z_target()) {
                ss << "[Z]";
            }
            if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
                ss << '[';
                ss << measure_offset;
                ss << ']';
            }
            if (!op.target_data.args.empty()) {
                ss << "(" << comma_sep(op.target_data.args, ",") << ")";
            }
            diagram.add_box(DiagramBox{
                {m2x(cur_moment), q2y(t.qubit_value()), 0},
                ss.str(),
                {},
            });
        }
        if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
            measure_offset++;
        }
        diagram.lines.push_back({{m2x(cur_moment), q2y(min_q), 0}, {m2x(cur_moment), q2y(max_q), 0}});
        cur_moment_used++;
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
            } else if (op.gate->id == gate_name_to_id("E")) {
                if (cur_moment_used) {
                    start_next_moment();
                }
                drawGateMQ(op, op.target_data.targets);
            } else if (op.gate->id == gate_name_to_id("ELSE_CORRELATED_ERROR")) {
                if (cur_moment_used) {
                    start_next_moment();
                }
                drawGateMQ(op, op.target_data.targets);
            } else if (op.gate->id == gate_name_to_id("TICK")) {
                cur_moment++;
            } else if (op.gate->id == gate_name_to_id("REPEAT")) {
                if (cur_moment_used) {
                    start_next_moment();
                }
                DiagramPos top{m2x(cur_moment), 0, 0};
                DiagramPos bot{m2x(cur_moment), q2y(num_qubits - 1) + 1, 0};
                diagram.lines.push_back({top, bot});
                size_t reps = op_data_rep_count(op.target_data);
                diagram.add_box(DiagramBox{
                    top,
                    "/REP " + std::to_string(reps),
                    {},
                });
                diagram.add_box(DiagramBox{
                    bot,
                    "\\",
                    {},
                });
                cur_moment++;
                size_t old_m = measure_offset;
                process_ops(op_data_block_body(circuit, op.target_data));
                measure_offset += (measure_offset - old_m) * (reps - 1);
                if (cur_moment_used) {
                    start_next_moment();
                }
                top.x = m2x(cur_moment);
                bot.x = m2x(cur_moment);
                diagram.lines.push_back({top, bot});
                diagram.add_box(DiagramBox{
                    top,
                    "\\",
                    {},
                });
                diagram.add_box(DiagramBox{
                    bot,
                    "/",
                    {},
                });
                cur_moment++;
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

    diagram.lines.insert(diagram.lines.begin(), num_qubits, {});
    for (size_t q = 0; q < num_qubits; q++) {
        diagram.lines[q] = {{0, q2y(q), 0}, {m2x(cur_moment) + 1, q2y(q), 0}};
        diagram.add_box(DiagramBox{{0, q2y(q), 0}, "q" + std::to_string(q) + ": ", {}});
    }

    return diagram;
}


std::string stim::draw(const Circuit &circuit) {
    Diagram diagram = to_diagram(circuit);
    DiagramLayout layout = diagram.to_layout();

    std::vector<std::string> lines;
    lines.resize(layout.y_offsets.back());
    for (auto &line : lines) {
        line.resize(layout.x_offsets.back(), ' ');
    }

    for (const auto &line : diagram.lines) {
        auto x = layout.x_offsets[line.p1.x];
        auto y = layout.y_offsets[line.p1.y];
        auto x2 = layout.x_offsets[line.p2.x];
        auto y2 = layout.y_offsets[line.p2.y];
        while (x != x2) {
            lines[y][x] = '-';
            x += x < x2 ? 1 : -1;
        }
        if (line.p1.x != line.p2.x && line.p1.y != line.p2.y) {
            lines[y][x] = '.';
        } else if (line.p1.x != line.p2.x) {
            lines[y][x] = '-';
        } else if (line.p1.y != line.p2.y) {
            lines[y][x] = '|';
        } else {
            lines[y][x] = '.';
        }
        while (y != y2) {
            y += y < y2 ? 1 : -1;
            lines[y][x] = '|';
        }
    }

    for (const auto &item : diagram.boxes) {
        const auto &box = item.second;
        auto x = layout.x_offsets[box.center.x];
        auto y = layout.y_offsets[box.center.y];
        for (size_t k = 0; k < box.label.size(); k++) {
            lines[y][x + k] = box.label[k];
        }
        for (size_t a = 0; a < box.annotations.size(); a++) {
            for (size_t k = 0; k < box.label.size(); k++) {
                lines[y + a][x + k] = box.label[k];
            }
        }
    }

    std::string result;
    for (const auto &line : lines) {
        result.append(line);
        while (result.size() > 0 && result[0] == '\n') {
            result.erase(result.begin());
        }
        while (result.size() > 0 && result.back() == ' ') {
            result.pop_back();
        }
        result.push_back('\n');
    }

    return result;
}
