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

#include "main_helper.h"

#include "arg_parse.h"
#include "gate_help.h"
#include "gen/circuit_gen_main.h"
#include "probability_util.h"
#include "simulators/detection_simulator.h"
#include "simulators/error_fuser.h"
#include "simulators/frame_simulator.h"
#include "simulators/tableau_simulator.h"

using namespace stim_internal;

static std::map<std::string, SampleFormat> format_name_to_enum_map{
    {"01", SAMPLE_FORMAT_01},
    {"b8", SAMPLE_FORMAT_B8},
    {"ptb64", SAMPLE_FORMAT_PTB64},
    {"hits", SAMPLE_FORMAT_HITS},
    {"r8", SAMPLE_FORMAT_R8},
    {"dets", SAMPLE_FORMAT_DETS},
};

int main_mode_detect(int argc, const char **argv) {
    check_for_unknown_arguments(
        {"--detect", "--prepend_observables", "--append_observables", "--out_format", "--out", "--in"},
        "--detect",
        argc,
        argv);
    SampleFormat out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map, argc, argv);
    bool prepend_observables = find_bool_argument("--prepend_observables", argc, argv);
    bool append_observables = find_bool_argument("--append_observables", argc, argv);
    uint64_t num_shots = (uint64_t)find_int64_argument("--detect", 1, 0, INT64_MAX, argc, argv);
    if (num_shots == 0) {
        return EXIT_SUCCESS;
    }
    if (out_format == SAMPLE_FORMAT_DETS && !append_observables) {
        prepend_observables = true;
    }

    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "w", argc, argv);
    auto circuit = Circuit::from_file(in);
    if (in != stdin) {
        fclose(in);
    }
    auto rng = externally_seeded_rng();
    detector_samples_out(circuit, num_shots, prepend_observables, append_observables, out, out_format, rng);
    if (out != stdout) {
        fclose(out);
    }
    return EXIT_SUCCESS;
}

int main_mode_sample(int argc, const char **argv) {
    check_for_unknown_arguments({"--sample", "--frame0", "--out_format", "--out", "--in"}, "--sample", argc, argv);
    SampleFormat out_format = find_enum_argument("--out_format", "01", format_name_to_enum_map, argc, argv);
    bool frame0 = find_bool_argument("--frame0", argc, argv);
    uint64_t num_shots = (uint64_t)find_int64_argument("--sample", 1, 0, INT64_MAX, argc, argv);
    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    FILE *out = find_open_file_argument("--out", stdout, "w", argc, argv);
    auto rng = externally_seeded_rng();

    if (num_shots == 1 && !frame0) {
        TableauSimulator::sample_stream(in, out, out_format, false, rng);
    } else if (num_shots > 0) {
        auto circuit = Circuit::from_file(in);
        simd_bits ref(0);
        if (!frame0) {
            ref = TableauSimulator::reference_sample_circuit(circuit);
        }
        FrameSimulator::sample_out(circuit, ref, num_shots, out, out_format, rng);
    }

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
            "--analyze_errors",
            "--allow_gauge_detectors",
            "--detector_hypergraph",
            "--find_reducible_errors",
            "--fold_loops",
            "--out",
            "--in",
        },
        "--analyze_errors",
        argc,
        argv);
    bool find_reducible_errors = find_bool_argument("--find_reducible_errors", argc, argv);
    bool fold_loops = find_bool_argument("--fold_loops", argc, argv);
    bool validate_detectors = !find_bool_argument("--allow_gauge_detectors", argc, argv);
    FILE *in = find_open_file_argument("--in", stdin, "r", argc, argv);
    auto out_stream = find_output_stream_argument("--out", true, argc, argv);
    std::ostream &out = out_stream.stream();
    auto circuit = Circuit::from_file(in);
    if (in != stdin) {
        fclose(in);
    }
    out << ErrorFuser::circuit_to_detector_error_model(circuit, find_reducible_errors, fold_loops, validate_detectors);
    return EXIT_SUCCESS;
}

int main_mode_repl(int argc, const char **argv) {
    check_for_unknown_arguments({"--repl"}, "--repl", argc, argv);
    auto rng = externally_seeded_rng();
    TableauSimulator::sample_stream(stdin, stdout, SAMPLE_FORMAT_01, true, rng);
    return EXIT_SUCCESS;
}

int stim_internal::main_helper(int argc, const char **argv) {
    const char *help = find_argument("--help", argc, argv);
    if (help != nullptr) {
        auto m = generate_gate_help_markdown();
        auto key = std::string(help);
        for (auto &c : key) {
            c = toupper(c);
        }
        auto p = m.find(key);
        if (p != m.end()) {
            std::cout << p->second;
            return EXIT_SUCCESS;
        } else if (help[0] != '\0') {
            std::cerr << "Unrecognized help topic '" << help << "'.\n";
            return EXIT_FAILURE;
        }
        std::cout << R"HELP(BASIC USAGE
===========
Gate reference:
    stim --help gates
    stim --help [gate_name]

Interactive measurement sampling mode:
    stim --repl

Bulk measurement sampling mode:
    stim --sample[=#shots] \
         [--frame0] \
         [--out_format=01|b8|ptb64|r8|hits|dets] \
         [--in=file] \
         [--out=file]

Detection event sampling mode:
    stim --detect[=#shots] \
         [--out_format=01|b8|ptb64|r8|hits|dets] \
         [--in=file] \
         [--out=file]

Error analysis mode:
    stim --analyze_errors \
         [--find_reducible_errors] \
         [--in=file] \
         [--out=file]

Circuit generation mode:
    stim --gen=repetition_code|surface_code|color_code \
         --rounds=# \
         --distance=# \
         --task=... \
         [--out=file] \
         [--after_clifford_depolarization=0] \
         [--after_reset_flip_probability=0] \
         [--before_measure_flip_probability=0] \
         [--before_round_data_depolarization=0]

EXAMPLE CIRCUIT (GHZ)
=====================
H 0
CNOT 0 1
CNOT 0 2
M 0 1 2
)HELP";
        return EXIT_SUCCESS;
    }

    bool mode_repl = find_bool_argument("--repl", argc, argv);
    bool mode_sample = find_argument("--sample", argc, argv) != nullptr;
    bool mode_detect = find_argument("--detect", argc, argv) != nullptr;
    bool mode_analyze_errors = find_bool_argument("--analyze_errors", argc, argv);
    bool mode_gen = find_argument("--gen", argc, argv) != nullptr;
    bool old_mode_detector_hypergraph = find_bool_argument("--detector_hypergraph", argc, argv);
    if (old_mode_detector_hypergraph) {
        std::cerr << "[DEPRECATION] Use `--analyze_errors` instead of `--detector_hypergraph`\n";
        mode_analyze_errors = true;
    }
    if (mode_repl + mode_sample + mode_detect + mode_analyze_errors + mode_gen != 1) {
        std::cerr << "\033[31m"
                     "Need to pick a mode by giving exactly one of the following command line arguments:\n"
                     "    --repl: Interactive mode. Eagerly sample measurements in input circuit.\n"
                     "    --sample #: Measurement sampling mode. Bulk sample measurement results from input circuit.\n"
                     "    --detect #: Detector sampling mode. Bulk sample detection events from input circuit.\n"
                     "    --analyze_errors: Error analysis mode. Convert circuit into a detector error model.\n"
                     "    --gen: Circuit generation mode. Produce common error correction circuits.\n"
                     "\033[0m";
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

    throw std::out_of_range("Mode not handled.");
}
