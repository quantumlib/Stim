#include "stim/diagram/detector_slice/detector_slice_set.h"

#include "stim/dem/detector_error_model.h"
#include "stim/diagram/coord.h"
#include "stim/diagram/diagram_util.h"
#include "stim/diagram/timeline/timeline_ascii_drawer.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/util_bot/arg_parse.h"
#include "stim/util_bot/str_util.h"

constexpr float SLICE_WINDOW_GAP = 1.1f;

using namespace stim;
using namespace stim_draw_internal;

template <typename T>
inline void write_key_val(std::ostream &out, const char *key, const T &val) {
    out << ' ' << key << "=\"" << val << "\"";
}

struct DetectorSliceSetComputer {
    SparseUnsignedRevFrameTracker tracker;
    uint64_t tick_cur;
    uint64_t first_yield_tick;
    uint64_t num_yield_ticks;
    std::set<uint32_t> used_qubits;
    std::function<void(void)> on_tick_callback;
    DetectorSliceSetComputer(const Circuit &circuit, uint64_t first_yield_tick, uint64_t num_yield_ticks);
    bool process_block_rev(const Circuit &block);
    bool process_op_rev(const Circuit &parent, const CircuitInstruction &op);
    bool process_tick();
};

bool DetectorSliceSetComputer::process_block_rev(const Circuit &block) {
    for (size_t k = block.operations.size(); k--;) {
        if (process_op_rev(block, block.operations[k])) {
            return true;
        }
    }
    return false;
}

bool DetectorSliceSetComputer::process_tick() {
    if (tick_cur >= first_yield_tick && tick_cur < first_yield_tick + num_yield_ticks) {
        on_tick_callback();
    }
    tick_cur--;
    return tick_cur < first_yield_tick;
}

bool DetectorSliceSetComputer::process_op_rev(const Circuit &parent, const CircuitInstruction &op) {
    if (op.gate_type == GateType::TICK) {
        return process_tick();
    } else if (op.gate_type == GateType::REPEAT) {
        const auto &loop_body = op.repeat_block_body(parent);
        uint64_t stop_iter = first_yield_tick + num_yield_ticks;
        uint64_t max_skip = std::max(tick_cur, stop_iter) - stop_iter;
        uint64_t reps = op.repeat_block_rep_count();
        uint64_t ticks_per_iteration = loop_body.count_ticks();
        uint64_t skipped_iterations = max_skip == 0              ? 0
                                      : ticks_per_iteration == 0 ? reps
                                                                 : std::min(reps, max_skip / ticks_per_iteration);
        if (skipped_iterations) {
            // We can allow the analyzer to fold parts of the loop we aren't yielding.
            tracker.undo_loop(loop_body, skipped_iterations);
            reps -= skipped_iterations;
            tick_cur -= ticks_per_iteration * skipped_iterations;
        }
        while (reps > 0) {
            if (process_block_rev(loop_body)) {
                return true;
            }
            reps--;
        }
        return false;
    } else {
        for (auto t : op.targets) {
            if (t.has_qubit_value()) {
                used_qubits.insert(t.qubit_value());
            }
        }
        tracker.undo_gate(op);
        return false;
    }
}
DetectorSliceSetComputer::DetectorSliceSetComputer(
    const Circuit &circuit, uint64_t first_yield_tick, uint64_t num_yield_ticks)
    : tracker(circuit.count_qubits(), circuit.count_measurements(), circuit.count_detectors(), false),
      first_yield_tick(first_yield_tick),
      num_yield_ticks(num_yield_ticks) {
    tick_cur = circuit.count_ticks() + 1;  // +1 because of artificial TICKs at start and end.
}

std::string DetectorSliceSet::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}
std::ostream &stim_draw_internal::operator<<(std::ostream &out, const DetectorSliceSet &slice) {
    slice.write_text_diagram_to(out);
    return out;
}
std::ostream &stim_draw_internal::operator<<(std::ostream &out, const CoordFilter &filter) {
    if (filter.use_target) {
        out << filter.exact_target;
    } else {
        out << comma_sep(filter.coordinates);
    }
    return out;
}

