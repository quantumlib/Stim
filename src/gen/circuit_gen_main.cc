#include "circuit_gen_main.h"

#include "../arg_parse.h"
#include "circuit_gen_params.h"
#include "gen_color_code.h"
#include "gen_rep_code.h"
#include "gen_surface_code.h"

using namespace stim_internal;

std::vector<const char *> code_names{
    "color_code",
    "repetition_code",
    "surface_code",
};
std::vector<GeneratedCircuit (*)(const CircuitGenParameters &)> code_functions{
    &generate_color_code_circuit,
    &generate_rep_code_circuit,
    &generate_surface_code_circuit,
};
std::vector<const char *> known_commands{
    "--after_clifford_depolarization",
    "--after_reset_flip_probability",
    "--task",
    "--before_measure_flip_probability",
    "--before_round_data_depolarization",
    "--distance",
    "--gen",
    "--out",
    "--in",
    "--rounds",
};

int stim_internal::main_generate_circuit(int argc, const char **argv, FILE *out) {
    check_for_unknown_arguments(known_commands, "--gen", argc, argv);
    auto code_index = find_enum_argument("--gen", -1, code_names, argc, argv);
    auto func = code_functions[code_index];
    CircuitGenParameters params(
        (uint64_t)find_int64_argument("--rounds", -1, 1, INT64_MAX, argc, argv),
        (uint32_t)find_int64_argument("--distance", -1, 2, 2047, argc, argv),
        require_find_argument("--task", argc, argv));
    params.before_round_data_depolarization = find_float_argument("--before_round_data_depolarization", 0, 0, 1, argc, argv);
    params.before_measure_flip_probability = find_float_argument("--before_measure_flip_probability", 0, 0, 1, argc, argv);
    params.after_reset_flip_probability = find_float_argument("--after_reset_flip_probability", 0, 0, 1, argc, argv);
    params.after_clifford_depolarization = find_float_argument("--after_clifford_depolarization", 0, 0, 1, argc, argv);

    std::stringstream ss;
    ss << "# Generated " << code_names[code_index] << " circuit.\n";
    ss << "# task: " << params.task << "\n";
    ss << "# rounds: " << params.rounds << "\n";
    ss << "# distance: " << params.distance << "\n";
    ss << "# before_round_data_depolarization: " << params.before_round_data_depolarization << "\n";
    ss << "# before_measure_flip_probability: " << params.before_measure_flip_probability << "\n";
    ss << "# after_reset_flip_probability: " << params.after_reset_flip_probability << "\n";
    ss << "# after_clifford_depolarization: " << params.after_clifford_depolarization << "\n";
    ss << "# layout:\n";
    auto generated = func(params);
    ss << generated.layout_str();
    ss << generated.hint_str;
    ss << generated.circuit;
    ss << "\n";
    fprintf(out, "%s", ss.str().data());
    return 0;
}
