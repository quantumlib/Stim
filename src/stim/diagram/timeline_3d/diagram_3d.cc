#include "stim/diagram/timeline_3d/diagram_3d.h"
#include "stim/mem/simd_bits.h"
#include "stim/diagram/diagram_util.h"

using namespace stim;
using namespace stim_draw_internal;

float get_median(ConstPointerRange<Coord<2>> points, size_t index) {
    std::vector<float> coords;
    for (const auto &p : points) {
        coords.push_back(p.xyz[index]);
    }
    auto m = coords.begin() + (coords.size() / 2);
    std::nth_element(coords.begin(), m, coords.end());
    return *m;
}

std::pair<std::vector<Coord<2>>, std::vector<Coord<2>>> split(ConstPointerRange<Coord<2>> points, size_t index, float cut) {
    std::vector<Coord<2>> low;
    std::vector<Coord<2>> high;
    for (const auto &p : points) {
        if (p.xyz[index] < cut) {
            low.push_back(p);
        } else {
            high.push_back(p);
        }
    }
    return {low, high};
}

float brute_min_distance(ConstPointerRange<Coord<2>> &points) {
    float best_dd = INFINITY;
    for (size_t k = 0; k < points.size(); k++) {
        for (size_t j = k + 1; j < points.size(); j++) {
            auto dx = points[k].xyz[0] - points[j].xyz[0];
            auto dy = points[k].xyz[1] - points[j].xyz[1];
            best_dd = std::min(best_dd, dx*dx + dy*dy);
        }
    }
    return sqrtf(best_dd);
}

float min_distance_unique(ConstPointerRange<Coord<2>> points, size_t axis = 0) {
    if (points.size() < 10) {
        return brute_min_distance(points);
    }

    auto median = get_median(points, axis);
    auto low_high = split(points, axis, median);
    auto other_axis = 1 - axis;
    auto min_dist_sides = std::min(
        min_distance_unique(low_high.first, other_axis),
        min_distance_unique(low_high.second, other_axis));
    auto mid_strip = split(
        split(points, axis, median - min_dist_sides).second,
        axis,
        median + min_dist_sides).first;
    auto min_dist_mid = min_distance_unique(mid_strip, other_axis);

    return std::min(min_dist_mid, min_dist_sides);
}

float stim_draw_internal::min_distance(ConstPointerRange<Coord<2>> points) {
    std::set<Coord<2>> seen;
    std::vector<Coord<2>> unique_points;
    for (const auto &p : points) {
        if (seen.insert(p).second) {
            unique_points.push_back(p);
        }
    }
    return min_distance_unique(points);
}

std::vector<Coord<2>> coordinates_for(const stim::Circuit &circuit, const simd_bits<MAX_BITWORD_WIDTH> used_qubits) {
    size_t num_qubits = circuit.count_qubits();
    std::vector<Coord<2>> result;
    for (size_t k = 0; k < num_qubits; k++) {
        result.push_back({(float)k, 0});
    }

    for (const auto &e : circuit.get_final_qubit_coords()) {
        if (e.second.size() == 1) {
            result[e.first] = {(float)e.second[0], 0};
        } else if (e.second.size() >= 2) {
            result[e.first] = {(float)e.second[0], (float)e.second[1]};
        }
    }

    std::vector<Coord<2>> used;
    for (size_t k = 0; k < num_qubits; k++) {
        if (used_qubits[k]) {
            used.push_back(result[k]);
        }
    }
    for (size_t k = 0; k < num_qubits; k++) {
        if (!used_qubits[k] && !used.empty()) {
            result[k] = used.front();
        }
    }
    float d = min_distance(used);
    if (d == 0) {
        throw std::invalid_argument("What");
    }
    if (d < 2) {
        float f = 2 / d;
        for (auto &c : result) {
            c.xyz[0] *= f;
            c.xyz[1] *= f;
        }
    }

    return result;
}

