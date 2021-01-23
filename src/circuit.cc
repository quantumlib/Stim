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
        if (k == end || std::isspace(line[k]) || line[k] == '(' || line[k] == ')') {
            if (s < k) {
                tokens.push_back(line.substr(s, k - s));
            }
            s = k + 1;
        }
        if (line[k] == '(' || line[k] == ')') {
            tokens.push_back(line.substr(k, 1));
        }
    }
    return tokens;
}

double parse_double(const std::string &text) {
    return std::stold(text);
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
        return Operation{"", OperationData({}, {}, 0)};
    }

    // Upper case the name.
    for (size_t k = 0; k < tokens[0].size(); k++) {
        tokens[0][k] = std::toupper(tokens[0][k]);
    }
    Operation op {tokens[0], OperationData({}, {}, 0)};

    size_t start_of_args = 1;
    if (tokens.size() >= 4 && tokens[1] == "(" && tokens[3] == ")") {
        try {
            op.target_data.arg = parse_double(tokens[2]);
        } catch (const std::out_of_range &ex) {
            throw std::runtime_error("Bad numeric (argument) in line '" + line + "'.");
        } catch (const std::invalid_argument &ex) {
            throw std::runtime_error("Bad numeric (argument) in line '" + line + "'.");
        }
        if (op.target_data.arg < 0 || op.target_data.arg > 999999999999 || op.target_data.arg != op.target_data.arg) {
            throw std::runtime_error("Bad numeric (argument) in line '" + line + "'.");
        }
        start_of_args = 4;
    }

    op.target_data.targets.reserve(tokens.size() - start_of_args);
    op.target_data.flags.reserve(tokens.size() - start_of_args);
    try {
        for (size_t k = start_of_args; k < tokens.size(); k++) {
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
        if (p.name == "M") {
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
    CircuitReader reader;
    while (reader.read_more(file)) {
    }
    return Circuit(reader.ops).with_fused_operations();
}

Circuit Circuit::from_text(const std::string &text) {
    size_t s = 0;
    CircuitReader reader;
    for (size_t k = 0; k <= text.size(); k++) {
        if (text[k] == '\n' || text[k] == '\0') {
            reader.read_operation(Operation::from_line(text, s, k));
            s = k + 1;
        }
    }
    return Circuit(reader.ops).with_fused_operations();
}

Circuit Circuit::with_fused_operations() const {
    std::vector<Operation> fused;
    for (auto op : operations) {
        if (fused.empty() || !fused.back().try_fuse_with(op)) {
            fused.push_back(op);
        }
    }
    return Circuit(fused);
}

bool Operation::try_fuse_with(const Operation &other) {
    if (name == other.name && target_data.arg == other.target_data.arg) {
        target_data += other.target_data;
        return true;
    }
    return false;
}

bool Operation::operator==(const Operation &other) const {
    return name == other.name
        && target_data.targets == other.target_data.targets
        && target_data.arg == other.target_data.arg
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

void CircuitReader::read_operation(const Operation &operation) {
    // Ignore empty operations.
    if (operation.name == "" && operation.target_data.targets.size() == 0) {
        return;
    }

    // Flatten loops.
    if (operation.name == "REPEATLAST") {
        if (fmod(operation.target_data.arg, 1.0) != 0.0
                || operation.target_data.targets.size() != 1
                || operation.target_data.flags[0]) {
            throw std::out_of_range(
                    "Expected `REPEATLAST(#back) #reps` but got " + operation.str());
        }
        size_t window = (size_t)operation.target_data.arg;
        size_t iterations = operation.target_data.targets[0];
        if (window > ops.size()) {
            throw std::out_of_range(
                    operation.str() + " reaches further back than the start of time.");
        }
        if (iterations <= 0) {
            throw std::out_of_range(
                    operation.str() + " has an invalid number of repetitions "
                                      "(repetitions include the initial execution of the gates, e.g. "
                                      "`REPEATLAST(X) 1` is equivalent to having no `REPEATLAST` instruction.");
        }
        auto end = ops.end();
        auto begin = end - window;
        for (size_t k = 1; k < iterations; k++) {
            ops.insert(ops.end(), begin, end);
        }
        return;
    }

    ops.push_back(operation);
}

bool CircuitReader::read_more(FILE *file, bool stop_after_measurement) {
    bool read_any = false;
    std::string line_buf {};
    while (true) {
        line_buf.clear();
        while (true) {
            int i = getc_unlocked(file);
            if (i == EOF) {
                return read_any;
            }
            if (i == '\n') {
                break;
            }
            line_buf.append(1, (char) i);
        }

        auto op = Operation::from_line(line_buf, 0, line_buf.size());
        read_operation(op);
        read_any = true;
        if (op.name == "TICK") {
            return true;
        }
        if (stop_after_measurement && op.name == "M") {
            return true;
        }
    }
}

std::ostream &operator<<(std::ostream &out, const Operation &op) {
    out << op.name;
    if (op.target_data.arg != 0) {
        out << '(';
        if ((size_t)op.target_data.arg == op.target_data.arg) {
            out << (size_t)op.target_data.arg;
        } else {
            out << op.target_data.arg;
        }
        out << ')';
    }
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

OperationData::OperationData(size_t target) :
    targets({target}), flags({false}), arg(0) {
}
OperationData::OperationData(std::initializer_list<size_t> init_targets) :
    targets(init_targets), flags(init_targets.size(), false), arg(0) {
}
OperationData::OperationData(const std::vector<size_t> &init_targets) :
    targets({init_targets}), flags(init_targets.size(), false), arg(0) {
}
OperationData::OperationData(const std::vector<size_t> &init_targets, std::vector<bool> init_flags, float init_arg) :
    targets(init_targets), flags(init_flags), arg(init_arg) {
}

OperationData &OperationData::operator+=(const OperationData &other) {
    targets.insert(targets.end(), other.targets.begin(), other.targets.end());
    flags.insert(flags.end(), other.flags.begin(), other.flags.end());
    return *this;
}