void DetectorSliceSet::write_text_diagram_to(std::ostream &out) const {
    DiagramTimelineAsciiDrawer drawer(num_qubits, false);
    drawer.moment_spacing = 2;

    for (const auto &s : anticommutations) {
        drawer.reserve_drawing_room_for_targets(s.second);
        for (const auto &t : s.second) {
            std::stringstream ss;
            ss << "ANTICOMMUTED";
            ss << ":";
            ss << s.first.second;
            drawer.diagram.add_entry(
                AsciiDiagramEntry{
                    AsciiDiagramPos{
                        drawer.m2x(drawer.cur_moment + 1),
                        drawer.q2y(t.qubit_value()),
                        0,
                        0.5,
                    },
                    ss.str(),
                });
        }
    }

    for (const auto &s : slices) {
        drawer.reserve_drawing_room_for_targets(s.second);
        for (const auto &t : s.second) {
            std::stringstream ss;
            if (t.is_x_target()) {
                ss << "X";
            } else if (t.is_y_target()) {
                ss << "Y";
            } else if (t.is_z_target()) {
                ss << "Z";
            } else {
                ss << "?";
            }
            ss << ":";
            ss << s.first.second;
            drawer.diagram.add_entry(
                AsciiDiagramEntry{
                    AsciiDiagramPos{
                        drawer.m2x(drawer.cur_moment),
                        drawer.q2y(t.qubit_value()),
                        0,
                        0.5,
                    },
                    ss.str(),
                });
        }
    }

    // Make sure qubit lines are drawn first, so they are in the background.
    drawer.diagram.lines.insert(drawer.diagram.lines.begin(), drawer.num_qubits, {{0, 0, 0.0, 0.5}, {0, 0, 1.0, 0.5}});
    for (size_t q = 0; q < drawer.num_qubits; q++) {
        drawer.diagram.lines[q] = {
            {0, drawer.q2y(q), 1.0, 0.5},
            {drawer.m2x(drawer.cur_moment) + 1, drawer.q2y(q), 1.0, 0.5},
        };
        std::stringstream ss;
        ss << "q";
        ss << q;
        ss << ":";
        auto p = coordinates.find(q);
        if (p != coordinates.end() && !p->second.empty()) {
            ss << "(" << comma_sep(p->second) << ")";
        }
        ss << " ";
        drawer.diagram.add_entry(
            AsciiDiagramEntry{
                {0, drawer.q2y(q), 1.0, 0.5},
                ss.str(),
            });
    }

    drawer.diagram.render(out);
}

std::set<uint64_t> DetectorSliceSet::used_qubits() const {
    std::set<uint64_t> result;
    for (const auto &e : coordinates) {
        result.insert(e.first);
    }
    for (const auto &e : slices) {
        for (const auto &t : e.second) {
            result.insert(t.qubit_value());
        }
    }
    return result;
}

bool CoordFilter::matches(stim::SpanRef<const double> coords, stim::DemTarget target) const {
    if (use_target) {
        return target == exact_target;
    }
    if (!target.is_relative_detector_id()) {
        return false;
    }
    for (size_t k = 0; k < coordinates.size(); k++) {
        if (!std::isnan(coordinates[k])) {
            if (coords.size() <= k || coords[k] != coordinates[k]) {
                return false;
            }
        }
    }
    return true;
}
CoordFilter CoordFilter::parse_from(std::string_view data) {
    CoordFilter filter;
    if (data.empty()) {
        // no filter
    } else if (data[0] == 'D') {
        filter.use_target = true;
        filter.exact_target = DemTarget::relative_detector_id(parse_exact_uint64_t_from_string(data.substr(1)));
    } else if (data[0] == 'L') {
        filter.use_target = true;
        filter.exact_target = DemTarget::observable_id(parse_exact_uint64_t_from_string(data.substr(1)));
    } else {
        for (const auto &v : split_view(',', data)) {
            if (v == "*") {
                filter.coordinates.push_back(std::numeric_limits<double>::quiet_NaN());
            } else {
                filter.coordinates.push_back(parse_exact_double_from_string(v));
            }
        }
    }
    return filter;
}

