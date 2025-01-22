#include "stim/diagram/timeline/timeline_svg_drawer.h"

#include "stim/circuit/gate_decomposition.h"
#include "stim/diagram/circuit_timeline_helper.h"
#include "stim/diagram/coord.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/diagram/diagram_util.h"
#include "stim/stabilizers/pauli_string.h"

using namespace stim;
using namespace stim_draw_internal;

constexpr uint16_t TIME_SLICE_PADDING = 64;
constexpr uint16_t PADDING = 32;
constexpr uint16_t CIRCUIT_START_X = 32;
constexpr uint16_t CIRCUIT_START_Y = 32;
constexpr uint16_t GATE_PITCH = 64;
constexpr uint16_t GATE_RADIUS = 16;
constexpr uint16_t CONTROL_RADIUS = 12;
constexpr float SLICE_WINDOW_GAP = 1.1f;

template <typename T>
inline void write_key_val(std::ostream &out, const char *key, const T &val) {
    out << ' ' << key << "=\"" << val << "\"";
}

size_t DiagramTimelineSvgDrawer::m2x(size_t m) const {
    return GATE_PITCH * m + GATE_RADIUS + GATE_RADIUS + CIRCUIT_START_X + PADDING;
}

Coord<2> DiagramTimelineSvgDrawer::qt2xy(uint64_t tick, uint64_t moment_delta, size_t q) const {
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
        Coord<2> result = coord_sys.qubit_coords[q];
        result.xyz[0] += moment_delta * 14;
        result.xyz[1] += moment_delta * 16;
        result.xyz[0] += TIME_SLICE_PADDING;
        result.xyz[1] += TIME_SLICE_PADDING;
        uint64_t s = tick - min_tick;
        uint64_t sx = s % num_cols;
        uint64_t sy = s / num_cols;
        result.xyz[0] += coord_sys.size.xyz[0] * sx * SLICE_WINDOW_GAP;
        result.xyz[1] += coord_sys.size.xyz[1] * sy * SLICE_WINDOW_GAP;
        return result;
    }
    return {(float)m2x(cur_moment), (float)q2y(q)};
}

Coord<2> DiagramTimelineSvgDrawer::q2xy(size_t q) const {
    return qt2xy(resolver.num_ticks_seen, cur_moment - tick_start_moment, q);
}

size_t DiagramTimelineSvgDrawer::q2y(size_t q) const {
    return GATE_PITCH * q + CIRCUIT_START_Y + PADDING;
}

void DiagramTimelineSvgDrawer::do_feedback(
    std::string_view gate, const GateTarget &qubit_target, const GateTarget &feedback_target) {
    std::stringstream exponent;
    if (feedback_target.is_sweep_bit_target()) {
        exponent << "sweep";
        if (mode == DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
            exponent << "[" << feedback_target.value() << "]";
        }
    } else if (feedback_target.is_measurement_record_target()) {
        exponent << "rec";
        if (mode == DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
            exponent << "[" << (feedback_target.value() + resolver.measure_offset) << "]";
        }
    }

    auto c = q2xy(qubit_target.qubit_value());
    draw_annotated_gate(
        c.xyz[0],
        c.xyz[1],
        SvgGateData{
            (uint16_t)(mode == DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE ? 2 : 1),
            std::string(gate),
            "",
            exponent.str(),
            "lightgray",
            "black",
            0,
            10},
        {});
}

void DiagramTimelineSvgDrawer::draw_x_control(float cx, float cy) {
    svg_out << "<circle";
    write_key_val(svg_out, "cx", cx);
    write_key_val(svg_out, "cy", cy);
    write_key_val(svg_out, "r", CONTROL_RADIUS);
    write_key_val(svg_out, "stroke", "black");
    write_key_val(svg_out, "fill", "white");
    svg_out << "/>\n";

    svg_out << "<path d=\"";
    svg_out << "M" << cx - CONTROL_RADIUS << "," << cy << " ";
    svg_out << "L" << cx + CONTROL_RADIUS << "," << cy << " ";
    svg_out << "M" << cx << "," << cy - CONTROL_RADIUS << " ";
    svg_out << "L" << cx << "," << cy + CONTROL_RADIUS << " ";
    svg_out << "\"";
    write_key_val(svg_out, "stroke", "black");
    svg_out << "/>\n";
}

