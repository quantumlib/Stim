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

#include "stim/arg_parse.h"
#include "stim/gen/circuit_gen_main.h"
#include "stim/help.h"
#include "stim/io/stim_data_formats.h"
#include "stim/probability_util.h"
#include "stim/simulators/detection_simulator.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/measurements_to_detection_events.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

std::mt19937_64 optionally_seeded_rng(int argc, const char **argv) {
    if (find_argument("--seed", argc, argv) == nullptr) {
        return externally_seeded_rng();
    }
    uint64_t seed = (uint64_t)find_int64_argument("--seed", 0, 0, INT64_MAX, argc, argv);
    return std::mt19937_64(seed ^ INTENTIONAL_VERSION_SEED_INCOMPATIBILITY);
}

int main_mode_detect(int argc, const char **argv) {
    check_for_unknown_arguments(
        {"--seed", "--shots", "--append_observables", "--out_format", "--out", "--in"},
        {"--detect", "--prepend_observables"},
        "detect",
        argc,
        argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map, argc, argv);
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
    if (num_shots == 0) {
        return EXIT_SUCCESS;
    }
    if (out_format.id == SAMPLE_FORMAT_DETS && !append_observables) {
        prepend_observables = true;
    }

    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "w", argc, argv);
    auto circuit = Circuit::from_file(in);
    if (in != stdin) {
        fclose(in);
    }
    auto rng = optionally_seeded_rng(argc, argv);
    detector_samples_out(circuit, num_shots, prepend_observables, append_observables, out, out_format.id, rng);
    if (out != stdout) {
        fclose(out);
    }
    return EXIT_SUCCESS;
}

int main_mode_sample(int argc, const char **argv) {
    check_for_unknown_arguments(
        {"--seed", "--skip_reference_sample", "--out_format", "--out", "--in", "--shots"},
        {"--sample", "--frame0"},
        "sample",
        argc,
        argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map, argc, argv);
    bool skip_reference_sample = find_bool_argument("--skip_reference_sample", argc, argv);
    uint64_t num_shots =
        find_argument("--shots", argc, argv)    ? (uint64_t)find_int64_argument("--shots", 1, 0, INT64_MAX, argc, argv)
        : find_argument("--sample", argc, argv) ? (uint64_t)find_int64_argument("--sample", 1, 0, INT64_MAX, argc, argv)
                                                : 1;
    if (num_shots == 0) {
        return EXIT_SUCCESS;
    }
    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "w", argc, argv);
    auto rng = optionally_seeded_rng(argc, argv);
    bool deprecated_frame0 = find_bool_argument("--frame0", argc, argv);
    if (deprecated_frame0) {
        std::cerr << "[DEPRECATION] Use `--skip_reference_sample` instead of `--frame0`\n";
        skip_reference_sample = true;
    }

    if (num_shots == 1 && !skip_reference_sample) {
        TableauSimulator::sample_stream(in, out, out_format.id, false, rng);
    } else if (num_shots > 0) {
        auto circuit = Circuit::from_file(in);
        simd_bits ref(0);
        if (!skip_reference_sample) {
            ref = TableauSimulator::reference_sample_circuit(circuit);
        }
        FrameSimulator::sample_out(circuit, ref, num_shots, out, out_format.id, rng);
    }

    if (in != stdin) {
        fclose(in);
    }
    if (out != stdout) {
        fclose(out);
    }
    return EXIT_SUCCESS;
}

int main_mode_measurements_to_detections(int argc, const char **argv) {
    check_for_unknown_arguments(
        {
            "--circuit",
            "--in_format",
            "--append_observables",
            "--out_format",
            "--out",
            "--in",
            "--skip_reference_sample",
        },
        {
            "--m2d",
            // Not deprecated but still experimental:
            "--sweep_data_in_format",
            "--sweep_data_in",
        },
        "m2d",
        argc,
        argv);
    const auto &in_format = find_enum_argument("--in_format", nullptr, format_name_to_enum_map, argc, argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map, argc, argv);
    const auto &frame_in_format =
        find_enum_argument("--sweep_data_in_format", "01", format_name_to_enum_map, argc, argv);
    bool append_observables = find_bool_argument("--append_observables", argc, argv);
    bool skip_reference_sample = find_bool_argument("--skip_reference_sample", argc, argv);
    FILE *circuit_file = find_open_file_argument("--circuit", nullptr, "r", argc, argv);
    auto circuit = Circuit::from_file(circuit_file);
    fclose(circuit_file);

    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "w", argc, argv);
    FILE *frame_in = find_open_file_argument("--sweep_data_in", stdin, "r", argc, argv);
    if (frame_in == stdin) {
        frame_in = nullptr;
    }

    stream_measurements_to_detection_events(
        in,
        in_format.id,
        frame_in,
        frame_in_format.id,
        out,
        out_format.id,
        circuit,
        append_observables,
        skip_reference_sample);
    if (in != stdin) {
        fclose(in);
    }
    if (out != stdout) {
        fclose(out);
    }
    return EXIT_SUCCESS;
}

