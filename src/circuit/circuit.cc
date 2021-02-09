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

#include "../simulators/tableau_simulator.h"
#include "../stabilizers/tableau.h"
#include "gate_data.h"

enum READ_CONDITION {
    READ_AS_LITTLE_AS_POSSIBLE,
    READ_UNTIL_END_OF_BLOCK,
    READ_UNTIL_END_OF_FILE,
};

MeasurementSet &MeasurementSet::operator*=(const MeasurementSet &other) {
    indices.insert(indices.end(), other.indices.begin(), other.indices.end());
    expected_parity ^= other.expected_parity;
    return *this;
}

std::pair<std::vector<MeasurementSet>, std::vector<MeasurementSet>> Circuit::list_detectors_and_observables() const {
    std::unordered_map<size_t, std::vector<size_t>> qubit_measure_indices;
    auto resolve = [&](const Operation &op) {
        MeasurementSet result{};
        for (size_t k = 0; k < op.target_data.targets.size(); k += 2) {
            auto q = op.target_data.targets[k];
            auto dt = op.target_data.targets[k + 1];
            const auto &v = qubit_measure_indices[q];
            if (dt < v.size()) {
                result.indices.push_back(v[v.size() - dt - 1]);
            }
        }
        return result;
    };

    size_t m = 0;
    std::vector<MeasurementSet> detectors;
    std::vector<MeasurementSet> observables;
    for (const auto &p : operations) {
        if (p.gate->flags & GATE_PRODUCES_RESULTS) {
            for (size_t k = 0; k < p.target_data.targets.size(); k++) {
                auto q = p.target_data.targets[k] & MEASURE_TARGET_MASK;
                qubit_measure_indices[q].push_back(m++);
            }
        } else if (p.gate->id == gate_name_to_id("DETECTOR")) {
            detectors.push_back(resolve(p));
        } else if (p.gate->id == gate_name_to_id("OBSERVABLE_INCLUDE")) {
            size_t obs = (size_t)p.target_data.arg;
            if (obs != p.target_data.arg) {
                throw std::out_of_range("Observable index must be an integer.");
            }
            while (observables.size() <= obs) {
                observables.push_back({});
            }
            observables[obs] *= resolve(p);
        }
    }

    return {detectors, observables};
}

Circuit::Circuit() : jagged_data(), operations(), num_qubits(0), num_measurements(0) {
}

Circuit::Circuit(const Circuit &circuit) : jagged_data(circuit.jagged_data), operations(circuit.operations), num_qubits(circuit.num_qubits), num_measurements(circuit.num_measurements) {
    for (auto &op : operations) {
        op.target_data.targets.arena = &jagged_data;
    }
}

Circuit::Circuit(Circuit &&circuit) noexcept :
        jagged_data(std::move(circuit.jagged_data)),
        operations(std::move(circuit.operations)),
        num_qubits(circuit.num_qubits),
        num_measurements(circuit.num_measurements) {
    for (auto &op : operations) {
        op.target_data.targets.arena = &jagged_data;
    }
}

Circuit& Circuit::operator=(const Circuit &circuit) {
    if (&circuit != this) {
        num_qubits = circuit.num_qubits;
        num_measurements = circuit.num_measurements;
        jagged_data = circuit.jagged_data;
        operations = circuit.operations;
        for (auto &op : operations) {
            op.target_data.targets.arena = &jagged_data;
        }
    }
    return *this;
}

Circuit& Circuit::operator=(Circuit &&circuit) noexcept {
    if (&circuit != this) {
        num_qubits = circuit.num_qubits;
        num_measurements = circuit.num_measurements;
        jagged_data = std::move(circuit.jagged_data);
        operations = std::move(circuit.operations);
        for (auto &op : operations) {
            op.target_data.targets.arena = &jagged_data;
        }
    }
    return *this;
}

bool Operation::can_fuse(const Operation &other) const {
    return gate->id == other.gate->id && target_data.arg == other.target_data.arg &&
           !(gate->flags & GATE_IS_NOT_FUSABLE);
}

bool Operation::operator==(const Operation &other) const {
    return gate->id == other.gate->id && target_data == other.target_data;
}

bool Operation::operator!=(const Operation &other) const {
    return !(*this == other);
}

bool OperationData::operator==(const OperationData &other) const {
    return arg == other.arg && targets == other.targets;
}
bool OperationData::operator!=(const OperationData &other) const {
    return !(*this == other);
}

bool Circuit::operator==(const Circuit &other) const {
    return num_qubits == other.num_qubits && num_measurements == other.num_measurements &&
           operations == other.operations;
}
bool Circuit::operator!=(const Circuit &other) const {
    return !(*this == other);
}