DetectorSliceSet DetectorSliceSet::from_circuit_ticks(
    const stim::Circuit &circuit, uint64_t start_tick, uint64_t num_ticks, SpanRef<const CoordFilter> coord_filter) {
    num_ticks = std::max(uint64_t{1}, std::min(num_ticks, circuit.count_ticks() - start_tick + 1));

    DetectorSliceSetComputer helper(circuit, start_tick, num_ticks);
    size_t num_qubits = helper.tracker.xs.size();

    std::set<DemTarget> xs;
    std::set<DemTarget> ys;
    std::set<DemTarget> zs;
    DetectorSliceSet result;
    result.num_qubits = num_qubits;
    result.min_tick = start_tick;
    result.num_ticks = num_ticks;

    helper.on_tick_callback = [&]() {
        // Process anticommutations.
        for (const auto &[d, g] : helper.tracker.anticommutations) {
            result.anticommutations[{helper.tick_cur, d}].push_back(g);

            // Stop propagating it backwards if it broke.
            for (size_t q = 0; q < num_qubits; q++) {
                if (helper.tracker.xs[q].contains(d)) {
                    helper.tracker.xs[q].xor_item(d);
                }
                if (helper.tracker.zs[q].contains(d)) {
                    helper.tracker.zs[q].xor_item(d);
                }
            }
        }
        helper.tracker.anticommutations.clear();

        // Record locations of detectors and observables.
        for (size_t q = 0; q < num_qubits; q++) {
            xs.clear();
            ys.clear();
            zs.clear();
            for (auto t : helper.tracker.xs[q]) {
                xs.insert(t);
            }
            for (auto t : helper.tracker.zs[q]) {
                if (xs.find(t) == xs.end()) {
                    zs.insert(t);
                } else {
                    xs.erase(t);
                    ys.insert(t);
                }
            }

            for (const auto &t : xs) {
                result.slices[{helper.tick_cur, t}].push_back(GateTarget::x(q));
            }
            for (const auto &t : ys) {
                result.slices[{helper.tick_cur, t}].push_back(GateTarget::y(q));
            }
            for (const auto &t : zs) {
                result.slices[{helper.tick_cur, t}].push_back(GateTarget::z(q));
            }
        }
    };

    if (!helper.process_tick()) {
        helper.process_block_rev(circuit);
    }

    std::set<uint64_t> included_detectors;
    for (const auto &t : result.slices) {
        if (t.first.second.is_relative_detector_id()) {
            included_detectors.insert(t.first.second.data);
        }
    }
    result.detector_coordinates = circuit.get_detector_coordinates(included_detectors);

    result.coordinates = circuit.get_final_qubit_coords();
    for (const auto &q : helper.used_qubits) {
        result.coordinates[q];  // Default construct if doesn't exist.
    }

    auto keep = [&](DemTarget t) {
        SpanRef<const double> coords{};
        if (t.is_relative_detector_id()) {
            auto coords_ptr = result.detector_coordinates.find(t.data);
            if (coords_ptr != result.detector_coordinates.end()) {
                coords = coords_ptr->second;
            }
        }
        for (const auto &filter : coord_filter) {
            if (filter.matches(coords, t)) {
                return true;
            }
        }
        return false;
    };
    std::vector<std::pair<uint64_t, DemTarget>> removed;
    for (const auto &t : result.slices) {
        if (!keep(t.first.second)) {
            removed.push_back(t.first);
        }
    }
    for (auto t : removed) {
        result.slices.erase(t);
        if (t.second.is_relative_detector_id()) {
            result.detector_coordinates.erase(t.second.raw_id());
        }
    }

    return result;
}

Coord<2> flattened_2d(SpanRef<const double> c) {
    float x = 0;
    float y = 0;
    if (c.size() >= 1) {
        x = (float)c[0];
    }
    if (c.size() >= 2) {
        y = (float)c[1];
    }

    // Arbitrary orthographic projection.
    for (size_t k = 2; k < c.size(); k++) {
        x += (float)c[k] / k;
        y += (float)c[k] / (k * k);
    }

    return {x, y};
}

float pick_characteristic_distance(const std::set<uint64_t> &used, const std::vector<Coord<2>> &coords_2d) {
    if (used.size() == 0) {
        return 1;
    }

    Coord<2> biggest{-INFINITY, -INFINITY};
    for (auto q : used) {
        biggest = std::max(biggest, coords_2d[q]);
    }

    float closest_squared_distance = INFINITY;
    for (auto pt : coords_2d) {
        if (biggest == pt) {
            continue;
        }
        auto delta = biggest - pt;
        auto d = delta.xyz[0] * delta.xyz[0] + delta.xyz[1] * delta.xyz[1];
        if (d < closest_squared_distance) {
            closest_squared_distance = d;
        }
    }

    float result = sqrtf(closest_squared_distance);
    if (result == INFINITY) {
        result = 1;
    }
    return result;
}

FlattenedCoords FlattenedCoords::from(const DetectorSliceSet &set, float desired_unit_distance) {
    auto used = set.used_qubits();
    FlattenedCoords result;

    for (uint64_t q = 0; q < set.num_qubits; q++) {
        Coord<2> c{(float)q, 0};
        auto p = set.coordinates.find(q);
        if (p != set.coordinates.end() && !p->second.empty()) {
            c = flattened_2d(p->second);
        }
        result.qubit_coords.push_back(c);
    }
    result.unscaled_qubit_coords = result.qubit_coords;

    for (const auto &e : set.detector_coordinates) {
        result.det_coords.insert({e.first, flattened_2d(e.second)});
    }

    float characteristic_distance = pick_characteristic_distance(used, result.qubit_coords);
    result.unit_distance = desired_unit_distance;
    float scale = desired_unit_distance / characteristic_distance;
    for (auto &c : result.qubit_coords) {
        c *= scale;
    }
    for (auto &e : result.det_coords) {
        e.second *= scale;
    }
    if (!used.empty()) {
        std::vector<Coord<2>> used_coords;
        for (const auto &u : used) {
            used_coords.push_back(result.qubit_coords[u]);
        }
        auto minmax = Coord<2>::min_max(used_coords);
        auto offset = minmax.first;
        offset *= -1;
        offset.xyz[0] += 16;
        offset.xyz[1] += 16;
        for (auto &c : result.qubit_coords) {
            c += offset;
        }
        for (auto &c : used_coords) {
            c += offset;
        }
        for (auto &e : result.det_coords) {
            e.second += offset;
        }
        result.size = minmax.second - minmax.first;
        result.size.xyz[0] += 32;
        result.size.xyz[1] += 32;
    } else {
        result.size.xyz[0] = 1;
        result.size.xyz[1] = 1;
    }

    return result;
}

