#include "circuit.h"
#include <string>
#include "tableau.h"
#include "chp_sim.h"

std::vector<std::string> tokenize(const std::string &line, size_t start, size_t end) {
    // Ignore comments.
    for (size_t k = start; k < end; k++) {
        if (line[k] == '#') {
            end = k;
            break;
        }
    }

    // Ignore leading and trailing whitespace.
    while (start < end && std::isspace(line[end-1])) {
        end--;
    }
    while (start < end && std::isspace(line[start])) {
        start++;
    }

    // Tokenize.
    std::vector<std::string> tokens {};
    size_t s = start;
    for (size_t k = start; k <= end; k++) {
        if (k == end || std::isspace(line[k])) {
            if (s < k) {
                tokens.push_back(line.substr(s, k - s));
            }
            s = k + 1;
        }
    }
    return tokens;
}

size_t parse_size_t(const std::string &text) {
    if (std::stoll(text) < 0) {
        throw std::out_of_range("");
    }
    return std::stoull(text);
}

Operation Operation::from_line(const std::string &line) {
    auto tokens = tokenize(line, 0, line.size());
    if (tokens.size() == 0) {
        return Operation{"", {}};
    }
    for (size_t k = 0; k < tokens[0].size(); k++) {
        tokens[0][k] = std::toupper(tokens[0][k]);
    }
    const auto &name = tokens[0];
    if (tokens.size() == 1) {
        if (tokens[0] == "TICK") {
            // Start new moment. Not represented.
            return Operation{"", {}};
        }
        throw std::runtime_error("Failed to parse line: '" + line + "'.");
    }
    if (tokens.size() == 2) {
        if (SINGLE_QUBIT_GATE_FUNCS.find(name) == SINGLE_QUBIT_GATE_FUNCS.end()) {
            throw std::runtime_error("Unrecognized single qubit gate " + name + " in line '" + line + "'.");
        }
        try {
            return {name, {parse_size_t(tokens[1])}};
        } catch (std::out_of_range ex) {
            throw std::runtime_error("Bad qubit id " + tokens[1] + " in line '" + line + "'.");
        } catch (std::invalid_argument ex) {
            throw std::runtime_error("Bad qubit id " + tokens[1] + " in line '" + line + "'.");
        }
    }
    if (tokens.size() == 3) {
        if (TWO_QUBIT_GATE_FUNCS.find(name) == TWO_QUBIT_GATE_FUNCS.end()) {
            throw std::runtime_error("Unrecognized two qubit gate " + name + " in line '" + line + "'.");
        }
        try {
            return {name, {parse_size_t(tokens[1]), parse_size_t(tokens[2])}};
        } catch (std::out_of_range ex)  {
            throw std::runtime_error("Bad qubit id " + tokens[1] + " or " + tokens[2] + " in line '" + line + "'.");
        } catch (std::invalid_argument ex)  {
            throw std::runtime_error("Bad qubit id " + tokens[1] + " or " + tokens[2] + " in line '" + line + "'.");
        }
    }
    throw std::runtime_error("Too many tokens in line '" + line + "'.");
}

Circuit Circuit::from_text(const std::string &text) {
    std::vector<Operation> operations;
    size_t s = 0;
    for (size_t k = 0; k <= text.size(); k++) {
        if (text[k] == '\n' || text[k] == '\0') {
            auto op = Operation::from_line(text.substr(s, k - s));
            s = k + 1;
            if (op.targets.size()) {
                if ((op.name == "M" || op.name == "R") && operations.size() && operations.back().name == op.name) {
                    operations.back().targets.push_back(op.targets[0]);
                } else {
                    operations.push_back(op);
                }
            }
        }
    }
    size_t n = 0;
    for (const auto &e : operations) {
        for (auto q : e.targets) {
            n = std::max(n, q + 1);
        }
    }
    return {n, operations};
}

bool Operation::operator==(const Operation &other) const {
    return name == other.name && targets == other.targets;
}

bool Operation::operator!=(const Operation &other) const {
    return !(*this == other);
}

bool Circuit::operator==(const Circuit &other) const {
    return num_qubits == other.num_qubits && operations == other.operations;
}
bool Circuit::operator!=(const Circuit &other) const {
    return !(*this == other);
}
