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

#include "stim/cmd/command_sample.h"

#include "stim/arg_parse.h"
#include "stim/io/stim_data_formats.h"
#include "stim/probability_util.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

int stim::command_sample(int argc, const char** argv) {
    check_for_unknown_arguments(
        {"--seed", "--skip_reference_sample", "--out_format", "--out", "--in", "--shots"},
        {"--sample", "--frame0"},
        "sample",
        argc,
        argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map, argc, argv);
    bool skip_reference_sample = find_bool_argument("--skip_reference_sample", argc, argv);
    uint64_t num_shots =
        find_argument("--shots", argc, argv)    ? (uint64_t)find_int64_argument("--shots", 1, 0, INT64_MAX, argc, argv)
        : find_argument("--sample", argc, argv) ? (uint64_t)find_int64_argument("--sample", 1, 0, INT64_MAX, argc, argv)
                                                : 1;
    if (num_shots == 0) {
        return EXIT_SUCCESS;
    }
    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "w", argc, argv);
    auto rng = optionally_seeded_rng(argc, argv);
    bool deprecated_frame0 = find_bool_argument("--frame0", argc, argv);
    if (deprecated_frame0) {
        std::cerr << "[DEPRECATION] Use `--skip_reference_sample` instead of `--frame0`\n";
        skip_reference_sample = true;
    }

    if (num_shots == 1 && !skip_reference_sample) {
        TableauSimulator::sample_stream(in, out, out_format.id, false, rng);
    } else if (num_shots > 0) {
        auto circuit = Circuit::from_file(in);
        simd_bits<MAX_BITWORD_WIDTH> ref(0);
        if (!skip_reference_sample) {
            ref = TableauSimulator::reference_sample_circuit(circuit);
        }
        FrameSimulator::sample_out(circuit, ref, num_shots, out, out_format.id, rng);
    }

    if (in != stdin) {
        fclose(in);
    }
    if (out != stdout) {
        fclose(out);
    }
    return EXIT_SUCCESS;
}
