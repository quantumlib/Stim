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

#include "stim/arg_parse.h"
#include "stim/simulators/error_matcher.h"

using namespace stim;

int stim::command_explain_errors(int argc, const char **argv) {
    check_for_unknown_arguments({"--dem_filter", "--single", "--out", "--in"}, {}, "explain_errors", argc, argv);

    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "w", argc, argv);
    std::unique_ptr<DetectorErrorModel> dem_filter;
    bool single = find_bool_argument("--single", argc, argv);
    bool has_filter = find_argument("--dem_filter", argc, argv) != nullptr;
    if (has_filter) {
        FILE *filter_file = find_open_file_argument("--dem_filter", stdin, "r", argc, argv);
        dem_filter =
            std::unique_ptr<DetectorErrorModel>(new DetectorErrorModel(DetectorErrorModel::from_file(filter_file)));
        fclose(filter_file);
    }
    auto circuit = Circuit::from_file(in);
    if (in != stdin) {
        fclose(in);
    }
    for (const auto &e : ErrorMatcher::explain_errors_from_circuit(circuit, dem_filter.get(), single)) {
        std::cout << e << "\n";
    }
    if (out != stdout) {
        fclose(out);
    }
    return EXIT_SUCCESS;
}
