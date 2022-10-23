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

#include "command_help.h"
#include "stim/arg_parse.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/diagram/timeline/timeline_3d_drawer.h"
#include "stim/diagram/timeline/timeline_ascii_drawer.h"
#include "stim/diagram/timeline/timeline_svg_drawer.h"
#include "stim/io/raii_file.h"
#include "stim/diagram/graph/match_graph_3d_drawer.h"
#include "stim/diagram/graph/match_graph_svg_drawer.h"
#include "stim/simulators/error_analyzer.h"

using namespace stim;
using namespace stim_draw_internal;

enum DiagramTypes {
    TIMELINE_TEXT,
    TIMELINE_SVG,
    TIMELINE_3D,
    TIMELINE_3D_HTML,
    MATCH_GRAPH_SVG,
    MATCH_GRAPH_3D,
    MATCH_GRAPH_3D_HTML,
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
    RaiiFile in(find_open_file_argument("--in", stdin, "rb", argc, argv));
    auto out_stream = find_output_stream_argument("--out", true, argc, argv);
    auto &out = out_stream.stream();
    int64_t tick = find_int64_argument("--tick", 0, 0, INT64_MAX, argc, argv);
    std::map<std::string, DiagramTypes> diagram_types{
        {"timeline-text", TIMELINE_TEXT},
        {"timeline-svg", TIMELINE_SVG},
        {"timeline-3d", TIMELINE_3D},
        {"timeline-3d-html", TIMELINE_3D_HTML},
        {"match-graph-svg", MATCH_GRAPH_SVG},
        {"match-graph-3d", MATCH_GRAPH_3D},
        {"match-graph-3d-html", MATCH_GRAPH_3D_HTML},
        {"detector-slice-text", DETECTOR_SLICE_TEXT},
        {"detector-slice-svg", DETECTOR_SLICE_SVG},
    };
    DiagramTypes type = find_enum_argument("--type", nullptr, diagram_types, argc, argv);

    auto read_circuit = [&]() {
        auto circuit = Circuit::from_file(in.f);
        in.done();
        if (find_bool_argument("--remove_noise", argc, argv)) {
            circuit = circuit.without_noise();
        }
        return circuit;
    };
    auto read_dem = [&]() {
        std::string content;
        while (true) {
            int c = getc(in.f);
            if (c == EOF) {
                break;
            }
            content.push_back(c);
        }
        in.done();

        try {
            return DetectorErrorModel(content.data());
        } catch (const std::exception &_) {
        }

        auto circuit = Circuit(content.data());
        if (find_bool_argument("--remove_noise", argc, argv)) {
            circuit = circuit.without_noise();
        }
        return ErrorAnalyzer::circuit_to_detector_error_model(
            circuit, true, true, false, 1, true, false);
    };
    switch (type) {
        case TIMELINE_TEXT: {
            auto circuit = read_circuit();
            out << DiagramTimelineAsciiDrawer::make_diagram(circuit);
            break;
        } case TIMELINE_SVG: {
            auto circuit = read_circuit();
            DiagramTimelineSvgDrawer::make_diagram_write_to(circuit, out);
            break;
        } case TIMELINE_3D: {
            auto circuit = read_circuit();
            DiagramTimeline3DDrawer::circuit_to_basic_3d_diagram(circuit).to_gltf_scene().to_json().write(out);
            break;
        } case TIMELINE_3D_HTML: {
            auto circuit = read_circuit();
            std::stringstream tmp_out;
            DiagramTimeline3DDrawer::circuit_to_basic_3d_diagram(circuit).to_gltf_scene().to_json().write(tmp_out);
            write_html_viewer_for_gltf_data(tmp_out.str(), out);
            break;
        } case MATCH_GRAPH_3D: {
            auto dem = read_dem();
            dem_match_graph_to_basic_3d_diagram(dem).to_gltf_scene().to_json().write(out);
            break;
        } case MATCH_GRAPH_3D_HTML: {
            auto dem = read_dem();
            std::stringstream tmp_out;
            dem_match_graph_to_basic_3d_diagram(dem).to_gltf_scene().to_json().write(tmp_out);
            write_html_viewer_for_gltf_data(tmp_out.str(), out);
            break;
        } case MATCH_GRAPH_SVG: {
            auto dem = read_dem();
            dem_match_graph_to_svg_diagram_write_to(dem, out);
            break;
        } case DETECTOR_SLICE_TEXT: {
            if (tick == -1) {
                throw std::invalid_argument("Must specify --tick=# with --type=detector-slice-text");
            }
            auto circuit = read_circuit();
            out << DetectorSliceSet::from_circuit_tick(circuit, (uint64_t)tick);
            break;
        } case DETECTOR_SLICE_SVG: {
            if (tick == -1) {
                throw std::invalid_argument("Must specify --tick=# with --type=detector-slice-svg");
            }
            auto circuit = read_circuit();
            DetectorSliceSet::from_circuit_tick(circuit, (uint64_t)tick).write_svg_diagram_to(out);
            break;
        } default: {
            throw std::invalid_argument("Unknown type");
        }
    }
    out << '\n';