const char *pick_color(SpanRef<const GateTarget> terms) {
    bool has_x = false;
    bool has_y = false;
    bool has_z = false;
    for (const auto &term : terms) {
        has_x |= term.is_x_target();
        has_y |= term.is_y_target();
        has_z |= term.is_z_target();
    }
    if (has_x + has_y + has_z != 1) {
        return nullptr;
    } else if (has_x) {
        return X_RED;
    } else if (has_y) {
        return Y_GREEN;
    } else {
        assert(has_z);
        return Z_BLUE;
    }
}

float offset_angle_from_to(Coord<2> origin, Coord<2> dst) {
    auto d = dst - origin;
    if (d.xyz[0] * d.xyz[0] + d.xyz[1] * d.xyz[1] < 1e-6) {
        return 0.0f;
    }
    float offset_angle = atan2f(d.xyz[1], d.xyz[0]);
    offset_angle += 2.0f * 3.14159265359f;
    offset_angle = fmodf(offset_angle, 2.0f * 3.14159265359f);
    // The -0.01f is to move the wraparound float error away from the common angle PI.
    if (offset_angle > 3.14159265359f - 0.01f) {
        offset_angle -= 2.0f * 3.14159265359f;
    }
    return offset_angle;
}

float _mirror_score(SpanRef<const Coord<2>> coords, size_t i, size_t j) {
    auto para = coords[j] - coords[i];
    float f = para.norm2();
    if (f < 1e-4) {
        return INFINITY;
    }
    para /= sqrtf(f);

    Coord<2> perp = {-para.xyz[0], para.xyz[1]};
    std::vector<Coord<2>> left;
    std::vector<Coord<2>> right;
    for (size_t k = 0; k < coords.size(); k++) {
        if (k == i || k == j) {
            continue;
        }
        auto d = coords[k] - coords[i];
        float x = d.dot(para);
        float y = d.dot(perp);
        if (y < 0) {
            right.push_back({x, -y});
        } else {
            left.push_back({x, y});
        }
    }
    if (left.size() != right.size()) {
        return INFINITY;
    }
    std::stable_sort(left.begin(), left.end());
    std::stable_sort(right.begin(), right.end());
    for (size_t k = 0; k < left.size(); k++) {
        if ((left[k] - right[k]).norm2() > 1e-2) {
            return INFINITY;
        }
    }

    float max_distance = 0;
    for (size_t k = 0; k < left.size(); k++) {
        max_distance = std::max(max_distance, left[k].xyz[1]);
    }

    return max_distance;
}

bool _pick_center_using_mirror_symmetry(SpanRef<const Coord<2>> coords, Coord<2> &out) {
    float best_score = INFINITY;
    for (size_t i = 0; i < coords.size(); i++) {
        for (size_t j = i + 1; j < coords.size(); j++) {
            float f = _mirror_score(coords, i, j);
            if (f < best_score) {
                out = (coords[i] + coords[j]) / 2;
                best_score = f;
            }
        }
    }
    return best_score < INFINITY;
}

Coord<2> stim_draw_internal::pick_polygon_center(SpanRef<const Coord<2>> coords) {
    Coord<2> center{0, 0};
    if (_pick_center_using_mirror_symmetry(coords, center)) {
        return center;
    }

    for (const auto &coord : coords) {
        center += coord;
    }
    center /= coords.size();
    return center;
}

bool stim_draw_internal::is_colinear(Coord<2> a, Coord<2> b, Coord<2> c, float atol) {
    for (size_t k = 0; k < 3; k++) {
        auto d1 = a - b;
        auto d2 = b - c;
        if (d1.norm() < atol || d2.norm() < atol) {
            return true;
        }
        d1 /= d1.norm();
        d2 /= d2.norm();
        if (fabs(d1.dot({d2.xyz[1], -d2.xyz[0]})) < atol) {
            return true;
        }
        std::swap(a, b);
        std::swap(b, c);
    }
    return false;
}