void DiagramTimelineSvgDrawer::draw_y_control(float cx, float cy) {
    float r = CONTROL_RADIUS * 1.4;
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
    write_key_val(svg_out, "r", CONTROL_RADIUS);
    write_key_val(svg_out, "stroke", "none");
    write_key_val(svg_out, "fill", "black");
    svg_out << "/>\n";
}

void DiagramTimelineSvgDrawer::draw_xswap_control(float cx, float cy) {
    svg_out << "<circle";
    write_key_val(svg_out, "cx", cx);
    write_key_val(svg_out, "cy", cy);
    write_key_val(svg_out, "r", CONTROL_RADIUS);
    write_key_val(svg_out, "stroke", "black");
    write_key_val(svg_out, "fill", "white");
    svg_out << "/>\n";

    float r = CONTROL_RADIUS / 2.2f;
    svg_out << "<path d=\"";
    svg_out << "M" << cx - r << "," << cy - r << " ";
    svg_out << "L" << cx + r << "," << cy + r << " ";
    svg_out << "M" << cx + r << "," << cy - r << " ";
    svg_out << "L" << cx - r << "," << cy + r << " ";
    svg_out << "\"";
    write_key_val(svg_out, "stroke", "black");
    write_key_val(svg_out, "stroke-width", 4);
    svg_out << "/>\n";
}

void DiagramTimelineSvgDrawer::draw_zswap_control(float cx, float cy) {
    svg_out << "<circle";
    write_key_val(svg_out, "cx", cx);
    write_key_val(svg_out, "cy", cy);
    write_key_val(svg_out, "r", CONTROL_RADIUS);
    write_key_val(svg_out, "stroke", "none");
    write_key_val(svg_out, "fill", "black");
    svg_out << "/>\n";

    float r = CONTROL_RADIUS / 2.2f;
    svg_out << "<path d=\"";
    svg_out << "M" << cx - r << "," << cy - r << " ";
    svg_out << "L" << cx + r << "," << cy + r << " ";
    svg_out << "M" << cx + r << "," << cy - r << " ";
    svg_out << "L" << cx - r << "," << cy + r << " ";
    svg_out << "\"";
    write_key_val(svg_out, "stroke", "white");
    write_key_val(svg_out, "stroke-width", 4);
    svg_out << "/>\n";
}

void DiagramTimelineSvgDrawer::draw_swap_control(float cx, float cy) {
    float r = CONTROL_RADIUS / 2.0f;
    svg_out << "<path d=\"";
    svg_out << "M" << cx - r << "," << cy - r << " ";
    svg_out << "L" << cx + r << "," << cy + r << " ";
    svg_out << "M" << cx + r << "," << cy - r << " ";
    svg_out << "L" << cx - r << "," << cy + r << " ";
    svg_out << "\"";
    write_key_val(svg_out, "stroke", "black");
    write_key_val(svg_out, "stroke-width", 3);
    svg_out << "/>\n";
}

void DiagramTimelineSvgDrawer::draw_iswap_control(float cx, float cy, bool inverse) {
    svg_out << "<circle";
    write_key_val(svg_out, "cx", cx);
    write_key_val(svg_out, "cy", cy);
    write_key_val(svg_out, "r", CONTROL_RADIUS);
    write_key_val(svg_out, "stroke", "none");
    write_key_val(svg_out, "fill", "gray");
    svg_out << "/>\n";

    draw_swap_control(cx, cy);

    if (inverse) {
        svg_out << "<path d=\"";
        svg_out << "M" << cx + CONTROL_RADIUS - 4 << "," << cy - CONTROL_RADIUS - 2 << " ";
        svg_out << "L" << cx + CONTROL_RADIUS + 4 << "," << cy - CONTROL_RADIUS - 2 << " ";
        svg_out << "M" << cx + CONTROL_RADIUS << "," << cy - CONTROL_RADIUS - 6 << " ";
        svg_out << "L" << cx + CONTROL_RADIUS << "," << cy - 2 << " ";
        svg_out << "\"";
        write_key_val(svg_out, "stroke", "black");
        svg_out << "/>\n";
    }
}

void DiagramTimelineSvgDrawer::draw_generic_box(
    float cx, float cy, std::string_view text, SpanRef<const double> end_args) {
    auto f = gate_data_map.find(text);
    if (f == gate_data_map.end()) {
        throw std::invalid_argument(
            "DiagramTimelineSvgDrawer::draw_generic_box unhandled gate case: " + std::string(text));
    }
    SvgGateData data = f->second;
    draw_annotated_gate(cx, cy, data, end_args);
}

