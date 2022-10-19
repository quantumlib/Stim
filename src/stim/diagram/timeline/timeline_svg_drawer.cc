#include "stim/diagram/timeline/timeline_svg_drawer.h"

#include "stim/diagram/circuit_timeline_helper.h"
#include "stim/diagram/diagram_util.h"

using namespace stim;
using namespace stim_draw_internal;

constexpr uint16_t PADDING = 32;
constexpr uint16_t CIRCUIT_START_X = 32;
constexpr uint16_t CIRCUIT_START_Y = 32;
constexpr uint16_t GATE_PITCH = 64;
constexpr uint16_t GATE_RADIUS = 16;

template <typename T>
inline void write_key_val(std::ostream &out, const char *key, const T &val) {
    out << ' ' << key << "=\"" << val << "\"";
}

size_t DiagramTimelineSvgDrawer::m2x(size_t m) const {
    return GATE_PITCH * m + GATE_RADIUS + GATE_RADIUS + CIRCUIT_START_X + PADDING;
}

size_t DiagramTimelineSvgDrawer::q2y(size_t q) const {
    return GATE_PITCH * q + CIRCUIT_START_Y + PADDING;
}

void DiagramTimelineSvgDrawer::do_feedback(
    const std::string &gate, const GateTarget &qubit_target, const GateTarget &feedback_target) {
    std::stringstream exponent;
    if (feedback_target.is_sweep_bit_target()) {
        exponent << "sweep[" << feedback_target.value() << "]";
    } else if (feedback_target.is_measurement_record_target()) {
        exponent << "rec[" << (feedback_target.value() + resolver.measure_offset) << "]";
    }

    float cx = m2x(cur_moment);
    float cy = q2y(qubit_target.qubit_value());
    draw_annotated_gate(cx, cy, SvgGateData{2, gate, "", exponent.str(), "lightgray"}, {});
}

void DiagramTimelineSvgDrawer::draw_x_control(float cx, float cy) {
    svg_out << "<circle";
    write_key_val(svg_out, "cx", cx);
    write_key_val(svg_out, "cy", cy);
    write_key_val(svg_out, "r", 8);
    write_key_val(svg_out, "stroke", "black");
    write_key_val(svg_out, "fill", "white");
    svg_out << "/>\n";

    svg_out << "<path d=\"";
    svg_out << "M" << cx - 8 << "," << cy << " ";
    svg_out << "L" << cx + 8 << "," << cy << " ";
    svg_out << "M" << cx << "," << cy - 8 << " ";
    svg_out << "L" << cx << "," << cy + 8 << " ";
    svg_out << "\"";
    write_key_val(svg_out, "stroke", "black");
    svg_out << "/>\n";
}

void DiagramTimelineSvgDrawer::draw_y_control(float cx, float cy) {
    float r = 10;
    float r_sin = r * sqrtf(3) * 0.5;
    float r_cos = r * 0.5;
    svg_out << "<path d=\"";
    svg_out << "M" << cx << "," << cy + r << " ";
    svg_out << "L" << cx - r_sin << "," << cy - r_cos << " ";
    svg_out << "L" << cx + r_sin << "," << cy - r_cos << " ";
    svg_out << "Z";
    svg_out << "\"";
    write_key_val(svg_out, "stroke", "black");
    write_key_val(svg_out, "fill", "gray");
    svg_out << "/>\n";
}

void DiagramTimelineSvgDrawer::draw_z_control(float cx, float cy) {
    svg_out << "<circle";
    write_key_val(svg_out, "cx", cx);
    write_key_val(svg_out, "cy", cy);
    write_key_val(svg_out, "r", 8);
    write_key_val(svg_out, "stroke", "none");
    write_key_val(svg_out, "fill", "black");
    svg_out << "/>\n";
}

void DiagramTimelineSvgDrawer::draw_swap_control(float cx, float cy) {
    svg_out << "<path d=\"";
    svg_out << "M" << cx - 4 << "," << cy - 4 << " ";
    svg_out << "L" << cx + 4 << "," << cy + 4 << " ";
    svg_out << "M" << cx + 4 << "," << cy - 4 << " ";
    svg_out << "L" << cx - 4 << "," << cy + 4 << " ";
    svg_out << "\"";
    write_key_val(svg_out, "stroke", "black");
    svg_out << "/>\n";
}