int main_mode_analyze_errors(int argc, const char **argv) {
    check_for_unknown_arguments(
        {
            "--allow_gauge_detectors",
            "--approximate_disjoint_errors",
            "--decompose_errors",
            "--fold_loops",
            "--out",
            "--in",
        },
        {"--analyze_errors", "--detector_hypergraph"},
        "analyze_errors",
        argc,
        argv);
    bool decompose_errors = find_bool_argument("--decompose_errors", argc, argv);
    bool fold_loops = find_bool_argument("--fold_loops", argc, argv);
    bool allow_gauge_detectors = find_bool_argument("--allow_gauge_detectors", argc, argv);

    const char *approximate_disjoint_errors_arg = find_argument("--approximate_disjoint_errors", argc, argv);
    float approximate_disjoint_errors_threshold = 0;
    if (approximate_disjoint_errors_arg != nullptr && *approximate_disjoint_errors_arg == '\0') {
        approximate_disjoint_errors_threshold = 1;
    } else {
        approximate_disjoint_errors_threshold =
            find_float_argument("--approximate_disjoint_errors", 0, 0, 1, argc, argv);
    }

    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    auto out_stream = find_output_stream_argument("--out", true, argc, argv);
    std::ostream &out = out_stream.stream();
    auto circuit = Circuit::from_file(in);
    if (in != stdin) {
        fclose(in);
    }
    out << ErrorAnalyzer::circuit_to_detector_error_model(
               circuit, decompose_errors, fold_loops, allow_gauge_detectors, approximate_disjoint_errors_threshold)
        << "\n";
    return EXIT_SUCCESS;
}

int main_mode_repl(int argc, const char **argv) {
    check_for_unknown_arguments({}, {"--repl"}, "repl", argc, argv);
    auto rng = externally_seeded_rng();
    TableauSimulator::sample_stream(stdin, stdout, SAMPLE_FORMAT_01, true, rng);
    return EXIT_SUCCESS;
}

int stim::main(int argc, const char **argv) {
    try {
        const char *mode = argc > 1 ? argv[1] : "";
        if (mode[0] == '-') {
            mode = "";
        }
        auto is_mode = [&](const char *name) {
            return find_argument(name, argc, argv) != nullptr || strcmp(mode, name + 2) == 0;
        };

        if (is_mode("--help")) {
            return main_help(argc, argv);
        }

        bool mode_repl = is_mode("--repl");
        bool mode_sample = is_mode("--sample");
        bool mode_detect = is_mode("--detect");
        bool mode_analyze_errors = is_mode("--analyze_errors");
        bool mode_gen = is_mode("--gen");
        bool mode_convert = is_mode("--m2d");
        bool old_mode_detector_hypergraph = find_bool_argument("--detector_hypergraph", argc, argv);
        if (old_mode_detector_hypergraph) {
            std::cerr << "[DEPRECATION] Use `stim analyze_errors` instead of `--detector_hypergraph`\n";
            mode_analyze_errors = true;
        }
        int modes_picked = mode_repl + mode_sample + mode_detect + mode_analyze_errors + mode_gen + mode_convert;
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
            return main_generate_circuit(argc, argv);
        }
        if (mode_repl) {
            return main_mode_repl(argc, argv);
        }
        if (mode_sample) {
            return main_mode_sample(argc, argv);
        }
        if (mode_detect) {
            return main_mode_detect(argc, argv);
        }
        if (mode_analyze_errors) {
            return main_mode_analyze_errors(argc, argv);
        }
        if (mode_convert) {
            return main_mode_measurements_to_detections(argc, argv);
        }

        throw std::out_of_range("Mode not handled.");
    } catch (const std::invalid_argument &ex) {
        const std::string &s = ex.what();
        std::cerr << "\033[31m";
        std::cerr << s;
        if (s.empty() || s.back() != '\n') {
            std::cerr << '\n';
        }
        std::cerr << "\033[0m";
        return EXIT_FAILURE;
    }
}
