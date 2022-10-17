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

#include "stim/cmd/command_sample_dem.h"

#include "stim/arg_parse.h"
#include "stim/io/raii_file.h"
#include "stim/probability_util.h"
#include "stim/simulators/dem_sampler.h"

using namespace stim;

int stim::command_sample_dem(int argc, const char **argv) {
    check_for_unknown_arguments(
        {
            "--seed",
            "--shots",
            "--out_format",
            "--out",
            "--in",
            "--obs_out",
            "--obs_out_format",
            "--err_out",
            "--err_out_format",
            "--replay_err_in",
            "--replay_err_in_format",
        },
        {},
        "sample_dem",
        argc,
        argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map, argc, argv);
    const auto &obs_out_format = find_enum_argument("--obs_out_format", "01", format_name_to_enum_map, argc, argv);
    const auto &err_out_format = find_enum_argument("--err_out_format", "01", format_name_to_enum_map, argc, argv);
    const auto &err_in_format = find_enum_argument("--replay_err_in_format", "01", format_name_to_enum_map, argc, argv);
    uint64_t num_shots = find_int64_argument("--shots", 1, 0, INT64_MAX, argc, argv);

    RaiiFile in(find_open_file_argument("--in", stdin, "r", argc, argv));
    RaiiFile out(find_open_file_argument("--out", stdout, "w", argc, argv));
    RaiiFile obs_out(find_open_file_argument("--obs_out", stdout, "w", argc, argv));
    RaiiFile err_out(find_open_file_argument("--err_out", stdout, "w", argc, argv));
    RaiiFile err_in(find_open_file_argument("--replay_err_in", stdin, "r", argc, argv));
    if (obs_out.f == stdout) {
        obs_out.f = nullptr;
    }
    if (err_out.f == stdout) {
        err_out.f = nullptr;
    }
    if (err_in.f == stdin) {
        err_in.f = nullptr;
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

    auto dem = DetectorErrorModel::from_file(in.f);
    in.done();

    DemSampler sampler(std::move(dem), optionally_seeded_rng(argc, argv), 1024);
    sampler.sample_write(
        num_shots,
        out.f,
        out_format.id,
        obs_out.f,
        obs_out_format.id,
        err_out.f,
        err_out_format.id,
        err_in.f,
        err_in_format.id);

    return EXIT_SUCCESS;
}