void DiagramTimelineSvgDrawer::draw_iswap_control(float cx, float cy, bool inverse) {
    svg_out << "<circle";
    write_key_val(svg_out, "cx", cx);
    write_key_val(svg_out, "cy", cy);
    write_key_val(svg_out, "r", 8);
    write_key_val(svg_out, "stroke", "none");
    write_key_val(svg_out, "fill", "gray");
    svg_out << "/>\n";

    draw_swap_control(cx, cy);

    if (inverse) {
        svg_out << "<path d=\"";
        svg_out << "M" << cx + 4 << "," << cy - 10 << " ";
        svg_out << "L" << cx + 12 << "," << cy - 10 << " ";
        svg_out << "M" << cx + 8 << "," << cy - 14 << " ";
        svg_out << "L" << cx + 8 << "," << cy - 2 << " ";
        svg_out << "\"";
        write_key_val(svg_out, "stroke", "black");
        svg_out << "/>\n";
    }
}

void DiagramTimelineSvgDrawer::draw_generic_box(
    float cx, float cy, const std::string &text, ConstPointerRange<double> end_args) {
    auto f = gate_data_map.find(text);
    if (f == gate_data_map.end()) {
        throw std::invalid_argument("Unhandled gate case: " + text);
    }
    SvgGateData data = f->second;
    draw_annotated_gate(cx, cy, data, end_args);
}

void DiagramTimelineSvgDrawer::draw_annotated_gate(
    float cx, float cy, const SvgGateData &data, ConstPointerRange<double> end_args) {
    cx += (data.span - 1) * GATE_PITCH * 0.5f;
    float w = GATE_PITCH * (data.span - 1) + GATE_RADIUS * 2.0f;
    float h = GATE_RADIUS * 2.0f;
    svg_out << "<rect";
    write_key_val(svg_out, "x", cx - w * 0.5);
    write_key_val(svg_out, "y", cy - h * 0.5);
    write_key_val(svg_out, "width", w);
    write_key_val(svg_out, "height", h);
    write_key_val(svg_out, "stroke", "black");
    write_key_val(svg_out, "fill", data.fill);
    svg_out << "/>\n";

    moment_width = std::max(moment_width, data.span);
    size_t n = utf8_char_count(data.body) + utf8_char_count(data.subscript) + +utf8_char_count(data.superscript);
    svg_out << "<text";
    write_key_val(svg_out, "dominant-baseline", "central");
    write_key_val(svg_out, "text-anchor", "middle");
    write_key_val(svg_out, "font-family", "monospace");
    write_key_val(svg_out, "font-size", n >= 4 && data.span == 1 ? 12 : 16);
    write_key_val(svg_out, "x", cx);
    write_key_val(svg_out, "y", cy);
    svg_out << ">";
    svg_out << data.body;
    if (data.superscript[0] != '\0') {
        svg_out << "<tspan";
        write_key_val(svg_out, "baseline-shift", "super");
        write_key_val(svg_out, "font-size", "10");
        svg_out << ">";
        svg_out << data.superscript;
        svg_out << "</tspan>";
    }
    if (data.subscript[0] != '\0') {
        svg_out << "<tspan";
        write_key_val(svg_out, "baseline-shift", "sub");
        write_key_val(svg_out, "font-size", 10);
        svg_out << ">";
        svg_out << data.subscript;
        svg_out << "</tspan>";
    }
    svg_out << "</text>\n";

    if (!end_args.empty()) {
        svg_out << "<text";
        write_key_val(svg_out, "dominant-baseline", "hanging");
        write_key_val(svg_out, "text-anchor", "middle");
        write_key_val(svg_out, "font-family", "monospace");
        write_key_val(svg_out, "font-size", 10);
        write_key_val(svg_out, "stroke", "red");
        write_key_val(svg_out, "x", cx);
        write_key_val(svg_out, "y", cy + GATE_RADIUS + 4);
        svg_out << ">";
        svg_out << comma_sep(end_args, ",");
        svg_out << "</text>\n";
    }
}