double stim_draw_internal::inv_space_fill_transform(Coord<2> a) {
    double dx = ldexp((double)a.xyz[0], 4);
    double dy = ldexp((double)a.xyz[1], 4);
    uint64_t x = (uint64_t)std::min((double)(1ULL << 31), std::max(dx, 0.0));
    uint64_t y = (uint64_t)std::min((double)(1ULL << 31), std::max(dy, 0.0));

    for (size_t k = 64; k-- > 0;) {
        uint64_t b = 1ULL << k;
        uint64_t m = b - 1;
        if ((x ^ y) & b) {
            x ^= m;
        }
        if (!(y & b)) {
            x ^= y & m;
            y ^= x & m;
            x ^= y & m;
        }
    }
    y ^= x;

    uint64_t gray = 0;
    for (size_t k = 64; k--;) {
        uint64_t b = 1ULL << k;
        if (y & b) {
            gray ^= b - 1;
        }
    }
    x ^= gray;
    y ^= gray;

    uint64_t interleave = 0;
    for (size_t k = 32; k--;) {
        interleave |= ((x >> k) & 1ULL) << (2 * k + 1);
        interleave |= ((y >> k) & 1ULL) << (2 * k);
    }

    return interleave;
}

void _draw_observable(
    std::ostream &out,
    uint64_t index,
    const std::function<Coord<2>(uint32_t qubit)> &unscaled_coords,
    const std::function<Coord<2>(uint64_t tick, uint32_t qubit)> &coords,
    uint64_t tick,
    SpanRef<const GateTarget> terms,
    std::vector<Coord<2>> &pts_workspace,
    bool next_tick_exists,
    size_t scale) {
    std::vector<GateTarget> terms_copy;
    terms_copy.insert(terms_copy.end(), terms.begin(), terms.end());
    std::stable_sort(terms_copy.begin(), terms_copy.end(), [&](GateTarget a, GateTarget b) {
        auto a2 = inv_space_fill_transform(unscaled_coords(a.qubit_value()));
        auto b2 = inv_space_fill_transform(unscaled_coords(b.qubit_value()));
        if (a2 != b2) {
            return a2 < b2;
        }
        a2 = inv_space_fill_transform(coords(tick, a.qubit_value()));
        b2 = inv_space_fill_transform(coords(tick, b.qubit_value()));
        return a2 < b2;
    });

    pts_workspace.clear();
    for (const auto &term : terms_copy) {
        pts_workspace.push_back(coords(tick, term.qubit_value()));
    }

    // TODO: CURRENTLY DISABLED BECAUSE IT'S UNCLEAR IF IT HELPS OR HURTS.
    //    // Draw a semi-janky path connecting the observable together.
    //    out << "<path d=\"";
    //    out << "M" << pts_workspace[0].xyz[0] << "," << pts_workspace[0].xyz[1];
    //    size_t n = pts_workspace.size();
    //    for (size_t k = 1; k < n; k++) {
    //        const auto &p = pts_workspace[(k - 1) % n];
    //        const auto &a = pts_workspace[k];
    //
    //        auto dif = a - p;
    //        auto average = (a + p) * 0.5;
    //        if (dif.norm() > 50 * scale) {
    //            dif /= dif.norm() / 50 / scale;
    //        }
    //        Coord<2> perp{-dif.xyz[1], dif.xyz[0]};
    //        auto c1 = average + perp * 0.1 - dif * 0.1;
    //        auto c2 = average + perp * 0.1 + dif * 0.1;
    //
    //        out << "C";
    //        out << c1.xyz[0] << " " << c1.xyz[1] << ", ";
    //        out << c2.xyz[0] << " " << c2.xyz[1] << ", ";
    //        out << a.xyz[0] << " " << a.xyz[1];
    //    }
    //    out << "\" id=\"obs-path:" << index << ":" << tick << "\"";
    //    write_key_val(out, "stroke", BG_GREY);
    //    write_key_val(out, "fill", "none");
    //    write_key_val(out, "stroke-width", scale);
    //    out << "/>\n";

    for (size_t k = 0; k < terms_copy.size(); k++) {
        const auto &t = terms_copy[k];
        out << "<circle";
        out << " id=\"obs-term:" << index << ":" << tick << ":" << k << "\"";
        auto c = coords(tick, t.qubit_value());
        write_key_val(out, "cx", c.xyz[0]);
        write_key_val(out, "cy", c.xyz[1]);
        write_key_val(out, "r", scale * 1.1f);
        write_key_val(out, "stroke", "none");
        write_key_val(out, "fill", t.is_x_target() ? X_RED : t.is_y_target() ? Y_GREEN : Z_BLUE);
        out << "/>\n";
    }
    if (next_tick_exists) {
        for (size_t k = 0; k < terms_copy.size(); k++) {
            const auto &t = terms_copy[k];
            out << "<circle";
            out << " id=\"obs-term-shadow:" << index << ":" << tick << ":" << k << "\"";
            auto c = coords(tick + 1, t.qubit_value());
            write_key_val(out, "cx", c.xyz[0]);
            write_key_val(out, "cy", c.xyz[1]);
            write_key_val(out, "r", scale * 1.1f);
            write_key_val(out, "stroke", t.is_x_target() ? X_RED : t.is_y_target() ? Y_GREEN : Z_BLUE);
            write_key_val(out, "stroke-width", 3);
            write_key_val(out, "fill", "none");
            out << "/>\n";
        }
    }
}

