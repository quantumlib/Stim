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
#include "stim/dem/detector_error_model_target.pybind.h"
#include "stim/diagram/base64.h"
#include "stim/diagram/crumble.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/diagram/graph/match_graph_3d_drawer.h"
#include "stim/diagram/graph/match_graph_svg_drawer.h"
#include "stim/diagram/timeline/timeline_3d_drawer.h"
#include "stim/diagram/timeline/timeline_ascii_drawer.h"
#include "stim/diagram/timeline/timeline_svg_drawer.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/util_bot/arg_parse.h"

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

std::string escape_html_for_srcdoc(std::string_view src) {
    // From https://stackoverflow.com/a/9907752
    std::stringstream dst;
    for (char ch : src) {
        switch (ch) {
            case '&':
                dst << "&amp;";
                break;
            case '\'':
                dst << "&apos;";
                break;
            case '"':
                dst << "&quot;";
                break;
            case '<':
                dst << "&lt;";
                break;
            case '>':
                dst << "&gt;";
                break;
            default:
                dst << ch;
                break;
        }
    }
    return dst.str();
}

pybind11::object diagram_as_html(const DiagramHelper &self) {
    std::string output = "None";
    if (self.type == DiagramType::DIAGRAM_TYPE_TEXT) {
        return pybind11::cast("<pre>" + self.content + "</pre>");
    }
    if (self.type == DiagramType::DIAGRAM_TYPE_SVG_HTML) {
        // Wrap the SVG image into an img tag.
        std::stringstream out;
        out << R"HTML(<img style="max-width: 100%; max-height: 100%" src="data:image/svg+xml;base64,)HTML";
        write_data_as_base64_to(self.content, out);
        out << R"HTML("/>)HTML";
        output = out.str();
    } else if (self.type == DiagramType::DIAGRAM_TYPE_SVG) {
        // Github's Jupyter notebook preview will fail to show SVG images if they are wrapped in HTML.
        // So, for SVG diagrams, we refuse to return an html repr in this case.
        return pybind11::none();
    }
    if (self.type == DiagramType::DIAGRAM_TYPE_GLTF) {
        std::stringstream out;
        write_html_viewer_for_gltf_data(self.content, out);
        output = out.str();
    }
    if (self.type == DiagramType::DIAGRAM_TYPE_HTML) {
        output = self.content;
    }
    if (output == "None") {
        return pybind11::none();
    }

    // Wrap the output into an iframe.
    // In a Jupyter notebook this is very important, because it prevents output
    // cells from seeing each others' elements when finding elements by id.
    // Because, for some insane reason, Jupyter notebooks don't isolate the cells
    // from each other by default! Colab does the right thing at least...
    std::string framed =
        R"HTML(<iframe style="width: 100%; height: 300px; overflow: hidden; resize: both; border: 1px dashed gray;" frameBorder="0" srcdoc=")HTML" +
        escape_html_for_srcdoc(output) + R"HTML("></iframe>)HTML";
    return pybind11::cast(framed);
}

void stim_pybind::pybind_diagram_methods(pybind11::module &m, pybind11::class_<DiagramHelper> &c) {
    c.def("_repr_html_", &diagram_as_html);
    c.def("_repr_svg_", [](const DiagramHelper &self) -> pybind11::object {
        if (self.type != DiagramType::DIAGRAM_TYPE_SVG) {
            return pybind11::none();
        }
        return pybind11::cast(self.content);
    });
    c.def("_repr_pretty_", [](const DiagramHelper &self, pybind11::object p, pybind11::object cycle) -> void {
        pybind11::getattr(p, "text")(self.content);
    });
    c.def("__repr__", [](const DiagramHelper &self) -> std::string {
        std::stringstream ss;
        ss << "<A stim._DiagramHelper containing ";
        switch (self.type) {
            case DiagramType::DIAGRAM_TYPE_GLTF:
                ss << "a GLTF 3d model";
                break;
            case DiagramType::DIAGRAM_TYPE_SVG:
                ss << "an SVG image";
                break;
            case DiagramType::DIAGRAM_TYPE_TEXT:
                ss << "text";
                break;
            case DiagramType::DIAGRAM_TYPE_HTML:
                ss << "an HTML document";
                break;
            case DiagramType::DIAGRAM_TYPE_SVG_HTML:
                ss << "an HTML SVG image viewer";
                break;
            default:
                ss << "???";
        }
        ss << " that will display inline in Jupyter notebooks. Use 'str' or 'print' to access the contents as text.>";
        return ss.str();
    });
    c.def("__str__", [](const DiagramHelper &self) -> pybind11::object {
        if (self.type == DiagramType::DIAGRAM_TYPE_SVG_HTML) {
            return diagram_as_html(self);
        }
        return pybind11::cast(self.content);
    });
}