void DiagramTimelineSvgDrawer::draw_two_qubit_gate_end_point(
    float cx, float cy, const std::string &type, ConstPointerRange<double> args) {
    if (type == "X") {
        draw_x_control(cx, cy);
    } else if (type == "Y") {
        draw_y_control(cx, cy);
    } else if (type == "Z") {
        draw_z_control(cx, cy);
    } else if (type == "SWAP") {
        draw_swap_control(cx, cy);
    } else if (type == "ISWAP") {
        draw_iswap_control(cx, cy, false);
    } else if (type == "ISWAP_DAG") {
        draw_iswap_control(cx, cy, true);
    } else {
        draw_generic_box(cx, cy, type, args);
    }
}

void DiagramTimelineSvgDrawer::do_two_qubit_gate_instance(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);

    const GateTarget &target1 = op.targets[0];
    const GateTarget &target2 = op.targets[1];
    auto ends = two_qubit_gate_pieces(op.gate->name);
    if (target1.is_measurement_record_target() || target1.is_sweep_bit_target()) {
        do_feedback(ends.second, target2, target1);
        return;
    }
    if (target2.is_measurement_record_target() || target2.is_sweep_bit_target()) {
        do_feedback(ends.first, target1, target2);
        return;
    }

    auto pieces = two_qubit_gate_pieces(op.gate->name);
    if (op.gate->id == gate_name_to_id("PAULI_CHANNEL_2")) {
        pieces.first.append("[0]");
        pieces.second.append("[1]");
    }

    auto x = m2x(cur_moment);
    auto y1 = q2y(target1.qubit_value());
    auto y2 = q2y(target2.qubit_value());
    draw_two_qubit_gate_end_point(x, y1, pieces.first, y1 > y2 ? op.args : ConstPointerRange<double>{});
    draw_two_qubit_gate_end_point(x, y2, pieces.second, y2 > y1 ? op.args : ConstPointerRange<double>{});
}

void DiagramTimelineSvgDrawer::start_next_moment() {
    cur_moment += moment_width;
    moment_width = 1;
    cur_moment_is_used = false;
    cur_moment_used_flags.clear();
    cur_moment_used_flags.resize(num_qubits);
}

void DiagramTimelineSvgDrawer::do_tick() {
    if (has_ticks && cur_moment > tick_start_moment) {
        float x1 = (float)m2x(tick_start_moment);
        float x2 = (float)m2x(cur_moment + moment_width - 1);
        float y1 = PADDING;
        float y2 = (float)q2y(num_qubits);
        x1 -= GATE_PITCH * 0.4375;
        x2 += GATE_PITCH * 0.4375;

        svg_out << "<path d=\"";
        svg_out << "M" << x1 << "," << y1 + GATE_RADIUS * 0.5f << " ";
        svg_out << "L" << x1 << "," << y1 << " ";
        svg_out << "L" << x2 << "," << y1 << " ";
        svg_out << "L" << x2 << "," << y1 + GATE_RADIUS * 0.5f << " ";
        svg_out << "\" stroke=\"black\" fill=\"none\"/>\n";

        svg_out << "<path d=\"";
        svg_out << "M" << x1 << "," << y2 - GATE_RADIUS * 0.5f << " ";
        svg_out << "L" << x1 << "," << y2 << " ";
        svg_out << "L" << x2 << "," << y2 << " ";
        svg_out << "L" << x2 << "," << y2 - GATE_RADIUS * 0.5f << " ";
        svg_out << "\" stroke=\"black\" fill=\"none\"/>\n";
    }

    start_next_moment();
    tick_start_moment = cur_moment;
}

void DiagramTimelineSvgDrawer::do_single_qubit_gate_instance(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);
    const auto &target = op.targets[0];

    std::stringstream ss;
    ss << op.gate->name;

    auto cx = m2x(cur_moment);
    auto cy = q2y(target.qubit_value());
    draw_generic_box(cx, cy, ss.str(), op.args);
    if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS) {
        draw_rec(cx, cy);
    }
}

