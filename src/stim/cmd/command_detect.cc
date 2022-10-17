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

#include "stim/cmd/command_detect.h"

#include "stim/arg_parse.h"
#include "stim/io/raii_file.h"
#include "stim/io/stim_data_formats.h"
#include "stim/probability_util.h"
#include "stim/simulators/detection_simulator.h"

using namespace stim;

int stim::command_detect(int argc, const char **argv) {
    check_for_unknown_arguments(
        {"--seed", "--shots", "--append_observables", "--out_format", "--out", "--in", "--obs_out", "--obs_out_format"},
        {"--detect", "--prepend_observables"},
        "detect",
        argc,
        argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map, argc, argv);
    const auto &obs_out_format = find_enum_argument("--obs_out_format", "01", format_name_to_enum_map, argc, argv);
    bool prepend_observables = find_bool_argument("--prepend_observables", argc, argv);
    if (prepend_observables) {
        std::cerr << "[DEPRECATION] Avoid using `--prepend_observables`. Data readers assume observables are appended, "
                     "not prepended.\n";
    }
    bool append_observables = find_bool_argument("--append_observables", argc, argv);
    uint64_t num_shots =
        find_argument("--shots", argc, argv)    ? (uint64_t)find_int64_argument("--shots", 1, 0, INT64_MAX, argc, argv)
        : find_argument("--detect", argc, argv) ? (uint64_t)find_int64_argument("--detect", 1, 0, INT64_MAX, argc, argv)
                                                : 1;
    if (out_format.id == SAMPLE_FORMAT_DETS && !append_observables) {
        prepend_observables = true;
    }

    RaiiFile in(find_open_file_argument("--in", stdin, "r", argc, argv));
    RaiiFile out(find_open_file_argument("--out", stdout, "w", argc, argv));
    RaiiFile obs_out(find_open_file_argument("--obs_out", stdout, "w", argc, argv));
    if (obs_out.f == stdout) {
        obs_out.f = nullptr;
    }
    if (out.f == stdout) {
        out.responsible_for_closing = false;
    }
    if (in.f == stdin) {
        out.responsible_for_closing = false;
    }
    if (num_shots == 0) {
        return EXIT_SUCCESS;
    }

    auto circuit = Circuit::from_file(in.f);
    in.done();
    auto rng = optionally_seeded_rng(argc, argv);
    detector_samples_out(
        circuit,
        num_shots,
        prepend_observables,
        append_observables,
        out.f,
        out_format.id,
        rng,
        obs_out.f,
        obs_out_format.id);
    return EXIT_SUCCESS;
}