void _start_many_body_svg_path(
    std::ostream &out,
    const std::function<Coord<2>(uint64_t tick, uint32_t qubit)> &coords,
    uint64_t tick,
    SpanRef<const GateTarget> terms,
    std::vector<Coord<2>> &pts_workspace) {
    pts_workspace.clear();
    for (const auto &term : terms) {
        pts_workspace.push_back(coords(tick, term.qubit_value()));
    }
    auto center = pick_polygon_center(pts_workspace);
    std::stable_sort(pts_workspace.begin(), pts_workspace.end(), [&](Coord<2> a, Coord<2> b) {
        return offset_angle_from_to(center, a) < offset_angle_from_to(center, b);
    });

    out << "<path d=\"";
    out << "M" << pts_workspace[0].xyz[0] << "," << pts_workspace[0].xyz[1];
    size_t n = pts_workspace.size();
    for (size_t k = 0; k < n; k++) {
        const auto &p = pts_workspace[(k + n - 1) % n];
        const auto &a = pts_workspace[k];
        const auto &b = pts_workspace[(k + 1) % n];
        const auto &c = pts_workspace[(k + 2) % n];
        if (is_colinear(p, a, b, 3e-2f) || is_colinear(a, b, c, 3e-2f)) {
            out << " C";
            auto d = b - a;
            d = {d.xyz[1], -d.xyz[0]};
            d *= -0.1;
            d += (a + b) / 2;
            out << d.xyz[0] << " " << d.xyz[1] << ",";
            out << d.xyz[0] << " " << d.xyz[1] << ",";
            out << b.xyz[0] << " " << b.xyz[1];
        } else {
            out << " L" << b.xyz[0] << "," << b.xyz[1];
        }
    }
    out << '"';
}

void _start_two_body_svg_path(
    std::ostream &out,
    const std::function<Coord<2>(uint64_t tick, uint32_t qubit)> &coords,
    uint64_t tick,
    SpanRef<const GateTarget> terms) {
    auto a = coords(tick, terms[0].qubit_value());
    auto b = coords(tick, terms[1].qubit_value());
    auto dif = b - a;
    auto average = (a + b) * 0.5;
    if (dif.norm() > 64) {
        dif /= dif.norm() / 64;
    }
    Coord<2> perp{-dif.xyz[1], dif.xyz[0]};
    auto ac1 = average + perp * 0.2f - dif * 0.2f;
    auto ac2 = average + perp * 0.2f + dif * 0.2f;
    auto bc1 = average + perp * -0.2f + dif * 0.2f;
    auto bc2 = average + perp * -0.2f - dif * 0.2f;

    out << "<path d=\"";
    out << "M" << a.xyz[0] << "," << a.xyz[1] << " ";
    out << "C";
    out << ac1.xyz[0] << " " << ac1.xyz[1] << ", ";
    out << ac2.xyz[0] << " " << ac2.xyz[1] << ", ";
    out << b.xyz[0] << " " << b.xyz[1] << " ";
    out << "C";
    out << bc1.xyz[0] << " " << bc1.xyz[1] << ", ";
    out << bc2.xyz[0] << " " << bc2.xyz[1] << ", ";
    out << a.xyz[0] << " " << a.xyz[1];
    out << '"';
}

void _start_one_body_svg_path(
    std::ostream &out,
    const std::function<Coord<2>(uint64_t tick, uint32_t qubit)> &coords,
    uint64_t tick,
    SpanRef<const GateTarget> terms,
    size_t scale) {
    auto c = coords(tick, terms[0].qubit_value());
    out << "<circle";
    write_key_val(out, "cx", c.xyz[0]);
    write_key_val(out, "cy", c.xyz[1]);
    write_key_val(out, "r", scale);
}