    return EXIT_SUCCESS;
}

SubCommandHelp stim::command_diagram_help() {
    SubCommandHelp result;
    result.subcommand_name = "diagram";
    result.description = "Produces various kinds of diagrams.";

    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> cat example_circuit.stim
            H 0
            CNOT 0 1

            >>> stim diagram \
                --in example_circuit.stim \
                --type timeline-text
            q0: -H-@-
                   |
            q1: ---X-
        )PARAGRAPH"));

    result.flags.push_back(SubCommandHelpFlag{
        "--remove_noise",
        "bool",
        "false",
        {"[none]", "[switch]"},
        clean_doc_string(R"PARAGRAPH(
            Removes noise from the input before turning it into a diagram.

            For example, if the input is a noisy circuit and you aren't
            interested in the details of the noise but rather in the structure
            of the circuit, you can specify this flag in order to filter out
            the noise.
        )PARAGRAPH"),
    });

    result.flags.push_back(SubCommandHelpFlag{
        "--tick",
        "int",
        "none",
        {"[none]", "int"},
        clean_doc_string(R"PARAGRAPH(
            Specifies that the diagram should apply to a specific TICK of the
            input circuit.

            In detector-slice diagrams, `--tick` identifies which TICK is the
            instant at which the time slice is taken. Note that `--tick=0` is
            the very beginning of the circuit and `--tick=1` is the instant of
            the first TICK instruction.
        )PARAGRAPH"),
    });

    result.flags.push_back(SubCommandHelpFlag{
        "--type",
        "name",
        "",
        {"name"},
        clean_doc_string(R"PARAGRAPH(
            The type of diagram to make.

            The available diagram types are:

            "timeline-text": Produces an ASCII text diagram of the operations
                performed by a circuit over time. The qubits are laid out into
                a line top to bottom, and time advances left to right. The input
                object should be a stim circuit.

                INPUT MUST BE A CIRCUIT.

            "timeline-svg": Produces an SVG image diagram of the operations
                performed by a circuit over time. The qubits are laid out into
                a line top to bottom, and time advances left to right. The input
                object should be a stim circuit.

                INPUT MUST BE A CIRCUIT.

            "timeline-3d": Produces a 3d model, in GLTF format, of the
                operations applied by a stim circuit over time.

                GLTF files can be opened with a variety of programs, or
                opened online in viewers such as
                https://gltf-viewer.donmccurdy.com/ .

                INPUT MUST BE A CIRCUIT.

            "timeline-3d-html": A web page containing a 3d model
                viewer of the operations applied by a stim circuit
                over time.

                INPUT MUST BE A CIRCUIT.

            "match-graph-svg": An image of the decoding graph of a detector
                error model. Red lines are errors crossing a logical observable.

                INPUT MUST BE A DETECTOR ERROR MODEL OR A CIRCUIT.

            "match-graph-3d": A 3d model, in GLTF format, of the
                decoding graph of a detector error model. Red lines are
                errors crossing a logical observable.

                GLTF files can be opened with a variety of programs, or
                opened online in viewers such as
                https://gltf-viewer.donmccurdy.com/ .

                INPUT MUST BE A DETECTOR ERROR MODEL OR A CIRCUIT.

            "match-graph-3d-html": A web page containing a 3d model
                viewer of the decoding graph of a detector error
                model or circuit.

                INPUT MUST BE A DETECTOR ERROR MODEL OR A CIRCUIT.

            "detector-slice-text": An ASCII diagram of the stabilizers
                that detectors declared by the circuit correspond to
                during the TICK instruction identified by the `tick`
                argument.

                INPUT MUST BE A CIRCUIT.

            "detector-slice-svg": An SVG image of the stabilizers
                that detectors declared by the circuit correspond to
                during the TICK instruction identified by the `tick`
                argument. For example, a detector slice diagram of a
                CSS surface code circuit during the TICK between a
                measurement layer and a reset layer will produce the
                usual diagram of a surface code. Uses the Pauli color convention
                XYZ=RGB.

                INPUT MUST BE A CIRCUIT.
        )PARAGRAPH"),
    });

    result.flags.push_back(SubCommandHelpFlag{
        "--in",
        "filepath",
        "{stdin}",
        {"[none]", "filepath"},
        clean_doc_string(R"PARAGRAPH(
            Where to read the object to diagram from.

            By default, the object is read from stdin. When `--in $FILEPATH` is
            specified, the object is instead read from the file at $FILEPATH.

            The expected type of object depends on the type of diagram.
        )PARAGRAPH"),
    });

    result.flags.push_back(SubCommandHelpFlag{
        "--out",
        "filepath",
        "{stdout}",
        {"[none]", "filepath"},
        clean_doc_string(R"PARAGRAPH(
            Chooses where to write the diagram to.

            By default, the output is written to stdout. When `--out $FILEPATH`
            is specified, the output is instead written to the file at $FILEPATH.

            The type of output produced depends on the type of diagram.
        )PARAGRAPH"),
    });

    return result;
}
