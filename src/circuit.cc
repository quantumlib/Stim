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

#include "circuit.h"

#include <cctype>
#include <string>
#include <utility>

#include "simulators/gate_data.h"
#include "simulators/tableau_simulator.h"
#include "stabilizers/tableau.h"

void tokenize_into(StringView line, std::vector<StringView> &buf) {
    // Ignore comments.
    for (size_t k = 0; k < line.n; k++) {
        if (line[k] == '#') {
            line = line.substr(0, k);
            break;
        }
    }

    // Ignore leading and trailing whitespace.
    while (line.n && isspace(line[line.n - 1])) {
        line.n--;
    }
    while (line.n && isspace(line[0])) {
        line.n--;
        line.c++;
    }

    // Tokenize.
    buf.clear();
    size_t s = 0;
    for (size_t k = 0; k <= line.n; k++) {
        if (k == line.n || isspace(line[k]) || line[k] == '(' || line[k] == ')') {
            if (s < k) {
                buf.push_back(line.substr(s, k - s));
            }
            s = k + 1;
        }
        if (line[k] == '(' || line[k] == ')') {
            buf.push_back(line.substr(k, 1));
        }
    }
}

size_t parse_size_t(StringView text) {
    size_t result = 0;
    if (text[0] == '\0') {
        throw std::out_of_range("No digits.");
    }
    for (size_t k = 0; k < text.n; k++) {
        if (text[k] < '0' || text[k] > '9') {
            throw std::out_of_range("Bad digit " + std::string(text.c));
        }
        size_t digit = text[k] - '0';
        if (result > (SIZE_MAX - digit) / 10) {
            throw std::out_of_range("Number too large " + std::string(text.c));
        }
        result *= 10;
        result += digit;
    }
    return result;
}

std::pair<size_t, size_t> parse_record_arg(StringView token, StringView line) {
    size_t n = 0;
    while (token.c[n] != '@' && n < token.n) {
        n++;
    }
    if (n == token.n) {
        throw std::out_of_range("Expected record arguments `qubit@-backtrack` like `3@-1` in '" + line + "'.");
    }
    if (token[n + 1] != '-') {
        throw std::out_of_range("Missing the '-' in `qubit@-backtrack` in '" + line + "'.");
    }
    auto a = parse_size_t(token.substr(0, n));
    auto b = parse_size_t(token.substr(n + 2));
    if (b == 0 || b > SIZE_MAX / 2) {
        throw std::out_of_range("Invalid backtrack. Note smallest backtrack is -1, not -0. '" + line + "'.");
    }
    return {a, b - 1};
}

void operation_from_tokens(const std::vector<StringView> &tokens, StringView line, Operation &op) {
    op.gate = &GATE_DATA.at(tokens[0]);

    size_t start_of_args = 1;
    bool has_argument = tokens.size() >= 4 && tokens[1] == "(" && tokens[3] == ")";
    bool expects_argument = op.gate->flags & GATE_TAKES_PARENS_ARGUMENT;
    if (has_argument && !expects_argument) {
        throw std::runtime_error("Don't use parens () with " + std::string(tokens[0].c) + ". '" + line.str() + "'.");
    }
    if (!has_argument && expects_argument) {
        throw std::runtime_error("Need a probability argument like '" + std::string(tokens[0].c) + "(0.001)'. '" + line.str() + "'.");
    }
    if (has_argument) {
        try {
            op.target_data.arg = std::stod(tokens[2].c);
        } catch (const std::out_of_range &) {
            throw std::runtime_error("Bad numeric (argument) in line '" + line.str() + "'.");
        } catch (const std::invalid_argument &) {
            throw std::runtime_error("Bad numeric (argument) in line '" + line.str() + "'.");
        }
        if (op.target_data.arg < 0 || op.target_data.arg > 999999999999 || op.target_data.arg != op.target_data.arg) {
            throw std::runtime_error("Bad numeric (argument) in line '" + line.str() + "'.");
        }
        start_of_args = 4;
    }

    bool has_record_args = op.gate->flags & GATE_TARGETS_MEASUREMENT_RECORD;
    try {
        for (size_t k = start_of_args; k < tokens.size(); k++) {
            if (has_record_args) {
                auto ab = parse_record_arg(tokens[k], line);
                op.target_data.targets.push_back(ab.first);
                op.target_data.metas.push_back(ab.second);
            } else {
                bool flag = tokens[k][0] == '!';
                op.target_data.metas.push_back(flag);
                op.target_data.targets.push_back(parse_size_t(tokens[k].substr(flag)));
            }
        }
    } catch (const std::out_of_range &) {
        throw std::runtime_error("Bad qubit id in line '" + line + "'.");
    }
}

OperationData::OperationData() : targets(), metas(), arg() {
}

