#include "circuit.h"

#include <string>

#include "simulators/gate_data.h"
#include "simulators/tableau_simulator.h"
#include "stabilizers/tableau.h"

std::vector<std::string> tokenize(const std::string &line, size_t start, size_t end) {
    // Ignore comments.
    for (size_t k = start; k < end; k++) {
        if (line[k] == '#') {
            end = k;
            break;
        }
    }

    // Ignore leading and trailing whitespace.
    while (start < end && std::isspace(line[end - 1])) {
        end--;
    }
    while (start < end && std::isspace(line[start])) {
        start++;
    }

    // Tokenize.
    std::vector<std::string> tokens{};
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

double parse_double(const std::string &text) { return std::stold(text); }

size_t parse_size_t(const std::string &text) {
    if (std::stoll(text) < 0) {
        throw std::out_of_range("");
    }
    auto r = std::stoull(text);
    if (r > SIZE_MAX) {
        throw std::out_of_range("");
    }
    return (size_t)r;
}

Instruction Instruction::from_line(const std::string &line, size_t start, size_t end) {
    auto tokens = tokenize(line, start, end);
    if (tokens.size() == 0) {
        return {Operation{"", OperationData({}, {}, 0)}, false, false};
    }

    bool ended_block = tokens[tokens.size() - 1] == "}";
    if (ended_block) {
        if (tokens.size() > 1) {
            throw std::out_of_range("End of block `}` must be on its own line.");
        }
        return {Operation{"", OperationData({}, {}, 0)}, false, true};
    }

    bool started_block = tokens[tokens.size() - 1] == "{";
    if (started_block) {
        tokens.pop_back();
    }

    // Upper case the name.
    for (size_t k = 0; k < tokens[0].size(); k++) {
        tokens[0][k] = std::toupper(tokens[0][k]);
    }
    Operation op{tokens[0], OperationData({}, {}, 0)};
    auto canonical_name_ptr = GATE_CANONICAL_NAMES.find(op.name);
    if (canonical_name_ptr != GATE_CANONICAL_NAMES.end()) {
        op.name = canonical_name_ptr->second;
    }

    size_t start_of_args = 1;
    if (tokens.size() >= 4 && tokens[1] == "(" && tokens[3] == ")") {
        try {
            op.target_data.arg = parse_double(tokens[2]);
        } catch (const std::out_of_range &) {
            throw std::runtime_error("Bad numeric (argument) in line '" + line + "'.");
        } catch (const std::invalid_argument &) {
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
    } catch (const std::out_of_range &) {
        throw std::runtime_error("Bad qubit id in line '" + line + "'.");
    } catch (const std::invalid_argument &) {
        throw std::runtime_error("Bad qubit id in line '" + line + "'.");
    }
    return {op, started_block, ended_block};
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

Circuit::Circuit(const std::vector<Operation> &init_operations)
    : operations(init_operations), num_qubits(0), num_measurements(0) {
    init_nums(*this);
}

Circuit::Circuit(std::vector<Operation> &&init_operations)
    : operations(init_operations), num_qubits(0), num_measurements(0) {
    init_nums(*this);
}

Circuit Circuit::from_file(FILE *file) {
    CircuitReader reader;
    while (reader.read_more(file, false, false)) {
    }
    return Circuit(reader.ops);
}

Circuit Circuit::from_text(const std::string &text) {
    CircuitReader reader;
    reader.read_more(text, false, false);
    return Circuit(reader.ops);
}

bool Operation::try_fuse_with(const Operation &other) {
    if (name == other.name && target_data.arg == other.target_data.arg) {
        target_data += other.target_data;
        return true;
    }
    return false;
}

bool Operation::operator==(const Operation &other) const {
    return name == other.name && target_data.targets == other.target_data.targets &&
           target_data.arg == other.target_data.arg && target_data.flags == other.target_data.flags;
}

bool Operation::operator!=(const Operation &other) const { return !(*this == other); }

bool Instruction::operator==(const Instruction &other) const {
    return started_block == other.started_block && ended_block == other.ended_block && operation == other.operation;
}

bool Instruction::operator!=(const Instruction &other) const { return !(*this == other); }

bool Instruction::operator==(const Operation &other) const {
    return !started_block && !ended_block && operation == other;
}

bool Instruction::operator!=(const Operation &other) const { return !(*this == other); }

bool Circuit::operator==(const Circuit &other) const {
    return num_qubits == other.num_qubits && operations == other.operations;
}
bool Circuit::operator!=(const Circuit &other) const { return !(*this == other); }

std::string read_line(FILE *file) {
    std::string result {};
    while (true) {
        int i = getc(file);
        if (i == EOF) {
            return  "\n";
        }
        if (i == '\n') {
            return result;
        }
        result.append(1, (char)i);
    }
}

bool CircuitReader::read_more(std::string text, bool inside_block, bool stop_after_measurement) {
    size_t s = 0;
    size_t k = 0;
    return read_more_helper([&](){
        if (k >= text.size() || text[k] == '\0') {
            return std::string(1, '\n');
        }
        while (true) {
            if (text[k] == '\n' || text[k] == '\0') {
                auto result = text.substr(s, k - s);
                k++;
                s = k;
                return result;
            }
            k++;
        }
    }, inside_block, stop_after_measurement);
}

bool CircuitReader::read_more(FILE *file, bool inside_block, bool stop_after_measurement) {
    return read_more_helper([&](){ return read_line(file); }, inside_block, stop_after_measurement);
}

bool CircuitReader::read_more_helper(const std::function<std::string(void)>& line_getter, bool inside_block, bool stop_after_measurement) {
    bool read_any = false;
    bool can_fuse = false;
    while (true) {
        auto line = line_getter();
        if (line == "\n") {
            if (inside_block) {
                throw std::out_of_range("Unterminated block. Got a '{' without a '}'.");
            }
            return read_any;
        }

        auto instruction = Instruction::from_line(line, 0, line.size());
        if (instruction.ended_block) {
            if (!inside_block) {
                throw std::out_of_range("Uninitiated block. Got a '}' without a '{'.");
            }
            return true;
        }

        if (instruction.started_block) {
            if (instruction.operation.name == "REPEAT") {
                if (instruction.operation.target_data.targets.size() != 1 || instruction.operation.target_data.arg) {
                    throw std::out_of_range("Invalid instruction. Expected one repetition count like `REPEAT 100 {`.");
                }
                CircuitReader sub;
                sub.read_more_helper(line_getter, true, false);
                for (size_t k = 0; k < instruction.operation.target_data.targets[0]; k++) {
                    ops.insert(ops.end(), sub.ops.begin(), sub.ops.end());
                }
                can_fuse = false;
                continue;
            } else {
                throw std::out_of_range("'" + instruction.operation.name + "' is not a block starting instruction.");
            }
        }

        if (instruction.operation.name == "") {
            continue;
        }

        if (!can_fuse || !ops.back().try_fuse_with(instruction.operation)) {
            ops.push_back(instruction.operation);
        }
        if (stop_after_measurement && instruction.operation.name == "M") {
            return true;
        }
        can_fuse = true;
        read_any = true;
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

std::string Instruction::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

std::string Operation::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

OperationData::OperationData(size_t target) : targets({target}), flags({false}), arg(0) {}
OperationData::OperationData(std::initializer_list<size_t> init_targets)
    : targets(init_targets), flags(init_targets.size(), false), arg(0) {}
OperationData::OperationData(const std::vector<size_t> &init_targets)
    : targets({init_targets}), flags(init_targets.size(), false), arg(0) {}
OperationData::OperationData(const std::vector<size_t> &init_targets, std::vector<bool> init_flags, float init_arg)
    : targets(init_targets), flags(init_flags), arg(init_arg) {}

OperationData &OperationData::operator+=(const OperationData &other) {
    targets.insert(targets.end(), other.targets.begin(), other.targets.end());
    flags.insert(flags.end(), other.flags.begin(), other.flags.end());
    return *this;
}

std::ostream &operator<<(std::ostream &out, const Instruction &inst) {
    if (inst.ended_block) {
        out << "}";
    }
    out << inst.operation;
    if (inst.started_block) {
        out << " {";
    }
    return out;
}