void DiagramTimelineSvgDrawer::draw_annotated_gate(
    float cx, float cy, const SvgGateData &data, SpanRef<const double> end_args) {
    cx += (data.span - 1) * GATE_PITCH * 0.5f;
    float w = GATE_PITCH * (data.span - 1) + GATE_RADIUS * 2.0f;
    float h = GATE_RADIUS * 2.0f;
    size_t n = utf8_char_count(data.body) + utf8_char_count(data.subscript) + +utf8_char_count(data.superscript);
    auto font_size = data.font_size != 0 ? data.font_size : n == 1 ? 30 : n >= 4 && data.span == 1 ? 12 : 16;
    svg_out << "<rect";
    write_key_val(svg_out, "x", cx - w * 0.5);
    write_key_val(svg_out, "y", cy - h * 0.5);
    write_key_val(svg_out, "width", w);
    write_key_val(svg_out, "height", h);
    write_key_val(svg_out, "stroke", "black");
    write_key_val(svg_out, "fill", data.fill);
    svg_out << "/>\n";

    moment_width = std::max(moment_width, data.span);
    svg_out << "<text";
    write_key_val(svg_out, "dominant-baseline", "central");
    write_key_val(svg_out, "text-anchor", "middle");
    write_key_val(svg_out, "font-family", "monospace");
    write_key_val(svg_out, "font-size", font_size);
    write_key_val(svg_out, "x", cx);
    write_key_val(svg_out, "y", cy + data.y_shift);
    if (data.text_color != "black") {
        write_key_val(svg_out, "fill", data.text_color);
    }
    svg_out << ">";
    svg_out << data.body;
    if (data.superscript[0] != '\0') {
        svg_out << "<tspan";
        write_key_val(svg_out, "baseline-shift", "super");
        write_key_val(svg_out, "font-size", data.sub_font_size);
        svg_out << ">";
        svg_out << data.superscript;
        svg_out << "</tspan>";
    }
    if (data.subscript[0] != '\0') {
        svg_out << "<tspan";
        write_key_val(svg_out, "baseline-shift", "sub");
        write_key_val(svg_out, "font-size", data.sub_font_size);
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
        write_key_val(svg_out, "font-size", data.sub_font_size);
        write_key_val(svg_out, "stroke", "red");
        write_key_val(svg_out, "x", cx);
        write_key_val(svg_out, "y", cy + GATE_RADIUS + 4);
        svg_out << ">";
        svg_out << comma_sep(end_args, ",");
        svg_out << "</text>\n";
    }
}

void DiagramTimelineSvgDrawer::draw_two_qubit_gate_end_point(
    float cx, float cy, std::string_view type, SpanRef<const double> args) {
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
    } else if (type == "XSWAP") {
        draw_xswap_control(cx, cy);
    } else if (type == "ZSWAP") {
        draw_zswap_control(cx, cy);
    } else {
        draw_generic_box(cx, cy, type, args);
    }
}

void DiagramTimelineSvgDrawer::do_two_qubit_gate_instance(const ResolvedTimelineOperation &op) {
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
    std::string piece1 = std::string(pieces.first);
    std::string piece2 = std::string(pieces.second);
    if (op.gate_type == GateType::PAULI_CHANNEL_2) {
        piece1.append("[0]");
        piece2.append("[1]");
    }

    auto c1 = q2xy(target1.qubit_value());
    auto c2 = q2xy(target2.qubit_value());
    bool b = c1.xyz[1] > c2.xyz[1];
    draw_two_qubit_gate_end_point(c1.xyz[0], c1.xyz[1], piece1, b ? op.args : SpanRef<const double>{});
    draw_two_qubit_gate_end_point(c2.xyz[0], c2.xyz[1], piece2, !b ? op.args : SpanRef<const double>{});
}

void DiagramTimelineSvgDrawer::start_next_moment() {
    cur_moment += moment_width;
    moment_width = 1;
    cur_moment_is_used = false;
    cur_moment_used_flags.clear();
    cur_moment_used_flags.resize(num_qubits);
}

