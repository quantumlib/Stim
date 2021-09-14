/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stim/dem/detector_error_model.h"

#include <cmath>
#include <iomanip>

#include "stim/simulators/error_analyzer.h"
#include "stim/str_util.h"

using namespace stim;

constexpr uint64_t OBSERVABLE_BIT = uint64_t{1} << 63;
constexpr uint64_t SEPARATOR_SYGIL = UINT64_MAX;

DemTarget DemTarget::observable_id(uint64_t id) {
    if (id > 0xFFFFFFFF) {
        throw std::invalid_argument("id > 0xFFFFFFFF");
    }
    return {OBSERVABLE_BIT | id};
}
DemTarget DemTarget::relative_detector_id(uint64_t id) {
    if (id >= (uint64_t{1} << 62)) {
        throw std::invalid_argument("Relative detector id too large.");
    }
    return {id};
}
bool DemTarget::is_observable_id() const {
    return data != SEPARATOR_SYGIL && (data & OBSERVABLE_BIT);
}
bool DemTarget::is_separator() const {
    return data == SEPARATOR_SYGIL;
}
bool DemTarget::is_relative_detector_id() const {
    return data != SEPARATOR_SYGIL && !(data & OBSERVABLE_BIT);
}
uint64_t DemTarget::raw_id() const {
    return data & ~OBSERVABLE_BIT;
}

uint64_t DemTarget::val() const {
    if (data == SEPARATOR_SYGIL) {
        throw std::invalid_argument("Separator doesn't have an integer value.");
    }
    return raw_id();
}

bool DemTarget::operator==(const DemTarget &other) const {
    return data == other.data;
}
bool DemTarget::operator!=(const DemTarget &other) const {
    return !(*this == other);
}
bool DemTarget::operator<(const DemTarget &other) const {
    return data < other.data;
}
std::ostream &stim::operator<<(std::ostream &out, const DemTarget &v) {
    if (v.is_separator()) {
        out << "^";
        return out;
    } else if (v.is_relative_detector_id()) {
        out << "D" << v.raw_id();
    } else {
        out << "L" << v.raw_id();
    }
    return out;
}

std::string DemTarget::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

void DemTarget::shift_if_detector_id(int64_t offset) {
    if (is_relative_detector_id()) {
        data = (uint64_t)((int64_t)data + offset);
    }
}

bool DemInstruction::operator==(const DemInstruction &other) const {
    return approx_equals(other, 0);
}
bool DemInstruction::operator!=(const DemInstruction &other) const {
    return !(*this == other);
}
bool DemInstruction::approx_equals(const DemInstruction &other, double atol) const {
    if (target_data != other.target_data) {
        return false;
    }
    if (type != other.type) {
        return false;
    }
    if (arg_data.size() != other.arg_data.size()) {
        return false;
    }
    for (size_t k = 0; k < arg_data.size(); k++) {
        if (fabs(arg_data[k] - other.arg_data[k]) > atol) {
            return false;
        }
    }
    return true;
}
std::string DemInstruction::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

std::ostream &stim::operator<<(std::ostream &out, const DemInstructionType &type) {
    switch (type) {
        case DEM_ERROR:
            out << "error";
            break;
        case DEM_DETECTOR:
            out << "detector";
            break;
        case DEM_LOGICAL_OBSERVABLE:
            out << "logical_observable";
            break;
        case DEM_SHIFT_DETECTORS:
            out << "shift_detectors";
            break;
        case DEM_REPEAT_BLOCK:
            out << "repeat";
            break;
        default:
            out << "???unknown_instruction_type???";
            break;
    }
    return out;
}

std::ostream &stim::operator<<(std::ostream &out, const DemInstruction &op) {
    out << op.type;
    if (!op.arg_data.empty()) {
        out << "(" << comma_sep(op.arg_data) << ")";
    }
    if (op.type == DEM_SHIFT_DETECTORS || op.type == DEM_REPEAT_BLOCK) {
        for (const auto &e : op.target_data) {
            out << " " << e.data;
        }
    } else {
        for (const auto &e : op.target_data) {
            out << " " << e;
        }
    }
    return out;
}

