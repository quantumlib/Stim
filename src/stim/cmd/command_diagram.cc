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

#include "stim/cmd/command_diagram.h"

#include "stim/arg_parse.h"
#include "stim/io/raii_file.h"
#include "stim/diagram/timeline/timeline_ascii_drawer.h"
#include "stim/diagram/timeline/timeline_svg_drawer.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/diagram/timeline_3d/diagram_3d.h"

using namespace stim;

enum DiagramTypes {
    TIMELINE_TEXT,
    TIMELINE_SVG,
    TIMELINE_3D,
    DETECTOR_SLICE_TEXT,
    DETECTOR_SLICE_SVG,
};

int stim::command_diagram(int argc, const char **argv) {
    check_for_unknown_arguments(
        {
            "--remove_noise",
            "--type",
            "--tick",
            "--in",
            "--out",
        },
        {},
        "diagram",
        argc,
        argv);
    RaiiFile in(find_open_file_argument("--in", stdin, "r", argc, argv));
    auto out_stream = find_output_stream_argument("--out", true, argc, argv);
    auto &out = out_stream.stream();
    int64_t tick = find_int64_argument("--tick", 0, 0, INT64_MAX, argc, argv);
    std::map<std::string, DiagramTypes> diagram_types{
        {"timeline-text", TIMELINE_TEXT},
        {"timeline-svg", TIMELINE_SVG},
        {"timeline-3d", TIMELINE_3D},
        {"detector-slice-text", DETECTOR_SLICE_TEXT},
        {"detector-slice-svg", DETECTOR_SLICE_SVG},
    };
    DiagramTypes type = find_enum_argument("--type", nullptr, diagram_types, argc, argv);

    auto circuit = Circuit::from_file(in.f);
    in.done();
    if (find_bool_argument("--remove_noise", argc, argv)) {
        circuit = circuit.without_noise();
    }
    switch (type) {
        case TIMELINE_TEXT:
            out << stim_draw_internal::DiagramTimelineAsciiDrawer::make_diagram(circuit);
            break;
        case TIMELINE_SVG:
            stim_draw_internal::DiagramTimelineSvgDrawer::make_diagram_write_to(circuit, out);
            break;
        case TIMELINE_3D: {
            out << stim_draw_internal::scene_from_circuit(circuit).to_json();
            break;
        } case DETECTOR_SLICE_TEXT:
            out << stim_draw_internal::DetectorSliceSet::from_circuit_tick(circuit, (uint64_t)tick);
            break;
        case DETECTOR_SLICE_SVG:
            stim_draw_internal::DetectorSliceSet::from_circuit_tick(circuit, (uint64_t)tick).write_svg_diagram_to(out);
            break;
        default:
            throw std::invalid_argument("Unknown type");
    }

    return EXIT_SUCCESS;
}