void DiagramTimelineSvgDrawer::do_tick() {
    if (has_ticks && cur_moment > tick_start_moment && mode == DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
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
    const auto &gate_data = GATE_DATA[op.gate_type];
    ss << gate_data.name;

    auto c = q2xy(target.qubit_value());
    draw_generic_box(c.xyz[0], c.xyz[1], ss.str(), op.args);
    if (gate_data.flags & GATE_PRODUCES_RESULTS) {
        draw_rec(c.xyz[0], c.xyz[1]);
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

void DiagramTimelineSvgDrawer::write_coords(std::ostream &out, SpanRef<const double> relative_coordinates) {
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

std::pair<size_t, size_t> compute_minmax_q(SpanRef<const GateTarget> targets) {
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

void DiagramTimelineSvgDrawer::reserve_drawing_room_for_targets(SpanRef<const GateTarget> targets) {
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
        for (const auto &t : targets) {
            if (t.has_qubit_value() && cur_moment_used_flags[t.qubit_value()]) {
                start_next_moment();
                break;
            }
        }
        std::vector<Coord<2>> coords;
        for (const auto &t : targets) {
            if (t.has_qubit_value()) {
                cur_moment_used_flags[t.qubit_value()] = true;
                coords.push_back(q2xy(t.qubit_value()));
            }
        }
        cur_moment_is_used = true;

        if (coords.size() > 1) {
            svg_out << "<path d=\"";
            svg_out << "M" << coords[0].xyz[0] << "," << coords[0].xyz[1] << " ";
            for (size_t k = 1; k < coords.size(); k++) {
                auto p1 = coords[k - 1];
                auto p2 = coords[k];
                auto dp = p2 - p1;
                if (dp.norm() < coord_sys.unit_distance * 1.1) {
                    svg_out << "L";
                    svg_out << p2.xyz[0] << "," << p2.xyz[1] << " ";
                } else {
                    auto dp2 = Coord<2>{-dp.xyz[1], dp.xyz[0]};
                    if (2 * dp2.xyz[0] + 3 * dp2.xyz[1] < 0) {
                        dp2 *= -1;
                    }
                    auto p3 = p1 + dp * 0.2 + dp2 * 0.2;
                    auto p4 = p2 + dp * -0.2 + dp2 * 0.2;
                    svg_out << "C";
                    svg_out << p3.xyz[0] << " " << p3.xyz[1] << ",";
                    svg_out << p4.xyz[0] << " " << p4.xyz[1] << ",";
                    svg_out << p2.xyz[0] << " " << p2.xyz[1] << " ";
                }
            }
            svg_out << "\"";
            write_key_val(svg_out, "fill", "none");
            write_key_val(svg_out, "stroke", "black");
            write_key_val(svg_out, "stroke-width", "5");
            svg_out << "/>\n";
        }

        return;
    }

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
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
        return;
    }
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
        const auto &gate_data = GATE_DATA[op.gate_type];
        ss << gate_data.name;
        if (t.is_x_target()) {
            ss << "[X]";
        } else if (t.is_y_target()) {
            ss << "[Y]";
        } else if (t.is_z_target()) {
            ss << "[Z]";
        }
        auto c = q2xy(t.qubit_value());
        draw_generic_box(
            c.xyz[0], c.xyz[1], ss.str(), t.qubit_value() == minmax_q.second ? op.args : SpanRef<const double>{});
        if (gate_data.flags & GATE_PRODUCES_RESULTS && t.qubit_value() == minmax_q.first) {
            draw_rec(c.xyz[0], c.xyz[1]);
        }
    }
}

void DiagramTimelineSvgDrawer::do_start_repeat(const CircuitTimelineLoopData &loop_data) {
    if (resolver.num_ticks_seen < min_tick || resolver.num_ticks_seen > max_tick) {
        return;
    }
    if (cur_moment_is_used) {
        do_tick();
    }
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
        return;
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
    if (resolver.num_ticks_seen < min_tick || resolver.num_ticks_seen > max_tick) {
        return;
    }
    if (cur_moment_is_used) {
        do_tick();
    }
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
        return;
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

void DiagramTimelineSvgDrawer::do_spp(const ResolvedTimelineOperation &op) {
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
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
        return;
    }
    reserve_drawing_room_for_targets(op.targets);
    assert(op.targets.size() == 1);
    const auto &target = op.targets[0];

    std::stringstream ss;
    ss << "COORDS";
    write_coords(ss, op.args);
    auto c = q2xy(target.qubit_value());
    draw_annotated_gate(
        c.xyz[0], c.xyz[1], SvgGateData{(uint16_t)(2 + op.args.size()), ss.str(), "", "", "white", "black", 0, 10}, {});
}

void DiagramTimelineSvgDrawer::do_detector(const ResolvedTimelineOperation &op) {
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
        return;
    }
    reserve_drawing_room_for_targets(op.targets);
    auto pseudo_target = op.targets[0];
    SpanRef<const GateTarget> rec_targets = op.targets;
    rec_targets.ptr_start++;

    auto c = q2xy(pseudo_target.qubit_value());
    auto span = (uint16_t)(1 + std::max(std::max(op.targets.size(), op.args.size()), (size_t)2));
    draw_annotated_gate(c.xyz[0], c.xyz[1], SvgGateData{span, "DETECTOR", "", "", "lightgray", "black", 0, 10}, {});
    c.xyz[0] += (span - 1) * GATE_PITCH * 0.5f;

    if (!op.args.empty()) {
        svg_out << "<text";
        write_key_val(svg_out, "dominant-baseline", "hanging");
        write_key_val(svg_out, "text-anchor", "middle");
        write_key_val(svg_out, "font-family", "monospace");
        write_key_val(svg_out, "font-size", 8);
        write_key_val(svg_out, "x", c.xyz[0]);
        write_key_val(svg_out, "y", c.xyz[1] + GATE_RADIUS + 4);
        svg_out << ">coords=";
        write_coords(svg_out, op.args);
        svg_out << "</text>\n";
    }

    svg_out << "<text";
    write_key_val(svg_out, "text-anchor", "middle");
    write_key_val(svg_out, "font-family", "monospace");
    write_key_val(svg_out, "font-size", 8);
    write_key_val(svg_out, "x", c.xyz[0]);
    write_key_val(svg_out, "y", c.xyz[1] - GATE_RADIUS - 4);
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
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
        return;
    }
    reserve_drawing_room_for_targets(op.targets);
    auto pseudo_target = op.targets[0];
    SpanRef<const GateTarget> rec_targets = op.targets;
    rec_targets.ptr_start++;

    // Draw per-quit L#*=P boxes.
    bool had_paulis = false;
    bool had_rec = false;
    for (const auto &t : rec_targets) {
        if (t.is_measurement_record_target()) {
            had_rec = true;
        }
        if (t.is_pauli_target()) {
            had_paulis = true;
            std::stringstream ss;
            ss << "L" << (op.args.empty() ? 0 : op.args[0]) << "*=";
            ss << t.pauli_type();
            auto c = q2xy(t.qubit_value());
            draw_annotated_gate(c.xyz[0], c.xyz[1], SvgGateData{
                .span=2,
                .body=ss.str(),
                .subscript="",
                .superscript="",
                .fill="lightgray",
                .text_color="black",
                .font_size=0,
                .sub_font_size=10
            }, {});
        }
    }

    // Draw OBS_INCLUDE(#) box with rec annotations above it, if there are measurement records.
    if (had_rec) {
        had_rec = false;
        auto span = (uint16_t)(1 + std::max(std::max(op.targets.size(), op.args.size()), (size_t)2));
        auto c = q2xy(pseudo_target.qubit_value());
        std::stringstream ss;
        ss << "OBS_INCLUDE(" << (op.args.empty() ? 0 : op.args[0]) << ")";
        if (!had_paulis) {
            draw_annotated_gate(c.xyz[0], c.xyz[1], SvgGateData{span, ss.str(), "", "", "lightgray", "black", 0, 10}, {});
        }
        c.xyz[0] += (span - 1) * GATE_PITCH * 0.5f;

        svg_out << "<text";
        write_key_val(svg_out, "text-anchor", "middle");
        write_key_val(svg_out, "font-family", "monospace");
        write_key_val(svg_out, "font-size", 8);
        write_key_val(svg_out, "x", c.xyz[0]);
        write_key_val(svg_out, "y", c.xyz[1] - GATE_RADIUS - 4);
        svg_out << ">";
        svg_out << "L" << op.args[0] << " *= ";
        ss << "*=";
        for (const auto &t : rec_targets) {
            if (t.is_measurement_record_target()) {
                if (had_rec) {
                    svg_out << "*";
                }
                had_rec = true;
                write_rec_index(svg_out, t.value());
            }
        }
        if (!had_rec && !had_paulis) {
            svg_out << "1 (vacuous)";
        }
        svg_out << "</text>\n";
    }
}