void DiagramTimelineSvgDrawer::write_det_index(std::ostream &out) {
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
void DiagramTimelineSvgDrawer::write_rec_index(std::ostream &out, int64_t lookback_shift) {
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

void DiagramTimelineSvgDrawer::write_coords(std::ostream &out, ConstPointerRange<double> relative_coordinates) {
    out.put('(');
    for (size_t k = 0; k < relative_coordinates.size(); k++) {
        if (k) {
            out.put(',');
        }
        write_coord(out, k, relative_coordinates[k]);
    }
    out.put(')');
}

void DiagramTimelineSvgDrawer::write_coord(std::ostream &out, size_t coord_index, double absolute_coordinate) {
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

std::pair<size_t, size_t> compute_minmax_q(ConstPointerRange<GateTarget> targets) {
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
    return {min_q, max_q};
}
void DiagramTimelineSvgDrawer::reserve_drawing_room_for_targets(ConstPointerRange<GateTarget> targets) {
    auto minmax_q = compute_minmax_q(targets);
    auto min_q = minmax_q.first;
    auto max_q = minmax_q.second;
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
        auto x = m2x(cur_moment);
        auto y1 = q2y(min_q);
        auto y2 = q2y(max_q);
        svg_out << "<path d=\"";
        svg_out << "M" << x << "," << y1 << " ";
        svg_out << "L" << x << "," << y2 << " ";
        svg_out << "\"";
        write_key_val(svg_out, "stroke", "black");
        svg_out << "/>\n";
    }
}

void DiagramTimelineSvgDrawer::draw_rec(float cx, float cy) {
    svg_out << "<text";
    write_key_val(svg_out, "text-anchor", "middle");
    write_key_val(svg_out, "font-family", "monospace");
    write_key_val(svg_out, "font-size", 8);
    write_key_val(svg_out, "x", cx);
    write_key_val(svg_out, "y", cy - GATE_RADIUS - 4);
    svg_out << ">";
    write_rec_index(svg_out);
    svg_out << "</text>\n";
}

void DiagramTimelineSvgDrawer::do_multi_qubit_gate_with_pauli_targets(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);
    auto minmax_q = compute_minmax_q(op.targets);

    for (const auto &t : op.targets) {
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
        auto cx = m2x(cur_moment);
        auto cy = q2y(t.qubit_value());
        draw_generic_box(cx, cy, ss.str(), t.qubit_value() == minmax_q.second ? op.args : ConstPointerRange<double>{});
        if (op.gate->flags & GATE_PRODUCES_NOISY_RESULTS && t.qubit_value() == minmax_q.first) {
            draw_rec(cx, cy);
        }
    }
}

void DiagramTimelineSvgDrawer::do_start_repeat(const CircuitTimelineLoopData &loop_data) {
    if (cur_moment_is_used) {
        do_tick();
    }

    auto x = m2x(cur_moment);
    auto y1 = PADDING;
    auto y2 = q2y(num_qubits);
    y1 += (resolver.cur_loop_nesting.size() - 1) * 4;
    y2 -= (resolver.cur_loop_nesting.size() - 1) * 4;

    svg_out << "<path d=\"";
    svg_out << "M" << x + GATE_RADIUS * 0.5 << "," << y1 << " ";
    svg_out << "L" << x << "," << y1 << " ";
    svg_out << "L" << x << "," << y2 << " ";
    svg_out << "L" << x + GATE_RADIUS * 0.5 << "," << y2 << " ";
    svg_out << "\" stroke=\"black\" fill=\"none\"/>\n";

    svg_out << "<text";
    write_key_val(svg_out, "dominant-baseline", "auto");
    write_key_val(svg_out, "text-anchor", "start");
    write_key_val(svg_out, "font-family", "monospace");
    write_key_val(svg_out, "font-size", 12);
    write_key_val(svg_out, "x", x + 4);
    write_key_val(svg_out, "y", y2 - 4);
    svg_out << ">";
    svg_out << "REP" << loop_data.num_repetitions;
    svg_out << "</text>\n";

    start_next_moment();
    tick_start_moment = cur_moment;
}

void DiagramTimelineSvgDrawer::do_end_repeat(const CircuitTimelineLoopData &loop_data) {
    if (cur_moment_is_used) {
        do_tick();
    }

    auto x = m2x(cur_moment);
    auto y1 = PADDING;
    auto y2 = q2y(num_qubits);
    y1 += (resolver.cur_loop_nesting.size() - 1) * 4;
    y2 -= (resolver.cur_loop_nesting.size() - 1) * 4;

    svg_out << "<path d=\"";
    svg_out << "M" << x - GATE_RADIUS * 0.5 << "," << y1 << " ";
    svg_out << "L" << x << "," << y1 << " ";
    svg_out << "L" << x << "," << y2 << " ";
    svg_out << "L" << x - GATE_RADIUS * 0.5 << "," << y2 << " ";
    svg_out << "\" stroke=\"black\" fill=\"none\"/>\n";

    start_next_moment();
    tick_start_moment = cur_moment;
}

void DiagramTimelineSvgDrawer::do_mpp(const ResolvedTimelineOperation &op) {
    do_multi_qubit_gate_with_pauli_targets(op);
}

void DiagramTimelineSvgDrawer::do_correlated_error(const ResolvedTimelineOperation &op) {
    if (cur_moment_is_used) {
        start_next_moment();
    }
    do_multi_qubit_gate_with_pauli_targets(op);
}

void DiagramTimelineSvgDrawer::do_else_correlated_error(const ResolvedTimelineOperation &op) {
    do_correlated_error(op);
}

void DiagramTimelineSvgDrawer::do_qubit_coords(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);
    assert(op.targets.size() == 1);
    const auto &target = op.targets[0];

    std::stringstream ss;
    ss << "COORDS";
    write_coords(ss, op.args);
    draw_annotated_gate(
        m2x(cur_moment),
        q2y(target.qubit_value()),
        SvgGateData{(uint16_t)(2 + op.args.size()), ss.str(), "", "", "white"},
        {});
}