void DemInstruction::validate() const {
    switch (type) {
        case DEM_ERROR:
            if (arg_data.size() != 1) {
                throw std::invalid_argument(
                    "'error' instruction takes 1 argument (a probability), but got " + std::to_string(arg_data.size()) +
                    " arguments.");
            }
            if (arg_data[0] < 0 || arg_data[0] > 1) {
                throw std::invalid_argument(
                    "'error' instruction argument must be a probability (0 to 1) but got " +
                    std::to_string(arg_data[0]));
            }
            if (!target_data.empty()) {
                if (target_data.front() == DemTarget::separator() || target_data.back() == DemTarget::separator()) {
                    throw std::invalid_argument(
                        "First/last targets of 'error' instruction shouldn't be separators (^).");
                }
            }
            for (size_t k = 1; k < target_data.size(); k++) {
                if (target_data[k - 1] == DemTarget::separator() && target_data[k] == DemTarget::separator()) {
                    throw std::invalid_argument("'error' instruction has adjacent separators (^ ^).");
                }
            }
            break;
        case DEM_SHIFT_DETECTORS:
            if (target_data.size() != 1) {
                throw std::invalid_argument(
                    "'shift_detectors' instruction takes 1 target, but got " + std::to_string(arg_data.size()) +
                    " targets.");
            }
            break;
        case DEM_DETECTOR:
            if (target_data.size() != 1) {
                throw std::invalid_argument(
                    "'detector' instruction takes 1 target but got " + std::to_string(target_data.size()) +
                    " arguments.");
            }
            if (!target_data[0].is_relative_detector_id()) {
                throw std::invalid_argument(
                    "'detector' instruction takes a relative detector target (D#) but got " + target_data[0].str() +
                    " arguments.");
            }
            break;
        case DEM_LOGICAL_OBSERVABLE:
            if (arg_data.size() != 0) {
                throw std::invalid_argument(
                    "'logical_observable' instruction takes 0 arguments but got " + std::to_string(arg_data.size()) +
                    " arguments.");
            }
            if (target_data.size() != 1) {
                throw std::invalid_argument(
                    "'logical_observable' instruction takes 1 target but got " + std::to_string(target_data.size()) +
                    " arguments.");
            }
            if (!target_data[0].is_observable_id()) {
                throw std::invalid_argument(
                    "'logical_observable' instruction takes a logical observable target (L#) but got " +
                    target_data[0].str() + " arguments.");
            }
            break;
        case DEM_REPEAT_BLOCK:
            // Handled elsewhere.
            break;
        default:
            throw std::invalid_argument("Unknown instruction type.");
    }
}

void DetectorErrorModel::append_error_instruction(double probability, ConstPointerRange<DemTarget> targets) {
    ConstPointerRange<double> args = {&probability};
    DemInstruction{args, targets, DEM_ERROR}.validate();
    auto stored_targets = target_buf.take_copy(targets);
    auto stored_args = arg_buf.take_copy(args);
    instructions.push_back(DemInstruction{stored_args, stored_targets, DEM_ERROR});
}

void DetectorErrorModel::append_shift_detectors_instruction(
    ConstPointerRange<double> coord_shift, uint64_t detector_shift) {
    DemTarget shift{detector_shift};
    ConstPointerRange<DemTarget> targets = {&shift};
    DemInstruction{coord_shift, targets, DEM_SHIFT_DETECTORS}.validate();

    auto stored_targets = target_buf.take_copy(targets);
    auto stored_args = arg_buf.take_copy(coord_shift);
    instructions.push_back(DemInstruction{stored_args, stored_targets, DEM_SHIFT_DETECTORS});
}

void DetectorErrorModel::append_detector_instruction(ConstPointerRange<double> coords, DemTarget target) {
    ConstPointerRange<DemTarget> targets = {&target};
    DemInstruction{coords, targets, DEM_DETECTOR}.validate();
    auto stored_targets = target_buf.take_copy(targets);
    auto stored_args = arg_buf.take_copy(coords);
    instructions.push_back(DemInstruction{stored_args, stored_targets, DEM_DETECTOR});
}

void DetectorErrorModel::append_logical_observable_instruction(DemTarget target) {
    ConstPointerRange<DemTarget> targets = {&target};
    DemInstruction{{}, targets, DEM_LOGICAL_OBSERVABLE}.validate();
    auto stored_targets = target_buf.take_copy(targets);
    instructions.push_back(DemInstruction{{}, stored_targets, DEM_LOGICAL_OBSERVABLE});
}

void DetectorErrorModel::append_repeat_block(uint64_t repeat_count, DetectorErrorModel &&body) {
    DemTarget data[2];
    data[0].data = repeat_count;
    data[1].data = blocks.size();
    auto stored_targets = target_buf.take_copy({&data[0], &data[2]});
    blocks.push_back(std::move(body));
    instructions.push_back({{}, stored_targets, DEM_REPEAT_BLOCK});
}

