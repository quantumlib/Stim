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

#include "stim/cmd/command_repl.h"

#include "command_help.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/util_bot/arg_parse.h"
#include "stim/util_bot/probability_util.h"

using namespace stim;

int stim::command_repl(int argc, const char **argv) {
    check_for_unknown_arguments({}, {"--repl"}, "repl", argc, argv);
    auto rng = externally_seeded_rng();
    TableauSimulator<MAX_BITWORD_WIDTH>::sample_stream(stdin, stdout, SampleFormat::SAMPLE_FORMAT_01, true, rng);
    return EXIT_SUCCESS;
}

SubCommandHelp stim::command_repl_help() {
    SubCommandHelp result;
    result.subcommand_name = "repl";
    result.description = clean_doc_string(R"PARAGRAPH(
        Runs stim in interactive read-evaluate-print (REPL) mode.

        Reads operations from stdin while immediately writing measurement
        results to stdout.
    )PARAGRAPH");

    result.examples.push_back(clean_doc_string(R"PARAGRAPH(
            >>> stim repl
            ... M 0
            0
            ... X 0
            ... M 0
            1
            ... X 2 3 9
            ... M 0 1 2 3 4 5 6 7 8 9
            1 0 1 1 0 0 0 0 0 1
            ... REPEAT 5 {
            ...     R 0 1
            ...     H 0
            ...     CNOT 0 1
            ...     M 0 1
            ... }
            00
            11
            11
            00
            11
        )PARAGRAPH"));

    return result;
}
