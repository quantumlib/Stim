#include "stim/diagram/timeline/timeline_3d_drawer.h"

#include "stim/circuit/gate_decomposition.h"
#include "stim/diagram/circuit_timeline_helper.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/diagram/diagram_util.h"
#include "stim/stabilizers/pauli_string.h"

using namespace stim;
using namespace stim_draw_internal;

Coord<3> trans(size_t m, Coord<2> xy) {
    return {-(float)m, xy.xyz[0] * -2.0f, xy.xyz[1] * -2.0f};
}

Coord<3> DiagramTimeline3DDrawer::mq2xyz(size_t m, size_t q) const {
    auto xy = qubit_coords[q];
    return trans(m, xy);
}

void DiagramTimeline3DDrawer::do_feedback(
    std::string_view gate, const GateTarget &qubit_target, const GateTarget &feedback_target) {
    std::string key = std::string(gate);
    if (feedback_target.is_sweep_bit_target()) {
        key.append(":SWEEP");
    } else if (feedback_target.is_measurement_record_target()) {
        key.append(":REC");
    }
    auto center = mq2xyz(cur_moment, qubit_target.qubit_value());
    diagram_out.elements.push_back({key, center});
}

void DiagramTimeline3DDrawer::draw_two_qubit_gate_end_point(Coord<3> center, std::string_view type) {
    if (type == "X") {
        diagram_out.elements.push_back({"X_CONTROL", center});
    } else if (type == "Y") {
        diagram_out.elements.push_back({"Y_CONTROL", center});
    } else if (type == "Z") {
        diagram_out.elements.push_back({"Z_CONTROL", center});
    } else {
        diagram_out.elements.push_back({std::string(type), center});
    }
}

void DiagramTimeline3DDrawer::draw_gate_connecting_line(Coord<3> a, Coord<3> b) {
    diagram_out.line_data.push_back(a);
    auto d = b - a;
    if (d.norm() > 2.2) {
        auto c = (a + b) * 0.5;
        c.xyz[0] -= 0.25;
        diagram_out.line_data.push_back(c);
        diagram_out.line_data.push_back(c);
    }
    diagram_out.line_data.push_back(b);
}

void DiagramTimeline3DDrawer::do_two_qubit_gate_instance(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);

    const GateTarget &target1 = op.targets[0];
    const GateTarget &target2 = op.targets[1];
    auto ends = two_qubit_gate_pieces(op.gate_type);
    if (target1.is_measurement_record_target() || target1.is_sweep_bit_target()) {
        do_feedback(ends.second, target2, target1);
        return;
    }
    if (target2.is_measurement_record_target() || target2.is_sweep_bit_target()) {
        do_feedback(ends.first, target1, target2);
        return;
    }

    auto pieces = two_qubit_gate_pieces(op.gate_type);
    auto c1 = mq2xyz(cur_moment, target1.qubit_value());
    auto c2 = mq2xyz(cur_moment, target2.qubit_value());
    draw_two_qubit_gate_end_point(c1, pieces.first);
    draw_two_qubit_gate_end_point(c2, pieces.second);
    draw_gate_connecting_line(c1, c2);
}

void DiagramTimeline3DDrawer::start_next_moment() {
    cur_moment += 1;
    cur_moment_is_used = false;
    cur_moment_used_flags.clear();
    cur_moment_used_flags.resize(num_qubits, false);
}

