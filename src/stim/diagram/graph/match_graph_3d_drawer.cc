#include "stim/diagram/graph/match_graph_3d_drawer.h"

using namespace stim;
using namespace stim_draw_internal;

Coord<3> flattened_3d(ConstPointerRange<double> c) {
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
    std::set<uint64_t> dets_left;
    uint64_t n = dem.count_detectors();
    for (uint64_t k = 0; k < n; k++) {
        dets_left.insert(k);
    }
    std::set<Coord<3>> used_coords;
    std::vector<Coord<3>> coords;
    coords.resize(n);
    float def_x = 0;
    for (auto &kv : dem.get_detector_coordinates(dets_left)) {
        dets_left.erase(kv.first);
        coords[kv.first] = flattened_3d(kv.second);
        used_coords.insert(coords[kv.first]);
        def_x = std::min(def_x, coords[kv.first].xyz[0] - 1);
    }
    float def_y = 0;
    float def_z = 0;
    for (auto u : dets_left) {
        def_y += 1.0f;
        def_z -= 1.0f;
        if (def_z < 0) {
            def_z = def_y;
            def_y = 0;
        }
        coords[u] = {def_x, def_y, def_z};
    }
    return coords;
}

Basic3dDiagram stim_draw_internal::dem_match_graph_to_basic_3d_diagram(const stim::DetectorErrorModel &dem) {
    Basic3dDiagram out;

    auto coords = pick_coordinates(dem);
    auto minmax = Coord<3>::min_max(coords);
    auto center = (minmax.first + minmax.second) * 0.5;

    std::vector<Coord<3>> det_coords;
    auto handle_contiguous_targets = [&](ConstPointerRange<DemTarget> targets) {
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
            Coord<3> c{0, 0 ,0};
            for (const auto &e : det_coords) {
                c += e;
            }
            c /= det_coords.size();
            for (const auto &e : det_coords) {
                out.blue_line_data.push_back(c);
                out.blue_line_data.push_back(e);
            }
        }
    };

    for (const auto &c : coords) {
        out.elements.push_back({"Z_CONTROL", c});
    }

    dem.iter_flatten_error_instructions([&](const DemInstruction &op) {
        if (op.type != stim::DEM_ERROR) {
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

    return out;
}