void DiagramTimelineSvgDrawer::do_detector(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);
    auto pseudo_target = op.targets[0];
    ConstPointerRange<GateTarget> rec_targets = op.targets;
    rec_targets.ptr_start++;

    auto cx = m2x(cur_moment);
    auto cy = q2y(pseudo_target.qubit_value());
    auto span = (uint16_t)(1 + std::max(std::max(op.targets.size(), op.args.size()), (size_t)2));
    draw_annotated_gate(cx, cy, SvgGateData{span, "DETECTOR", "", "", "lightgray"}, {});
    cx += (span - 1) * GATE_PITCH * 0.5f;

    if (!op.args.empty()) {
        svg_out << "<text";
        write_key_val(svg_out, "dominant-baseline", "hanging");
        write_key_val(svg_out, "text-anchor", "middle");
        write_key_val(svg_out, "font-family", "monospace");
        write_key_val(svg_out, "font-size", 8);
        write_key_val(svg_out, "x", cx);
        write_key_val(svg_out, "y", cy + GATE_RADIUS + 4);
        svg_out << ">coords=";
        write_coords(svg_out, op.args);
        svg_out << "</text>\n";
    }

    svg_out << "<text";
    write_key_val(svg_out, "text-anchor", "middle");
    write_key_val(svg_out, "font-family", "monospace");
    write_key_val(svg_out, "font-size", 8);
    write_key_val(svg_out, "x", cx);
    write_key_val(svg_out, "y", cy - GATE_RADIUS - 4);
    svg_out << ">";
    write_det_index(svg_out);
    svg_out << " = ";
    for (size_t k = 0; k < rec_targets.size(); k++) {
        if (k) {
            svg_out << "*";
        }
        write_rec_index(svg_out, rec_targets[k].value());
    }
    if (rec_targets.empty()) {
        svg_out << "1 (vacuous)";
    }
    svg_out << "</text>\n";
}

