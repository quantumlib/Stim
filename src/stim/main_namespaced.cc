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
#include "stim/io/raii_file.h"
#include "stim/io/stim_data_formats.h"
#include "stim/probability_util.h"
#include "stim/simulators/dem_sampler.h"
#include "stim/simulators/detection_simulator.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/error_matcher.h"
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

int main_mode_explain_errors(int argc, const char **argv) {
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

int main_mode_detect(int argc, const char **argv) {
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
        simd_bits<MAX_BITWORD_WIDTH> ref(0);
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
            "--sweep",
            "--sweep_format",
            "--obs_out",
            "--obs_out_format",
        },
        {
            "--m2d",
        },
        "m2d",
        argc,
        argv);
    const auto &in_format = find_enum_argument("--in_format", nullptr, format_name_to_enum_map, argc, argv);
    const auto &out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map, argc, argv);
    const auto &sweep_format = find_enum_argument("--sweep_format", "01", format_name_to_enum_map, argc, argv);
    const auto &obs_out_format = find_enum_argument("--obs_out_format", "01", format_name_to_enum_map, argc, argv);
    bool append_observables = find_bool_argument("--append_observables", argc, argv);
    bool skip_reference_sample = find_bool_argument("--skip_reference_sample", argc, argv);
    FILE *circuit_file = find_open_file_argument("--circuit", nullptr, "r", argc, argv);
    auto circuit = Circuit::from_file(circuit_file);
    fclose(circuit_file);

    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "w", argc, argv);
    FILE *sweep_in = find_open_file_argument("--sweep", stdin, "r", argc, argv);
    FILE *obs_out = find_open_file_argument("--obs_out", stdout, "w", argc, argv);
    if (sweep_in == stdin) {
        sweep_in = nullptr;
    }
    if (obs_out == stdout) {
        obs_out = nullptr;
    }

    stream_measurements_to_detection_events(
        in,
        in_format.id,
        sweep_in,
        sweep_format.id,
        out,
        out_format.id,
        circuit,
        append_observables,
        skip_reference_sample,
        obs_out,
        obs_out_format.id);
    if (in != stdin) {
        fclose(in);
    }
    if (sweep_in != nullptr) {
        fclose(sweep_in);
    }
    if (obs_out != nullptr) {
        fclose(obs_out);
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
            "--block_decompose_from_introducing_remnant_edges",
            "--decompose_errors",
            "--fold_loops",
            "--ignore_decomposition_failures",
            "--in",
            "--out",
        },
        {"--analyze_errors", "--detector_hypergraph"},
        "analyze_errors",
        argc,
        argv);
    bool decompose_errors = find_bool_argument("--decompose_errors", argc, argv);
    bool fold_loops = find_bool_argument("--fold_loops", argc, argv);
    bool allow_gauge_detectors = find_bool_argument("--allow_gauge_detectors", argc, argv);
    bool ignore_decomposition_failures = find_bool_argument("--ignore_decomposition_failures", argc, argv);
    bool block_decompose_from_introducing_remnant_edges =
        find_bool_argument("--block_decompose_from_introducing_remnant_edges", argc, argv);

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
               circuit,
               decompose_errors,
               fold_loops,
               allow_gauge_detectors,
               approximate_disjoint_errors_threshold,
               ignore_decomposition_failures,
               block_decompose_from_introducing_remnant_edges)
        << "\n";
    return EXIT_SUCCESS;
}

int main_mode_repl(int argc, const char **argv) {
    check_for_unknown_arguments({}, {"--repl"}, "repl", argc, argv);
    auto rng = externally_seeded_rng();
    TableauSimulator::sample_stream(stdin, stdout, SAMPLE_FORMAT_01, true, rng);
    return EXIT_SUCCESS;
}

int main_mode_sample_dem(int argc, const char **argv) {
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
            return main_help(argc, argv);
        }

        bool mode_repl = is_mode("--repl");
        bool mode_sample = is_mode("--sample");
        bool mode_sample_dem = is_mode("sample_dem");
        bool mode_detect = is_mode("--detect");
        bool mode_analyze_errors = is_mode("--analyze_errors");
        bool mode_gen = is_mode("--gen");
        bool mode_convert = is_mode("--m2d");
        bool mode_explain_errors = is_mode("--explain_errors");
        bool old_mode_detector_hypergraph = find_bool_argument("--detector_hypergraph", argc, argv);
        if (old_mode_detector_hypergraph) {
            std::cerr << "[DEPRECATION] Use `stim analyze_errors` instead of `--detector_hypergraph`\n";
            mode_analyze_errors = true;
        }
        int modes_picked =
            (mode_repl + mode_sample + mode_sample_dem + mode_detect + mode_analyze_errors + mode_gen + mode_convert +
             mode_explain_errors);
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
        if (mode_explain_errors) {
            return main_mode_explain_errors(argc, argv);
        }
        if (mode_sample_dem) {
            return main_mode_sample_dem(argc, argv);
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