void DiagramTimeline3DDrawer::do_tick() {
    if (has_ticks && cur_moment > tick_start_moment) {
        auto x1 = coord_bounds.first.xyz[0] - 0.2f;
        auto x2 = coord_bounds.first.xyz[0] - 0.4f;
        auto y1 = coord_bounds.first.xyz[1];
        auto y2 = coord_bounds.second.xyz[1];
        y1 -= 0.25f;
        y2 += 0.25f;
        Coord<3> p0 = trans(tick_start_moment, {x1, y1});
        Coord<3> p1 = trans(tick_start_moment, {x1, y2});
        Coord<3> p2 = trans(tick_start_moment, {x2, y1});
        Coord<3> p3 = trans(tick_start_moment, {x2, y2});
        Coord<3> p4 = trans(cur_moment, {x1, y1});
        Coord<3> p5 = trans(cur_moment, {x1, y2});
        Coord<3> p6 = trans(cur_moment, {x2, y1});
        Coord<3> p7 = trans(cur_moment, {x2, y2});
        p0.xyz[0] += 0.25;
        p1.xyz[0] += 0.25;
        p2.xyz[0] += 0.25;
        p3.xyz[0] += 0.25;
        p4.xyz[0] -= 0.25;
        p5.xyz[0] -= 0.25;
        p6.xyz[0] -= 0.25;
        p7.xyz[0] -= 0.25;

        diagram_out.blue_line_data.push_back(p0);
        diagram_out.blue_line_data.push_back(p2);

        diagram_out.blue_line_data.push_back(p1);
        diagram_out.blue_line_data.push_back(p3);

        diagram_out.blue_line_data.push_back(p2);
        diagram_out.blue_line_data.push_back(p3);
        diagram_out.blue_line_data.push_back(p2);
        diagram_out.blue_line_data.push_back(p6);

        diagram_out.blue_line_data.push_back(p3);
        diagram_out.blue_line_data.push_back(p7);

        diagram_out.blue_line_data.push_back(p4);
        diagram_out.blue_line_data.push_back(p6);

        diagram_out.blue_line_data.push_back(p5);
        diagram_out.blue_line_data.push_back(p7);

        diagram_out.blue_line_data.push_back(p6);
        diagram_out.blue_line_data.push_back(p7);
    }

    start_next_moment();
    tick_start_moment = cur_moment;
}

void DiagramTimeline3DDrawer::do_single_qubit_gate_instance(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);
    const auto &target = op.targets[0];

    auto center = mq2xyz(cur_moment, target.qubit_value());
    const auto &gate_data = GATE_DATA[op.gate_type];
    diagram_out.elements.push_back({std::string(gate_data.name), center});
}

void DiagramTimeline3DDrawer::reserve_drawing_room_for_targets(SpanRef<const GateTarget> targets) {
    bool already_used = false;
    for (auto t : targets) {
        if (t.is_x_target() || t.is_y_target() || t.is_z_target() || t.is_qubit_target()) {
            already_used |= cur_moment_used_flags[t.qubit_value()];
        }
    }
    if (already_used) {
        start_next_moment();
    }
    for (auto t : targets) {
        if (t.is_x_target() || t.is_y_target() || t.is_z_target() || t.is_qubit_target()) {
            cur_moment_used_flags[t.qubit_value()] = true;
        }
    }
}

void DiagramTimeline3DDrawer::do_multi_qubit_gate_with_pauli_targets(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);

    Coord<3> prev{};
    bool has_prev = false;
    for (const auto &t : op.targets) {
        if (t.is_combiner()) {
            continue;
        }
        std::stringstream ss;
        const auto &gate_data = GATE_DATA[op.gate_type];
        ss << gate_data.name;
        if (t.is_x_target()) {
            ss << ":X";
        } else if (t.is_y_target()) {
            ss << ":Y";
        } else if (t.is_z_target()) {
            ss << ":Z";
        }
        auto center = mq2xyz(cur_moment, t.qubit_value());
        diagram_out.elements.push_back({ss.str(), center});
        if (has_prev) {
            draw_gate_connecting_line(center, prev);
        }
        prev = center;
        has_prev = true;
    }
}

void DiagramTimeline3DDrawer::do_start_repeat(const CircuitTimelineLoopData &loop_data) {
    if (cur_moment_is_used) {
        do_tick();
    }
    start_next_moment();
    loop_start_moment_stack.push_back(cur_moment);
    tick_start_moment = cur_moment;
}

