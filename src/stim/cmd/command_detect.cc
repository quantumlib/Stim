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

#include "command_help.h"
#include "stim/io/raii_file.h"
#include "stim/io/stim_data_formats.h"
#include "stim/simulators/frame_simulator_util.h"
#include "stim/util_bot/arg_parse.h"
#include "stim/util_bot/probability_util.h"

using namespace stim;

int stim::command_detect(int argc, const char **argv) {
    check_for_unknown_arguments(
        {"--seed", "--shots", "--append_observables", "--out_format", "--out", "--in", "--obs_out", "--obs_out_format"},
        {"--detect", "--prepend_observables"},
        "detect",
        argc,
        argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map(), argc, argv);
    const auto &obs_out_format = find_enum_argument("--obs_out_format", "01", format_name_to_enum_map(), argc, argv);
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
    if (out_format.id == SampleFormat::SAMPLE_FORMAT_DETS && !append_observables) {
        prepend_observables = true;
    }

    RaiiFile in(find_open_file_argument("--in", stdin, "rb", argc, argv));
    RaiiFile out(find_open_file_argument("--out", stdout, "wb", argc, argv));
    RaiiFile obs_out(find_open_file_argument("--obs_out", stdout, "wb", argc, argv));
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
    sample_batch_detection_events_writing_results_to_disk<MAX_BITWORD_WIDTH>(
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

SubCommandHelp stim::command_detect_help() {
    SubCommandHelp result;
    result.subcommand_name = "detect";
    result.description = "Sample detection events and observable flips from a circuit.";

    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> cat example.stim
            H 0
            CNOT 0 1
            X_ERROR(0.1) 0 1
            M 0 1
            DETECTOR rec[-1] rec[-2]

            >>> stim detect --shots 5 --in example.stim
            0
            1
            0
            0
            0
        )PARAGRAPH"));
    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> cat example.stim
            # Single-shot X-basis rep code circuit.
            RX 0 1 2 3 4 5 6
            MPP X0*X1 X1*X2 X2*X3 X3*X4 X4*X5 X5*X6
            Z_ERROR(0.1) 0 1 2 3 4 5 6
            MPP X0 X1 X2 X3 X4 X5 X6
            DETECTOR rec[-1] rec[-2] rec[-8]   # X6 X5 now = X5*X6 before
            DETECTOR rec[-2] rec[-3] rec[-9]   # X5 X4 now = X4*X5 before
            DETECTOR rec[-3] rec[-4] rec[-10]  # X4 X3 now = X3*X4 before
            DETECTOR rec[-4] rec[-5] rec[-11]  # X3 X2 now = X2*X3 before
            DETECTOR rec[-5] rec[-6] rec[-12]  # X2 X1 now = X1*X2 before
            DETECTOR rec[-6] rec[-7] rec[-13]  # X1 X0 now = X0*X1 before
            OBSERVABLE_INCLUDE(0) rec[-1]

            >>> stim detect \
                --in example.stim \
                --out_format dets \
                --shots 10
            shot
            shot
            shot L0 D0 D5
            shot D1 D2
            shot
            shot L0 D0
            shot D5
            shot
            shot D3 D4
            shot D0 D1
        )PARAGRAPH"));

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
            Specifies the number of samples to take from the circuit.

            Defaults to 1.
            Must be an integer between 0 and a quintillion (10^18).
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--append_observables",
            "bool",
            "false",
            {"[none]", "[switch]"},
            clean_doc_string(R"PARAGRAPH(
            Appends observable flips to the end of samples as extra detectors.

            PREFER --obs_out OVER THIS FLAG. Mixing the observable flip data
            into detection event data tends to require simply separating them
            again immediately, creating unnecessary work. For example, when
            testing a decoder, you do not want to give the observable flips to
            the decoder because that is the information the decoder is supposed
            to be predicting from the detection events.

            This flag causes observable flip data to be appended to each sample,
            as if the observables were extra detectors at the end of the
            circuit. For example, if there are 100 detectors and 10 observables
            in the circuit, then the output will contain 110 detectors and the
            last 10 are the observables.

            Note that, when using `--out_format dets`, this option is implicitly
            activated but observables are not appended as if they were
            detectors (because `dets` has type hinting information). For
            example, in the example from the last paragraph, the observables
            would be named `L0` through `L9` instead of `D100` through `D109`.
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
            "--in",
            "filepath",
            "{stdin}",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Chooses the stim circuit file to read the circuit to sample from.

            By default, the circuit is read from stdin. When `--in $FILEPATH` is
            specified, the circuit is instead read from the file at $FILEPATH.

            The input should be a stim circuit. See:
            https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md
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
