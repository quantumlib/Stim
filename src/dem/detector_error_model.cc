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

#include "detector_error_model.h"

#include <cmath>
#include <iomanip>

#include "../simulators/error_fuser.h"

using namespace stim_internal;

constexpr uint64_t MASK_REL_BIT = uint64_t{1} << 63;
constexpr uint64_t UNSPECIFIED_SYGIL = UINT64_MAX - uint64_t{2};
constexpr uint64_t OBSERVABLE_SYGIL = UINT64_MAX - uint64_t{3};
constexpr uint64_t SEPARATOR_SYGIL = UINT64_MAX - uint64_t{4};

uint64_t DemRelValue::raw_value() const {
    if (is_unspecified()) {
        return 0;
    }
    return data & ~MASK_REL_BIT;
}
uint64_t DemRelValue::absolute_value(uint64_t t) const {
    if (is_unspecified()) {
        return 0;
    }
    return raw_value() + is_relative() * t;
}
DemRelValue DemRelValue::unspecified() {
    return {UNSPECIFIED_SYGIL};
}
bool DemRelValue::is_relative() const {
    return data != UNSPECIFIED_SYGIL && (data & MASK_REL_BIT);
}
bool DemRelValue::is_absolute() const {
    return data != UNSPECIFIED_SYGIL && !(data & MASK_REL_BIT);
}
bool DemRelValue::is_unspecified() const {
    return data == UNSPECIFIED_SYGIL;
}
DemRelValue DemRelValue::relative(uint64_t v) {
    if (v & MASK_REL_BIT) {
        throw std::out_of_range("v & MASK_REL_BIT");
    }
    return {v | MASK_REL_BIT};
}
DemRelValue DemRelValue::absolute(uint64_t v) {
    if (v & MASK_REL_BIT) {
        throw std::out_of_range("v & MASK_REL_BIT");
    }
    return {v};
}
bool DemRelValue::operator==(const DemRelValue &other) const {
    return data == other.data;
}
bool DemRelValue::operator!=(const DemRelValue &other) const {
    return data != other.data;
}

std::string DemRelValue::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

std::ostream &operator<<(std::ostream &out, const DemRelValue &v) {
    if (v.is_unspecified()) {
        out << "unspecified";
    } else {
        out << v.raw_value();
        if (v.is_relative()) {
            out << "+t";
        }
    }
    return out;
}

DemRelativeSymptom DemRelativeSymptom::observable_id(uint32_t id) {
    return {{OBSERVABLE_SYGIL}, {0}, DemRelValue::absolute(id)};
}
DemRelativeSymptom DemRelativeSymptom::detector_id(DemRelValue x, DemRelValue y, DemRelValue t) {
    return {x, y, t};
}
DemRelativeSymptom DemRelativeSymptom::separator() {
    return {{SEPARATOR_SYGIL}, {0}, {0}};
}
bool DemRelativeSymptom::is_observable_id() const {
    return x.data == OBSERVABLE_SYGIL;
}
bool DemRelativeSymptom::is_separator() const {
    return x.data == SEPARATOR_SYGIL;
}
bool DemRelativeSymptom::is_detector_id() const {
    return !is_observable_id() && !is_separator();
}

bool DemRelativeSymptom::operator==(const DemRelativeSymptom &other) const {
    return t == other.t && y == other.y && x == other.x;
}
bool DemRelativeSymptom::operator!=(const DemRelativeSymptom &other) const {
    return !(*this == other);
}
std::ostream &operator<<(std::ostream &out, const DemRelativeSymptom &v) {
    std::array<DemRelValue, 3> coords{v.x, v.y, v.t};
    if (v.is_separator()) {
        out << "^";
        return out;
    } else if (v.is_detector_id()) {
        out << "D";
        bool first = true;
        for (const auto &e : coords) {
            if (!e.is_unspecified()) {
                if (first) {
                    first = false;
                } else {
                    out << ",";
                }
                out << e;
            }
        }
    } else if (v.is_observable_id()) {
        out << "L" << v.t.raw_value();
    } else {
        out << "???";
    }
    return out;
}

std::string DemRelativeSymptom::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

bool DemInstruction::operator==(const DemInstruction &other) const {
    return approx_equals(other, 0);
}
bool DemInstruction::operator!=(const DemInstruction &other) const {
    return !(*this == other);
}
bool DemInstruction::approx_equals(const DemInstruction &other, double atol) const {
    return fabs(probability - other.probability) <= atol && target_data == other.target_data && type == other.type;
}
std::string DemInstruction::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}
std::ostream &operator<<(std::ostream &out, const DemInstruction &op) {
    switch (op.type) {
        case DEM_ERROR:
            out << "error(" << op.probability << ")";
            for (const auto &e : op.target_data) {
                out << " " << e;
            }
            break;
        case DEM_REDUCIBLE_ERROR:
            out << "reducible_error(" << op.probability << ")";
            for (const auto &e : op.target_data) {
                out << " " << e;
            }
            break;
        case DEM_TICK:
            out << "tick " << op.target_data[0].t;
            break;
        case DEM_REPEAT_BLOCK:
            out << "repeat " << op.target_data[0].t << " { ... }";
            break;
        default:
            out << "???";
    }
    return out;
}