Operation::Operation() : gate(&GATE_DATA.at("I")), target_data() {
}

Operation::Operation(const Gate &gate, OperationData target_data) : gate(&gate), target_data(std::move(target_data)) {
}

Instruction::Instruction(InstructionType type, Operation op) : type(type), op(std::move(op)) {
}

Instruction Instruction::from_line(StringView line) {
    std::vector<StringView> buf;
    Instruction result(INSTRUCTION_TYPE_EMPTY, {});
    from_line(line, buf, result);
    return result;
}

void Instruction::from_line(StringView line, std::vector<StringView> &view_buffer, Instruction &instruction_out) {
    tokenize_into(line, view_buffer);
    if (view_buffer.empty()) {
        instruction_out.type = INSTRUCTION_TYPE_EMPTY;
        return;
    }

    if (view_buffer[view_buffer.size() - 1] == "}") {
        if (view_buffer.size() > 1) {
            throw std::out_of_range("End of block `}` must be on its own line.");
        }
        instruction_out.type = INSTRUCTION_TYPE_BLOCK_END;
        return;
    }

    bool started_block = view_buffer[view_buffer.size() - 1] == "{";
    if (started_block) {
        view_buffer.pop_back();
    }
    instruction_out.type = started_block ? INSTRUCTION_TYPE_BLOCK_OPERATION_START : INSTRUCTION_TYPE_OPERATION;
    operation_from_tokens(view_buffer, line, instruction_out.op);
}

MeasurementSet &MeasurementSet::operator*=(const MeasurementSet &other) {
    indices.insert(indices.end(), other.indices.begin(), other.indices.end());
    expected_parity ^= other.expected_parity;
    return *this;
}

void init_nums(Circuit &circuit) {
    std::unordered_map<size_t, std::vector<size_t>> qubit_measure_indices;
    auto resolve = [&](const Operation &op) {
        MeasurementSet result{};
        for (size_t k = 0; k < op.target_data.targets.size(); k++) {
            auto q = op.target_data.targets[k];
            auto b = op.target_data.metas[k];
            const auto &v = qubit_measure_indices[q];
            if (b < v.size()) {
                result.indices.push_back(v[v.size() - b - 1]);
            }
        }
        return result;
    };
    for (const auto &p : circuit.operations) {
        if (p.gate->flags & GATE_PRODUCES_RESULTS) {
            for (auto q : p.target_data.targets) {
                qubit_measure_indices[q].push_back(circuit.num_measurements++);
            }
        } else if (p.gate->id == gate_name_to_id("DETECTOR")) {
            circuit.detectors.push_back(resolve(p));
        } else if (p.gate->id == gate_name_to_id("OBSERVABLE_INCLUDE")) {
            size_t obs = (size_t)p.target_data.arg;
            if (obs != p.target_data.arg) {
                throw std::out_of_range("Observable index must be an integer.");
            }
            while (circuit.observables.size() <= obs) {
                circuit.observables.push_back({});
            }
            circuit.observables[obs] *= resolve(p);
        }
        for (auto q : p.target_data.targets) {
            circuit.num_qubits = std::max(circuit.num_qubits, q + 1);
        }
    }
}

Circuit::Circuit(std::vector<Operation> init_operations)
    : operations(std::move(init_operations)), num_qubits(0), num_measurements(0), detectors(), observables() {
    init_nums(*this);
}

Circuit Circuit::from_file(FILE *file) {
    CircuitReader reader;
    while (reader.read_more(file, false, false)) {
    }
    return {reader.ops};
}

Circuit Circuit::from_text(const std::string &text) {
    CircuitReader reader;
    reader.read_all(text);
    return {reader.ops};
}

bool Operation::try_fuse_with(const Operation &other) {
    if (gate->flags & GATE_IS_NOT_FUSABLE) {
        return false;
    }
    if (gate->id == other.gate->id && target_data.arg == other.target_data.arg) {
        target_data += other.target_data;
        return true;
    }
    return false;
}

bool Operation::operator==(const Operation &other) const {
    return gate->id == other.gate->id && target_data.targets == other.target_data.targets &&
           target_data.arg == other.target_data.arg && target_data.metas == other.target_data.metas;
}

bool Operation::operator!=(const Operation &other) const {
    return !(*this == other);
}

bool Instruction::operator==(const Instruction &other) const {
    return type == other.type && op == other.op;
}

bool Instruction::operator!=(const Instruction &other) const {
    return !(*this == other);
}

bool Instruction::operator==(const Operation &other) const {
    return type == INSTRUCTION_TYPE_OPERATION && op == other;
}

bool Instruction::operator!=(const Operation &other) const {
    return !(*this == other);
}

bool Circuit::operator==(const Circuit &other) const {
    return num_qubits == other.num_qubits && operations == other.operations;
}
bool Circuit::operator!=(const Circuit &other) const {
    return !(*this == other);
}

