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

using namespace stim;

enum DiagramTypes {
    TIMELINE_TEXT,
    TIMELINE_SVG,
};

int stim::command_diagram(int argc, const char **argv) {
    check_for_unknown_arguments(
        {
            "--remove_noise",
            "--type",
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
    std::map<std::string, DiagramTypes> diagram_types{
        {"timeline-text", TIMELINE_TEXT},
        {"timeline-svg", TIMELINE_SVG},
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
        default:
            throw std::invalid_argument("Unknown type");
    }

    return EXIT_SUCCESS;
}
