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
#include "gen/circuit_gen_main.h"
#include "probability_util.h"
#include "simulators/detection_simulator.h"
#include "simulators/error_fuser.h"
#include "simulators/frame_simulator.h"
#include "simulators/tableau_simulator.h"

using namespace stim_internal;

static std::vector<const char *> sample_mode_known_arguments{
    "--sample", "--frame0", "--out_format", "--out", "--in",
};
static std::vector<const char *> detect_mode_known_arguments{
    "--detect", "--prepend_observables", "--append_observables", "--out_format", "--out", "--in",
};
static std::vector<const char *> detector_hypergraph_mode_known_arguments{
    "--detector_hypergraph",
    "--basis_analysis",
    "--out",
    "--in",
};
static std::vector<const char *> repl_mode_known_arguments{
    "--repl",
};
static std::vector<const char *> format_names{"01", "b8", "ptb64", "hits", "r8", "dets"};
static std::vector<const char *> basis_analysis_names{"none", "irreducible_per_error"};
static std::vector<SampleFormat> format_values{
    SAMPLE_FORMAT_01, SAMPLE_FORMAT_B8, SAMPLE_FORMAT_PTB64, SAMPLE_FORMAT_HITS, SAMPLE_FORMAT_R8, SAMPLE_FORMAT_DETS,
};

struct RaiiFiles {
    std::vector<FILE *> files;
    ~RaiiFiles() {
        for (auto *f : files) {
            fclose(f);
        }
        files.clear();
    }
};

int stim_internal::main_helper(int argc, const char **argv) {
    if (find_argument("--help", argc, argv) != nullptr) {
        std::cerr << R"HELP(BASIC USAGE
===========
Interactive measurement sampling mode:
    stim --repl

Bulk measurement sampling mode:
    stim --sample[=#shots] [--frame0] [--out_format=01|b8|ptb64|r8|hits|dets] [--in=file] [--out=file]

Detection event sampling mode:
    stim --detect[=#shots] [--out_format=01|b8|ptb64] [--in=file] [--out=file]

Error analysis mode:
    stim --detector_hypergraph [--in=file] [--out=file] [--basis_analysis]

Circuit generation mode:
    stim --gen=repetition_code|surface_code_unrotated|surface_code_rotated \
         --rounds=# \
         --distance=# \
         [--in=file] \
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

    bool mode_interactive = find_bool_argument("--repl", argc, argv);
    bool mode_sampling = find_argument("--sample", argc, argv) != nullptr;
    bool mode_detecting = find_argument("--detect", argc, argv) != nullptr;
    bool mode_detector_hypergraph = find_bool_argument("--detector_hypergraph", argc, argv);
    bool mode_gen = find_argument("--gen", argc, argv) != nullptr;
    if (mode_interactive + mode_sampling + mode_detecting + mode_detector_hypergraph + mode_gen != 1) {
        std::cerr << "\033[31m"
                     "Need to pick a mode by giving exactly one of the following command line arguments:\n"
                     "    --repl: Interactive mode. Eagerly sample measurements in input circuit.\n"
                     "    --sample #: Measurement sampling mode. Bulk sample measurement results from input circuit.\n"
                     "    --detect #: Detector sampling mode. Bulk sample detection events from input circuit.\n"
                     "    --detector_hypergraph: Error analysis mode. Convert circuit into a detector error model.\n"
                     "    --gen: Circuit generation mode. Produce common error correction circuits.\n"
                     "\033[0m";
        return EXIT_FAILURE;
    }

    RaiiFiles raii_files;

    FILE *out = stdout;
    const char *out_path = find_argument("--out", argc, argv);
    if (out_path != nullptr) {
        out = fopen(out_path, "w");
        if (out == nullptr) {
            std::cerr << "\033[31mFailed to open '" << out_path << "' to write.\033[0m";
            return EXIT_FAILURE;
        }
        raii_files.files.push_back(out);
    }

    FILE *in = stdin;
    const char *in_path = find_argument("--in", argc, argv);
    if (in_path != nullptr) {
        in = fopen(in_path, "r");
        if (in == nullptr) {
            std::cerr << "\033[31mFailed to open '" << in_path << "' to read.\033[0m";
            return EXIT_FAILURE;
        }
        raii_files.files.push_back(in);
    }

    std::mt19937_64 rng = externally_seeded_rng();
    if (mode_gen) {
        return main_generate_circuit(argc, argv, out);
    }
    if (mode_interactive) {
        check_for_unknown_arguments(repl_mode_known_arguments, "--repl", argc, argv);
        TableauSimulator::sample_stream(in, out, SAMPLE_FORMAT_01, mode_interactive, rng);
        return EXIT_SUCCESS;
    }
    if (mode_sampling) {
        check_for_unknown_arguments(sample_mode_known_arguments, "--sample", argc, argv);
        SampleFormat out_format =
            format_values[find_enum_argument("--out_format", SAMPLE_FORMAT_01, format_names, argc, argv)];
        bool frame0 = find_bool_argument("--frame0", argc, argv);
        auto num_shots = (size_t)find_int_argument("--sample", 1, 0, 1 << 30, argc, argv);
        if (num_shots == 0) {
            return EXIT_SUCCESS;
        }
        if (num_shots == 1 && !frame0) {
            TableauSimulator::sample_stream(in, out, out_format, false, rng);
            return EXIT_SUCCESS;
        }

        auto circuit = Circuit::from_file(in);
        simd_bits ref(0);
        if (!frame0) {
            ref = TableauSimulator::reference_sample_circuit(circuit);
        }
        FrameSimulator::sample_out(circuit, ref, num_shots, out, out_format, rng);
        return EXIT_SUCCESS;
    }
    if (mode_detecting) {
        check_for_unknown_arguments(detect_mode_known_arguments, "--detect", argc, argv);
        SampleFormat out_format =
            format_values[find_enum_argument("--out_format", 0, format_names, argc, argv)];
        bool prepend_observables = find_bool_argument("--prepend_observables", argc, argv);
        bool append_observables = find_bool_argument("--append_observables", argc, argv);
        auto num_shots = (size_t)find_int_argument("--detect", 1, 0, 1 << 30, argc, argv);
        if (num_shots == 0) {
            return EXIT_SUCCESS;
        }
        if (out_format == SAMPLE_FORMAT_DETS && !append_observables) {
            prepend_observables = true;
        }

        auto circuit = Circuit::from_file(in);
        detector_samples_out(circuit, num_shots, prepend_observables, append_observables, out, out_format, rng);
        return EXIT_SUCCESS;
    }
    if (mode_detector_hypergraph) {
        check_for_unknown_arguments(detector_hypergraph_mode_known_arguments, "--detector_hypergraph", argc, argv);
        bool use_basis_analysis = find_enum_argument("--basis_analysis", 0, basis_analysis_names, argc, argv);
        ErrorFuser::convert_circuit_out(Circuit::from_file(in), out, use_basis_analysis);
        return EXIT_SUCCESS;
    }

    throw std::out_of_range("Mode not handled.");
}
