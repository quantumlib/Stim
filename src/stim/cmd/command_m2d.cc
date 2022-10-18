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

#include "stim/cmd/command_m2d.h"

#include "stim/arg_parse.h"
#include "stim/io/stim_data_formats.h"
#include "stim/simulators/measurements_to_detection_events.h"

using namespace stim;

int stim::command_m2d(int argc, const char **argv) {
    check_for_unknown_arguments(
        {
            "--circuit",
            "--in_format",
            "--append_observables",
            "--out_format",
            "--out",
            "--in",
            "--skip_reference_sample",
            "--sweep",
            "--sweep_format",
            "--obs_out",
            "--obs_out_format",
        },
        {
            "--m2d",
        },
        "m2d",
        argc,
        argv);
    const auto &in_format = find_enum_argument("--in_format", nullptr, format_name_to_enum_map, argc, argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map, argc, argv);
    const auto &sweep_format = find_enum_argument("--sweep_format", "01", format_name_to_enum_map, argc, argv);
    const auto &obs_out_format = find_enum_argument("--obs_out_format", "01", format_name_to_enum_map, argc, argv);
    bool append_observables = find_bool_argument("--append_observables", argc, argv);
    bool skip_reference_sample = find_bool_argument("--skip_reference_sample", argc, argv);
    FILE *circuit_file = find_open_file_argument("--circuit", nullptr, "r", argc, argv);
    auto circuit = Circuit::from_file(circuit_file);
    fclose(circuit_file);

    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "w", argc, argv);
    FILE *sweep_in = find_open_file_argument("--sweep", stdin, "r", argc, argv);
    FILE *obs_out = find_open_file_argument("--obs_out", stdout, "w", argc, argv);
    if (sweep_in == stdin) {
        sweep_in = nullptr;
    }
    if (obs_out == stdout) {
        obs_out = nullptr;
    }

    stream_measurements_to_detection_events(
        in,
        in_format.id,
        sweep_in,
        sweep_format.id,
        out,
        out_format.id,
        circuit,
        append_observables,
        skip_reference_sample,
        obs_out,
        obs_out_format.id);
    if (in != stdin) {
        fclose(in);
    }
    if (sweep_in != nullptr) {
        fclose(sweep_in);
    }
    if (obs_out != nullptr) {
        fclose(obs_out);
    }
    if (out != stdout) {
        fclose(out);
    }
    return EXIT_SUCCESS;
}