template <typename SOURCE>
inline void read_past_within_line_whitespace(int &c, SOURCE read_char) {
    while (c == ' ' || c == '\t') {
        c = read_char();
    }
}

inline bool is_name_char(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

template <typename SOURCE>
inline const Gate &read_gate_name(int &c, SOURCE read_char) {
    char name_buf[32];
    size_t n = 0;
    while (is_name_char(c) && n < sizeof(name_buf)) {
        name_buf[n] = (char)c;
        c = read_char();
        n++;
    }
    // Note: in the name-too-long case, the full buffer name won't match any gate and an exception will fire.
    return GATE_DATA.at(name_buf, n);
}

template <typename SOURCE>
double read_parens_argument(int &c, const Gate &gate, SOURCE read_char) {
    if (c != '(') {
        throw std::out_of_range("Gate " + std::string(gate.name) + "(X) missing a parens argument.");
    }
    c = read_char();
    read_past_within_line_whitespace(c, read_char);
    double result = read_non_negative_double(c, read_char);
    read_past_within_line_whitespace(c, read_char);
    if (c != ')') {
        throw std::out_of_range("Gate " + std::string(gate.name) + "(X) missing a closing parens for its argument.");
    }
    c = read_char();
    return result;
}

template <typename SOURCE>
uint32_t read_uint24_t(int &c, SOURCE read_char) {
    if (!(c >= '0' && c <= '9')) {
        throw std::out_of_range("Expected a digit but got " + std::string(1, c));
    }
    uint32_t result = 0;
    do {
        result *= 10;
        result += c - '0';
        if (result >= uint32_t{1} << 24) {
            throw std::out_of_range("Number too large.");
        }
        c = read_char();
    } while (c >= '0' && c <= '9');
    return result;
}

inline bool is_double_char(int c) {
    return (c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-';
}

template <typename SOURCE>
double read_non_negative_double(int &c, SOURCE read_char) {
    char buf[64];
    size_t n = 0;
    while (n < sizeof(buf) - 1 && is_double_char(c)) {
        buf[n] = (char)c;
        c = read_char();
        n++;
    }
    buf[n] = '\0';

    char *end;
    double result = strtod(buf, &end);
    if (end != buf + n || !(result >= 0)) {
        throw std::out_of_range("Not a non-negative real number: " + std::string(buf));
    }
    return result;
}

template <typename SOURCE>
bool read_until_next_line_arg(int &c, SOURCE read_char) {
    if (c != ' ' && c != '#' && c != '\t' && c != '\n' && c != '{' && c != EOF) {
        throw std::out_of_range("Gate targets must be separated by spacing.");
    }
    while (c == ' ' || c == '\t') {
        c = read_char();
    }
    if (c == '#') {
        do {
            c = read_char();
        } while (c != '\n' && c != EOF);
    }
    return c != '\n' && c != '{' && c != EOF;
}

template <typename SOURCE>
inline void read_normal_targets_into(int &c, SOURCE read_char, Circuit &circuit) {
    while (read_until_next_line_arg(c, read_char)) {
        uint32_t q = read_uint24_t(c, read_char);
        circuit.jagged_data.push_back(q);
        circuit.num_qubits = std::max(circuit.num_qubits, (size_t)q + 1);
    }
}

template <typename SOURCE>
inline void read_result_targets_into(int &c, SOURCE read_char, const Gate &gate, Circuit &circuit) {
    while (read_until_next_line_arg(c, read_char)) {
        uint32_t flipped = c == '!' ? uint32_t{1} << 31 : 0;
        if (flipped) {
            c = read_char();
        }
        uint32_t q = read_uint24_t(c, read_char);
        circuit.num_qubits = std::max(circuit.num_qubits, (size_t)q + 1);
        circuit.jagged_data.push_back(q ^ flipped);
        circuit.num_measurements++;
    }
}

template <typename SOURCE>
inline void read_record_targets_into(int &c, SOURCE read_char, Circuit &circuit) {
    while (read_until_next_line_arg(c, read_char)) {
        uint32_t q = read_uint24_t(c, read_char);
        circuit.jagged_data.push_back(q);
        circuit.num_qubits = std::max(circuit.num_qubits, (size_t)q + 1);

        if (c != '@') {
            throw std::out_of_range("Missing @ in record target (like '2@-3')");
        }
        if (read_char() != '-') {
            throw std::out_of_range("Missing - after @ in record target (like '2@-3')");
        }
        c = read_char();
        size_t dt = read_uint24_t(c, read_char);
        if (dt == 0) {
            throw std::out_of_range("Minimum lookback in record target (like 2@-3) is -1, not -0.");
        }
        circuit.jagged_data.push_back(dt - 1);
    }
}

template <typename SOURCE>
void read_past_dead_space_between_commands(int &c, SOURCE read_char) {
    while (true) {
        while (isspace(c)) {
            c = read_char();
        }
        if (c == EOF) {
            break;
        }
        if (c != '#') {
            break;
        }
        while (c != '\n' && c != EOF) {
            c = read_char();
        }
    }
}

template <typename SOURCE>
void circuit_read_single_operation(Circuit &circuit, char lead_char, SOURCE read_char) {
    int c = lead_char;
    const auto &gate = read_gate_name(c, read_char);
    double val = 0;
    if (gate.flags & GATE_TAKES_PARENS_ARGUMENT) {
        read_past_within_line_whitespace(c, read_char);
        val = read_parens_argument(c, gate, read_char);
    }
    size_t offset = circuit.jagged_data.size();
    if (!(gate.flags & (GATE_IS_BLOCK | GATE_TARGETS_MEASUREMENT_RECORD | GATE_PRODUCES_RESULTS))) {
        read_normal_targets_into(c, read_char, circuit);
    } else if (gate.flags & GATE_TARGETS_MEASUREMENT_RECORD) {
        read_record_targets_into(c, read_char, circuit);
    } else if (gate.flags & GATE_PRODUCES_RESULTS) {
        read_result_targets_into(c, read_char, gate, circuit);
    } else {
        while (read_until_next_line_arg(c, read_char)) {
            circuit.jagged_data.push_back(read_uint24_t(c, read_char));
        }
    }
    if (c != '{' && (gate.flags & GATE_IS_BLOCK && c != '{')) {
        throw std::out_of_range("Missing '{' at start of " + std::string(gate.name) + " block.");
    }
    if (c == '{' && !(gate.flags & GATE_IS_BLOCK)) {
        throw std::out_of_range("Unexpected '{' after non-block command " + std::string(gate.name) + ".");
    }

    circuit.operations.push_back({&gate, {val, {&circuit.jagged_data, offset, circuit.jagged_data.size() - offset}}});
}

template <typename SOURCE>
void circuit_read_operations(Circuit &circuit, SOURCE read_char, READ_CONDITION read_condition) {
    auto &ops = circuit.operations;
    bool can_fuse = false;
    do {
        int c = read_char();
        read_past_dead_space_between_commands(c, read_char);
        if (c == EOF) {
            if (read_condition == READ_UNTIL_END_OF_BLOCK) {
                throw std::out_of_range("Unterminated block. Got a '{' without an eventual '}'.");
            }
            return;
        }
        if (c == '}') {
            if (read_condition != READ_UNTIL_END_OF_BLOCK) {
                throw std::out_of_range("Uninitiated block. Got a '}' without a '{'.");
            }
            return;
        }
        size_t s = ops.size();
        circuit_read_single_operation(circuit, c, read_char);

        if (ops[s].gate->id == gate_name_to_id("REPEAT")) {
            if (ops[s].target_data.targets.size() != 1) {
                throw std::out_of_range("Invalid instruction. Expected one repetition arg like `REPEAT 100 {`.");
            }
            size_t rep_count = circuit.jagged_data.back();
            circuit.jagged_data.pop_back();
            ops.pop_back();
            if (rep_count == 0) {
                throw std::out_of_range("Repeating 0 times is not supported.");
            }
            size_t ops_start = ops.size();
            size_t num_measure_start = circuit.num_measurements;
            circuit_read_operations(circuit, read_char, READ_UNTIL_END_OF_BLOCK);
            size_t ops_end = ops.size();
            circuit.num_measurements += (circuit.num_measurements - num_measure_start) * (rep_count - 1);
            while (rep_count > 1) {
                ops.insert(ops.end(), ops.data() + ops_start, ops.data() + ops_end);
                rep_count--;
            }
            can_fuse = false;
        } else if (can_fuse && ops[s - 1].can_fuse(ops[s])) {
            ops[s - 1].target_data.targets.length += ops[s].target_data.targets.length;
            ops.pop_back();
        } else {
            can_fuse = true;
        }
    } while (read_condition != READ_AS_LITTLE_AS_POSSIBLE);
}

bool Circuit::append_from_text(const std::string &text) {
    size_t before = operations.size();
    size_t k = 0;
    circuit_read_operations(
        *this,
        [&]() {
            return k < text.size() ? text[k++] : EOF;
        },
        READ_UNTIL_END_OF_FILE);
    return operations.size() > before;
}

void _circuit_incremental_update_from_back(Circuit &circuit) {
    const auto &op = circuit.operations.back();
    const auto &gate = *op.gate;
    const auto &vec = op.target_data.targets;
    if (gate.flags & GATE_PRODUCES_RESULTS) {
        circuit.num_measurements += vec.size();
        for (auto q : vec) {
            circuit.num_qubits = std::max(circuit.num_qubits, (size_t)((q & MEASURE_TARGET_MASK) + 1));
        }
    } else if (gate.flags & GATE_TARGETS_MEASUREMENT_RECORD) {
        for (size_t k = 0; k < vec.size(); k += 2) {
            circuit.num_qubits = std::max(circuit.num_qubits, (size_t)(vec[k] + 1));
        }
    } else {
        for (auto q : vec) {
            circuit.num_qubits = std::max(circuit.num_qubits, (size_t)(q + 1));
        }
    }
}

void Circuit::append_circuit(const Circuit &circuit, size_t repetitions) {
    if (!repetitions) {
        return;
    }
    auto original_size = operations.size();

    if (&circuit == this) {
        num_measurements *= repetitions + 1;
        do {
            operations.insert(operations.end(), operations.begin(), operations.begin() + original_size);
        } while (--repetitions);
        return;
    }

    for (const auto &op : circuit.operations) {
        append_operation(op);
    }
    auto single_rep_end = operations.end();
    while (--repetitions) {
        operations.insert(operations.end(), operations.begin() + original_size, single_rep_end);
    }
}

void Circuit::append_operation(const Operation &operation) {
    Operation converted{
        operation.gate,
        {operation.target_data.arg, {&jagged_data, jagged_data.size(), operation.target_data.targets.length}}};
    operations.push_back(converted);
    jagged_data.insert(jagged_data.end(), operation.target_data.targets.begin(), operation.target_data.targets.end());
    _circuit_incremental_update_from_back(*this);
}

void Circuit::append_op(const std::string &gate_name, const std::vector<uint32_t> &vec, double arg) {
    Operation converted{&GATE_DATA.at(gate_name), {arg, {&jagged_data, jagged_data.size(), vec.size()}}};
    operations.push_back(converted);
    jagged_data.insert(jagged_data.end(), vec.begin(), vec.end());
    _circuit_incremental_update_from_back(*this);
}

bool Circuit::append_from_file(FILE *file, bool stop_asap) {
    size_t before = operations.size();
    circuit_read_operations(
            *this,
            [&]() {
            return getc(file);
        },
            stop_asap ? READ_AS_LITTLE_AS_POSSIBLE : READ_UNTIL_END_OF_FILE);
    return operations.size() > before;
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
        if (op.gate->flags & GATE_PRODUCES_RESULTS) {
            if (op.target_data.targets[k] & (uint32_t{1} << 31)) {
                out << '!';
            }
            out << (op.target_data.targets[k] & ~(uint32_t{1} << 31));
        } else if (op.gate->flags & GATE_TARGETS_MEASUREMENT_RECORD) {
            out << op.target_data.targets[k];
            out << '@';
            k++;
            if (k < op.target_data.targets.size()) {
                out << "-" << (op.target_data.targets[k] + 1);
            }
        } else {
            out << op.target_data.targets[k];
        }
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const Circuit &c) {
    out << "# Circuit [num_qubits=" << c.num_qubits << ", num_measurements=" << c.num_measurements << "]";
    for (const auto &op : c.operations) {
        out << "\n" << op;
    }
    return out;
}

void Circuit::clear() {
    num_qubits = 0;
    num_measurements = 0;
    jagged_data.clear();
    operations.clear();
}

Circuit Circuit::operator+(const Circuit &other) const {
    Circuit result = *this;
    result += other;
    return result;
}
Circuit Circuit::operator*(size_t repetitions) const {
    Circuit result = *this;
    result *= repetitions;
    return result;
}
Circuit &Circuit::operator+=(const Circuit &other) {
    append_circuit(other, 1);
    return *this;
}
Circuit &Circuit::operator*=(size_t repetitions) {
    if (repetitions == 0) {
        clear();
    } else {
        append_circuit(*this, repetitions - 1);
    }
    return *this;
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

Circuit Circuit::from_file(FILE *file) {
    Circuit result;
    result.append_from_file(file, false);
    return result;
}

Circuit Circuit::from_text(const std::string &text) {
    Circuit result;
    result.append_from_text(text);
    return result;
}
