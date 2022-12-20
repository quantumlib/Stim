#include "stim/diagram/detector_slice/detector_slice_set.h"

#include "stim/diagram/coord.h"
#include "stim/diagram/diagram_util.h"
#include "stim/diagram/timeline/timeline_ascii_drawer.h"
#include "stim/simulators/error_analyzer.h"
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
    DetectorSliceSetComputer(
        const Circuit &circuit,
        uint64_t first_yield_tick,
        uint64_t num_yield_ticks);
    bool process_block_rev(const Circuit &block);
    bool process_op_rev(const Circuit &parent, const Operation &op);
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

bool DetectorSliceSetComputer::process_op_rev(const Circuit &parent, const Operation &op) {
    if (op.gate->id == gate_name_to_id("TICK")) {
        return process_tick();
    } else if (op.gate->id == gate_name_to_id("REPEAT")) {
        const auto &loop_body = op_data_block_body(parent, op.target_data);
        uint64_t stop_iter = first_yield_tick + num_yield_ticks;
        uint64_t max_skip = std::max(tick_cur, stop_iter) - stop_iter;
        uint64_t reps = op_data_rep_count(op.target_data);
        uint64_t ticks_per_iteration = loop_body.count_ticks();
        uint64_t skipped_iterations = std::min(reps, max_skip / ticks_per_iteration);
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
        for (auto t : op.target_data.targets) {
            if (t.has_qubit_value()) {
                used_qubits.insert(t.qubit_value());
            }
        }
        (tracker.*(op.gate->sparse_unsigned_rev_frame_tracker_function))(op.target_data);
        return false;
    }
}
DetectorSliceSetComputer::DetectorSliceSetComputer(
    const Circuit &circuit,
    uint64_t first_yield_tick,
    uint64_t num_yield_ticks)
    : tracker(circuit.count_qubits(), circuit.count_measurements(), circuit.count_detectors()),
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

