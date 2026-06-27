#include "stim/gen/circuit_gen_params.h"

#include "stim/util_bot/arg_parse.h"

using namespace stim;

void append_anti_basis_error(Circuit &circuit, const std::vector<uint32_t> &targets, double p, char basis) {
    if (p > 0) {
        if (basis == 'X') {
            circuit.safe_append_ua("Z_ERROR", targets, p);
        } else {
            circuit.safe_append_ua("X_ERROR", targets, p);
        }
    }
}

void CircuitGenParameters::validate_params() const {
    if (before_measure_flip_probability < 0 || before_measure_flip_probability > 1) {
        throw std::invalid_argument("not 0 <= before_measure_flip_probability <= 1");
    }
    if (before_round_data_depolarization < 0 || before_round_data_depolarization > 1) {
        throw std::invalid_argument("not 0 <= before_round_data_depolarization <= 1");
    }
    if (after_clifford_depolarization < 0 || after_clifford_depolarization > 1) {
        throw std::invalid_argument("not 0 <= after_clifford_depolarization <= 1");
    }
    if (after_reset_flip_probability < 0 || after_reset_flip_probability > 1) {
        throw std::invalid_argument("not 0 <= after_reset_flip_probability <= 1");
    }
}

CircuitGenParameters::CircuitGenParameters(uint64_t rounds, uint32_t distance, std::string task)
    : rounds(rounds), distance(distance), task(task) {
}

void CircuitGenParameters::append_begin_round_tick(Circuit &circuit, const std::vector<uint32_t> &data_qubits) const {
    circuit.safe_append_u("TICK", {});
    if (before_round_data_depolarization > 0) {
        circuit.safe_append_ua("DEPOLARIZE1", data_qubits, before_round_data_depolarization);
    }
}

void CircuitGenParameters::append_unitary_1(
    Circuit &circuit, std::string_view name, const std::vector<uint32_t> targets) const {
    circuit.safe_append_u(name, targets);
    if (after_clifford_depolarization > 0) {
        circuit.safe_append_ua("DEPOLARIZE1", targets, after_clifford_depolarization);
    }
}

void CircuitGenParameters::append_unitary_2(
    Circuit &circuit, std::string_view name, const std::vector<uint32_t> targets) const {
    circuit.safe_append_u(name, targets);
    if (after_clifford_depolarization > 0) {
        circuit.safe_append_ua("DEPOLARIZE2", targets, after_clifford_depolarization);
    }
}

void CircuitGenParameters::append_reset(Circuit &circuit, const std::vector<uint32_t> targets, char basis) const {
    std::string gate("R");
    gate.push_back(basis);
    circuit.safe_append_u(gate, targets);
    append_anti_basis_error(circuit, targets, after_reset_flip_probability, basis);
}

void CircuitGenParameters::append_measure(Circuit &circuit, const std::vector<uint32_t> targets, char basis) const {
    std::string gate("M");
    gate.push_back(basis);
    append_anti_basis_error(circuit, targets, before_measure_flip_probability, basis);
    circuit.safe_append_u(gate, targets);
}

void CircuitGenParameters::append_measure_reset(
    Circuit &circuit, const std::vector<uint32_t> targets, char basis) const {
    std::string gate("MR");
    gate.push_back(basis);
    append_anti_basis_error(circuit, targets, before_measure_flip_probability, basis);
    circuit.safe_append_u(gate, targets);
    append_anti_basis_error(circuit, targets, after_reset_flip_probability, basis);
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
    for (auto p = lines.crbegin(); p != lines.crend(); p++) {
        const auto &line = *p;
        ss << "#";
        for (const auto &entry : line) {
            ss << ' ' << entry;
            ss << std::string(max_len - entry.size(), ' ');
        }
        ss << "\n";
    }
    return ss.str();
}
