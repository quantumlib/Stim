#include "stim/diagram/timeline/timeline_ascii_drawer.h"

#include "stim/diagram/circuit_timeline_helper.h"
#include "stim/diagram/diagram_util.h"
#include "stim/util_bot/str_util.h"

using namespace stim;
using namespace stim_draw_internal;

constexpr double GATE_ALIGNMENT_X = 0;    // Left-justify gates when time moves right.
constexpr double GATE_ALIGNMENT_Y = 0.5;  // Center-justify gates when time moves down.

size_t DiagramTimelineAsciiDrawer::m2x(size_t m) const {
    return m * (1 + moment_spacing) + 2;
}

size_t DiagramTimelineAsciiDrawer::q2y(size_t q) const {
    return q * 2 + 1;
}

void DiagramTimelineAsciiDrawer::do_feedback(
    std::string_view gate, const GateTarget &qubit_target, const GateTarget &feedback_target) {
    std::stringstream ss;
    ss << gate;
    ss << "^";
    if (feedback_target.is_sweep_bit_target()) {
        ss << "sweep[" << feedback_target.value() << "]";
    } else if (feedback_target.is_measurement_record_target()) {
        ss << "rec[" << (feedback_target.value() + resolver.measure_offset) << "]";
    }
    diagram.add_entry(
        AsciiDiagramEntry{
            {
                m2x(cur_moment),
                q2y(qubit_target.qubit_value()),
                GATE_ALIGNMENT_X,
                GATE_ALIGNMENT_Y,
            },
            ss.str(),
        });
}

