#include "circuit_gen_main.h"

#include "../arg_parse.h"
#include "rep_code.h"
#include "surface_code.h"

using namespace stim_internal;

std::vector<const char *> basis_names{
    "Z",
    "X",
};
std::vector<const char *> code_names{
    "repetition_code",
    "surface_code_rotated",
    "surface_code_unrotated",
};
std::vector<GeneratedCircuit (*)(const CircuitGenParameters &)> code_functions{
    &generate_rep_code_circuit,
    &generate_rotated_surface_code_circuit,
    &generate_unrotated_surface_code_circuit,
};
std::vector<const char *> known_commands{
    "--after_clifford_depolarization",
    "--after_reset_flip_probability",
    "--basis",
    "--before_measure_flip_probability",
    "--before_round_data_depolarization",
    "--distance",
    "--gen",
    "--out",
    "--rounds",
};

int stim_internal::main_generate_circuit(int argc, const char **argv, FILE *out) {
    check_for_unknown_arguments(known_commands, "--gen", argc, argv);
    auto code_index = find_enum_argument("--gen", -1, code_names, argc, argv);
    auto func = code_functions[code_index];
    CircuitGenParameters params(
        find_int_argument("--rounds", -1, 1, 16777215, argc, argv),
        find_int_argument("--distance", -1, 2, 2047, argc, argv));
    params.before_round_data_depolarization = find_float_argument("--before_round_data_depolarization", 0, 0, 1, argc, argv);
    params.before_measure_flip_probability = find_float_argument("--before_measure_flip_probability", 0, 0, 1, argc, argv);
    params.after_reset_flip_probability = find_float_argument("--after_reset_flip_probability", 0, 0, 1, argc, argv);
    params.after_clifford_depolarization = find_float_argument("--after_clifford_depolarization", 0, 0, 1, argc, argv);
    params.basis = basis_names[find_enum_argument("--basis", 0, basis_names, argc, argv)];

    std::stringstream ss;
    ss << "# Generated " << code_names[code_index] << " circuit.\n";
    ss << "# rounds: " << params.rounds << "\n";
    ss << "# distance: " << params.distance << "\n";
    ss << "# before_round_data_depolarization: " << params.before_round_data_depolarization << "\n";
    ss << "# before_measure_flip_probability: " << params.before_measure_flip_probability << "\n";
    ss << "# after_reset_flip_probability: " << params.after_reset_flip_probability << "\n";
    ss << "# after_clifford_depolarization: " << params.after_clifford_depolarization << "\n";
    ss << "# Index layout (d=data qubit, X|Z=measurement qubit, L=logical observable data qubit):\n";
    auto generated = func(params);
    ss << generated.layout_str();
    ss << generated.circuit;
    ss << "\n";
    fprintf(out, "%s", ss.str().data());
    return 0;
}

CircuitGenParameters::CircuitGenParameters(size_t rounds, uint32_t distance) : rounds(rounds), distance(distance) {}

void CircuitGenParameters::append_round_transition(Circuit &circuit, const std::vector<uint32_t> &data_qubits) const {
    if (before_round_data_depolarization > 0) {
        circuit.append_op("DEPOLARIZE1", data_qubits, before_round_data_depolarization);
    }
    circuit.append_op("TICK", {});
}

void CircuitGenParameters::append_unitary_1(Circuit &circuit, const std::string &name, const std::vector<uint32_t> targets) const {
    circuit.append_op(name, targets);
    if (after_clifford_depolarization > 0) {
        circuit.append_op("DEPOLARIZE1", targets, after_clifford_depolarization);
    }
}

void CircuitGenParameters::append_unitary_2(Circuit &circuit, const std::string &name, const std::vector<uint32_t> targets) const {
    circuit.append_op(name, targets);
    if (after_clifford_depolarization > 0) {
        circuit.append_op("DEPOLARIZE2", targets, after_clifford_depolarization);
    }
}

void CircuitGenParameters::append_reset(Circuit &circuit, const std::vector<uint32_t> targets) const {
    circuit.append_op("R", targets);
    if (after_reset_flip_probability > 0) {
        circuit.append_op("X_ERROR", targets, after_reset_flip_probability);
    }
}

void CircuitGenParameters::append_measure(Circuit &circuit, const std::vector<uint32_t> targets) const {
    if (before_measure_flip_probability > 0) {
        circuit.append_op("X_ERROR", targets, before_measure_flip_probability);
    }
    circuit.append_op("M", targets);
}

void CircuitGenParameters::append_measure_reset(Circuit &circuit, const std::vector<uint32_t> targets) const {
    if (before_measure_flip_probability > 0) {
        circuit.append_op("X_ERROR", targets, before_measure_flip_probability);
    }
    circuit.append_op("MR", targets);
    if (after_reset_flip_probability > 0) {
        circuit.append_op("X_ERROR", targets, after_reset_flip_probability);
    }
}

std::string GeneratedCircuit::layout_str() const {
    std::stringstream ss;
    std::vector<std::vector<std::string>> lines;
    for (const auto &kv : layout) {
        auto x = kv.first.first;
        auto y = kv.first.second;
        while (lines.size() <= y) {
            lines.push_back({});
        }
        while (lines[y].size() <= x) {
            lines[y].push_back("");
        }
        lines[y][x] = kv.second.first + std::to_string(kv.second.second);
    }
    size_t max_len = 0;
    for (const auto &line : lines) {
        for (const auto &entry : line) {
            max_len = std::max(max_len, entry.size());
        }
    }
    for (const auto &line : lines) {
        ss << "#";
        for (const auto &entry : line) {
            ss << ' ' << entry;
            ss << std::string(max_len - entry.size(), ' ');
        }
        ss << "\n";
    }
    return ss.str();
}
