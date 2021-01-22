#include "circuit.h"
#include <string>
#include "stabilizers/tableau.h"
#include "simulators/tableau_simulator.h"
#include "simulators/gate_data.h"

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
        return Operation{"", OperationData({}, {})};
    }
    for (size_t k = 0; k < tokens[0].size(); k++) {
        tokens[0][k] = std::toupper(tokens[0][k]);
    }
    const auto &name = tokens[0];
    Operation op {name, OperationData({}, {})};
    op.target_data.targets.reserve(tokens.size() - 1);
    op.target_data.flags.reserve(tokens.size() - 1);
    try {
        for (size_t k = 1; k < tokens.size(); k++) {
            bool flag = tokens[k][0] == '!';
            op.target_data.flags.push_back(flag);
            op.target_data.targets.push_back(parse_size_t(tokens[k].substr(flag)));
        }
    } catch (const std::out_of_range &ex) {
        throw std::runtime_error("Bad qubit id in line '" + line + "'.");
    } catch (const std::invalid_argument &ex) {
        throw std::runtime_error("Bad qubit id in line '" + line + "'.");
    }
    return op;
}

void init_nums(Circuit &circuit) {
    for (const auto &p : circuit.operations) {
        if (p.name == "M" || p.name == "M_REF" || p.name == "M_PREFER_0") {
            circuit.num_measurements += p.target_data.targets.size();
        }
        for (auto q : p.target_data.targets) {
            circuit.num_qubits = std::max(circuit.num_qubits, q + 1);
        }
    }
}

Circuit::Circuit(const std::vector<Operation> &init_operations) :
        operations(init_operations), num_qubits(0), num_measurements(0) {
    init_nums(*this);
}

Circuit::Circuit(std::vector<Operation> &&init_operations) :
        operations(init_operations), num_qubits(0), num_measurements(0) {
    init_nums(*this);
}

Circuit Circuit::from_file(FILE *file) {
    CircuitReader reader(file);
    std::vector<Operation> operations {};
    while (reader.read_next_moment()) {
        operations.insert(operations.end(), reader.operations.begin(), reader.operations.end());
    }
    return Circuit(operations);
}

Circuit Circuit::from_text(const std::string &text, bool fuse_operations) {
    std::vector<Operation> operations;
    size_t s = 0;
    for (size_t k = 0; k <= text.size(); k++) {
        if (text[k] == '\n' || text[k] == '\0') {
            auto op = Operation::from_line(text, s, k);
            s = k + 1;
            if (op.name == "" && op.target_data.targets.size() == 0) {
                continue;
            }
            auto &back_op = operations.back();
            if (fuse_operations && operations.size() && back_op.name == op.name) {
                back_op.target_data += op.target_data;
            } else {
                operations.push_back(op);
            }
        }
    }
    return Circuit(operations);
}

bool Operation::operator==(const Operation &other) const {
    return name == other.name
        && target_data.targets == other.target_data.targets
        && target_data.flags == other.target_data.flags;
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
        if (op.name == "TICK") {
            return true;
        }
        auto &back_op = operations.back();
        if (operations.size() && back_op.name == op.name) {
            back_op.target_data += op.target_data;
        } else {
            operations.push_back(op);
        }
        if (stop_after_measurement && op.name == "M") {
            return true;
        }
    }
}

std::ostream &operator<<(std::ostream &out, const Operation &op) {
    out << op.name;
    for (size_t k = 0; k < op.target_data.targets.size(); k++) {
        out << ' ';
        if (op.target_data.flags.size() <= k) {
            out << "?!?";
        } else if (op.target_data.flags[k]) {
            out << '!';
        }
        out << op.target_data.targets[k];
    }
    if (op.target_data.flags.size() > op.target_data.targets.size()) {
        out << " ?!?";
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const Circuit &c) {
    for (const auto &op : c.operations) {
        out << op << "\n";
    }
    return out;
}

Circuit Circuit::with_reference_measurements_from_tableau_simulation() const {
    std::vector<Operation> deterministic_operations {};
    for (const auto &op : operations) {
        if (op.name == "M") {
            deterministic_operations.push_back({"M_PREFER_0", op.target_data});
        } else if (op.name == "R") {
            deterministic_operations.push_back({"R_PREFER_0", op.target_data});
        } else if (NOISY_GATE_NAMES.find(op.name) == NOISY_GATE_NAMES.end()) {
            deterministic_operations.push_back(op);
        }
    }

    std::mt19937_64 irrelevant_rng(0);
    std::vector<bool> reference_samples = TableauSimulator::sample_circuit(Circuit(deterministic_operations), irrelevant_rng);

    size_t next_result_index = 0;
    std::vector<Operation> reference_operations {};
    for (const auto &op : operations) {
        if (op.name == "I" || op.name == "X" || op.name == "Y" || op.name == "Z") {
            // Pauli operations are implicitly accounted for in the sign of the reference measurements.
            continue;
        } else if (op.name == "M" || op.name == "M_PREFER_0") {
            Operation new_op = op;
            new_op.name = "M_REF";
            for (size_t k = 0; k < new_op.target_data.flags.size(); k++) {
                new_op.target_data.flags[k] = reference_samples[next_result_index];
                next_result_index++;
            }
            reference_operations.push_back(new_op);
        } else {
            reference_operations.push_back(op);
        }
    }

    return Circuit(reference_operations);
}

std::string Circuit::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

std::string Operation::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

OperationData::OperationData(size_t target) : targets({target}), flags({false}) {
}
OperationData::OperationData(std::initializer_list<size_t> init_targets) : targets(init_targets), flags(init_targets.size(), false) {
}
OperationData::OperationData(const std::vector<size_t> &init_targets) : targets({init_targets}), flags(init_targets.size(), false) {
}
OperationData::OperationData(const std::vector<size_t> &init_targets, std::vector<bool> init_flags) : targets(init_targets), flags(init_flags) {
}

OperationData &OperationData::operator+=(const OperationData &other) {
    targets.insert(targets.end(), other.targets.begin(), other.targets.end());
    flags.insert(flags.end(), other.flags.begin(), other.flags.end());
    return *this;
}