void DiagramTimelineSvgDrawer::do_observable_include(const ResolvedTimelineOperation &op) {
    reserve_drawing_room_for_targets(op.targets);
    auto pseudo_target = op.targets[0];
    ConstPointerRange<GateTarget> rec_targets = op.targets;
    rec_targets.ptr_start++;

    auto cx = m2x(cur_moment);
    auto cy = q2y(pseudo_target.qubit_value());
    auto span = (uint16_t)(1 + std::max(std::max(op.targets.size(), op.args.size()), (size_t)2));
    std::stringstream ss;
    ss << "OBS_INCLUDE(" << op.args[0] << ")";
    draw_annotated_gate(cx, cy, SvgGateData{span, ss.str(), "", "", "lightgray"}, {});
    cx += (span - 1) * GATE_PITCH * 0.5f;

    svg_out << "<text";
    write_key_val(svg_out, "text-anchor", "middle");
    write_key_val(svg_out, "font-family", "monospace");
    write_key_val(svg_out, "font-size", 8);
    write_key_val(svg_out, "x", cx);
    write_key_val(svg_out, "y", cy - GATE_RADIUS - 4);
    svg_out << ">";
    svg_out << "L" << op.args[0] << " *= ";
    for (size_t k = 0; k < rec_targets.size(); k++) {
        if (k) {
            svg_out << "*";
        }
        write_rec_index(svg_out, rec_targets[k].value());
    }
    if (rec_targets.empty()) {
        svg_out << "1 (vacuous)";
    }
    svg_out << "</text>\n";
}

void DiagramTimelineSvgDrawer::do_resolved_operation(const ResolvedTimelineOperation &op) {
    if (op.gate->id == gate_name_to_id("MPP")) {
        do_mpp(op);
    } else if (op.gate->id == gate_name_to_id("DETECTOR")) {
        do_detector(op);
    } else if (op.gate->id == gate_name_to_id("OBSERVABLE_INCLUDE")) {
        do_observable_include(op);
    } else if (op.gate->id == gate_name_to_id("QUBIT_COORDS")) {
        do_qubit_coords(op);
    } else if (op.gate->id == gate_name_to_id("E")) {
        do_correlated_error(op);
    } else if (op.gate->id == gate_name_to_id("ELSE_CORRELATED_ERROR")) {
        do_else_correlated_error(op);
    } else if (op.gate->id == gate_name_to_id("TICK")) {
        do_tick();
    } else if (op.gate->flags & GATE_TARGETS_PAIRS) {
        do_two_qubit_gate_instance(op);
    } else {
        do_single_qubit_gate_instance(op);
    }
}

DiagramTimelineSvgDrawer::DiagramTimelineSvgDrawer(std::ostream &svg_out, size_t num_qubits, bool has_ticks)
    : svg_out(svg_out), num_qubits(num_qubits), has_ticks(has_ticks), gate_data_map(SvgGateData::make_gate_data_map()) {
    cur_moment_used_flags.resize(num_qubits);
}

void DiagramTimelineSvgDrawer::make_diagram_write_to(const Circuit &circuit, std::ostream &svg_out) {
    auto num_qubits = circuit.count_qubits();
    auto has_ticks = circuit.count_ticks() > 0;
    std::stringstream buffer;
    DiagramTimelineSvgDrawer obj(buffer, num_qubits, has_ticks);
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

    auto w = obj.m2x(obj.cur_moment);
    svg_out << R"SVG(<svg viewBox="0 0 )SVG";
    svg_out << w + PADDING;
    svg_out << " ";
    svg_out << obj.q2y(obj.num_qubits) + PADDING;
    svg_out << '"' << ' ';
    write_key_val(svg_out, "version", "1.1");
    write_key_val(svg_out, "xmlns", "http://www.w3.org/2000/svg");
    svg_out << ">\n";

    // Make sure qubit lines are drawn first, so they are in the background.
    for (size_t q = 0; q < obj.num_qubits; q++) {
        auto x1 = PADDING + CIRCUIT_START_X;
        auto x2 = w;
        auto y = obj.q2y(q);

        svg_out << "<path d=\"";
        svg_out << "M" << x1 << "," << y << " ";
        svg_out << "L" << x2 << "," << y << " ";
        svg_out << "\"";
        write_key_val(svg_out, "stroke", "black");
        svg_out << "/>\n";

        svg_out << "<text";
        write_key_val(svg_out, "dominant-baseline", "central");
        write_key_val(svg_out, "text-anchor", "end");
        write_key_val(svg_out, "font-family", "monospace");
        write_key_val(svg_out, "font-size", 12);
        write_key_val(svg_out, "x", x1);
        write_key_val(svg_out, "y", y);
        svg_out << ">";
        svg_out << "q" << q;
        svg_out << "</text>\n";
    }

    svg_out << buffer.str();
    svg_out << "</svg>";
}