void DiagramTimelineSvgDrawer::do_resolved_operation(const ResolvedTimelineOperation &op) {
    if (resolver.num_ticks_seen < min_tick || resolver.num_ticks_seen > max_tick) {
        return;
    }
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

DiagramTimelineSvgDrawer::DiagramTimelineSvgDrawer(std::ostream &svg_out, size_t num_qubits, bool has_ticks)
    : svg_out(svg_out), num_qubits(num_qubits), has_ticks(has_ticks), gate_data_map(SvgGateData::make_gate_data_map()) {
    cur_moment_used_flags.resize(num_qubits);
}

void DiagramTimelineSvgDrawer::make_diagram_write_to(
    const Circuit &circuit,
    std::ostream &svg_out,
    uint64_t tick_slice_start,
    uint64_t tick_slice_num,
    DiagramTimelineSvgDrawerMode mode,
    SpanRef<const CoordFilter> filter,
    size_t num_rows) {
    uint64_t circuit_num_ticks = circuit.count_ticks();
    auto circuit_has_ticks = circuit_num_ticks > 0;
    auto num_qubits = circuit.count_qubits();
    std::stringstream buffer;
    DiagramTimelineSvgDrawer obj(buffer, num_qubits, circuit_has_ticks);
    tick_slice_num = std::min(tick_slice_num, circuit_num_ticks - tick_slice_start + 1);
    if (!circuit.operations.empty() && circuit.operations.back().gate_type == GateType::TICK) {
        tick_slice_num = std::min(tick_slice_num, circuit_num_ticks - tick_slice_start);
    }
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
        // The +1 is because we're showing the detector slice at the end of each tick region.
        obj.detector_slice_set =
            DetectorSliceSet::from_circuit_ticks(circuit, tick_slice_start + 1, tick_slice_num, filter);
        obj.coord_sys = FlattenedCoords::from(obj.detector_slice_set, GATE_PITCH);
        obj.coord_sys.size.xyz[0] += TIME_SLICE_PADDING * 2;
        obj.coord_sys.size.xyz[1] += TIME_SLICE_PADDING * 2;
        if (num_rows == 0) {
            obj.num_cols = (uint64_t)ceil(sqrt((double)tick_slice_num));
            obj.num_rows = tick_slice_num / obj.num_cols;
        } else {
            obj.num_rows = num_rows;
            obj.num_cols = (tick_slice_num + num_rows - 1) / num_rows;
        }
        while (obj.num_cols * obj.num_rows < tick_slice_num) {
            obj.num_rows++;
        }
        while (obj.num_cols * obj.num_rows >= tick_slice_num + obj.num_rows) {
            obj.num_cols--;
        }
    }
    obj.min_tick = tick_slice_start;
    obj.max_tick = tick_slice_start + tick_slice_num - 1;
    obj.mode = mode;
    obj.resolver.unroll_loops = mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE;
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

    auto w = obj.m2x(obj.cur_moment) - GATE_PITCH * 0.5f;
    svg_out << R"SVG(<svg viewBox="0 0 )SVG";
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
        auto sw = obj.coord_sys.size.xyz[0] * ((obj.num_cols - 1) * SLICE_WINDOW_GAP + 1);
        auto sh = obj.coord_sys.size.xyz[1] * ((obj.num_rows - 1) * SLICE_WINDOW_GAP + 1);
        svg_out << sw << " " << sh;
    } else {
        svg_out << w + PADDING;
        svg_out << " ";
        svg_out << obj.q2y(obj.num_qubits) + PADDING;
    }
    svg_out << '"' << ' ';
    write_key_val(svg_out, "version", "1.1");
    write_key_val(svg_out, "xmlns", "http://www.w3.org/2000/svg");
    svg_out << ">\n";

    if (mode == DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE) {
        obj.detector_slice_set.write_svg_contents_to(
            svg_out,
            [&](uint32_t qubit) {
                return obj.coord_sys.unscaled_qubit_coords[qubit];
            },
            [&](uint64_t tick, uint32_t qubit) {
                return obj.qt2xy(tick - 1, 0, qubit);
            },
            obj.max_tick + 2,
            24);
    }

    // Make sure qubit lines/points are drawn first, so they are in the background.
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE) {
        // Draw qubit points.
        auto tick = tick_slice_start;
        svg_out << "<g id=\"qubit_dots\">\n";
        for (uint64_t col = 0; col < obj.num_cols; col++) {
            for (uint64_t row = 0; row < obj.num_rows && row * obj.num_cols + col < tick_slice_num; row++) {
                for (auto q : obj.detector_slice_set.used_qubits()) {
                    std::stringstream id_ss;
                    id_ss << "qubit_dot";
                    id_ss << ":" << q;
                    add_coord_summary_to_ss(
                        id_ss,
                        obj.detector_slice_set.coordinates.at(q));  // the raw qubit coordinates, not projected to 2D
                    id_ss << ":" << tick;                           // the absolute tick

                    auto c = obj.coord_sys.qubit_coords[q];  // the flattened coordinates in 2D

                    svg_out << "<circle";
                    write_key_val(svg_out, "id", id_ss.str());
                    write_key_val(
                        svg_out,
                        "cx",
                        TIME_SLICE_PADDING + c.xyz[0] + obj.coord_sys.size.xyz[0] * SLICE_WINDOW_GAP * col);
                    write_key_val(
                        svg_out,
                        "cy",
                        TIME_SLICE_PADDING + c.xyz[1] + obj.coord_sys.size.xyz[1] * SLICE_WINDOW_GAP * row);
                    write_key_val(svg_out, "r", 2);
                    write_key_val(svg_out, "stroke", "none");
                    write_key_val(svg_out, "fill", "black");
                    svg_out << "/>\n";
                }
            }
            tick++;
        }
        svg_out << "</g>\n";
    } else {
        svg_out << "<g id=\"qubit_lines\">\n";
        // Draw qubit lines.
        for (size_t q = 0; q < obj.num_qubits; q++) {
            std::stringstream id_ss;
            id_ss << "qubit_line";
            id_ss << ":" << q;

            auto x1 = PADDING + CIRCUIT_START_X;
            auto x2 = w;
            auto y = obj.q2y(q);

            svg_out << "<path";
            write_key_val(svg_out, "id", id_ss.str());
            svg_out << " d=\"";
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
        svg_out << "</g>\n";
    }

    svg_out << buffer.str();

    // Border around different slices.
    if (mode != DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE && tick_slice_num > 1) {
        auto k = 0;
        svg_out << "<g id=\"tick_borders\">\n";
        for (uint64_t row = 0; row < obj.num_rows; row++) {
            for (uint64_t col = 0; col < obj.num_cols && row * obj.num_cols + col < tick_slice_num; col++) {
                auto sw = obj.coord_sys.size.xyz[0];
                auto sh = obj.coord_sys.size.xyz[1];

                std::stringstream id_ss;
                auto tick = k + tick_slice_start;  // the absolute tick
                id_ss << "tick_border:" << k;
                id_ss << ":" << row << "_" << col;
                id_ss << ":" << tick;

                svg_out << "<text";
                write_key_val(svg_out, "dominant-baseline", "hanging");
                write_key_val(svg_out, "text-anchor", "middle");
                write_key_val(svg_out, "font-family", "serif");
                write_key_val(svg_out, "font-size", 18);
                write_key_val(svg_out, "transform", "rotate(90)");
                write_key_val(svg_out, "x", sh * row * SLICE_WINDOW_GAP + sh / 2);
                write_key_val(svg_out, "y", -(sw * col * SLICE_WINDOW_GAP + sw - 6));
                svg_out << ">";
                svg_out << "Tick " << tick;
                svg_out << "</text>\n";

                svg_out << "<rect";
                write_key_val(svg_out, "id", id_ss.str());
                write_key_val(svg_out, "x", sw * col * SLICE_WINDOW_GAP);
                write_key_val(svg_out, "y", sh * row * SLICE_WINDOW_GAP);
                write_key_val(svg_out, "width", sw);
                write_key_val(svg_out, "height", sh);
                write_key_val(svg_out, "stroke", "black");
                write_key_val(svg_out, "fill", "none");
                svg_out << "/>\n";

                k++;
            }
        }
        svg_out << "</g>\n";
    }
    svg_out << "</svg>";
}