DiagramHelper stim_pybind::dem_diagram(const DetectorErrorModel &dem, std::string_view type) {
    if (type == "matchgraph-svg" || type == "match-graph-svg" || type == "match-graph-svg-html" ||
        type == "matchgraph-svg-html") {
        std::stringstream out;
        dem_match_graph_to_svg_diagram_write_to(dem, out);
        DiagramType d_type =
            type.find("html") != std::string::npos ? DiagramType::DIAGRAM_TYPE_SVG_HTML : DiagramType::DIAGRAM_TYPE_SVG;
        return DiagramHelper{d_type, out.str()};
    } else if (type == "matchgraph-3d" || type == "match-graph-3d") {
        std::stringstream out;
        dem_match_graph_to_basic_3d_diagram(dem).to_gltf_scene().to_json().write(out);
        return DiagramHelper{DiagramType::DIAGRAM_TYPE_GLTF, out.str()};
    } else if (type == "matchgraph-3d-html" || type == "match-graph-3d-html") {
        std::stringstream out;
        dem_match_graph_to_basic_3d_diagram(dem).to_gltf_scene().to_json().write(out);
        std::stringstream out_html;
        write_html_viewer_for_gltf_data(out.str(), out_html);
        return DiagramHelper{DiagramType::DIAGRAM_TYPE_GLTF, out_html.str()};
    } else {
        std::stringstream ss;
        ss << "Unrecognized diagram type: " << type;
        throw std::invalid_argument(ss.str());
    }
}

CoordFilter item_to_filter_single(const pybind11::handle &obj) {
    if (pybind11::isinstance<ExposedDemTarget>(obj)) {
        CoordFilter filter;
        filter.exact_target = pybind11::cast<ExposedDemTarget>(obj).internal();
        filter.use_target = true;
        return filter;
    }

    try {
        std::string_view text = pybind11::cast<std::string_view>(obj);
        if (text.size() > 1 && text[0] == 'D') {
            CoordFilter filter;
            filter.exact_target = DemTarget::relative_detector_id(parse_exact_uint64_t_from_string(text.substr(1)));
            filter.use_target = true;
            return filter;
        }
        if (text.size() > 1 && text[0] == 'L') {
            CoordFilter filter;
            filter.exact_target = DemTarget::observable_id(parse_exact_uint64_t_from_string(text.substr(1)));
            filter.use_target = true;
            return filter;
        }
    } catch (const pybind11::cast_error &) {
    } catch (const std::invalid_argument &) {
    }

    CoordFilter filter;
    for (const auto &c : obj) {
        filter.coordinates.push_back(pybind11::cast<double>(c));
    }
    return filter;
}

std::vector<CoordFilter> item_to_filter_multi(const pybind11::object &obj) {
    if (obj.is_none()) {
        return {CoordFilter{}};
    }

    try {
        return {item_to_filter_single(obj)};
    } catch (const pybind11::cast_error &) {
    } catch (const std::invalid_argument &) {
    }

    std::vector<CoordFilter> filters;
    for (const auto &filter_case : obj) {
        filters.push_back(item_to_filter_single(filter_case));
    }
    return filters;
}

