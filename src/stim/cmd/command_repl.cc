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

#include "stim/arg_parse.h"
#include "stim/probability_util.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

int stim::command_repl(int argc, const char **argv) {
    check_for_unknown_arguments({}, {"--repl"}, "repl", argc, argv);
    auto rng = externally_seeded_rng();
    TableauSimulator::sample_stream(stdin, stdout, SAMPLE_FORMAT_01, true, rng);
    return EXIT_SUCCESS;
}
