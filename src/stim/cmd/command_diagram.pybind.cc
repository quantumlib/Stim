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
#include "stim/diagram/timeline/timeline_ascii_drawer.h"
#include "stim/diagram/timeline/timeline_svg_drawer.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/diagram/base64.h"

using namespace stim;
using namespace stim_pybind;
using namespace stim_draw_internal;


pybind11::class_<DiagramHelper> stim_pybind::pybind_diagram(pybind11::module &m) {
    auto c = pybind11::class_<DiagramHelper>(
        m,
        "_DiagramHelper",
        clean_doc_string(u8R"DOC(
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
            out << R"HTML(<div style="border: 1px dashed gray; margin-bottom: 50px; width: 300px; resize: both; overflow: hidden">)HTML";
            out << R"HTML(<img style="max-width: 100%; max-height: 100%" src="data:image/svg+xml;base64,)HTML";
            write_data_as_base64_to(self.content.data(), self.content.size(), out);
            out << R"HTML("/></div>)HTML";
            return pybind11::cast(out.str());
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

DiagramHelper stim_pybind::circuit_diagram(const Circuit &self, const std::string &type, const pybind11::object &tick) {
    if (type == "timeline-text") {
        if (!tick.is_none()) {
            throw std::invalid_argument("`tick` isn't used with type='timeline-text'");
        }
        std::stringstream out;
        out << DiagramTimelineAsciiDrawer::make_diagram(self);
        return DiagramHelper{DIAGRAM_TYPE_TEXT, out.str()};
    } else if (type == "timeline-svg") {
        if (!tick.is_none()) {
            throw std::invalid_argument("`tick` isn't used with type='timeline-svg'");
        }
        std::stringstream out;
        DiagramTimelineSvgDrawer::make_diagram_write_to(self, out);
        return DiagramHelper{DIAGRAM_TYPE_SVG, out.str()};
    } else if (type == "detector-slice-text") {
        if (tick.is_none()) {
            throw std::invalid_argument("You must specify the tick= argument when using type='detector-slice-text'");
        }
        std::stringstream out;
        DetectorSliceSet::from_circuit_tick(self, pybind11::cast<uint64_t>(tick)).write_text_diagram_to(out);
        return DiagramHelper{DIAGRAM_TYPE_TEXT, out.str()};
    } else if (type == "detector-slice-svg") {
        if (tick.is_none()) {
            throw std::invalid_argument("You must specify the tick= argument when using type='detector-slice-svg'");
        }
        std::stringstream out;
        DetectorSliceSet::from_circuit_tick(self, pybind11::cast<uint64_t>(tick)).write_svg_diagram_to(out);
        return DiagramHelper{DIAGRAM_TYPE_SVG, out.str()};
    } else {
        throw std::invalid_argument("Unrecognized diagram type: " + type);
    }
}
