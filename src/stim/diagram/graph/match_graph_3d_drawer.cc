#include "stim/diagram/graph/match_graph_3d_drawer.h"

using namespace stim;
using namespace stim_draw_internal;

Coord<3> flattened_3d(SpanRef<const double> c) {
    float x = 0;
    float y = 0;
    float z = 0;
    if (c.size() >= 1) {
        x = c[0];
    }
    if (c.size() >= 2) {
        y = c[1];
    }
    if (c.size() >= 3) {
        z = c[2];
    }

    // Arbitrary orthographic projection.
    for (size_t k = 3; k < c.size(); k++) {
        x += c[k] / k;
        y += c[k] / (k * k);
        x += c[k] / (k * k * k);
    }

    return {x * 3, y * 3, z * 3};
}

std::vector<Coord<3>> pick_coordinates(const DetectorErrorModel &dem) {
    Basic3dDiagram out;
    std::set<uint64_t> det_set;
    uint64_t n = dem.count_detectors();
    std::set<Coord<3>> used_coords;
    std::vector<Coord<3>> coords;
    coords.resize(n);
    float min_z = 0;
    for (uint64_t k = 0; k < n; k++) {
        det_set.insert(k);
    }
    auto dem_coords = dem.get_detector_coordinates(det_set);
    for (auto &kv : dem_coords) {
        if (kv.second.size() == 0) {
            continue;
        }
        coords[kv.first] = flattened_3d(kv.second);
        used_coords.insert(coords[kv.first]);
        min_z = std::min(min_z, coords[kv.first].xyz[2]);
    }

    float next_x = 0;
    float next_y = 0;
    float dx = 1;
    float dy = -1;
    for (size_t d = 0; d < n; d++) {
        auto p = dem_coords.find(d);
        if (p == dem_coords.end() || p->second.size() == 0) {
            coords[d] = {next_x * 3, next_y * 3, min_z - 1};
            next_x += dx;
            next_y += dy;
            if (next_y < 0 || next_x < 0) {
                next_x = std::max(next_x, 0.0f);
                next_y = std::max(next_y, 0.0f);
                dx *= -1;
                dy *= -1;
            }
        }
    }
    if (next_x || next_y) {
        std::cerr << "Warning: not all detectors had coordinates. Placed them arbitrarily.\n";
    }
    return coords;
}

Basic3dDiagram stim_draw_internal::dem_match_graph_to_basic_3d_diagram(const stim::DetectorErrorModel &dem) {
    Basic3dDiagram out;

    auto coords = pick_coordinates(dem);
    auto minmax = Coord<3>::min_max(coords);
    auto center = (minmax.first + minmax.second) * 0.5;

    std::set<uint64_t> boundary_observable_detectors;
    std::vector<Coord<3>> det_coords;
    auto handle_contiguous_targets = [&](SpanRef<const DemTarget> targets) {
        bool has_observables = false;
        det_coords.clear();
        for (const auto &t : targets) {
            has_observables |= t.is_observable_id();
            if (t.is_relative_detector_id()) {
                det_coords.push_back(coords[t.raw_id()]);
            }
        }
        if (det_coords.empty()) {
            return;
        }
        if (det_coords.size() == 1) {
            auto d = det_coords[0] - center;
            auto r = d.norm();
            if (r < 1e-4) {
                d = {1, 0, 0};
            } else {
                d /= r;
            }
            auto a = det_coords[0];
            det_coords.push_back(a + d * 10);
            if (has_observables) {
                for (auto t : targets) {
                    if (t.is_relative_detector_id()) {
                        boundary_observable_detectors.insert(t.val());
                    }
                }
            }
        }
        if (det_coords.size() == 2) {
            if (has_observables) {
                out.red_line_data.push_back(det_coords[0]);
                out.red_line_data.push_back(det_coords[1]);
            } else {
                out.line_data.push_back(det_coords[0]);
                out.line_data.push_back(det_coords[1]);
            }
        } else {
            Coord<3> c{0, 0, 0};
            for (const auto &e : det_coords) {
                c += e;
            }
            c /= det_coords.size();
            for (const auto &e : det_coords) {
                if (has_observables) {
                    out.purple_line_data.push_back(c);
                    out.purple_line_data.push_back(e);
                } else {
                    out.blue_line_data.push_back(c);
                    out.blue_line_data.push_back(e);
                }
            }
        }
    };

    dem.iter_flatten_error_instructions([&](const DemInstruction &op) {
        if (op.type != DemInstructionType::DEM_ERROR) {
            return;
        }
        auto *p = op.target_data.ptr_start;
        size_t start = 0;
        for (size_t k = 0; k < op.target_data.size(); k++) {
            if (op.target_data[k].is_separator()) {
                handle_contiguous_targets({p + start, p + k});
                start = k + 1;
            }
        }
        handle_contiguous_targets({p + start, op.target_data.ptr_end});
    });

    for (size_t k = 0; k < coords.size(); k++) {
        bool excited = boundary_observable_detectors.find(k) != boundary_observable_detectors.end();
        out.elements.push_back({excited ? "EXCITED_DETECTOR" : "DETECTOR", coords[k]});
    }

    return out;
}