void DetectorErrorModel::append_error(double probability, ConstPointerRange<DemRelativeSymptom> data) {
    for (auto e : data) {
        if (e.is_separator()) {
            throw std::invalid_argument("Use append_reducible_error for symptoms containing separators.");
        }
    }
    symptom_buf.append_tail(data);
    auto stored = symptom_buf.commit_tail();
    instructions.push_back(DemInstruction{probability, stored, DEM_ERROR});
}

void DetectorErrorModel::append_reducible_error(double probability, ConstPointerRange<DemRelativeSymptom> data) {
    symptom_buf.append_tail(data);
    auto stored = symptom_buf.commit_tail();
    instructions.push_back(DemInstruction{probability, stored, DEM_REDUCIBLE_ERROR});
}

void DetectorErrorModel::append_tick(uint64_t tick_count) {
    symptom_buf.append_tail(DemRelativeSymptom{{0}, {0}, {tick_count}});
    instructions.push_back({0, symptom_buf.commit_tail(), DEM_TICK});
}

void DetectorErrorModel::append_repeat_block(uint64_t repeat_count, DetectorErrorModel &&body) {
    uint64_t block_id = blocks.size();
    symptom_buf.append_tail(DemRelativeSymptom{{block_id}, {0}, {repeat_count}});
    blocks.push_back(std::move(body));
    instructions.push_back({0, symptom_buf.commit_tail(), DEM_REPEAT_BLOCK});
}