void DiagramTimeline3DDrawer::do_end_repeat(const CircuitTimelineLoopData &loop_data) {
    if (cur_moment_is_used) {
        do_tick();
    }

    auto start = loop_start_moment_stack.back();
    loop_start_moment_stack.pop_back();

    auto x1 = coord_bounds.first.xyz[0];
    auto x2 = coord_bounds.second.xyz[0];
    auto y1 = coord_bounds.first.xyz[1];
    auto y2 = coord_bounds.second.xyz[1];
    x1 -= 0.5f * 3.0f / (2 + resolver.cur_loop_nesting.size());
    x2 += 0.5f * 3.0f / (2 + resolver.cur_loop_nesting.size());
    y1 -= 0.5f * 3.0f / (2 + resolver.cur_loop_nesting.size());
    y2 += 0.5f * 3.0f / (2 + resolver.cur_loop_nesting.size());
    Coord<3> p0 = trans(start, {x1, y1});
    Coord<3> p1 = trans(start, {x1, y2});
    Coord<3> p2 = trans(start, {x2, y1});
    Coord<3> p3 = trans(start, {x2, y2});
    Coord<3> p4 = trans(cur_moment, {x1, y1});
    Coord<3> p5 = trans(cur_moment, {x1, y2});
    Coord<3> p6 = trans(cur_moment, {x2, y1});
    Coord<3> p7 = trans(cur_moment, {x2, y2});
    p0.xyz[0] += 0.25;
    p1.xyz[0] += 0.25;
    p2.xyz[0] += 0.25;
    p3.xyz[0] += 0.25;
    p4.xyz[0] -= 0.25;
    p5.xyz[0] -= 0.25;
    p6.xyz[0] -= 0.25;
    p7.xyz[0] -= 0.25;

    diagram_out.red_line_data.push_back(p0);
    diagram_out.red_line_data.push_back(p1);
    diagram_out.red_line_data.push_back(p0);
    diagram_out.red_line_data.push_back(p2);
    diagram_out.red_line_data.push_back(p0);
    diagram_out.red_line_data.push_back(p4);

    diagram_out.red_line_data.push_back(p1);
    diagram_out.red_line_data.push_back(p3);
    diagram_out.red_line_data.push_back(p1);
    diagram_out.red_line_data.push_back(p5);

    diagram_out.red_line_data.push_back(p2);
    diagram_out.red_line_data.push_back(p3);
    diagram_out.red_line_data.push_back(p2);
    diagram_out.red_line_data.push_back(p6);

    diagram_out.red_line_data.push_back(p3);
    diagram_out.red_line_data.push_back(p7);

    diagram_out.red_line_data.push_back(p4);
    diagram_out.red_line_data.push_back(p5);
    diagram_out.red_line_data.push_back(p4);
    diagram_out.red_line_data.push_back(p6);

    diagram_out.red_line_data.push_back(p5);
    diagram_out.red_line_data.push_back(p7);

    diagram_out.red_line_data.push_back(p6);
    diagram_out.red_line_data.push_back(p7);

    start_next_moment();
    tick_start_moment = cur_moment;
}

void DiagramTimeline3DDrawer::do_mpp(const ResolvedTimelineOperation &op) {
    do_multi_qubit_gate_with_pauli_targets(op);
}

void DiagramTimeline3DDrawer::do_spp(const ResolvedTimelineOperation &op) {
    do_multi_qubit_gate_with_pauli_targets(op);
}

void DiagramTimeline3DDrawer::do_correlated_error(const ResolvedTimelineOperation &op) {
    if (cur_moment_is_used) {
        start_next_moment();
    }
    do_multi_qubit_gate_with_pauli_targets(op);
}

void DiagramTimeline3DDrawer::do_else_correlated_error(const ResolvedTimelineOperation &op) {
    do_correlated_error(op);
}

void DiagramTimeline3DDrawer::do_qubit_coords(const ResolvedTimelineOperation &op) {
    // Not drawn.
}

void DiagramTimeline3DDrawer::do_detector(const ResolvedTimelineOperation &op) {
    // Not drawn.
}

void DiagramTimeline3DDrawer::do_observable_include(const ResolvedTimelineOperation &op) {
    // Not drawn.
}

void DiagramTimeline3DDrawer::do_resolved_operation(const ResolvedTimelineOperation &op) {
    if (op.gate_type == GateType::MPP) {
        do_mpp(op);
    } else if (op.gate_type == GateType::SPP || op.gate_type == GateType::SPP_DAG) {
        do_spp(op);
    } else if (op.gate_type == GateType::DETECTOR) {
        do_detector(op);
    } else if (op.gate_type == GateType::OBSERVABLE_INCLUDE) {
        do_observable_include(op);
    } else if (op.gate_type == GateType::QUBIT_COORDS) {
        do_qubit_coords(op);
    } else if (op.gate_type == GateType::E) {
        do_correlated_error(op);
    } else if (op.gate_type == GateType::ELSE_CORRELATED_ERROR) {
        do_else_correlated_error(op);
    } else if (op.gate_type == GateType::TICK) {
        do_tick();
    } else if (GATE_DATA[op.gate_type].flags & GATE_TARGETS_PAIRS) {
        do_two_qubit_gate_instance(op);
    } else {
        do_single_qubit_gate_instance(op);
    }
}