void qubits_used_by_circuit_helper(const stim::Circuit &circuit, simd_bits<MAX_BITWORD_WIDTH> &out) {
    for (const auto &op : circuit.operations) {
        if (op.gate->id == gate_name_to_id("REPEAT")) {
            qubits_used_by_circuit_helper(op_data_block_body(circuit, op.target_data), out);
        } else {
            for (const auto &t : op.target_data.targets) {
                if (t.is_qubit_target() || t.is_x_target() || t.is_y_target() || t.is_z_target()) {
                    out[t.qubit_value()] = true;
                }
            }
        }
    }
}

simd_bits<MAX_BITWORD_WIDTH> qubits_used_by_circuit(const stim::Circuit &circuit) {
    simd_bits<MAX_BITWORD_WIDTH> out(circuit.count_qubits());
    qubits_used_by_circuit_helper(circuit, out);
    return out;
}

Diagram3D Diagram3D::from_circuit(const stim::Circuit &circuit) {
    constexpr float time_scale = 2;
    size_t num_qubits = circuit.count_qubits();
    auto used_qubits = qubits_used_by_circuit(circuit);
    auto coords = coordinates_for(circuit, used_qubits);
    auto box = Coord<2>::min_max(coords);

    bool cur_moment_any_used = false;
    simd_bits<MAX_BITWORD_WIDTH> cur_moment_used_flags(num_qubits);
    simd_bits<MAX_BITWORD_WIDTH> qubit_was_used(num_qubits);

    Diagram3D diagram;

    float time = 0;
    auto q2y = [&](size_t q) { return coords[q].xyz[0]; };
    auto q2z = [&](size_t q) { return coords[q].xyz[1]; };
    auto start_next_moment = [&]() {
        if (cur_moment_any_used) {
            time += time_scale;
            cur_moment_any_used = false;
            qubit_was_used |= cur_moment_used_flags;
            cur_moment_used_flags.clear();
        }
    };
    auto do_tick = [&]() {
        start_next_moment();
        time += time_scale;
    };

    auto drawGate1Q = [&](const Operation &op, const GateTarget &target) {
        size_t q = target.qubit_value();
        if (cur_moment_used_flags[q]) {
            start_next_moment();
        }
        cur_moment_used_flags[q] = true;
        cur_moment_any_used = true;
        diagram.gates.push_back({op.gate->name, {time, q2y(q), q2z(q)}});
    };

    auto drawFeedback = [&](char gate, const GateTarget &qubit_target, const GateTarget &feedback_target) {
        size_t q = qubit_target.qubit_value();
        if (cur_moment_used_flags[q]) {
            start_next_moment();
        }
        cur_moment_used_flags[q] = true;
        cur_moment_any_used = true;
        diagram.gates.push_back({std::string(1, gate), {time, q2y(q), q2z(q)}});
    };

    auto drawGate2Q = [&](const Operation &op, const GateTarget &target1, const GateTarget &target2) {
        auto pair = two_qubit_gate_pieces(op.gate->name);
        auto a = pair.first;
        auto b = pair.second;
        if (target1.is_measurement_record_target() || target1.is_sweep_bit_target()) {
            drawFeedback(a[0], target2, target1);
            return;
        }
        if (target2.is_measurement_record_target() || target2.is_sweep_bit_target()) {
            drawFeedback(b[0], target1, target2);
            return;
        }

        size_t q1 = target1.qubit_value();
        size_t q2 = target2.qubit_value();
        if (q1 > q2) {
            std::swap(q1, q2);
        }
        if (cur_moment_used_flags[q1] || cur_moment_used_flags[q2]) {
            start_next_moment();
        }
        cur_moment_used_flags[q1] = true;
        cur_moment_used_flags[q2] = true;

        if (a == "X" || a == "Y" || a == "Z") {
            a.append("_CONTROL");
        }
        if (b == "X" || b == "Y" || b == "Z") {
            b.append("_CONTROL");
        }
        diagram.line_data.push_back({time, q2y(q1), q2z(q1)});
        diagram.gates.push_back({a, diagram.line_data.back()});
        diagram.line_data.push_back({time, q2y(q2), q2z(q2)});
        diagram.gates.push_back({b, diagram.line_data.back()});
        cur_moment_any_used = true;
    };

    auto drawGateMultiQubit = [&](const Operation &op, ConstPointerRange<GateTarget> targets) {
        if (targets.empty()) {
            return;
        }

        std::vector<size_t> qubits;
        std::vector<char> basis;
        for (const auto &t : targets) {
            if (t.is_combiner()) {
                continue;
            }
            qubits.push_back(t.qubit_value());
            if (t.is_x_target()) {
                basis.push_back('X');
            } else if (t.is_y_target()) {
                basis.push_back('Y');
            } else if (t.is_z_target()) {
                basis.push_back('Z');
            } else {
                basis.push_back('?');
            }
        }
        for (auto q : qubits) {
            if (cur_moment_used_flags[q]) {
                start_next_moment();
                break;
            }
        }
        for (auto q : qubits) {
            cur_moment_used_flags[q] = true;
        }
        cur_moment_any_used = true;
        std::string prefix = op.gate->name;
        prefix.push_back(':');
        for (size_t k = 0; k < qubits.size(); k++) {
            auto q = qubits[k];
            auto b = basis[k];
            Coord<3> p{time, q2y(q), q2z(q)};
            if (k > 0) {
                diagram.line_data.push_back(p);
            }
            diagram.gates.push_back({prefix + b, p});
            if (k < qubits.size() - 1) {
                diagram.line_data.push_back(p);
            }
        }
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
                    drawGateMultiQubit(op, {&op.target_data.targets[start], &op.target_data.targets[end]});
                    start = end;
                }
            } else if (op.gate->id == gate_name_to_id("DETECTOR")) {
                // TODO.
            } else if (op.gate->id == gate_name_to_id("OBSERVABLE_INCLUDE")) {
                // TODO.
            } else if (op.gate->id == gate_name_to_id("SHIFT_COORDS")) {
                // TODO.
            } else if (op.gate->id == gate_name_to_id("E")) {
                start_next_moment();
                drawGateMultiQubit(op, op.target_data.targets);
            } else if (op.gate->id == gate_name_to_id("ELSE_CORRELATED_ERROR")) {
                start_next_moment();
                drawGateMultiQubit(op, op.target_data.targets);
            } else if (op.gate->id == gate_name_to_id("TICK")) {
                do_tick();
            } else if (op.gate->id == gate_name_to_id("REPEAT")) {
                start_next_moment();

                do_tick();
                auto t1 = time;
                do_tick();
                process_ops(op_data_block_body(circuit, op.target_data));
                start_next_moment();
                do_tick();
                auto t2 = time;
                do_tick();

                auto y1 = box.first.xyz[0] - 1;
                auto z1 = box.first.xyz[1] - 1;
                auto y2 = box.second.xyz[0] + 1;
                auto z2 = box.second.xyz[1] + 1;

                for (size_t k1 = 0; k1 < 2; k1++) {
                    auto tk1 = k1 == 0 ? t1 : t2;
                    auto yk1 = k1 == 0 ? y1 : y2;
                    for (size_t k2 = 0; k2 < 2; k2++) {
                        auto yk2 = k2 == 0 ? y1 : y2;
                        auto zk2 = k2 == 0 ? z1 : z2;
                        diagram.red_line_data.push_back({tk1, yk2, z1});
                        diagram.red_line_data.push_back({tk1, yk2, z2});
                        diagram.red_line_data.push_back({tk1, y1, zk2});
                        diagram.red_line_data.push_back({tk1, y2, zk2});
                        diagram.red_line_data.push_back({t1, yk1, zk2});
                        diagram.red_line_data.push_back({t2, yk1, zk2});
                    }
                }
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
    start_next_moment();

    for (size_t q = 0; q < num_qubits; q++) {
        diagram.line_data.push_back({0, q2y(q), q2z(q)});
        diagram.line_data.push_back({time, q2y(q), q2z(q)});
    }

    return diagram;
}