void DetectorSliceSet::write_text_diagram_to(std::ostream &out) const {
    DiagramTimelineAsciiDrawer drawer(num_qubits, false);
    drawer.moment_spacing = 2;

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
            drawer.diagram.add_entry(AsciiDiagramEntry{
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
        drawer.diagram.add_entry(AsciiDiagramEntry{
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

DetectorSliceSet DetectorSliceSet::from_circuit_ticks(
    const stim::Circuit &circuit,
    uint64_t start_tick,
    uint64_t num_ticks,
    ConstPointerRange<std::vector<double>> coord_filter) {
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
    result.coordinates = circuit.get_final_qubit_coords();
    for (const auto &q : helper.used_qubits) {
        result.coordinates[q];  // Default construct if doesn't exist.
    }
    result.detector_coordinates = circuit.get_detector_coordinates(included_detectors);

    auto matches_filter = [](ConstPointerRange<double> coord, ConstPointerRange<double> filter) {
        if (coord.size() < filter.size()) {
            return false;
        }
        for (size_t k = 0; k < filter.size(); k++) {
            if (filter[k] != coord[k]) {
                return false;
            }
        }
        return true;
    };
    auto keep = [&](DemTarget t) {
        if (!t.is_relative_detector_id()) {
            return true;
        }
        auto coords_ptr = result.detector_coordinates.find(t.data);
        ConstPointerRange<double> coords;
        if (coords_ptr == result.detector_coordinates.end()) {
            coords = {};
        } else {
            coords = coords_ptr->second;
        }
        for (const auto &filter : coord_filter) {
            if (matches_filter(coords, filter)) {
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
        result.detector_coordinates.erase(t.second.raw_id());
    }

    return result;
}

Coord<2> flattened_2d(ConstPointerRange<double> c) {
    float x = 0;
    float y = 0;
    if (c.size() >= 1) {
        x = c[0];
    }
    if (c.size() >= 2) {
        y = c[1];
    }

    // Arbitrary orthographic projection.
    for (size_t k = 2; k < c.size(); k++) {
        x += c[k] / k;
        y += c[k] / (k * k);
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

    for (const auto &e : set.detector_coordinates) {
        result.det_coords.insert({e.first, flattened_2d(e.second)});
    }

    float characteristic_distance = pick_characteristic_distance(used, result.qubit_coords);
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

const char *pick_color(ConstPointerRange<GateTarget> terms) {
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

float angle_from_to(Coord<2> origin, Coord<2> dst) {
    auto d = dst - origin;
    if (d.xyz[0] * d.xyz[0] + d.xyz[1] * d.xyz[1] < 1e-6) {
        return 0;
    }
    return atan2f(d.xyz[1], d.xyz[0]);
}

float _mirror_score(ConstPointerRange<Coord<2>> coords, size_t i, size_t j) {
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
    std::sort(left.begin(), left.end());
    std::sort(right.begin(), right.end());
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

bool _pick_center_using_mirror_symmetry(ConstPointerRange<Coord<2>> coords, Coord<2> &out) {
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

Coord<2> stim_draw_internal::pick_polygon_center(ConstPointerRange<Coord<2>> coords) {
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

bool stim_draw_internal::is_colinear(Coord<2> a, Coord<2> b, Coord<2> c) {
    auto d1 = a - b;
    auto d2 = b - c;
    if (d1.norm2() < 1e-4 || d2.norm2() < 1e-4) {
        return true;
    }
    d1 /= d1.norm();
    d2 /= d2.norm();
    return fabs(d1.dot({d2.xyz[1], -d2.xyz[0]})) < 1e-4;
}

void _start_many_body_svg_path(
        std::ostream &out,
        const std::function<Coord<2>(uint64_t tick, uint32_t qubit)> &coords,
        uint64_t tick,
        ConstPointerRange<GateTarget> terms,
        std::vector<Coord<2>> &pts_workspace) {
    pts_workspace.clear();
    for (const auto &term : terms) {
        pts_workspace.push_back(coords(tick, term.qubit_value()));
    }
    auto center = pick_polygon_center(pts_workspace);
    std::sort(pts_workspace.begin(), pts_workspace.end(), [&](Coord<2> a, Coord<2> b) {
        return angle_from_to(center, a) < angle_from_to(center, b);
    });

    out << "<path d=\"";
    out << "M" << pts_workspace[0].xyz[0] << "," << pts_workspace[0].xyz[1];
    size_t n = pts_workspace.size();
    for (size_t k = 0; k < n; k++) {
        const auto &p = pts_workspace[(k + n - 1) % n];
        const auto &a = pts_workspace[k];
        const auto &b = pts_workspace[(k + 1) % n];
        const auto &c = pts_workspace[(k + 2) % n];
        if (is_colinear(p, a, b) || is_colinear(a, b, c)) {
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
        ConstPointerRange<GateTarget> terms,
        std::vector<Coord<2>> &pts_workspace) {
    auto a = coords(tick, terms[0].qubit_value());
    auto b = coords(tick, terms[1].qubit_value());
    auto dif = b - a;
    auto average = (a + b) * 0.5;
    Coord<2> perp{-dif.xyz[1], dif.xyz[0]};
    auto ac1 = average + perp * 0.2 - dif * 0.2;
    auto ac2 = average + perp * 0.2 + dif * 0.2;
    auto bc1 = average + perp * -0.2 + dif * 0.2;
    auto bc2 = average + perp * -0.2 - dif * 0.2;

    out << "<path d=\"";
    out << "M" << a.xyz[0] << "," << a.xyz[1] << " ";
    out << "C ";
    out << ac1.xyz[0] << " " << ac1.xyz[1] << ", ";
    out << ac2.xyz[0] << " " << ac2.xyz[1] << ", ";
    out << b.xyz[0] << " " << b.xyz[1] << " ";
    out << "C ";
    out << bc1.xyz[0] << " " << bc1.xyz[1] << ", ";
    out << bc2.xyz[0] << " " << bc2.xyz[1] << ", ";
    out << a.xyz[0] << " " << a.xyz[1];
    out << '"';
}

void _start_one_body_svg_path(
        std::ostream &out,
        const std::function<Coord<2>(uint64_t tick, uint32_t qubit)> &coords,
        uint64_t tick,
        ConstPointerRange<GateTarget> terms,
        std::vector<Coord<2>> &pts_workspace,
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
    ConstPointerRange<GateTarget> terms,
    std::vector<Coord<2>> &pts_workspace,
    size_t scale) {
    if (terms.size() > 2) {
        _start_many_body_svg_path(out, coords, tick, terms, pts_workspace);
    } else if (terms.size() == 2) {
        _start_two_body_svg_path(out, coords, tick, terms, pts_workspace);
    } else if (terms.size() == 1) {
        _start_one_body_svg_path(out, coords, tick, terms, pts_workspace, scale);
    }
}

void DetectorSliceSet::write_svg_diagram_to(std::ostream &out) const {
    size_t num_cols = (uint64_t)ceil(sqrt((double)num_ticks));
    size_t num_rows = num_ticks / num_cols;
    while (num_cols * num_rows < num_ticks) {
        num_rows++;
    }
    while (num_cols * num_rows >= num_ticks + num_rows) {
        num_cols--;
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
    write_svg_contents_to(out, coords, 6);

    for (size_t k = 0; k < num_ticks; k++) {
        for (auto q : used_qubits()) {
            auto c = coords(min_tick + k, q);
            out << "<circle";
            write_key_val(out, "cx", c.xyz[0]);
            write_key_val(out, "cy", c.xyz[1]);
            write_key_val(out, "r", 2);
            write_key_val(out, "stroke", "none");
            write_key_val(out, "fill", "black");
            out << "/>\n";
        }
    }

    // Boundaries around different slices.
    if (num_ticks > 1) {
        for (uint64_t col = 0; col < num_cols; col++) {
            for (uint64_t row = 0; row < num_rows && row * num_cols + col < num_ticks; row++) {
                auto sw = coordsys.size.xyz[0];
                auto sh = coordsys.size.xyz[1];
                out << "<rect";
                write_key_val(out, "x", sw * col * SLICE_WINDOW_GAP);
                write_key_val(out, "y", sh * row * SLICE_WINDOW_GAP);
                write_key_val(out, "width", sw);
                write_key_val(out, "height", sh);
                write_key_val(out, "stroke", "black");
                write_key_val(out, "fill", "none");
                out << "/>\n";
            }
        }
    }

    out << R"SVG(</svg>)SVG";
}

void DetectorSliceSet::write_svg_contents_to(
    std::ostream &out, const std::function<Coord<2>(uint64_t tick, uint32_t qubit)> &coords, size_t scale) const {
    size_t clip_id = 0;

    std::vector<Coord<2>> pts_workspace;

    bool haveDrawnCorners = false;

    // Draw multi-qubit detector slices.
    for (size_t layer = 0; layer < 3; layer++) {
        for (const auto &e : slices) {
            uint64_t tick = e.first.first;
            if (e.first.second.is_observable_id()) {
                continue;
            }

            const auto &terms = e.second;
            if (layer == 0) {
                if (terms.size() <= 2) {
                    continue;
                }
            } else if (layer == 1) {
                if (terms.size() != 2) {
                    continue;
                }
            } else {
                if (terms.size() != 1) {
                    continue;
                }
            }

            const char *color = pick_color(terms);
            bool drawCorners = false;
            if (color == nullptr) {
                drawCorners = true;
                color = "#AAAAAA";
            }

            _start_slice_shape_command(out,
                                       coords,
                                       tick,
                                       terms,
                                       pts_workspace,
                                       scale);
            write_key_val(out, "stroke", "none");
            if (terms.size() > 2) {
                write_key_val(out, "fill-opacity", 0.75);
            }
            write_key_val(out, "fill", color);
            out << "/>\n";

            if (drawCorners) {
                if (!haveDrawnCorners) {
                    out << "<defs>\n";
                    static const char* const names[] = {"xgrad", "ygrad", "zgrad"};
                    static const char* const colors[] = {X_RED, Y_GREEN, Z_BLUE};
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
                    haveDrawnCorners = true;
                }
                out << R"SVG(<clipPath id="clip)SVG";
                out << clip_id;
                out << "\">";
                _start_slice_shape_command(out,
                                           coords,
                                           tick,
                                           terms,
                                           pts_workspace,
                                           scale);
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

            _start_slice_shape_command(out,
                                       coords,
                                       tick,
                                       terms,
                                       pts_workspace,
                                       scale);
            write_key_val(out, "stroke", "black");
            write_key_val(out, "fill", "none");
            out << "/>\n";
        }
    }
}
