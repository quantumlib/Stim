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

#include "stim/cmd/command_explain_errors.h"

#include "command_help.h"
#include "stim/simulators/error_matcher.h"
#include "stim/util_bot/arg_parse.h"

using namespace stim;

int stim::command_explain_errors(int argc, const char **argv) {
    check_for_unknown_arguments({"--dem_filter", "--single", "--out", "--in"}, {}, "explain_errors", argc, argv);

    FILE *in = find_open_file_argument("--in", stdin, "rb", argc, argv);
    auto out_stream = find_output_stream_argument("--out", true, argc, argv);
    std::unique_ptr<DetectorErrorModel> dem_filter;
    bool single = find_bool_argument("--single", argc, argv);
    bool has_filter = find_argument("--dem_filter", argc, argv) != nullptr;
    if (has_filter) {
        FILE *filter_file = find_open_file_argument("--dem_filter", stdin, "rb", argc, argv);
        dem_filter =
            std::unique_ptr<DetectorErrorModel>(new DetectorErrorModel(DetectorErrorModel::from_file(filter_file)));
        fclose(filter_file);
    }
    auto circuit = Circuit::from_file(in);
    if (in != stdin) {
        fclose(in);
    }
    for (const auto &e : ErrorMatcher::explain_errors_from_circuit(circuit, dem_filter.get(), single)) {
        out_stream.stream() << e << "\n";
    }
    return EXIT_SUCCESS;
}

SubCommandHelp stim::command_explain_errors_help() {
    SubCommandHelp result;
    result.subcommand_name = "explain_errors";
    result.description = clean_doc_string(R"PARAGRAPH(
        Find circuit errors that produce certain detection events.

        Note that this command does not attempt to explain detection events
        by using multiple errors. This command can only tell you how to
        produce a set of detection events if they correspond to a specific
        single physical error annotated into the circuit.

        If you need to explain a detection event set using multiple errors,
        use a decoder such as pymatching to find the set of single detector
        error model errors that are needed and then use this command to
        convert those specific errors into circuit errors.
    )PARAGRAPH");

    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> stim gen \
                --code surface_code \
                --task rotated_memory_z \
                --distance 5 \
                --rounds 10 \
                --after_clifford_depolarization 0.001 \
                > example.stim
            >>> echo "error(1) D97 D102" > example.dem

            >>> stim explain_errors \
                --single \
                --in example.stim \
                --dem_filter example.dem
            ExplainedError {
                dem_error_terms: D97[coords 4,6,4] D102[coords 2,8,4]
                CircuitErrorLocation {
                    flipped_pauli_product: Z36[coords 3,7]
                    Circuit location stack trace:
                        (after 25 TICKs)
                        at instruction #83 (a REPEAT 9 block) in the circuit
                        after 2 completed iterations
                        at instruction #12 (DEPOLARIZE2) in the REPEAT block
                        at targets #3 to #4 of the instruction
                        resolving to DEPOLARIZE2(0.001) 46[coords 2,8] 36[coords 3,7]
                }
            }
        )PARAGRAPH"));

    result.flags.push_back(
        SubCommandHelpFlag{
            "--dem_filter",
            "filepath",
            "01",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Specifies a detector error model to use as a filter.

            If `--dem_filter` isn't specified, an explanation of every single
            set of symptoms that can be produced by the circuit.

            If `--dem_filter` is specified, only explanations of the error
            mechanisms present in the filter will be output. This is useful when
            you are interested in a specific set of detection events.

            The filter is specified as a detector error model file. See
            https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--single",
            "bool",
            "false",
            {"[none]", "[switch]"},
            clean_doc_string(R"PARAGRAPH(
            Explain using a single simple error instead of all possible errors.

            When `--single` isn't specified, every single circuit error that
            produces a specific detector error model is output as a possible
            explanation of that error.

            When `--single` is specified, only the simplest circuit error is
            output. The "simplest" error is chosen by using heuristics such as
            "has fewer Pauli terms" and "happens earlier".
        )PARAGRAPH"),
        });

    result.flags.push_back(
        SubCommandHelpFlag{
            "--in",
            "filepath",
            "{stdin}",
            {"[none]", "filepath"},
            clean_doc_string(R"PARAGRAPH(
            Chooses the stim circuit file to read the explanatory circuit from.

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
            Chooses where to write the explanations to.

            By default, the output is written to stdout. When `--out $FILEPATH`
            is specified, the output is instead written to the file at $FILEPATH.

            The output is in an arbitrary semi-human-readable format.
        )PARAGRAPH"),
        });

    return result;
}
