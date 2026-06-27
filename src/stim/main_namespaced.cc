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

#include "stim/main_namespaced.h"

#include <cstring>

#include "stim/cmd/command_analyze_errors.h"
#include "stim/cmd/command_convert.h"
#include "stim/cmd/command_detect.h"
#include "stim/cmd/command_diagram.h"
#include "stim/cmd/command_explain_errors.h"
#include "stim/cmd/command_gen.h"
#include "stim/cmd/command_help.h"
#include "stim/cmd/command_m2d.h"
#include "stim/cmd/command_repl.h"
#include "stim/cmd/command_sample.h"
#include "stim/cmd/command_sample_dem.h"
#include "stim/util_bot/arg_parse.h"

using namespace stim;

int stim::main(int argc, const char **argv) {
    try {
        const char *mode = argc > 1 ? argv[1] : "";
        if (mode[0] == '-') {
            mode = "";
        }
        auto is_mode = [&](const char *name) {
            if (name[0] == '-') {
                return find_argument(name, argc, argv) != nullptr || strcmp(mode, name + 2) == 0;
            }
            return strcmp(mode, name) == 0;
        };

        if (is_mode("--help")) {
            return command_help(argc, argv);
        }

        bool mode_repl = is_mode("--repl");
        bool mode_sample = is_mode("--sample");
        bool mode_sample_dem = is_mode("sample_dem");
        bool mode_diagram = is_mode("diagram");
        bool mode_detect = is_mode("--detect");
        bool mode_analyze_errors = is_mode("--analyze_errors");
        bool mode_gen = is_mode("--gen");
        bool mode_m2d = is_mode("--m2d");
        bool mode_explain_errors = is_mode("--explain_errors");
        bool old_mode_detector_hypergraph = find_bool_argument("--detector_hypergraph", argc, argv);
        if (old_mode_detector_hypergraph) {
            std::cerr << "[DEPRECATION] Use `stim analyze_errors` instead of `--detector_hypergraph`\n";
            mode_analyze_errors = true;
        }
        bool mode_convert = is_mode("--convert");
        int modes_picked =
            (mode_repl + mode_sample + mode_sample_dem + mode_detect + mode_analyze_errors + mode_gen + mode_m2d +
             mode_explain_errors + mode_diagram + mode_convert);
        if (modes_picked != 1) {
            std::cerr << "\033[31m";
            if (modes_picked > 1) {
                std::cerr << "More than one mode was specified.\n\n";
            } else {
                std::cerr << "No mode was given.\n\n";
            }
            std::cerr << help_for("");
            std::cerr << "\033[0m";
            return EXIT_FAILURE;
        }

        if (mode_gen) {
            return command_gen(argc, argv);
        }
        if (mode_repl) {
            return command_repl(argc, argv);
        }
        if (mode_sample) {
            return command_sample(argc, argv);
        }
        if (mode_detect) {
            return command_detect(argc, argv);
        }
        if (mode_analyze_errors) {
            return command_analyze_errors(argc, argv);
        }
        if (mode_m2d) {
            return command_m2d(argc, argv);
        }
        if (mode_explain_errors) {
            return command_explain_errors(argc, argv);
        }
        if (mode_sample_dem) {
            return command_sample_dem(argc, argv);
        }
        if (mode_diagram) {
            return command_diagram(argc, argv);
        }
        if (mode_convert) {
            return command_convert(argc, argv);
        }

        throw std::out_of_range("Mode not handled.");
    } catch (const std::invalid_argument &ex) {
        std::string_view s = ex.what();
        std::cerr << "\033[31m";
        std::cerr << s;
        if (s.empty() || s.back() != '\n') {
            std::cerr << '\n';
        }
        std::cerr << "\033[0m";
        return EXIT_FAILURE;
    }
}