void _start_slice_shape_command(
    std::ostream &out,
    const std::function<Coord<2>(uint64_t tick, uint32_t qubit)> &coords,
    uint64_t tick,
    SpanRef<const GateTarget> terms,
    std::vector<Coord<2>> &pts_workspace,
    size_t scale) {
    if (terms.size() > 2) {
        _start_many_body_svg_path(out, coords, tick, terms, pts_workspace);
    } else if (terms.size() == 2) {
        _start_two_body_svg_path(out, coords, tick, terms);
    } else if (terms.size() == 1) {
        _start_one_body_svg_path(out, coords, tick, terms, scale);
    }
}

void DetectorSliceSet::write_svg_diagram_to(std::ostream &out, size_t num_rows) const {
    size_t num_cols;
    if (num_rows == 0) {
        num_cols = (uint64_t)ceil(sqrt((double)num_ticks));
        num_rows = num_ticks / num_cols;
        while (num_cols * num_rows < num_ticks) {
            num_rows++;
        }
        while (num_cols * num_rows >= num_ticks + num_rows) {
            num_cols--;
        }
    } else {
        num_cols = (num_ticks + num_rows - 1) / num_rows;
    }

    auto coordsys = FlattenedCoords::from(*this, 32);
    out << R"SVG(<svg viewBox="0 0 )SVG";
    out << coordsys.size.xyz[0] * ((num_cols - 1) * SLICE_WINDOW_GAP + 1);
    out << " ";
    out << coordsys.size.xyz[1] * ((num_rows - 1) * SLICE_WINDOW_GAP + 1);
    out << R"SVG(" xmlns="http://www.w3.org/2000/svg">)SVG";
    out << "\n";

    auto coords = [&](uint64_t tick, uint32_t qubit) {
        auto result = coordsys.qubit_coords[qubit];
        uint64_t s = tick - min_tick;
        uint64_t sx = s % num_cols;
        uint64_t sy = s / num_cols;
        result.xyz[0] += coordsys.size.xyz[0] * sx * SLICE_WINDOW_GAP;
        result.xyz[1] += coordsys.size.xyz[1] * sy * SLICE_WINDOW_GAP;
        return result;
    };
    auto unscaled_coords = [&](uint32_t qubit) {
        return coordsys.unscaled_qubit_coords[qubit];
    };
    write_svg_contents_to(out, unscaled_coords, coords, min_tick + num_ticks, 6);

    out << "<g id=\"qubit_dots\">\n";
    for (size_t k = 0; k < num_ticks; k++) {
        for (auto q : used_qubits()) {
            auto t = min_tick + k;

            std::stringstream id_ss;
            id_ss << "qubit_dot";
            id_ss << ":" << q;
            add_coord_summary_to_ss(id_ss, coordinates.at(q));  // the raw qubit coordinates, not projected to 2D
            id_ss << ":" << t;                                  // the absolute tick

            auto sc = coords(t, q);  // the svg coordinates, offset to the correct slice plot

            out << "<circle";
            write_key_val(out, "id", id_ss.str());
            write_key_val(out, "cx", sc.xyz[0]);
            write_key_val(out, "cy", sc.xyz[1]);
            write_key_val(out, "r", 2);
            write_key_val(out, "stroke", "none");
            write_key_val(out, "fill", "black");
            out << "/>\n";
        }
    }
    out << "</g>\n";

    // Border around different slices.
    if (num_ticks > 1) {
        size_t k = 0;
        out << "<g id=\"tick_borders\">\n";
        for (uint64_t col = 0; col < num_cols; col++) {
            for (uint64_t row = 0; row < num_rows && row * num_cols + col < num_ticks; row++) {
                auto sw = coordsys.size.xyz[0];
                auto sh = coordsys.size.xyz[1];

                std::stringstream id_ss;
                id_ss << "tick_border:" << k;
                id_ss << ":" << row << "_" << col;
                id_ss << ":" << k + min_tick;

                out << "<rect";
                write_key_val(out, "id", id_ss.str());
                write_key_val(out, "x", sw * col * SLICE_WINDOW_GAP);
                write_key_val(out, "y", sh * row * SLICE_WINDOW_GAP);
                write_key_val(out, "width", sw);
                write_key_val(out, "height", sh);
                write_key_val(out, "stroke", "black");
                write_key_val(out, "fill", "none");
                out << "/>\n";
                k++;
            }
        }
        out << "</g>\n";
    }

    out << R"SVG(</svg>)SVG";
}

