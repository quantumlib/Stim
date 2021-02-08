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
#include "circuit/common_circuits.h"
#include "probability_util.h"
#include "simulators/detection_simulator.h"
#include "simulators/frame_simulator.h"
#include "simulators/tableau_simulator.h"

static std::vector<const char *> known_arguments{
    "--help",

    "--repl",
    "--sample",
    "--detect",
    "--make_circuit",

    "--append_observables",
    "--prepend_observables",
    "--distance",
    "--frame0",
    "--in",
    "--noise_level",
    "--out",
    "--out_format",
    "--rounds",
};
static std::vector<const char *> sample_mode_known_arguments{
    "--sample", "--frame0", "--out_format", "--out", "--in",
};
static std::vector<const char *> detect_mode_known_arguments{
    "--detect", "--append_observables", "--prepend_observables", "--out_format", "--out", "--in",
};
static std::vector<const char *> repl_mode_known_arguments{
    "--repl",
};
static std::vector<const char *> make_circuit_mode_known_arguments{
    "--make_circuit", "--out", "--distance", "--rounds", "--noise_level",
};
static std::vector<const char *> format_names{
    "01",
    "b8",
    "ptb64",
    "hits",
};
static std::vector<const char *> circuit_names{
    "surface_unrotated",
};
static std::vector<SampleFormat> format_values{
    SAMPLE_FORMAT_01,
    SAMPLE_FORMAT_B8,
    SAMPLE_FORMAT_PTB64,
    SAMPLE_FORMAT_HITS,
};

struct RaiiFiles {
    std::vector<FILE *> files;
    ~RaiiFiles() {
        for (auto f : files) {
            fclose(f);
        }
        files.clear();
    }
};

int main_helper(int argc, const char **argv) {
    if (find_argument("--help", argc, argv) != nullptr) {
        std::cerr << R"HELP(BASIC USAGE
===========
Interactive measurement sampling mode:
    stim --repl

Measurement sampling mode:
    stim --sample[=#samples] [--frame0] [--out_format=01|b8|ptb64] [--in=file] [--out=file]

Detection event sampling mode:
    stim --detect[=#samples] [--out_format=01|b8|ptb64] [--in=file] [--out=file]

EXAMPLE CIRCUIT (GHZ)
=====================
H 0
CNOT 0 1
CNOT 0 2
M 0 1 2
)HELP";
        return EXIT_SUCCESS;
    }

    check_for_unknown_arguments(known_arguments, nullptr, argc, argv);

    bool mode_make_circuit = find_argument("--make_circuit", argc, argv);
    bool mode_interactive = find_bool_argument("--repl", argc, argv);
    bool mode_sampling = find_argument("--sample", argc, argv) != nullptr;
    bool mode_detecting = find_argument("--detect", argc, argv) != nullptr;
    if ((int)mode_make_circuit + (int)mode_interactive + (int)mode_sampling + (int)mode_detecting != 1) {
        std::cerr << "Need to specify exactly one of --sample or --repl or --detect or --make_circuit\n";
        return EXIT_FAILURE;
    }

    RaiiFiles raii_files;

    FILE *out = stdout;
    const char *out_path = find_argument("--out", argc, argv);
    if (out_path != nullptr) {
        out = fopen(out_path, "w");
        if (out == nullptr) {
            std::cerr << "Failed to open '" << out_path << "' to write.";
            return EXIT_FAILURE;
        }
        raii_files.files.push_back(out);
    }

    FILE *in = stdin;
    const char *in_path = find_argument("--in", argc, argv);
    if (in_path != nullptr) {
        in = fopen(in_path, "r");
        if (in == nullptr) {
            std::cerr << "Failed to open '" << in_path << "' to read.";
            return EXIT_FAILURE;
        }
        raii_files.files.push_back(in);
    }

    std::mt19937_64 rng = externally_seeded_rng();
    if (mode_interactive) {
        check_for_unknown_arguments(repl_mode_known_arguments, "--repl", argc, argv);
        TableauSimulator::sample_stream(in, out, mode_interactive, rng);
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
        if (num_shots == 1 && out_format == SAMPLE_FORMAT_01 && !frame0) {
            TableauSimulator::sample_stream(in, out, false, rng);
            return EXIT_SUCCESS;
        }

        auto circuit = Circuit::from_file(in);
        simd_bits ref(circuit.num_measurements);
        if (!frame0) {
            ref = TableauSimulator::reference_sample_circuit(circuit);
        }
        FrameSimulator::sample_out(circuit, ref, num_shots, out, out_format, rng);
        return EXIT_SUCCESS;
    }
    if (mode_detecting) {
        check_for_unknown_arguments(detect_mode_known_arguments, "--detect", argc, argv);
        SampleFormat out_format =
            format_values[find_enum_argument("--out_format", SAMPLE_FORMAT_01, format_names, argc, argv)];
        bool prepend_observables = find_bool_argument("--prepend_observables", argc, argv);
        bool append_observables = find_bool_argument("--append_observables", argc, argv);
        auto num_shots = (size_t)find_int_argument("--detect", 1, 0, 1 << 30, argc, argv);
        if (num_shots == 0) {
            return EXIT_SUCCESS;
        }

        auto circuit = Circuit::from_file(in);
        simd_bits ref(circuit.num_measurements);
        detector_samples_out(circuit, num_shots, prepend_observables, append_observables, out, out_format, rng);
        return EXIT_SUCCESS;
    }
    if (mode_make_circuit) {
        check_for_unknown_arguments(make_circuit_mode_known_arguments, "--make_circuit", argc, argv);
        std::string name = circuit_names[find_enum_argument("--make_circuit", -1, circuit_names, argc, argv)];

        size_t distance = find_int_argument("--distance", 3, 2, 999999, argc, argv);
        size_t rounds = find_int_argument("--rounds", distance, 2, 999999, argc, argv);
        float noise_level = find_float_argument("--noise_level", 0, 0, 1, argc, argv);

        if (name == "surface_unrotated") {
            fprintf(out, "%s", unrotated_surface_code_program_text(distance, rounds, noise_level).data());
        } else {
            throw std::out_of_range("Circuit not handled " + name);
        }
        return EXIT_SUCCESS;
    }

    throw std::out_of_range("Mode not handled.");
}
