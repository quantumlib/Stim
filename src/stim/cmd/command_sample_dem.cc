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

#include "command_help.h"
#include "stim/io/raii_file.h"
#include "stim/simulators/dem_sampler.h"
#include "stim/util_bot/arg_parse.h"
#include "stim/util_bot/probability_util.h"

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
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map(), argc, argv);
    const auto &obs_out_format = find_enum_argument("--obs_out_format", "01", format_name_to_enum_map(), argc, argv);
    const auto &err_out_format = find_enum_argument("--err_out_format", "01", format_name_to_enum_map(), argc, argv);
    const auto &err_in_format =
        find_enum_argument("--replay_err_in_format", "01", format_name_to_enum_map(), argc, argv);
    uint64_t num_shots = find_int64_argument("--shots", 1, 0, INT64_MAX, argc, argv);

    RaiiFile in(find_open_file_argument("--in", stdin, "rb", argc, argv));
    RaiiFile out(find_open_file_argument("--out", stdout, "wb", argc, argv));
    RaiiFile obs_out(find_open_file_argument("--obs_out", stdout, "wb", argc, argv));
    RaiiFile err_out(find_open_file_argument("--err_out", stdout, "wb", argc, argv));
    RaiiFile err_in(find_open_file_argument("--replay_err_in", stdin, "rb", argc, argv));
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

    DemSampler<MAX_BITWORD_WIDTH> sampler(std::move(dem), optionally_seeded_rng(argc, argv), 1024);
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

SubCommandHelp stim::command_sample_dem_help() {
    SubCommandHelp result;
    result.subcommand_name = "sample_dem";
    result.description = clean_doc_string(R"PARAGRAPH(
        Samples detection events from a detector error model.

        Supports recording and replaying the errors that occurred.
    )PARAGRAPH");

    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> cat example.dem
            error(0) D0
            error(0.5) D1 L0
            error(1) D2 D3

            >>> stim sample_dem \
                --shots 5 \
                --in example.dem \
                --out dets.01 \
                --out_format 01 \
                --obs_out obs_flips.01 \
                --obs_out_format 01

            >>> cat dets.01
            0111
            0011
            0011
            0111
            0111

            >>> cat obs_flips.01
            1
            0
            0
            1
            1
        )PARAGRAPH"));

    result.flags.push_back(
        SubCommandHelpFlag{
            "--replay_err_in",
            "filepath",
            "",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Specifies a file to read error data to replay from.

            When replaying error information, errors are no longer sampled
            randomly but are instead driven by the file data. For example, this
            file data could come from a previous run that wrote error data using
            `--err_out`.

            The input is in a format specified by `--err_in_format`. See:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--replay_err_in_format",
            "01|b8|r8|ptb64|hits|dets",
            "01",
            {"[none]", "format"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the data format to use when reading error data to replay.

            Irrelevant unless `--replay_err_in` is specified.

            The available formats are:

                01 (default): dense human readable
                b8: bit packed binary
                r8: run length binary
                ptb64: partially transposed bit packed binary for SIMD
                hits: sparse human readable
                dets: sparse human readable with type hints

            For a detailed description of each result format, see the result
            format reference:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--err_out",
            "filepath",
            "",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Specifies a file to write a record of which errors occurred.

            For example, the errors that occurred can be analyzed, modified, and
            then given to `--replay_err_in` to see the effects of changes.

            The output is in a format specified by `--err_out_format`. See:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--err_out_format",
            "01|b8|r8|ptb64|hits|dets",
            "01",
            {"[none]", "format"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the data format to use when writing recorded error data.

            Irrelevant unless `--err_out` is specified.

            The available formats are:

                01 (default): dense human readable
                b8: bit packed binary
                r8: run length binary
                ptb64: partially transposed bit packed binary for SIMD
                hits: sparse human readable
                dets: sparse human readable with type hints

            For a detailed description of each result format, see the result
            format reference:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--obs_out",
            "filepath",
            "",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the file to write observable flip data to.

            When sampling detection event data, the goal is typically to predict
            whether or not the logical observables were flipped by using the
            detection events. This argument specifies where to write that
            observable flip data.

            If this argument isn't specified, the observable flip data isn't
            written to a file.

            The output is in a format specified by `--obs_out_format`. See:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--obs_out_format",
            "01|b8|r8|ptb64|hits|dets",
            "01",
            {"[none]", "format"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the data format to use when writing observable flip data.

            Irrelevant unless `--obs_out` is specified.

            The available formats are:

                01 (default): dense human readable
                b8: bit packed binary
                r8: run length binary
                ptb64: partially transposed bit packed binary for SIMD
                hits: sparse human readable
                dets: sparse human readable with type hints

            For a detailed description of each result format, see the result
            format reference:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--out_format",
            "01|b8|r8|ptb64|hits|dets",
            "01",
            {"[none]", "format"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the data format to use when writing output data.

            The available formats are:

                01 (default): dense human readable
                b8: bit packed binary
                r8: run length binary
                ptb64: partially transposed bit packed binary for SIMD
                hits: sparse human readable
                dets: sparse human readable with type hints

            For a detailed description of each result format, see the result
            format reference:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--seed",
            "int",
            "system_entropy",
            {"[none]", "int"},
            clean_doc_string(R"PARAGRAPH(
            Makes simulation results PARTIALLY deterministic.

            The seed integer must be a non-negative 64 bit signed integer.

            When `--seed` isn't specified, the random number generator is seeded
            using fresh entropy requested from the operating system.

            When `--seed #` is set, the exact same simulation results will be
            produced every time ASSUMING:

            - the exact same other flags are specified
            - the exact same version of Stim is being used
            - the exact same machine architecture is being used (for example,
                you're not switching from a machine that has AVX2 instructions
                to one that doesn't).

            CAUTION: simulation results *WILL NOT* be consistent between
            versions of Stim. This restriction is present to make it possible to
            have future optimizations to the random sampling, and is enforced by
            introducing intentional differences in the seeding strategy from
            version to version.

            CAUTION: simulation results *MAY NOT* be consistent across machines.
            For example, using the same seed on a machine that supports AVX
            instructions and one that only supports SSE instructions may produce
            different simulation results.

            CAUTION: simulation results *MAY NOT* be consistent if you vary
            other flags and modes. For example, `--skip_reference_sample` may
            result in fewer calls the to the random number generator before
            reported sampling begins. More generally, using the same seed for
            `stim sample` and `stim detect` will not result in detection events
            corresponding to the measurement results.
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--shots",
            "int",
            "1",
            {"[none]", "int"},
            clean_doc_string(R"PARAGRAPH(
            Specifies the number of samples to take from the detector error model.

            Defaults to 1.
            Must be an integer between 0 and a quintillion (10^18).
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--in",
            "filepath",
            "{stdin}",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Chooses the file to read the detector error model to sample from.

            By default, the detector error model is read from stdin. When
            `--in $FILEPATH` is specified, the detector error model is instead
            read from the file at $FILEPATH.

            The input should be a stim detector error model. See:
            https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--out",
            "filepath",
            "{stdout}",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Chooses where to write the sampled data to.

            By default, the output is written to stdout. When `--out $FILEPATH`
            is specified, the output is instead written to the file at $FILEPATH.

            The output is in a format specified by `--out_format`. See:
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        )PARAGRAPH"),
        });

    return result;
}