void DetectorSliceSet::write_svg_contents_to(
    std::ostream &out,
    const std::function<Coord<2>(uint32_t qubit)> &unscaled_coords,
    const std::function<Coord<2>(uint64_t tick, uint32_t qubit)> &coords,
    uint64_t end_tick,
    size_t scale) const {
    size_t clip_id = 0;

    std::vector<Coord<2>> pts_workspace;

    bool haveDrawnCorners = false;

    using tup = std::tuple<uint64_t, stim::DemTarget, SpanRef<const GateTarget>, bool>;
    std::vector<tup> sorted_terms;
    for (const auto &e : slices) {
        sorted_terms.push_back({e.first.first, e.first.second, e.second, false});
    }
    for (const auto &e : anticommutations) {
        sorted_terms.push_back({e.first.first, e.first.second, e.second, true});
    }
    std::stable_sort(sorted_terms.begin(), sorted_terms.end(), [](const tup &e1, const tup &e2) -> int {
        int a = (int)std::get<2>(e1).size();
        int b = (int)std::get<2>(e2).size();
        return a > b;
    });

    // Draw detector slices.
    for (const auto &e : sorted_terms) {
        uint64_t tick = std::get<0>(e);
        DemTarget target = std::get<1>(e);
        SpanRef<const GateTarget> terms = std::get<2>(e);
        bool is_anticommutation = std::get<3>(e);
        if (is_anticommutation) {
            for (const auto &anti_target : terms) {
                auto c = coords(tick + 1, anti_target.qubit_value());
                out << R"SVG(<circle)SVG";
                write_key_val(out, "cx", c.xyz[0]);
                write_key_val(out, "cy", c.xyz[1]);
                write_key_val(out, "r", scale);
                write_key_val(out, "fill", "none");
                write_key_val(out, "stroke", "magenta");
                write_key_val(out, "stroke-width", scale / 2);
                out << "/>\n";
            }
            continue;
        }

        if (target.is_observable_id()) {
            _draw_observable(
                out, target.val(), unscaled_coords, coords, tick, terms, pts_workspace, tick < end_tick - 1, scale);
            continue;
        }

        const char *color = pick_color(terms);
        bool drawCorners = false;
        if (color == nullptr) {
            drawCorners = true;
            color = BG_GREY;
        }

        // Open the group element for this slice
        out << "<g id=\"slice:" << target.val();
        if (target.is_relative_detector_id()) {
            add_coord_summary_to_ss(out, detector_coordinates.at(target.val()));
        }
        out << ":" << tick << "\">\n";

        _start_slice_shape_command(out, coords, tick, terms, pts_workspace, scale);
        write_key_val(out, "stroke", "none");
        if (terms.size() > 2) {
            write_key_val(out, "fill-opacity", 0.75);
        }
        write_key_val(out, "fill", color);
        out << "/>\n";

        if (drawCorners) {
            haveDrawnCorners = true;  // controls later writing out the universal gradients we'll reference here
            out << R"SVG(<clipPath id="clip)SVG";
            out << clip_id;
            out << "\">";
            _start_slice_shape_command(out, coords, tick, terms, pts_workspace, scale);
            out << "/></clipPath>\n";

            size_t blur_radius = scale == 6 ? 20 : scale * 1.8f;
            for (const auto &t : terms) {
                auto c = coords(tick, t.qubit_value());
                out << R"SVG(<circle clip-path="url(#clip)SVG";
                out << clip_id;
                out << ")\"";
                write_key_val(out, "cx", c.xyz[0]);
                write_key_val(out, "cy", c.xyz[1]);
                write_key_val(out, "r", blur_radius);
                write_key_val(out, "stroke", "none");
                if (t.is_x_target()) {
                    write_key_val(out, "fill", "url('#xgrad')");
                } else if (t.is_y_target()) {
                    write_key_val(out, "fill", "url('#ygrad')");
                } else {
                    write_key_val(out, "fill", "url('#zgrad')");
                }
                out << "/>\n";
            }

            clip_id++;
        }

        // Draw outline
        _start_slice_shape_command(out, coords, tick, terms, pts_workspace, scale);
        write_key_val(out, "stroke", "black");
        write_key_val(out, "fill", "none");
        out << "/>\n";

        // Close the group element for this slice
        out << "</g>\n";
    }
    if (haveDrawnCorners) {
        // write out the universal radialGradients that all corners reference
        out << "<defs>\n";
        static const char *const names[] = {"xgrad", "ygrad", "zgrad"};
        static const char *const colors[] = {X_RED, Y_GREEN, Z_BLUE};
        for (int i = 0; i < 3; ++i) {
            out << "<radialGradient";
            write_key_val(out, "id", names[i]);
            out << "><stop";
            write_key_val(out, "offset", "50%");
            write_key_val(out, "stop-color", colors[i]);
            write_key_val(out, "stop-opacity", "1");
            out << "/><stop";
            write_key_val(out, "offset", "100%");
            write_key_val(out, "stop-color", "#AAAAAA");
            write_key_val(out, "stop-opacity", "0");
            out << "/></radialGradient>\n";
        }
        out << "</defs>\n";
    }
}