void DetectorErrorModel::append_repeat_block(uint64_t repeat_count, const DetectorErrorModel &body) {
    uint64_t block_id = blocks.size();
    symptom_buf.append_tail(DemRelativeSymptom{{block_id}, {0}, {repeat_count}});
    blocks.push_back(body);
    instructions.push_back({0, symptom_buf.commit_tail(), DEM_REPEAT_BLOCK});
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

void stream_out_indent(std::ostream &out, const DetectorErrorModel &v, size_t indent) {
    for (const auto &e : v.instructions) {
        for (size_t k = 0; k < indent; k++) {
            out << " ";
        }
        if (e.type == DEM_REPEAT_BLOCK) {
            out << "repeat " << e.target_data[0].t.data << " {\n";
            stream_out_indent(out, v.blocks[(size_t)e.target_data[0].x.data], indent + 4);
            for (size_t k = 0; k < indent; k++) {
                out << " ";
            }
            out << "}";
        } else {
            out << e;
        }
        out << "\n";
    }
}

std::ostream &operator<<(std::ostream &out, const DetectorErrorModel &v) {
    out << std::setprecision(std::numeric_limits<long double>::digits10 + 1);
    stream_out_indent(out, v, 0);
    return out;
}

DetectorErrorModel::DetectorErrorModel() {
}

DetectorErrorModel::DetectorErrorModel(const DetectorErrorModel &other)
    : symptom_buf(other.symptom_buf.total_allocated()), instructions(other.instructions), blocks(other.blocks) {
    // Keep local copy of operation data.
    for (auto &e : instructions) {
        e.target_data = symptom_buf.take_copy(e.target_data);
    }
}

DetectorErrorModel::DetectorErrorModel(DetectorErrorModel &&other) noexcept
    : symptom_buf(std::move(other.symptom_buf)),
      instructions(std::move(other.instructions)),
      blocks(std::move(other.blocks)) {
}

DetectorErrorModel &DetectorErrorModel::operator=(const DetectorErrorModel &other) {
    if (&other != this) {
        instructions = other.instructions;
        blocks = other.blocks;

        // Keep local copy of operation data.
        symptom_buf = MonotonicBuffer<DemRelativeSymptom>(other.symptom_buf.total_allocated());
        for (auto &e : instructions) {
            e.target_data = symptom_buf.take_copy(e.target_data);
        }
    }
    return *this;
}

DetectorErrorModel &DetectorErrorModel::operator=(DetectorErrorModel &&other) noexcept {
    if (&other != this) {
        instructions = std::move(other.instructions);
        blocks = std::move(other.blocks);
        symptom_buf = std::move(other.symptom_buf);
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
    if (!strcmp(name_buf, "reducible_error")) {
        return DEM_REDUCIBLE_ERROR;
    }
    if (!strcmp(name_buf, "tick")) {
        return DEM_TICK;
    }
    if (!strcmp(name_buf, "repeat")) {
        return DEM_REPEAT_BLOCK;
    }
    throw std::out_of_range("Unrecognized instruction name: " + std::string(name_buf));
}

template <typename SOURCE>
uint64_t read_uint60_t(int &c, SOURCE read_char) {
    if (!(c >= '0' && c <= '9')) {
        throw std::out_of_range("Expected a digit but got " + std::string(1, c));
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
inline void read_tick(int &c, SOURCE read_char, DetectorErrorModel &model) {
    read_past_within_line_whitespace(c, read_char);
    model.append_tick(read_uint60_t(c, read_char));
}

template <typename SOURCE>
inline void read_repeat(int &c, SOURCE read_char, DetectorErrorModel &model) {
    read_past_within_line_whitespace(c, read_char);
    model.append_repeat_block(read_uint60_t(c, read_char), {});
    read_until_next_line_arg(c, read_char);
}

template <typename SOURCE>
inline DemRelValue read_rel_value(int &c, SOURCE read_char) {
    uint64_t v = read_uint60_t(c, read_char);
    if (c == '+') {
        c = read_char();
        if (c != 't') {
            throw std::invalid_argument("'+' not followed by 't' at end of detector id.");
        }
        c = read_char();
        return DemRelValue::relative(v);
    }
    return DemRelValue::absolute(v);
}

template <typename SOURCE>
inline bool read_symptom(int &c, SOURCE read_char, bool allow_separators, MonotonicBuffer<DemRelativeSymptom> &out) {
    read_until_next_line_arg(c, read_char);
    if (c == 'L') {
        c = read_char();
        out.append_tail(DemRelativeSymptom::observable_id(read_uint60_t(c, read_char)));
        return true;
    }
    if (c == 'D') {
        c = read_char();
        DemRelValue v0 = read_rel_value(c, read_char);
        if (c == ',') {
            c = read_char();
            DemRelValue v1 = read_rel_value(c, read_char);
            if (c == ',') {
                c = read_char();
                DemRelValue v2 = read_rel_value(c, read_char);
                out.append_tail(DemRelativeSymptom::detector_id(v0, v1, v2));
            } else {
                out.append_tail(DemRelativeSymptom::detector_id(v0, DemRelValue::unspecified(), v1));
            }
        } else {
            out.append_tail(
                DemRelativeSymptom::detector_id(DemRelValue::unspecified(), DemRelValue::unspecified(), v0));
        }
        return true;
    }
    if (c == '^') {
        if (!allow_separators) {
            throw std::invalid_argument("`error` with `^` separators (needs `reducible_error`).");
        }
        c = read_char();
        out.append_tail(DemRelativeSymptom::separator());
        return true;
    }
    return false;
}

template <typename SOURCE>
inline void read_error(int &c, SOURCE read_char, DetectorErrorModel &model, bool reducible) {
    const char *name = reducible ? "reducible_error" : "error";
    read_past_within_line_whitespace(c, read_char);
    if (c != '(') {
        throw std::invalid_argument("Expected a probability argument for '" + std::string(name) + "'.");
    }
    c = read_char();

    read_past_within_line_whitespace(c, read_char);
    auto p = read_non_negative_double(c, read_char);

    read_past_within_line_whitespace(c, read_char);
    if (c != ')') {
        throw std::invalid_argument("Missing close parens for probability argument of '" + std::string(name) + "'.");
    }
    c = read_char();

    while (read_symptom(c, read_char, reducible, model.symptom_buf)) {
    }

    model.instructions.push_back(
        DemInstruction{p, model.symptom_buf.commit_tail(), reducible ? DEM_REDUCIBLE_ERROR : DEM_ERROR});
}

template <typename SOURCE>
void model_read_single_operation(DetectorErrorModel &model, char lead_char, SOURCE read_char) {
    int c = (int)lead_char;
    DemInstructionType type = read_instruction_name(c, read_char);
    switch (type) {
        case DEM_ERROR:
            read_error(c, read_char, model, false);
            break;
        case DEM_REDUCIBLE_ERROR:
            read_error(c, read_char, model, true);
            break;
        case DEM_TICK:
            read_tick(c, read_char, model);
            break;
        case DEM_REPEAT_BLOCK:
            read_repeat(c, read_char, model);
            break;
        default:
            throw std::out_of_range("Instruction type not implemented.");
    }
    if (c != '{' && type == DEM_REPEAT_BLOCK) {
        throw std::out_of_range("Missing '{' at start of repeat block.");
    }
    if (c == '{' && type != DEM_REPEAT_BLOCK) {
        throw std::out_of_range("Unexpected '{' after non-block command.");
    }
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
        model_read_single_operation(model, c, read_char);
        auto &new_op = ops.back();

        if (new_op.type == DEM_REPEAT_BLOCK) {
            // Recursively read the block contents.
            auto &block = model.blocks[new_op.target_data[0].x.data];
            model_read_operations(block, read_char, DEM_READ_UNTIL_END_OF_BLOCK);
        }
    } while (read_condition != DEM_READ_AS_LITTLE_AS_POSSIBLE);
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

DetectorErrorModel::DetectorErrorModel(const char *text) {
    append_from_text(text);
}
void DetectorErrorModel::clear() {
    symptom_buf.clear();
    instructions.clear();
    blocks.clear();
}
