// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/cmd/command_diagram.pybind.h"

#include "stim/cmd/command_help.h"
#include "stim/diagram/base64.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/diagram/graph/match_graph_3d_drawer.h"
#include "stim/diagram/graph/match_graph_svg_drawer.h"
#include "stim/diagram/timeline/timeline_3d_drawer.h"
#include "stim/diagram/timeline/timeline_ascii_drawer.h"
#include "stim/diagram/timeline/timeline_svg_drawer.h"
#include "stim/simulators/error_analyzer.h"

using namespace stim;
using namespace stim_pybind;
using namespace stim_draw_internal;

pybind11::class_<DiagramHelper> stim_pybind::pybind_diagram(pybind11::module &m) {
    auto c = pybind11::class_<DiagramHelper>(
        m,
        "_DiagramHelper",
        clean_doc_string(R"DOC(
            A helper class for displaying diagrams in IPython notebooks.

            To write the diagram's contents to a file (for example, to write an
            SVG image to an SVG file), use `print(diagram, file=file)`.
        )DOC")
            .data());

    return c;
}

void stim_pybind::pybind_diagram_methods(pybind11::module &m, pybind11::class_<DiagramHelper> &c) {
    c.def("_repr_html_", [](const DiagramHelper &self) -> pybind11::object {
        if (self.type == DIAGRAM_TYPE_TEXT) {
            return pybind11::cast("<pre>" + self.content + "</pre>");
        }
        if (self.type == DIAGRAM_TYPE_SVG) {
            std::stringstream out;
            out << R"HTML(<div style="border: 1px dashed gray; margin-bottom: 50px; height: 512px; resize: both; overflow: hidden">)HTML";
            out << R"HTML(<img style="max-width: 100%; max-height: 100%" src="data:image/svg+xml;base64,)HTML";
            write_data_as_base64_to(self.content.data(), self.content.size(), out);
            out << R"HTML("/></div>)HTML";
            return pybind11::cast(out.str());
        }
        if (self.type == DIAGRAM_TYPE_GLTF) {
            std::stringstream out;
            write_html_viewer_for_gltf_data(self.content, out);
            return pybind11::cast(out.str());
        }
        if (self.type == DIAGRAM_TYPE_HTML) {
            return pybind11::cast(self.content);
        }
        return pybind11::none();
    });
    c.def("_repr_svg_", [](const DiagramHelper &self) -> pybind11::object {
        if (self.type != DIAGRAM_TYPE_SVG) {
            return pybind11::none();
        }
        return pybind11::cast(self.content);
    });
    c.def("_repr_pretty_", [](const DiagramHelper &self, pybind11::object p, pybind11::object cycle) -> void {
        pybind11::getattr(p, "text")(self.content);
    });
    c.def("__str__", [](const DiagramHelper &self) {
        return self.content;
    });
}