void DetectorErrorModel::append_repeat_block(uint64_t repeat_count, const DetectorErrorModel &body) {
    DemTarget data[2];
    data[0].data = repeat_count;
    data[1].data = blocks.size();
    auto stored_targets = target_buf.take_copy({&data[0], &data[2]});
    blocks.push_back(body);
    instructions.push_back({{}, stored_targets, DEM_REPEAT_BLOCK});
}

bool DetectorErrorModel::operator==(const DetectorErrorModel &other) const {
    return instructions == other.instructions && blocks == other.blocks;
}
bool DetectorErrorModel::operator!=(const DetectorErrorModel &other) const {
    return !(*this == other);
}
bool DetectorErrorModel::approx_equals(const DetectorErrorModel &other, double atol) const {
    if (instructions.size() != other.instructions.size() || blocks.size() != other.blocks.size()) {
        return false;
    }
    for (size_t k = 0; k < instructions.size(); k++) {
        if (!instructions[k].approx_equals(other.instructions[k], atol)) {
            return false;
        }
    }
    for (size_t k = 0; k < blocks.size(); k++) {
        if (!blocks[k].approx_equals(other.blocks[k], atol)) {
            return false;
        }
    }
    return true;
}
std::string DetectorErrorModel::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

void stim::print_detector_error_model(std::ostream &out, const DetectorErrorModel &v, size_t indent) {
    bool first = true;
    for (const auto &e : v.instructions) {
        if (first) {
            first = false;
        } else {
            out << "\n";
        }
        for (size_t k = 0; k < indent; k++) {
            out << " ";
        }
        if (e.type == DEM_REPEAT_BLOCK) {
            out << "repeat " << e.target_data[0].data << " {\n";
            print_detector_error_model(out, v.blocks[(size_t)e.target_data[1].data], indent + 4);
            out << "\n";
            for (size_t k = 0; k < indent; k++) {
                out << " ";
            }
            out << "}";
        } else {
            out << e;
        }
    }
}

std::ostream &stim::operator<<(std::ostream &out, const DetectorErrorModel &v) {
    out << std::setprecision(std::numeric_limits<long double>::digits10 + 1);
    print_detector_error_model(out, v, 0);
    return out;
}

DetectorErrorModel::DetectorErrorModel() {
}

DetectorErrorModel::DetectorErrorModel(const DetectorErrorModel &other)
    : arg_buf(other.arg_buf.total_allocated()),
      target_buf(other.target_buf.total_allocated()),
      instructions(other.instructions),
      blocks(other.blocks) {
    // Keep local copy of buffer data.
    for (auto &e : instructions) {
        e.arg_data = arg_buf.take_copy(e.arg_data);
        e.target_data = target_buf.take_copy(e.target_data);
    }
}

DetectorErrorModel::DetectorErrorModel(DetectorErrorModel &&other) noexcept
    : arg_buf(std::move(other.arg_buf)),
      target_buf(std::move(other.target_buf)),
      instructions(std::move(other.instructions)),
      blocks(std::move(other.blocks)) {
}

DetectorErrorModel &DetectorErrorModel::operator=(const DetectorErrorModel &other) {
    if (&other != this) {
        instructions = other.instructions;
        blocks = other.blocks;

        // Keep local copy of operation data.
        arg_buf = MonotonicBuffer<double>(other.arg_buf.total_allocated());
        target_buf = MonotonicBuffer<DemTarget>(other.target_buf.total_allocated());
        for (auto &e : instructions) {
            e.arg_data = arg_buf.take_copy(e.arg_data);
            e.target_data = target_buf.take_copy(e.target_data);
        }
    }
    return *this;
}

DetectorErrorModel &DetectorErrorModel::operator=(DetectorErrorModel &&other) noexcept {
    if (&other != this) {
        instructions = std::move(other.instructions);
        blocks = std::move(other.blocks);
        arg_buf = std::move(other.arg_buf);
        target_buf = std::move(other.target_buf);
    }
    return *this;
}

enum DEM_READ_CONDITION {
    DEM_READ_AS_LITTLE_AS_POSSIBLE,
    DEM_READ_UNTIL_END_OF_BLOCK,
    DEM_READ_UNTIL_END_OF_FILE,
};