void DiagramTimelineAsciiDrawer::do_two_qubit_gate_instance(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);

    const GateTarget &target1 = op.targets[0];
    const GateTarget &target2 = op.targets[1];
    const auto &gate_data = GATE_DATA[op.gate_type];
    auto ends = two_qubit_gate_pieces(op.gate_type);
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
    if (!op.args.empty()) {
        if (op.gate_type == GateType::PAULI_CHANNEL_2) {
            first << "[0]";
            second << "[1]";
        }
        first << "(" << comma_sep(op.args, ",") << ")";
        second << "(" << comma_sep(op.args, ",") << ")";
    }
    if (gate_data.flags & GATE_PRODUCES_RESULTS) {
        first << ':';
        write_rec_index(first);
    }

    diagram.add_entry(
        AsciiDiagramEntry{
            {
                m2x(cur_moment),
                q2y(target1.qubit_value()),
                GATE_ALIGNMENT_X,
                GATE_ALIGNMENT_Y,
            },
            first.str(),
        });
    diagram.add_entry(
        AsciiDiagramEntry{
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
    if (has_ticks && cur_moment > tick_start_moment) {
        size_t x1 = m2x(tick_start_moment);
        size_t x2 = m2x(cur_moment);
        size_t y1 = 0;
        size_t y2 = q2y(num_qubits - 1) + 1;

        diagram.add_entry(
            AsciiDiagramEntry{
                {x1, y1, 0, 0},
                "/",
            });
        diagram.add_entry(
            AsciiDiagramEntry{
                {x2, y1, 1, 0},
                "\\",
            });
        diagram.add_entry(
            AsciiDiagramEntry{
                {x1, y2, 0, 1},
                "\\",
            });
        diagram.add_entry(
            AsciiDiagramEntry{
                {x2, y2, 1, 0},
                "/",
            });

        diagram.lines.push_back({{x1, y1, 0.0, 0.0}, {x2, y1, 1.0, 0.0}});
        diagram.lines.push_back({{x1, y2, 0.0, 0.0}, {x2, y2, 1.0, 0.0}});
    }

    start_next_moment();
    tick_start_moment = cur_moment;
}

void DiagramTimelineAsciiDrawer::do_single_qubit_gate_instance(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);
    const auto &target = op.targets[0];
    const auto &gate_data = GATE_DATA[op.gate_type];

    std::stringstream ss;
    ss << gate_data.name;
    if (!op.args.empty()) {
        ss << "(" << comma_sep(op.args, ",") << ")";
    }
    if (gate_data.flags & GATE_PRODUCES_RESULTS) {
        ss << ':';
        write_rec_index(ss);
    }
    diagram.add_entry(
        AsciiDiagramEntry{
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
    if (!resolver.cur_loop_nesting.empty()) {
        out.put('[');
    }
    out << resolver.detector_offset;
    for (size_t k = 0; k < resolver.cur_loop_nesting.size(); k++) {
        out << "+iter";
        if (k > 0) {
            out << (k + 1);
        }
        auto p = resolver.cur_loop_nesting[k].detectors_per_iteration;
        if (p != 1) {
            out << '*' << p;
        }
    }
    if (!resolver.cur_loop_nesting.empty()) {
        out.put(']');
    }
}
void DiagramTimelineAsciiDrawer::write_rec_index(std::ostream &out, int64_t lookback_shift) {
    out << "rec[";
    out << resolver.measure_offset + (decltype(resolver.measure_offset))lookback_shift;
    for (size_t k = 0; k < resolver.cur_loop_nesting.size(); k++) {
        auto n = resolver.cur_loop_nesting[k].measurements_per_iteration;
        if (n != 0) {
            out << "+iter";
            if (k > 0) {
                out << (k + 1);
            }
            if (n != 1) {
                out << '*' << n;
            }
        }
    }
    out << ']';
}

void DiagramTimelineAsciiDrawer::write_coords(std::ostream &out, SpanRef<const double> relative_coordinates) {
    out.put('(');
    for (size_t k = 0; k < relative_coordinates.size(); k++) {
        if (k) {
            out.put(',');
        }
        write_coord(out, k, relative_coordinates[k]);
    }
    out.put(')');
}

void DiagramTimelineAsciiDrawer::write_coord(std::ostream &out, size_t coord_index, double absolute_coordinate) {
    out << absolute_coordinate;
    for (size_t k = 0; k < resolver.cur_loop_nesting.size(); k++) {
        const auto &p = resolver.cur_loop_nesting[k].shift_per_iteration;
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

void DiagramTimelineAsciiDrawer::reserve_drawing_room_for_targets(SpanRef<const GateTarget> targets) {
    size_t min_q = SIZE_MAX;
    size_t max_q = 0;
    for (const auto &t : targets) {
        if (t.is_combiner() || t.is_measurement_record_target() || t.is_sweep_bit_target()) {
            continue;
        }
        size_t q = t.qubit_value();
        min_q = std::min(min_q, q);
        max_q = std::max(max_q, q);
    }
    if (min_q == SIZE_MAX) {
        return;
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
        diagram.lines.push_back(
            {{
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
             }});
    }
}
void DiagramTimelineAsciiDrawer::do_multi_qubit_gate_with_pauli_targets(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);

    for (const auto &t : op.targets) {
        if (t.is_combiner()) {
            continue;
        }
        std::stringstream ss;
        const auto &gate_data = GATE_DATA[op.gate_type];
        ss << gate_data.name;
        if (t.is_x_target()) {
            ss << "[X]";
        } else if (t.is_y_target()) {
            ss << "[Y]";
        } else if (t.is_z_target()) {
            ss << "[Z]";
        }
        if (!op.args.empty()) {
            ss << "(" << comma_sep(op.args, ",") << ")";
        }
        if (gate_data.flags & GATE_PRODUCES_RESULTS) {
            ss << ':';
            write_rec_index(ss);
        }
        diagram.add_entry(
            AsciiDiagramEntry{
                {
                    m2x(cur_moment),
                    q2y(t.qubit_value()),
                    GATE_ALIGNMENT_X,
                    GATE_ALIGNMENT_Y,
                },
                ss.str(),
            });
    }
}

void DiagramTimelineAsciiDrawer::do_start_repeat(const CircuitTimelineLoopData &loop_data) {
    if (cur_moment_is_used) {
        do_tick();
    }

    AsciiDiagramPos top{m2x(cur_moment), 0, 0.0, 0.0};
    AsciiDiagramPos bot{m2x(cur_moment), q2y(num_qubits - 1) + 1, 0.0, 1.0};
    diagram.add_entry(
        AsciiDiagramEntry{
            top,
            "/REP " + std::to_string(loop_data.num_repetitions),
        });
    diagram.add_entry(
        AsciiDiagramEntry{
            bot,
            "\\",
        });
    diagram.lines.push_back({bot, top});

    start_next_moment();
    tick_start_moment = cur_moment;
}

void DiagramTimelineAsciiDrawer::do_end_repeat(const CircuitTimelineLoopData &loop_data) {
    if (cur_moment_is_used) {
        do_tick();
    }

    AsciiDiagramPos top{m2x(cur_moment), 0, 0.5, 0.0};
    AsciiDiagramPos bot{m2x(cur_moment), q2y(num_qubits - 1) + 1, 0.5, 1.0};

    diagram.lines.push_back({top, bot});
    diagram.add_entry(
        AsciiDiagramEntry{
            top,
            "\\",
        });
    diagram.add_entry(
        AsciiDiagramEntry{
            bot,
            "/",
        });

    start_next_moment();
    tick_start_moment = cur_moment;
}

void DiagramTimelineAsciiDrawer::do_correlated_error(const ResolvedTimelineOperation &op) {
    if (cur_moment_is_used) {
        start_next_moment();
    }
    do_multi_qubit_gate_with_pauli_targets(op);
}

void DiagramTimelineAsciiDrawer::do_else_correlated_error(const ResolvedTimelineOperation &op) {
    do_correlated_error(op);
}

void DiagramTimelineAsciiDrawer::do_qubit_coords(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);
    assert(op.targets.size() == 1);
    const auto &target = op.targets[0];

    std::stringstream ss;
    const auto &gate_data = GATE_DATA[op.gate_type];
    ss << gate_data.name;
    write_coords(ss, op.args);
    diagram.add_entry(
        AsciiDiagramEntry{
            {
                m2x(cur_moment),
                q2y(target.qubit_value()),
                GATE_ALIGNMENT_X,
                GATE_ALIGNMENT_Y,
            },
            ss.str(),
        });
}

void DiagramTimelineAsciiDrawer::do_detector(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);
    auto pseudo_target = op.targets[0];
    SpanRef<const GateTarget> rec_targets = op.targets;
    rec_targets.ptr_start++;

    std::stringstream ss;
    ss << "DETECTOR";
    if (!op.args.empty()) {
        write_coords(ss, op.args);
    }
    ss.put(':');
    write_det_index(ss);
    ss.put('=');
    for (size_t k = 0; k < rec_targets.size(); k++) {
        if (k) {
            ss << "*";
        }
        write_rec_index(ss, rec_targets[k].value());
    }
    if (rec_targets.empty()) {
        ss.put('1');
    }
    diagram.add_entry(
        AsciiDiagramEntry{
            {
                m2x(cur_moment),
                q2y(pseudo_target.qubit_value()),
                GATE_ALIGNMENT_X,
                GATE_ALIGNMENT_Y,
            },
            ss.str(),
        });
}

void DiagramTimelineAsciiDrawer::do_observable_include(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);
    auto pseudo_target = op.targets[0];
    SpanRef<const GateTarget> rec_targets = op.targets;
    rec_targets.ptr_start++;

    bool had_paulis = false;
    for (const auto &t : rec_targets) {
        if (t.is_pauli_target()) {
            had_paulis = true;
            std::stringstream ss;
            ss << "L" << (op.args.empty() ? 0 : op.args[0]) << "*=";
            ss << t.pauli_type();
            diagram.add_entry(
                AsciiDiagramEntry{
                    {
                        m2x(cur_moment),
                        q2y(t.qubit_value()),
                        GATE_ALIGNMENT_X,
                        GATE_ALIGNMENT_Y,
                    },
                    ss.str(),
                });
        }
    }

    bool had_rec = false;
    std::stringstream ss;
    ss << "OBSERVABLE_INCLUDE:L" << (op.args.empty() ? 0 : op.args[0]);
    ss << "*=";
    for (const auto &t : rec_targets) {
        if (t.is_measurement_record_target()) {
            if (had_rec) {
                ss << "*";
            }
            had_rec = true;
            write_rec_index(ss, t.value());
        }
    }
    if (had_rec || !had_paulis) {
        if (rec_targets.empty()) {
            ss.put('1');
        }
        diagram.add_entry(
            AsciiDiagramEntry{
                {
                    m2x(cur_moment),
                    q2y(pseudo_target.qubit_value()),
                    GATE_ALIGNMENT_X,
                    GATE_ALIGNMENT_Y,
                },
                ss.str(),
            });
    }
}