DiagramTimeline3DDrawer::DiagramTimeline3DDrawer(size_t num_qubits, bool has_ticks)
    : num_qubits(num_qubits), has_ticks(has_ticks) {
    cur_moment_used_flags.resize(num_qubits, false);
}

void add_used_qubits(const Circuit &circuit, std::set<uint64_t> &out) {
    for (const auto &op : circuit.operations) {
        if (op.gate_type == GateType::REPEAT) {
            add_used_qubits(op.repeat_block_body(circuit), out);
        } else {
            for (const auto &t : op.targets) {
                if (t.is_x_target() || t.is_y_target() || t.is_z_target() || t.is_qubit_target()) {
                    out.insert(t.qubit_value());
                }
            }
        }
    }
}

std::pair<std::vector<Coord<2>>, std::pair<Coord<2>, Coord<2>>> pick_coords_for_circuit(const Circuit &circuit) {
    DetectorSliceSet set;
    set.num_qubits = circuit.count_qubits();
    set.coordinates = circuit.get_final_qubit_coords();
    auto coords = FlattenedCoords::from(set, 1).qubit_coords;
    float default_y = 0;
    for (auto e : set.coordinates) {
        default_y = std::min(default_y, coords[e.first].xyz[1] - 1);
    }
    for (uint64_t q = 0; q < set.num_qubits; q++) {
        if (set.coordinates.find(q) == set.coordinates.end()) {
            coords[q].xyz = {(float)q, default_y};
        }
    }

    std::set<uint64_t> used;
    add_used_qubits(circuit, used);
    std::vector<Coord<2>> used_coords;
    for (const auto &q : used) {
        used_coords.push_back(coords[q]);
    }
    if (used_coords.empty()) {
        used_coords.push_back({0, 0});
    }
    auto bounds = Coord<2>::min_max(used_coords);
    //    auto center = (bounds.first + bounds.second) * 0.5;
    //    for (auto &c : coords) {
    //        c -= center;
    //    }
    //    bounds.first -= center;
    //    bounds.second -= center;
    return {coords, bounds};
}

Basic3dDiagram DiagramTimeline3DDrawer::circuit_to_basic_3d_diagram(const Circuit &circuit) {
    auto num_qubits = circuit.count_qubits();
    auto has_ticks = circuit.count_ticks() > 0;
    DiagramTimeline3DDrawer obj(num_qubits, has_ticks);
    auto all_used = pick_coords_for_circuit(circuit);
    obj.qubit_coords = all_used.first;
    obj.coord_bounds = all_used.second;
    auto minmax = obj.coord_bounds;

    // Draw an arrow indicating the time direction.
    auto y = -2 * (minmax.first.xyz[0] - 1);
    auto z = -2 * (minmax.first.xyz[1] * 0.5f + minmax.second.xyz[1] * 0.5f);
    obj.diagram_out.red_line_data.push_back({0, y, z});
    obj.diagram_out.red_line_data.push_back({-3, y, z});
    obj.diagram_out.red_line_data.push_back({-2.5f, y - 0.5f, z});
    obj.diagram_out.red_line_data.push_back({-3, y, z});
    obj.diagram_out.red_line_data.push_back({-2.5f, y + 0.5f, z});
    obj.diagram_out.red_line_data.push_back({-3, y, z});

    obj.resolver.resolved_op_callback = [&](const ResolvedTimelineOperation &op) {
        obj.do_resolved_operation(op);
    };
    obj.resolver.start_repeat_callback = [&](const CircuitTimelineLoopData &loop_data) {
        obj.do_start_repeat(loop_data);
    };
    obj.resolver.end_repeat_callback = [&](const CircuitTimelineLoopData &loop_data) {
        obj.do_end_repeat(loop_data);
    };
    obj.resolver.do_circuit(circuit);
    if (obj.cur_moment_is_used) {
        obj.start_next_moment();
    }

    std::set<uint64_t> used;
    add_used_qubits(circuit, used);
    for (auto q : used) {
        auto p1 = obj.mq2xyz(0, q);
        p1.xyz[0] += 1;
        auto p2 = obj.mq2xyz(obj.cur_moment + 1, q);
        obj.diagram_out.line_data.push_back(p1);
        obj.diagram_out.line_data.push_back(p2);
    }

    return obj.diagram_out;
}
