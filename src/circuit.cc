#include "circuit.h"
#include <string>
#include "tableau.h"
#include "sim_tableau.h"

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

Operation Operation::from_line(const std::string &line, size_t start, size_t end) {
    auto tokens = tokenize(line, start, end);
    if (tokens.size() == 0) {
        return Operation{"", {}};
    }
    for (size_t k = 0; k < tokens[0].size(); k++) {
        tokens[0][k] = std::toupper(tokens[0][k]);
    }
    const auto &name = tokens[0];
    Operation op {name, {}};
    op.targets.reserve(tokens.size() - 1);
    try {
        for (size_t k = 1; k < tokens.size(); k++) {
            op.targets.push_back(parse_size_t(tokens[k]));
        }
    } catch (const std::out_of_range &ex) {
        throw std::runtime_error("Bad qubit id in line '" + line + "'.");
    } catch (const std::invalid_argument &ex) {
        throw std::runtime_error("Bad qubit id in line '" + line + "'.");
    }
    return op;
}

Circuit Circuit::from_file(FILE *file) {
    Circuit result;
    CircuitReader reader(file);
    while (reader.read_next_moment()) {
        result.operations.insert(result.operations.end(), reader.operations.begin(), reader.operations.end());
    }
    for (const auto &e : result.operations) {
        for (auto q : e.targets) {
            result.num_qubits = std::max(result.num_qubits, q + 1);
        }
    }
    return result;
}

Circuit Circuit::from_text(const std::string &text) {
    std::vector<Operation> operations;
    size_t s = 0;
    for (size_t k = 0; k <= text.size(); k++) {
        if (text[k] == '\n' || text[k] == '\0') {
            auto op = Operation::from_line(text, s, k);
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

CircuitReader::CircuitReader(FILE *file) : input_file(file) {
}

bool CircuitReader::read_next_moment(bool stop_after_measurement) {
    operations.clear();

    while (true) {
        line_buf.clear();
        while (true) {
            int i = getc_unlocked(input_file);
            if (i == EOF) {
                return !operations.empty();
            }
            if (i == '\n') {
                break;
            }
            line_buf.append(1, (char) i);
        }

        auto op = Operation::from_line(line_buf, 0, line_buf.size());
        if ((op.name == "M" || op.name == "R") && operations.size() && operations.back().name == op.name) {
            operations.back().targets.push_back(op.targets[0]);
        } else if (op.targets.size()) {
            operations.push_back(op);
        } else if (op.name == "TICK") {
            return true;
        }
        if (stop_after_measurement && op.name == "M") {
            return true;
        }
    }
}

std::ostream &operator<<(std::ostream &out, const Circuit &c) {
    for (const auto &op : c.operations) {
        out << op.name;
        for (auto q : op.targets) {
            out << " " << q;
        }
        out << "\n";
    }
    return out;
}

std::string Circuit::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}