void DiagramTimelineAsciiDrawer::do_resolved_operation(const ResolvedTimelineOperation &op) {
    if (op.gate_type == GateType::MPP || op.gate_type == GateType::SPP || op.gate_type == GateType::SPP_DAG) {
        do_multi_qubit_gate_with_pauli_targets(op);
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

DiagramTimelineAsciiDrawer::DiagramTimelineAsciiDrawer(size_t num_qubits, bool has_ticks)
    : num_qubits(num_qubits), has_ticks(has_ticks) {
    cur_moment_used_flags.resize(num_qubits);
}

AsciiDiagram DiagramTimelineAsciiDrawer::make_diagram(const Circuit &circuit) {
    auto num_qubits = circuit.count_qubits();
    auto has_ticks = circuit.count_ticks() > 0;
    DiagramTimelineAsciiDrawer obj(num_qubits, has_ticks);
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
        obj.do_tick();
    }

    // Make space for the qubit lines to be drawn before other things.
    obj.diagram.lines.insert(obj.diagram.lines.begin(), obj.num_qubits, {{0, 0, 0.0, 0.5}, {0, 0, 1.0, 0.5}});
    // Overwrite the reserved space with the actual qubit lines.
    for (size_t q = 0; q < obj.num_qubits; q++) {
        obj.diagram.lines[q] = {
            {0, obj.q2y(q), 1.0, 0.5},
            {obj.m2x(obj.cur_moment), obj.q2y(q), 0.0, 0.5},
        };
        std::stringstream qubit;
        qubit << 'q' << q << ": ";
        obj.diagram.add_entry(
            AsciiDiagramEntry{
                {0, obj.q2y(q), 1.0, 0.5},
                qubit.str(),
            });
    }

    return obj.diagram;
}