void read_line_into(FILE *file, std::string &buffer) {
    while (true) {
        int i = getc(file);
        if (i == EOF) {
            buffer.push_back('\n');
            return;
        }
        if (i == '\n') {
            return;
        }
        buffer.push_back((char)i);
    }
}

void CircuitReader::read_all(const std::string &text) {
    size_t s = 0;
    size_t k = 0;
    while (read_more_helper(
        [&]() {
            buffer.clear();
            if (k >= text.size() || text[k] == '\0') {
                buffer.push_back('\n');
                return StringView(buffer);
            }
            while (true) {
                if (text[k] == '\n' || text[k] == '\0') {
                    auto result = StringView(text.data() + s, k - s);
                    k++;
                    s = k;
                    return result;
                }
                k++;
            }
        },
        false, false)) {
    }
}

bool CircuitReader::read_more(FILE *file, bool inside_block, bool stop_after_measurement) {
    return read_more_helper(
        [&]() {
            buffer.clear();
            read_line_into(file, buffer);
            return StringView(buffer);
        },
        inside_block, stop_after_measurement);
}

bool CircuitReader::read_more_helper(
    const std::function<StringView(void)> &line_getter, bool inside_block, bool stop_after_measurement) {
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

        instruction.op.target_data.clear();
        Instruction::from_line(line, view_buffer, instruction);
        if (instruction.type == INSTRUCTION_TYPE_EMPTY) {
            continue;
        }

        if (instruction.type == INSTRUCTION_TYPE_OPERATION) {
            if (!can_fuse || !ops.back().try_fuse_with(instruction.op)) {
                ops.push_back(instruction.op);
            }
            if (stop_after_measurement && (instruction.op.gate->flags & GATE_PRODUCES_RESULTS)) {
                return true;
            }
            read_any = true;
            can_fuse = true;
        } else if (instruction.type == INSTRUCTION_TYPE_BLOCK_OPERATION_START) {
            if (instruction.op.gate->id == gate_name_to_id("REPEAT")) {
                if (instruction.op.target_data.targets.size() != 1 || (bool)instruction.op.target_data.arg) {
                    throw std::out_of_range("Invalid instruction. Expected one repetition count like `REPEAT 100 {`.");
                }
                CircuitReader sub;
                sub.read_more_helper(line_getter, true, false);
                for (size_t k = 0; k < instruction.op.target_data.targets[0]; k++) {
                    for (auto &e : sub.ops) {
                        ops.push_back(e);
                    }
                }
            } else {
                throw std::out_of_range(
                    "'" + std::string(instruction.op.gate->name) + "' is not a block starting instruction.");
            }
            can_fuse = false;
            if (stop_after_measurement && !inside_block) {
                return true;
            }
        } else if (instruction.type == INSTRUCTION_TYPE_BLOCK_END) {
            if (!inside_block) {
                throw std::out_of_range("Uninitiated block. Got a '}' without a '{'.");
            }
            return true;
        } else {
            throw std::out_of_range("Unrecognized instruction type.");
        }
    }
}

std::ostream &operator<<(std::ostream &out, const Operation &op) {
    out << op.gate->name;
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
        if (op.target_data.metas.size() <= k) {
            out << "?!?";
        } else if (op.target_data.metas[k]) {
            out << '!';
        }
        out << op.target_data.targets[k];
    }
    if (op.target_data.metas.size() > op.target_data.targets.size()) {
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

OperationData::OperationData(size_t target) : targets({target}), metas({false}), arg(0) {
}
OperationData::OperationData(std::initializer_list<size_t> init_targets)
    : targets(init_targets), metas(init_targets.size(), false), arg(0) {
}
OperationData::OperationData(const std::vector<size_t> &init_targets)
    : targets({init_targets}), metas(init_targets.size(), false), arg(0) {
}
OperationData::OperationData(const std::vector<size_t> &init_targets, std::vector<size_t> init_flags, float init_arg)
    : targets(init_targets), metas(init_flags), arg(init_arg) {
}

OperationData &OperationData::operator+=(const OperationData &other) {
    targets.insert(targets.end(), other.targets.begin(), other.targets.end());
    metas.insert(metas.end(), other.metas.begin(), other.metas.end());
    return *this;
}

std::ostream &operator<<(std::ostream &out, const Instruction &inst) {
    if (inst.type == INSTRUCTION_TYPE_BLOCK_END) {
        out << "}";
    }
    if (inst.type == INSTRUCTION_TYPE_BLOCK_OPERATION_START || inst.type == INSTRUCTION_TYPE_OPERATION) {
        out << inst.op;
    }
    if (inst.type == INSTRUCTION_TYPE_BLOCK_OPERATION_START) {
        out << " {";
    }
    return out;
}