DiagramHelper stim_pybind::dem_diagram(const DetectorErrorModel &dem, const std::string &type) {
    if (type == "match-graph-svg") {
        std::stringstream out;
        dem_match_graph_to_svg_diagram_write_to(dem, out);
        return DiagramHelper{DIAGRAM_TYPE_SVG, out.str()};
    } else if (type == "match-graph-3d") {
        std::stringstream out;
        dem_match_graph_to_basic_3d_diagram(dem).to_gltf_scene().to_json().write(out);
        return DiagramHelper{DIAGRAM_TYPE_GLTF, out.str()};
    } else if (type == "match-graph-3d-html") {
        std::stringstream out;
        dem_match_graph_to_basic_3d_diagram(dem).to_gltf_scene().to_json().write(out);
        std::stringstream out_html;
        write_html_viewer_for_gltf_data(out.str(), out_html);
        return DiagramHelper{DIAGRAM_TYPE_GLTF, out_html.str()};
    } else {
        throw std::invalid_argument("Unrecognized diagram type: " + type);
    }
}
DiagramHelper stim_pybind::circuit_diagram(
    const Circuit &circuit,
    const std::string &type,
    const pybind11::object &tick,
    const pybind11::object &filter_coords_obj) {
    std::vector<std::vector<double>> filter_coords;
    try {
        if (filter_coords_obj.is_none()) {
            filter_coords.push_back({});
        } else {
            for (const auto &e : filter_coords_obj) {
                filter_coords.push_back({});
                for (const auto &c : e) {
                    filter_coords.back().push_back(pybind11::cast<double>(c));
                }
            }
        }
    } catch (const std::exception &_) {
        throw std::invalid_argument("filter_coords wasn't a list of list of floats.");
    }

    uint64_t tick_min;
    uint64_t num_ticks;
    if (tick.is_none()) {
        tick_min = 0;
        num_ticks = UINT64_MAX;
    } else if (pybind11::isinstance(tick, pybind11::module::import("builtins").attr("range"))) {
        tick_min = pybind11::cast<uint64_t>(tick.attr("start"));
        auto tick_stop = pybind11::cast<uint64_t>(tick.attr("stop"));
        auto tick_step = pybind11::cast<uint64_t>(tick.attr("step"));
        if (tick_step != 1) {
            throw std::invalid_argument("tick.step != 1");
        }
        if (tick_stop <= tick_min) {
            throw std::invalid_argument("tick.stop <= tick.start");
        }
        num_ticks = tick_stop - tick_min;
    } else {
        tick_min = pybind11::cast<uint64_t>(tick);
        num_ticks = 1;
    }

    if (type == "timeline-text") {
        if (!tick.is_none()) {
            throw std::invalid_argument("`tick` isn't used with type='timeline-text'");
        }
        std::stringstream out;
        out << DiagramTimelineAsciiDrawer::make_diagram(circuit);
        return DiagramHelper{DIAGRAM_TYPE_TEXT, out.str()};
    } else if (type == "timeline-svg") {
        std::stringstream out;
        DiagramTimelineSvgDrawer::make_diagram_write_to(circuit, out, tick_min, num_ticks, SVG_MODE_TIMELINE, filter_coords);
        return DiagramHelper{DIAGRAM_TYPE_SVG, out.str()};
    } else if (type == "time-slice-svg") {
        std::stringstream out;
        DiagramTimelineSvgDrawer::make_diagram_write_to(circuit, out, tick_min, num_ticks, SVG_MODE_TIME_SLICE, filter_coords);
        return DiagramHelper{DIAGRAM_TYPE_SVG, out.str()};
    } else if (type == "time+detector-slice-svg") {
        std::stringstream out;
        DiagramTimelineSvgDrawer::make_diagram_write_to(circuit, out, tick_min, num_ticks, SVG_MODE_TIME_DETECTOR_SLICE, filter_coords);
        return DiagramHelper{DIAGRAM_TYPE_SVG, out.str()};
    } else if (type == "timeline-3d") {
        std::stringstream out;
        DiagramTimeline3DDrawer::circuit_to_basic_3d_diagram(circuit).to_gltf_scene().to_json().write(out);
        return DiagramHelper{DIAGRAM_TYPE_GLTF, out.str()};
    } else if (type == "timeline-3d-html") {
        std::stringstream out;
        DiagramTimeline3DDrawer::circuit_to_basic_3d_diagram(circuit).to_gltf_scene().to_json().write(out);
        std::stringstream out_html;
        write_html_viewer_for_gltf_data(out.str(), out_html);
        return DiagramHelper{DIAGRAM_TYPE_GLTF, out_html.str()};
    } else if (type == "detector-slice-text") {
        if (tick.is_none()) {
            throw std::invalid_argument("You must specify the tick= argument when using type='detector-slice-text'");
        }
        std::stringstream out;
        DetectorSliceSet::from_circuit_ticks(circuit, tick_min, num_ticks, filter_coords)
            .write_text_diagram_to(out);
        return DiagramHelper{DIAGRAM_TYPE_TEXT, out.str()};
    } else if (type == "detector-slice-svg") {
        if (tick.is_none()) {
            throw std::invalid_argument("You must specify the tick= argument when using type='detector-slice-svg'");
        }
        std::stringstream out;
        DetectorSliceSet::from_circuit_ticks(circuit, tick_min, num_ticks, filter_coords)
            .write_svg_diagram_to(out);
        return DiagramHelper{DIAGRAM_TYPE_SVG, out.str()};
    } else if (type == "match-graph-svg" || type == "match-graph-3d" || type == "match-graph-3d-html") {
        auto dem = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, 1, true, false);
        return dem_diagram(dem, type);
    } else {
        throw std::invalid_argument("Unrecognized diagram type: " + type);
    }
}