DiagramHelper stim_pybind::circuit_diagram(
    const Circuit &circuit,
    std::string_view type,
    const pybind11::object &tick,
    const pybind11::object &rows,
    const pybind11::object &filter_coords_obj) {
    std::vector<CoordFilter> filter_coords;
    try {
        filter_coords = item_to_filter_multi(filter_coords_obj);
    } catch (const std::exception &_) {
        throw std::invalid_argument("filter_coords wasn't an Iterable[stim.DemTarget | Iterable[float]].");
    }

    size_t num_rows = 0;
    if (!rows.is_none()) {
        num_rows = pybind11::cast<size_t>(rows);
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
        return DiagramHelper{DiagramType::DIAGRAM_TYPE_TEXT, out.str()};
    } else if (type == "timeline-svg" || type == "timeline" || type == "timeline-svg-html" || type == "timeline-html") {
        std::stringstream out;
        DiagramTimelineSvgDrawer::make_diagram_write_to(
            circuit, out, tick_min, num_ticks, DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE, filter_coords);
        DiagramType d_type =
            type.find("html") != std::string::npos ? DiagramType::DIAGRAM_TYPE_SVG_HTML : DiagramType::DIAGRAM_TYPE_SVG;
        return DiagramHelper{d_type, out.str()};
    } else if (
        type == "time-slice-svg" || type == "timeslice-svg" || type == "timeslice-html" ||
        type == "timeslice-svg-html" || type == "time-slice-html" || type == "time-slice-svg-html" ||
        type == "timeslice" || type == "time-slice") {
        std::stringstream out;
        DiagramTimelineSvgDrawer::make_diagram_write_to(
            circuit,
            out,
            tick_min,
            num_ticks,
            DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_SLICE,
            filter_coords,
            num_rows);
        DiagramType d_type =
            type.find("html") != std::string::npos ? DiagramType::DIAGRAM_TYPE_SVG_HTML : DiagramType::DIAGRAM_TYPE_SVG;
        return DiagramHelper{d_type, out.str()};
    } else if (
        type == "detslice-svg" || type == "detslice" || type == "detslice-html" || type == "detslice-svg-html" ||
        type == "detector-slice-svg" || type == "detector-slice") {
        std::stringstream out;
        DetectorSliceSet::from_circuit_ticks(circuit, tick_min, num_ticks, filter_coords)
            .write_svg_diagram_to(out, num_rows);
        DiagramType d_type =
            type.find("html") != std::string::npos ? DiagramType::DIAGRAM_TYPE_SVG_HTML : DiagramType::DIAGRAM_TYPE_SVG;
        return DiagramHelper{d_type, out.str()};
    } else if (
        type == "detslice-with-ops" || type == "detslice-with-ops-svg" || type == "detslice-with-ops-html" ||
        type == "detslice-with-ops-svg-html" || type == "time+detector-slice-svg") {
        std::stringstream out;
        DiagramTimelineSvgDrawer::make_diagram_write_to(
            circuit,
            out,
            tick_min,
            num_ticks,
            DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE,
            filter_coords,
            num_rows);
        DiagramType d_type =
            type.find("html") != std::string::npos ? DiagramType::DIAGRAM_TYPE_SVG_HTML : DiagramType::DIAGRAM_TYPE_SVG;
        return DiagramHelper{d_type, out.str()};
    } else if (type == "timeline-3d") {
        std::stringstream out;
        DiagramTimeline3DDrawer::circuit_to_basic_3d_diagram(circuit).to_gltf_scene().to_json().write(out);
        return DiagramHelper{DiagramType::DIAGRAM_TYPE_GLTF, out.str()};
    } else if (type == "timeline-3d-html") {
        std::stringstream out;
        DiagramTimeline3DDrawer::circuit_to_basic_3d_diagram(circuit).to_gltf_scene().to_json().write(out);
        std::stringstream out_html;
        write_html_viewer_for_gltf_data(out.str(), out_html);
        return DiagramHelper{DiagramType::DIAGRAM_TYPE_GLTF, out_html.str()};
    } else if (type == "detslice-text" || type == "detector-slice-text") {
        std::stringstream out;
        DetectorSliceSet::from_circuit_ticks(circuit, tick_min, num_ticks, filter_coords).write_text_diagram_to(out);
        return DiagramHelper{DiagramType::DIAGRAM_TYPE_TEXT, out.str()};
    } else if (type == "interactive" || type == "interactive-html") {
        std::stringstream out;
        write_crumble_html_with_preloaded_circuit(circuit, out);
        return DiagramHelper{DiagramType::DIAGRAM_TYPE_HTML, out.str()};
    } else if (
        type == "match-graph-svg" || type == "matchgraph-svg" || type == "matchgraph-svg-html" ||
        type == "matchgraph-html" || type == "match-graph-svg-html" || type == "match-graph-html" ||
        type == "match-graph-3d" || type == "matchgraph-3d" || type == "match-graph-3d-html" ||
        type == "matchgraph-3d-html") {
        DetectorErrorModel dem;
        try {
            // By default, try to decompose the errors.
            dem = ErrorAnalyzer::circuit_to_detector_error_model(circuit, true, true, false, 1, false, false);
        } catch (const std::invalid_argument &) {
            // If any decomposition fails, don't decompose at all.
            dem = ErrorAnalyzer::circuit_to_detector_error_model(circuit, false, true, false, 1, false, false);
        }
        return dem_diagram(dem, type);
    } else {
        std::stringstream ss;
        ss << "Unrecognized diagram type: '";
        ss << type << "'";
        throw std::invalid_argument(ss.str());
    }
}