inline bool is_name_char(int c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

template <typename SOURCE>
inline DemInstructionType read_instruction_name(int &c, SOURCE read_char) {
    char name_buf[32];
    size_t n = 0;
    while (is_name_char(c) && n < sizeof(name_buf) - 1) {
        name_buf[n] = tolower((char)c);
        c = read_char();
        n++;
    }
    name_buf[n] = 0;
    if (!strcmp(name_buf, "error")) {
        return DEM_ERROR;
    }
    if (!strcmp(name_buf, "shift_detectors")) {
        return DEM_SHIFT_DETECTORS;
    }
    if (!strcmp(name_buf, "detector")) {
        return DEM_DETECTOR;
    }
    if (!strcmp(name_buf, "logical_observable")) {
        return DEM_LOGICAL_OBSERVABLE;
    }
    if (!strcmp(name_buf, "repeat")) {
        return DEM_REPEAT_BLOCK;
    }
    throw std::out_of_range("Unrecognized instruction name: " + std::string(name_buf));
}

template <typename SOURCE>
uint64_t read_uint60_t(int &c, SOURCE read_char) {
    if (!(c >= '0' && c <= '9')) {
        throw std::invalid_argument("Expected a digit but got '" + std::string(1, c) + "'");
    }
    uint64_t result = 0;
    do {
        result *= 10;
        result += c - '0';
        if (result >= uint64_t{1} << 60) {
            throw std::out_of_range("Number too large.");
        }
        c = read_char();
    } while (c >= '0' && c <= '9');
    return result;
}

template <typename SOURCE>
inline void read_arbitrary_dem_targets_into(int &c, SOURCE read_char, DetectorErrorModel &model) {
    while (read_until_next_line_arg(c, read_char)) {
        switch (c) {
            case 'd':
            case 'D':
                c = read_char();
                model.target_buf.append_tail(DemTarget::relative_detector_id(read_uint60_t(c, read_char)));
                break;
            case 'l':
            case 'L':
                c = read_char();
                model.target_buf.append_tail(DemTarget::observable_id(read_uint60_t(c, read_char)));
                break;
            case '^':
                c = read_char();
                model.target_buf.append_tail(DemTarget::separator());
                break;
            default:
                throw std::invalid_argument("Unrecognized target prefix '" + std::string(1, c) + "'.");
        }
    }
}

template <typename SOURCE>
void dem_read_instruction(DetectorErrorModel &model, char lead_char, SOURCE read_char) {
    int c = (int)lead_char;
    auto type = read_instruction_name(c, read_char);
    try {
        if (type == DEM_REPEAT_BLOCK) {
            if (!read_until_next_line_arg(c, read_char)) {
                throw std::invalid_argument("Missing repeat count of repeat block.");
            }
            model.target_buf.append_tail(DemTarget{read_uint60_t(c, read_char)});
            if (read_until_next_line_arg(c, read_char)) {
                throw std::invalid_argument("Too many numeric values given to repeat block.");
            }
            if (c != '{') {
                throw std::invalid_argument("Missing '{' at start of repeat block.");
            }
        } else {
            read_parens_arguments(c, "detector error model instruction", read_char, model.arg_buf);
            if (type == DEM_SHIFT_DETECTORS) {
                if (read_until_next_line_arg(c, read_char)) {
                    model.target_buf.append_tail(DemTarget{read_uint60_t(c, read_char)});
                }
            }
            read_arbitrary_dem_targets_into(c, read_char, model);
            if (c == '{') {
                throw std::invalid_argument("Unexpected '{'.");
            }
            DemInstruction{model.arg_buf.tail, model.target_buf.tail, type}.validate();
        }
    } catch (const std::invalid_argument &ex) {
        model.target_buf.discard_tail();
        model.arg_buf.discard_tail();
        throw ex;
    }

    model.instructions.push_back(DemInstruction{model.arg_buf.commit_tail(), model.target_buf.commit_tail(), type});
}

template <typename SOURCE>
void model_read_operations(DetectorErrorModel &model, SOURCE read_char, DEM_READ_CONDITION read_condition) {
    auto &ops = model.instructions;
    do {
        int c = read_char();
        read_past_dead_space_between_commands(c, read_char);
        if (c == EOF) {
            if (read_condition == DEM_READ_UNTIL_END_OF_BLOCK) {
                throw std::out_of_range("Unterminated block. Got a '{' without an eventual '}'.");
            }
            return;
        }
        if (c == '}') {
            if (read_condition != DEM_READ_UNTIL_END_OF_BLOCK) {
                throw std::out_of_range("Uninitiated block. Got a '}' without a '{'.");
            }
            return;
        }
        dem_read_instruction(model, c, read_char);

        if (ops.back().type == DEM_REPEAT_BLOCK) {
            // Temporarily remove instruction until block is parse.
            auto repeat_count = ops.back().target_data[0].data;
            ops.pop_back();

            // Recursively read the block contents.
            DetectorErrorModel block;
            model_read_operations(block, read_char, DEM_READ_UNTIL_END_OF_BLOCK);

            // Restore repeat block instruction, including block reference.
            model.append_repeat_block(repeat_count, std::move(block));
        }
    } while (read_condition != DEM_READ_AS_LITTLE_AS_POSSIBLE);
}

void DetectorErrorModel::append_from_file(FILE *file, bool stop_asap) {
    model_read_operations(
        *this,
        [&]() {
            return getc(file);
        },
        stop_asap ? DEM_READ_AS_LITTLE_AS_POSSIBLE : DEM_READ_UNTIL_END_OF_FILE);
}

void DetectorErrorModel::append_from_text(const char *text) {
    size_t k = 0;
    model_read_operations(
        *this,
        [&]() {
            return text[k] != 0 ? text[k++] : EOF;
        },
        DEM_READ_UNTIL_END_OF_FILE);
}

DetectorErrorModel DetectorErrorModel::from_file(FILE *file) {
    DetectorErrorModel result;
    result.append_from_file(file, false);
    return result;
}

DetectorErrorModel::DetectorErrorModel(const char *text) {
    append_from_text(text);
}
void DetectorErrorModel::clear() {
    target_buf.clear();
    arg_buf.clear();
    instructions.clear();
    blocks.clear();
}

uint64_t DetectorErrorModel::total_detector_shift() const {
    uint64_t result = 0;
    for (const auto &e : instructions) {
        if (e.type == DEM_SHIFT_DETECTORS) {
            result += e.target_data[0].data;
        } else if (e.type == DEM_REPEAT_BLOCK) {
            result += e.target_data[0].data * blocks[e.target_data[1].data].total_detector_shift();
        }
    }
    return result;
}

uint64_t DetectorErrorModel::count_detectors() const {
    uint64_t offset = 1;
    uint64_t max_num = 0;
    for (const auto &e : instructions) {
        switch (e.type) {
            case DEM_LOGICAL_OBSERVABLE:
                break;
            case DEM_SHIFT_DETECTORS:
                offset += e.target_data[0].data;
                break;
            case DEM_REPEAT_BLOCK: {
                auto &block = blocks[e.target_data[1].data];
                auto n = block.count_detectors();
                auto reps = e.target_data[0].data;
                auto block_shift = block.total_detector_shift();  // Note: quadratic overhead in nesting level.
                offset += block_shift * reps;
                if (reps > 0 && n > 0) {
                    max_num = std::max(max_num, offset + n - 1 - block_shift);
                }
            } break;
            case DEM_DETECTOR:
            case DEM_ERROR:
                for (const auto &t : e.target_data) {
                    if (t.is_relative_detector_id()) {
                        max_num = std::max(max_num, offset + t.raw_id());
                    }
                }
                break;
            default:
                throw std::invalid_argument("Instruction type not implemented in count_detectors: " + e.str());
        }
    }
    return max_num;
}

uint64_t DetectorErrorModel::count_observables() const {
    uint64_t max_num = 0;
    for (const auto &e : instructions) {
        switch (e.type) {
            case DEM_SHIFT_DETECTORS:
            case DEM_DETECTOR:
                break;
            case DEM_REPEAT_BLOCK: {
                auto &block = blocks[e.target_data[1].data];
                max_num = std::max(max_num, block.count_observables());
            } break;
            case DEM_LOGICAL_OBSERVABLE:
            case DEM_ERROR:
                for (const auto &t : e.target_data) {
                    if (t.is_observable_id()) {
                        max_num = std::max(max_num, t.raw_id() + 1);
                    }
                }
                break;
            default:
                throw std::invalid_argument("Instruction type not implemented in count_observables: " + e.str());
        }
    }
    return max_num;
}

DetectorErrorModel DetectorErrorModel::py_get_slice(int64_t start, int64_t step, int64_t slice_length) const {
    assert(start >= 0);
    assert(slice_length >= 0);
    DetectorErrorModel result;
    for (size_t k = 0; k < (size_t)slice_length; k++) {
        const auto &op = instructions[start + step * k];
        if (op.type == DEM_REPEAT_BLOCK) {
            result.append_repeat_block(op.target_data[0].data, blocks[op.target_data[1].data]);
        } else {
            auto args = result.arg_buf.take_copy(op.arg_data);
            auto targets = result.target_buf.take_copy(op.target_data);
            result.instructions.push_back(DemInstruction{args, targets, op.type});
        }
    }
    return result;
}
