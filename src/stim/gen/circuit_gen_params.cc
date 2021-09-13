#include "stim/gen/circuit_gen_params.h"

#include "stim/arg_parse.h"

using namespace stim;

void append_anti_basis_error(Circuit &circuit, const std::vector<uint32_t> &targets, double p, char basis) {
    if (p > 0) {
        if (basis == 'X') {
            circuit.append_op("Z_ERROR", targets, p);
        } else {
            circuit.append_op("X_ERROR", targets, p);
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
    circuit.append_op("TICK", {});
    if (before_round_data_depolarization > 0) {
        circuit.append_op("DEPOLARIZE1", data_qubits, before_round_data_depolarization);
    }
}

void CircuitGenParameters::append_unitary_1(
    Circuit &circuit, const std::string &name, const std::vector<uint32_t> targets) const {
    circuit.append_op(name, targets);
    if (after_clifford_depolarization > 0) {
        circuit.append_op("DEPOLARIZE1", targets, after_clifford_depolarization);
    }
}

void CircuitGenParameters::append_unitary_2(
    Circuit &circuit, const std::string &name, const std::vector<uint32_t> targets) const {
    circuit.append_op(name, targets);
    if (after_clifford_depolarization > 0) {
        circuit.append_op("DEPOLARIZE2", targets, after_clifford_depolarization);
    }
}

void CircuitGenParameters::append_reset(Circuit &circuit, const std::vector<uint32_t> targets, char basis) const {
    circuit.append_op(std::string("R") + basis, targets);
    append_anti_basis_error(circuit, targets, after_reset_flip_probability, basis);
}

void CircuitGenParameters::append_measure(Circuit &circuit, const std::vector<uint32_t> targets, char basis) const {
    append_anti_basis_error(circuit, targets, before_measure_flip_probability, basis);
    circuit.append_op(std::string("M") + basis, targets);
}

void CircuitGenParameters::append_measure_reset(
    Circuit &circuit, const std::vector<uint32_t> targets, char basis) const {
    append_anti_basis_error(circuit, targets, before_measure_flip_probability, basis);
    circuit.append_op(std::string("MR") + basis, targets);
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
