#include "stim/diagram/detector_slice/detector_slice_set.h"

#include "stim/diagram/coord.h"
#include "stim/diagram/timeline/timeline_ascii_drawer.h"
#include "stim/simulators/error_analyzer.h"

using namespace stim;
using namespace stim_draw_internal;

template <typename T>
inline void write_key_val(std::ostream &out, const char *key, const T &val) {
    out << ' ' << key << "=\"" << val << "\"";
}

struct DetectorSliceSetComputer {
    ErrorAnalyzer analyzer;
    uint64_t num_ticks_left;
    ConstPointerRange<std::vector<double>> coord_filter;
    DetectorSliceSetComputer(
        const Circuit &circuit, uint64_t tick_index, ConstPointerRange<std::vector<double>> coord_filter);
    bool process_block_rev(const Circuit &block);
    bool process_op_rev(const Circuit &parent, const Operation &op);
};

bool DetectorSliceSetComputer::process_block_rev(const Circuit &block) {
    for (size_t k = block.operations.size(); k--;) {
        if (process_op_rev(block, block.operations[k])) {
            return true;
        }
    }
    return false;
}

bool DetectorSliceSetComputer::process_op_rev(const Circuit &parent, const Operation &op) {
    if (op.gate->id == gate_name_to_id("TICK")) {
        num_ticks_left--;
        return num_ticks_left == 0;
    } else if (op.gate->id == gate_name_to_id("REPEAT")) {
        const auto &loop_body = op_data_block_body(parent, op.target_data);
        uint64_t reps = op_data_rep_count(op.target_data);
        uint64_t num_loop_ticks = loop_body.count_ticks();
        if (num_loop_ticks * reps < num_ticks_left) {
            analyzer.run_loop(loop_body, reps);
            num_ticks_left -= num_loop_ticks * reps;
            return false;
        }

        uint64_t iterations = (num_ticks_left - 1) / num_loop_ticks;
        analyzer.run_loop(loop_body, iterations);
        num_ticks_left -= iterations * num_loop_ticks;
        while (!process_block_rev(loop_body)) {
        }
        return true;
    } else {
        (analyzer.*(op.gate->reverse_error_analyzer_function))(op.target_data);
        return false;
    }
}
DetectorSliceSetComputer::DetectorSliceSetComputer(
    const Circuit &circuit, uint64_t tick_index, ConstPointerRange<std::vector<double>> coord_filter)
    : analyzer(circuit.count_detectors(), circuit.count_qubits(), false, true, true, 1, false, false),
      coord_filter(coord_filter) {
    num_ticks_left = circuit.count_ticks() + 1;  // + 1 because "first tick" is start of circuit.
    if (num_ticks_left == 0) {
        throw std::invalid_argument("Circuit contains no TICK instructions to slice at.");
    }
    if (tick_index >= num_ticks_left) {
        std::stringstream ss;
        ss << "tick_index=" << tick_index << " >= circuit.num_ticks=" << num_ticks_left;
        throw std::invalid_argument(ss.str());
    }
    num_ticks_left -= tick_index;
    analyzer.accumulate_errors = false;
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
            ss << s.first;
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
        if (p != coordinates.end()) {
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

DetectorSliceSet DetectorSliceSet::from_circuit_tick(
    const stim::Circuit &circuit, uint64_t tick_index, ConstPointerRange<std::vector<double>> coord_filter) {
    DetectorSliceSetComputer helper(circuit, tick_index, coord_filter);
    size_t num_qubits = helper.analyzer.xs.size();
    helper.process_block_rev(circuit);

    std::set<DemTarget> xs;
    std::set<DemTarget> ys;
    std::set<DemTarget> zs;
    DetectorSliceSet result;
    result.num_qubits = num_qubits;
    for (size_t q = 0; q < num_qubits; q++) {
        xs.clear();
        ys.clear();
        zs.clear();
        for (auto t : helper.analyzer.xs[q]) {
            xs.insert(t);
        }
        for (auto t : helper.analyzer.zs[q]) {
            if (xs.find(t) == xs.end()) {
                zs.insert(t);
            } else {
                xs.erase(t);
                ys.insert(t);
            }
        }

        for (const auto &t : xs) {
            result.slices[t].push_back(GateTarget::x(q));
        }
        for (const auto &t : ys) {
            result.slices[t].push_back(GateTarget::y(q));
        }
        for (const auto &t : zs) {
            result.slices[t].push_back(GateTarget::z(q));
        }
    }

    result.coordinates = circuit.get_final_qubit_coords();
    std::set<uint64_t> included_detectors;
    for (const auto &t : result.slices) {
        if (t.first.is_relative_detector_id()) {
            included_detectors.insert(t.first.data);
        }
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
    std::vector<DemTarget> removed;
    for (const auto &t : result.slices) {
        if (!keep(t.first)) {
            removed.push_back(t.first);
        }
    }
    for (auto t : removed) {
        result.slices.erase(t);
        result.detector_coordinates.erase(t.raw_id());
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
        return "#FF4444";
    } else if (has_y) {
        return "#40FF40";
    } else {
        assert(has_z);
        return "#4848FF";
    }
}

float angle_from_to(Coord<2> origin, Coord<2> dst) {
    auto d = dst - origin;
    if (d.xyz[0] * d.xyz[0] + d.xyz[1] * d.xyz[1] < 1e-6) {
        return 0;
    }
    return atan2f(d.xyz[1], d.xyz[0]);
}

void write_terms_svg_path(
    std::ostream &out,
    DemTarget src,
    const FlattenedCoords &coordsys,
    ConstPointerRange<GateTarget> terms,
    std::vector<Coord<2>> &pts_workspace) {
    if (terms.size() > 2) {
        Coord<2> center{0, 0};
        for (const auto &term : terms) {
            center += coordsys.qubit_coords[term.qubit_value()];
        }
        center /= terms.size();

        pts_workspace.clear();
        for (const auto &term : terms) {
            pts_workspace.push_back(coordsys.qubit_coords[term.qubit_value()]);
        }
        std::sort(pts_workspace.begin(), pts_workspace.end(), [&](Coord<2> a, Coord<2> b) {
            return angle_from_to(center, a) < angle_from_to(center, b);
        });

        out << "M";
        for (const auto &pt : pts_workspace) {
            out << pt.xyz[0] << "," << pt.xyz[1] << " ";
        }
        out << "Z";
    } else if (terms.size() == 2) {
        auto a = coordsys.qubit_coords[terms[0].qubit_value()];
        auto b = coordsys.qubit_coords[terms[1].qubit_value()];
        auto dif = b - a;
        auto average = (a + b) * 0.5;
        Coord<2> perp{-dif.xyz[1], dif.xyz[0]};
        auto ac1 = average + perp * 0.2 - dif * 0.2;
        auto ac2 = average + perp * 0.2 + dif * 0.2;
        auto bc1 = average + perp * -0.2 + dif * 0.2;
        auto bc2 = average + perp * -0.2 - dif * 0.2;

        out << "M" << a.xyz[0] << "," << a.xyz[1] << " ";
        out << "C ";
        out << ac1.xyz[0] << " " << ac1.xyz[1] << ", ";
        out << ac2.xyz[0] << " " << ac2.xyz[1] << ", ";
        out << b.xyz[0] << " " << b.xyz[1] << " ";
        out << "C ";
        out << bc1.xyz[0] << " " << bc1.xyz[1] << ", ";
        out << bc2.xyz[0] << " " << bc2.xyz[1] << ", ";
        out << a.xyz[0] << " " << a.xyz[1];
    } else if (terms.size() == 1) {
        auto c = coordsys.qubit_coords[terms[0].qubit_value()];
        out << "M" << (c.xyz[0] - 6) << "," << c.xyz[1] << " a 6 6 0 0 0 12 0 a 6 6 0 0 0 -12 0";
    }
}

void DetectorSliceSet::write_svg_diagram_to(std::ostream &out) const {
    auto coordsys = FlattenedCoords::from(*this, 32);

    out << R"SVG(<svg viewBox="0 0 )SVG";
    out << coordsys.size.xyz[0];
    out << " ";
    out << coordsys.size.xyz[1];
    out << R"SVG(" xmlns="http://www.w3.org/2000/svg">)SVG";
    out << "\n";
    size_t clip_id = 0;

    std::vector<Coord<2>> pts_workspace;

    bool haveDrawnCorners = false;

    // Draw multi-qubit detector slices.
    for (size_t layer = 0; layer < 3; layer++) {
        for (const auto &e : slices) {
            if (e.first.is_observable_id()) {
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

            out << R"SVG(<path d=")SVG";
            write_terms_svg_path(out, e.first, coordsys, terms, pts_workspace);
            out << R"SVG(" stroke="none" fill-opacity=")SVG";
            out << (terms.size() > 2 ? 0.75 : 1);
            out << R"SVG(" fill=")SVG";
            out << color;
            out << '"';
            out << " />\n";

            if (drawCorners) {
                if (!haveDrawnCorners) {
                    out << R"SVG(<defs>
<radialGradient id="xgrad"><stop offset="50%" stop-color="#FF4444" stop-opacity="1"/><stop offset="100%" stop-color="#AAAAAA" stop-opacity="0"/></radialGradient>
<radialGradient id="ygrad"><stop offset="50%" stop-color="#40FF40" stop-opacity="1"/><stop offset="100%" stop-color="#AAAAAA" stop-opacity="0"/></radialGradient>
<radialGradient id="zgrad"><stop offset="50%" stop-color="#4848FF" stop-opacity="1"/><stop offset="100%" stop-color="#AAAAAA" stop-opacity="0"/></radialGradient>
</defs>
)SVG";
                    haveDrawnCorners = true;
                }
                out << R"SVG(<clipPath id="clip)SVG";
                out << clip_id;
                out << R"SVG("><path d=")SVG";
                write_terms_svg_path(out, e.first, coordsys, terms, pts_workspace);
                out << "\" /></clipPath>\n";

                for (const auto &t : terms) {
                    auto c = coordsys.qubit_coords[t.qubit_value()];
                    out << R"SVG(<circle clip-path="url(#clip)SVG";
                    out << clip_id;
                    out << ")\"";
                    write_key_val(out, "cx", c.xyz[0]);
                    write_key_val(out, "cy", c.xyz[1]);
                    write_key_val(out, "r", 20);
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

            out << R"SVG(<path d=")SVG";
            write_terms_svg_path(out, e.first, coordsys, terms, pts_workspace);
            out << R"SVG(" stroke="black" fill="none")SVG";
            out << " />\n";
        }
    }

    for (auto q : used_qubits()) {
        auto c = coordsys.qubit_coords[q];
        out << R"SVG(<circle cx=")SVG";
        out << c.xyz[0];
        out << R"SVG(" cy=")SVG";
        out << c.xyz[1];
        out << R"SVG(" r="2" stroke="none" fill="black" />)SVG";
        out << "\n";
    }

    out << R"SVG(</svg>)SVG";
}
