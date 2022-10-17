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

#include "stim/cmd/command_analyze_errors.h"

#include "stim/arg_parse.h"
#include "stim/simulators/error_analyzer.h"

using namespace stim;

int stim::command_analyze_errors(int argc, const char **argv) {
    check_for_unknown_arguments(
        {
            "--allow_gauge_detectors",
            "--approximate_disjoint_errors",
            "--block_decompose_from_introducing_remnant_edges",
            "--decompose_errors",
            "--fold_loops",
            "--ignore_decomposition_failures",
            "--in",
            "--out",
        },
        {"--analyze_errors", "--detector_hypergraph"},
        "analyze_errors",
        argc,
        argv);
    bool decompose_errors = find_bool_argument("--decompose_errors", argc, argv);
    bool fold_loops = find_bool_argument("--fold_loops", argc, argv);
    bool allow_gauge_detectors = find_bool_argument("--allow_gauge_detectors", argc, argv);
    bool ignore_decomposition_failures = find_bool_argument("--ignore_decomposition_failures", argc, argv);
    bool block_decompose_from_introducing_remnant_edges =
        find_bool_argument("--block_decompose_from_introducing_remnant_edges", argc, argv);

    const char *approximate_disjoint_errors_arg = find_argument("--approximate_disjoint_errors", argc, argv);
    float approximate_disjoint_errors_threshold;
    if (approximate_disjoint_errors_arg != nullptr && *approximate_disjoint_errors_arg == '\0') {
        approximate_disjoint_errors_threshold = 1;
    } else {
        approximate_disjoint_errors_threshold =
            find_float_argument("--approximate_disjoint_errors", 0, 0, 1, argc, argv);
    }

    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    auto out_stream = find_output_stream_argument("--out", true, argc, argv);
    std::ostream &out = out_stream.stream();
    auto circuit = Circuit::from_file(in);
    if (in != stdin) {
        fclose(in);
    }
    out << ErrorAnalyzer::circuit_to_detector_error_model(
               circuit,
               decompose_errors,
               fold_loops,
               allow_gauge_detectors,
               approximate_disjoint_errors_threshold,
               ignore_decomposition_failures,
               block_decompose_from_introducing_remnant_edges)
        << "\n";
    return EXIT_SUCCESS;
}
