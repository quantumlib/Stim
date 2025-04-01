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
#include <limits>

#include "stim/util_bot/str_util.h"

using namespace stim;

void DetectorErrorModel::append_error_instruction(double probability, SpanRef<const DemTarget> targets, std::string_view tag) {
    append_dem_instruction(DemInstruction{&probability, targets, tag, DemInstructionType::DEM_ERROR});
}

void DetectorErrorModel::append_shift_detectors_instruction(
    SpanRef<const double> coord_shift, uint64_t detector_shift, std::string_view tag) {
    DemTarget shift{detector_shift};
    append_dem_instruction(DemInstruction{coord_shift, &shift, tag, DemInstructionType::DEM_SHIFT_DETECTORS});
}

void DetectorErrorModel::append_detector_instruction(SpanRef<const double> coords, DemTarget target, std::string_view tag) {
    append_dem_instruction(DemInstruction{coords, &target, tag, DemInstructionType::DEM_DETECTOR});
}

void DetectorErrorModel::append_logical_observable_instruction(DemTarget target, std::string_view tag) {
    append_dem_instruction(DemInstruction{{}, &target, tag, DemInstructionType::DEM_LOGICAL_OBSERVABLE});
}

void DetectorErrorModel::append_dem_instruction(const DemInstruction &instruction) {
    assert(instruction.type != DemInstructionType::DEM_REPEAT_BLOCK);
    instruction.validate();
    auto stored_targets = target_buf.take_copy(instruction.target_data);
    auto stored_args = arg_buf.take_copy(instruction.arg_data);
    auto tag = tag_buf.take_copy(instruction.tag);
    instructions.push_back(DemInstruction{stored_args, stored_targets, tag, instruction.type});
}

void DetectorErrorModel::append_repeat_block(uint64_t repeat_count, DetectorErrorModel &&body, std::string_view tag) {
    std::array<DemTarget, 2> data;
    data[0].data = repeat_count;
    data[1].data = blocks.size();
    auto stored_targets = target_buf.take_copy(data);
    blocks.push_back(std::move(body));
    tag = tag_buf.take_copy(tag);
    instructions.push_back({{}, stored_targets, tag, DemInstructionType::DEM_REPEAT_BLOCK});
}

void DetectorErrorModel::append_repeat_block(uint64_t repeat_count, const DetectorErrorModel &body, std::string_view tag) {
    DemTarget data[2];
    data[0].data = repeat_count;
    data[1].data = blocks.size();
    auto stored_targets = target_buf.take_copy({&data[0], &data[2]});
    blocks.push_back(body);
    tag = tag_buf.take_copy(tag);
    instructions.push_back({{}, stored_targets, tag, DemInstructionType::DEM_REPEAT_BLOCK});
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
        if (e.type == DemInstructionType::DEM_REPEAT_BLOCK) {
            out << "repeat";
            if (!e.tag.empty()) {
                out << '[';
                write_tag_escaped_string_to(e.tag, out);
                out << ']';
            }
            out << " " << e.repeat_block_rep_count() << " {\n";
            print_detector_error_model(out, e.repeat_block_body(v), indent + 4);
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
      tag_buf(other.tag_buf.total_allocated()),
      instructions(other.instructions),
      blocks(other.blocks) {
    // Keep local copy of buffer data.
    for (auto &e : instructions) {
        e.arg_data = arg_buf.take_copy(e.arg_data);
        e.target_data = target_buf.take_copy(e.target_data);
        e.tag = tag_buf.take_copy(e.tag);
    }
}

DetectorErrorModel::DetectorErrorModel(DetectorErrorModel &&other) noexcept
    : arg_buf(std::move(other.arg_buf)),
      target_buf(std::move(other.target_buf)),
      tag_buf(std::move(other.tag_buf)),
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
        tag_buf = MonotonicBuffer<char>(other.tag_buf.total_allocated());
        for (auto &e : instructions) {
            e.arg_data = arg_buf.take_copy(e.arg_data);
            e.target_data = target_buf.take_copy(e.target_data);
            e.tag = tag_buf.take_copy(e.tag);
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
        tag_buf = std::move(other.tag_buf);
    }
    return *this;
}

enum class DEM_READ_CONDITION {
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
        return DemInstructionType::DEM_ERROR;
    }
    if (!strcmp(name_buf, "shift_detectors")) {
        return DemInstructionType::DEM_SHIFT_DETECTORS;
    }
    if (!strcmp(name_buf, "detector")) {
        return DemInstructionType::DEM_DETECTOR;
    }
    if (!strcmp(name_buf, "logical_observable")) {
        return DemInstructionType::DEM_LOGICAL_OBSERVABLE;
    }
    if (!strcmp(name_buf, "repeat")) {
        return DemInstructionType::DEM_REPEAT_BLOCK;
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
    int c = lead_char;
    DemInstructionType type = read_instruction_name(c, read_char);
    std::string_view tail_tag;
    try {
        read_tag(c, "", read_char, model.tag_buf);
        if (!model.tag_buf.tail.empty()) {
            tail_tag = std::string_view(model.tag_buf.tail.ptr_start, model.tag_buf.tail.size());
        }
        if (type == DemInstructionType::DEM_REPEAT_BLOCK) {
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
            if (type == DemInstructionType::DEM_SHIFT_DETECTORS) {
                if (read_until_next_line_arg(c, read_char)) {
                    model.target_buf.append_tail(DemTarget{read_uint60_t(c, read_char)});
                }
            }
            read_arbitrary_dem_targets_into(c, read_char, model);
            if (c == '{') {
                throw std::invalid_argument("Unexpected '{'.");
            }
            DemInstruction{model.arg_buf.tail, model.target_buf.tail, tail_tag, type}.validate();
        }
    } catch (const std::invalid_argument &) {
        model.tag_buf.discard_tail();
        model.target_buf.discard_tail();
        model.arg_buf.discard_tail();
        throw;
    }

    model.tag_buf.commit_tail();
    model.instructions.push_back(DemInstruction{
        .arg_data=model.arg_buf.commit_tail(),
        .target_data=model.target_buf.commit_tail(),
        .tag=tail_tag,
        .type=type,
    });
}

template <typename SOURCE>
void model_read_operations(DetectorErrorModel &model, SOURCE read_char, DEM_READ_CONDITION read_condition) {
    auto &ops = model.instructions;
    do {
        int c = read_char();
        read_past_dead_space_between_commands(c, read_char);
        if (c == EOF) {
            if (read_condition == DEM_READ_CONDITION::DEM_READ_UNTIL_END_OF_BLOCK) {
                throw std::out_of_range("Unterminated block. Got a '{' without an eventual '}'.");
            }
            return;
        }
        if (c == '}') {
            if (read_condition != DEM_READ_CONDITION::DEM_READ_UNTIL_END_OF_BLOCK) {
                throw std::out_of_range("Uninitiated block. Got a '}' without a '{'.");
            }
            return;
        }
        dem_read_instruction(model, c, read_char);

        if (ops.back().type == DemInstructionType::DEM_REPEAT_BLOCK) {
            // Temporarily remove instruction until block is parsed.
            auto repeat_count = ops.back().repeat_block_rep_count();
            auto tag = ops.back().tag;
            ops.pop_back();

            // Recursively read the block contents.
            DetectorErrorModel block;
            model_read_operations(block, read_char, DEM_READ_CONDITION::DEM_READ_UNTIL_END_OF_BLOCK);

            // Restore repeat block instruction, including block reference.
            model.append_repeat_block(repeat_count, std::move(block), tag);
        }
    } while (read_condition != DEM_READ_CONDITION::DEM_READ_AS_LITTLE_AS_POSSIBLE);
}

void DetectorErrorModel::append_from_file(FILE *file, bool stop_asap) {
    model_read_operations(
        *this,
        [&]() {
            return getc(file);
        },
        stop_asap ? DEM_READ_CONDITION::DEM_READ_AS_LITTLE_AS_POSSIBLE
                  : DEM_READ_CONDITION::DEM_READ_UNTIL_END_OF_FILE);
}

void DetectorErrorModel::append_from_text(std::string_view text) {
    size_t k = 0;
    model_read_operations(
        *this,
        [&]() {
            return k < text.size() ? text[k++] : EOF;
        },
        DEM_READ_CONDITION::DEM_READ_UNTIL_END_OF_FILE);
}

DetectorErrorModel DetectorErrorModel::from_file(FILE *file) {
    DetectorErrorModel result;
    result.append_from_file(file, false);
    return result;
}

DetectorErrorModel::DetectorErrorModel(std::string_view text) {
    append_from_text(text);
}
void DetectorErrorModel::clear() {
    target_buf.clear();
    arg_buf.clear();
    instructions.clear();
    blocks.clear();
}

DetectorErrorModel DetectorErrorModel::rounded(uint8_t digits) const {
    double scale = 1;
    for (size_t k = 0; k < digits; k++) {
        scale *= 10;
    }

    DetectorErrorModel result;
    for (const auto &e : instructions) {
        if (e.type == DemInstructionType::DEM_REPEAT_BLOCK) {
            auto reps = e.repeat_block_rep_count();
            auto &block = e.repeat_block_body(*this);
            result.append_repeat_block(reps, block.rounded(digits), e.tag);
        } else if (e.type == DemInstructionType::DEM_ERROR) {
            std::vector<double> rounded_args;
            for (auto a : e.arg_data) {
                rounded_args.push_back(round(a * scale) / scale);
            }
            result.append_dem_instruction({rounded_args, e.target_data, e.tag, DemInstructionType::DEM_ERROR});
        } else {
            result.append_dem_instruction(e);
        }
    }
    return result;
}

uint64_t DetectorErrorModel::total_detector_shift() const {
    uint64_t result = 0;
    for (const auto &e : instructions) {
        if (e.type == DemInstructionType::DEM_SHIFT_DETECTORS) {
            result += e.target_data[0].data;
        } else if (e.type == DemInstructionType::DEM_REPEAT_BLOCK) {
            result += e.repeat_block_rep_count() * e.repeat_block_body(*this).total_detector_shift();
        }
    }
    return result;
}

void flattened_helper(
    const DetectorErrorModel &body,
    std::vector<double> &cur_coordinate_shift,
    uint64_t &cur_detector_shift,
    DetectorErrorModel &out) {
    for (const auto &op : body.instructions) {
        if (op.type == DemInstructionType::DEM_SHIFT_DETECTORS) {
            while (cur_coordinate_shift.size() < op.arg_data.size()) {
                cur_coordinate_shift.push_back(0);
            }
            for (size_t k = 0; k < op.arg_data.size(); k++) {
                cur_coordinate_shift[k] += op.arg_data[k];
            }
            if (!op.target_data.empty()) {
                cur_detector_shift += op.target_data[0].data;
            }
        } else if (op.type == DemInstructionType::DEM_REPEAT_BLOCK) {
            const auto &loop_body = op.repeat_block_body(body);
            auto reps = op.repeat_block_rep_count();
            for (uint64_t k = 0; k < reps; k++) {
                flattened_helper(loop_body, cur_coordinate_shift, cur_detector_shift, out);
            }
        } else if (op.type == DemInstructionType::DEM_LOGICAL_OBSERVABLE) {
            out.append_dem_instruction(DemInstruction{{}, op.target_data, op.tag, DemInstructionType::DEM_LOGICAL_OBSERVABLE});
        } else if (op.type == DemInstructionType::DEM_DETECTOR) {
            while (cur_coordinate_shift.size() < op.arg_data.size()) {
                cur_coordinate_shift.push_back(0);
            }

            std::vector<double> shifted_coords;
            for (size_t k = 0; k < op.arg_data.size(); k++) {
                shifted_coords.push_back(op.arg_data[k] + cur_coordinate_shift[k]);
            }
            std::vector<DemTarget> shifted_detectors;
            for (DemTarget t : op.target_data) {
                t.shift_if_detector_id(cur_detector_shift);
                shifted_detectors.push_back(t);
            }

            out.append_dem_instruction(
                DemInstruction{shifted_coords, shifted_detectors, op.tag, DemInstructionType::DEM_DETECTOR});
        } else if (op.type == DemInstructionType::DEM_ERROR) {
            std::vector<DemTarget> shifted_detectors;
            for (DemTarget t : op.target_data) {
                t.shift_if_detector_id(cur_detector_shift);
                shifted_detectors.push_back(t);
            }

            out.append_dem_instruction(DemInstruction{op.arg_data, shifted_detectors, op.tag, DemInstructionType::DEM_ERROR});
        } else {
            throw std::invalid_argument("Unrecognized instruction type: " + op.str());
        }
    }
}

DetectorErrorModel DetectorErrorModel::flattened() const {
    DetectorErrorModel result;
    std::vector<double> shift;
    uint64_t det_shift = 0;
    flattened_helper(*this, shift, det_shift, result);
    return result;
}

uint64_t DetectorErrorModel::count_detectors() const {
    uint64_t offset = 1;
    uint64_t max_num = 0;
    for (const auto &e : instructions) {
        switch (e.type) {
            case DemInstructionType::DEM_LOGICAL_OBSERVABLE:
                break;
            case DemInstructionType::DEM_SHIFT_DETECTORS:
                offset += e.target_data[0].data;
                break;
            case DemInstructionType::DEM_REPEAT_BLOCK: {
                auto &block = e.repeat_block_body(*this);
                auto n = block.count_detectors();
                auto reps = e.repeat_block_rep_count();
                auto block_shift = block.total_detector_shift();  // Note: quadratic overhead in nesting level.
                offset += block_shift * reps;
                if (reps > 0 && n > 0) {
                    max_num = std::max(max_num, offset + n - 1 - block_shift);
                }
            } break;
            case DemInstructionType::DEM_DETECTOR:
            case DemInstructionType::DEM_ERROR:
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

uint64_t DetectorErrorModel::count_errors() const {
    uint64_t total = 0;
    for (const auto &e : instructions) {
        switch (e.type) {
            case DemInstructionType::DEM_LOGICAL_OBSERVABLE:
                break;
            case DemInstructionType::DEM_SHIFT_DETECTORS:
                break;
            case DemInstructionType::DEM_REPEAT_BLOCK: {
                auto &block = e.repeat_block_body(*this);
                auto n = block.count_errors();
                auto reps = e.repeat_block_rep_count();
                total += n * reps;
                break;
            }
            case DemInstructionType::DEM_DETECTOR:
                break;
            case DemInstructionType::DEM_ERROR:
                total++;
                break;
            default:
                throw std::invalid_argument("Instruction type not implemented in count_errors: " + e.str());
        }
    }
    return total;
}

uint64_t DetectorErrorModel::count_observables() const {
    uint64_t max_num = 0;
    for (const auto &e : instructions) {
        switch (e.type) {
            case DemInstructionType::DEM_SHIFT_DETECTORS:
            case DemInstructionType::DEM_DETECTOR:
                break;
            case DemInstructionType::DEM_REPEAT_BLOCK: {
                auto &block = e.repeat_block_body(*this);
                max_num = std::max(max_num, block.count_observables());
            } break;
            case DemInstructionType::DEM_LOGICAL_OBSERVABLE:
            case DemInstructionType::DEM_ERROR:
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
    assert(slice_length >= 0);
    assert(slice_length == 0 || start >= 0);
    DetectorErrorModel result;
    for (size_t k = 0; k < (size_t)slice_length; k++) {
        const auto &op = instructions[start + step * k];
        if (op.type == DemInstructionType::DEM_REPEAT_BLOCK) {
            result.append_repeat_block(op.repeat_block_rep_count(), op.repeat_block_body(*this), op.tag);
        } else {
            auto args = result.arg_buf.take_copy(op.arg_data);
            auto targets = result.target_buf.take_copy(op.target_data);
            result.instructions.push_back(DemInstruction{args, targets, op.tag, op.type});
        }
    }
    return result;
}

DetectorErrorModel DetectorErrorModel::operator*(size_t repetitions) const {
    DetectorErrorModel copy = *this;
    copy *= repetitions;
    return copy;
}

DetectorErrorModel &DetectorErrorModel::operator*=(size_t repetitions) {
    if (repetitions == 0) {
        clear();
    }
    if (repetitions <= 1) {
        return *this;
    }
    DetectorErrorModel other = std::move(*this);
    clear();
    append_repeat_block(repetitions, std::move(other), "");
    return *this;
}

DetectorErrorModel DetectorErrorModel::operator+(const DetectorErrorModel &other) const {
    DetectorErrorModel result = *this;
    result += other;
    return result;
}

DetectorErrorModel &DetectorErrorModel::operator+=(const DetectorErrorModel &other) {
    if (&other == this) {
        instructions.insert(instructions.end(), instructions.begin(), instructions.end());
        return *this;
    }

    for (auto &e : other.instructions) {
        if (e.type == DemInstructionType::DEM_REPEAT_BLOCK) {
            uint64_t repeat_count = e.repeat_block_rep_count();
            const DetectorErrorModel &block = e.repeat_block_body(other);
            append_repeat_block(repeat_count, block, e.tag);
        } else {
            append_dem_instruction(e);
        }
    }
    return *this;
}

std::pair<uint64_t, std::vector<double>> DetectorErrorModel::final_detector_and_coord_shift() const {
    uint64_t detector_offset = 0;
    std::vector<double> coord_shift;
    for (const auto &op : instructions) {
        if (op.type == DemInstructionType::DEM_SHIFT_DETECTORS) {
            vec_pad_add_mul(coord_shift, op.arg_data);
            detector_offset += op.target_data[0].data;
        } else if (op.type == DemInstructionType::DEM_REPEAT_BLOCK) {
            const auto &block = op.repeat_block_body(*this);
            uint64_t reps = op.repeat_block_rep_count();
            auto rec = block.final_detector_and_coord_shift();
            vec_pad_add_mul(coord_shift, rec.second, reps);
            detector_offset += reps * rec.first;
        }
    }
    return {detector_offset, coord_shift};
}

bool get_detector_coordinates_helper(
    const DetectorErrorModel &dem,
    const std::set<uint64_t> &included_detector_indices,
    std::set<uint64_t>::const_iterator &iter_desired_detector_index,
    std::vector<double> &coord_shift,
    uint64_t &detector_offset,
    std::map<uint64_t, std::vector<double>> &out,
    bool top) {
    if (iter_desired_detector_index == included_detector_indices.end()) {
        return true;
    }

    // Fills in data for a detector that was found while iterating.
    // Returns true if all data has been filled in.
    auto fill_in_data = [&](uint64_t fill_index, SpanRef<const double> fill_data) {
        if (!included_detector_indices.contains(fill_index)) {
            // Not interested in the index for this data.
            return false;
        }
        if (out.contains(fill_index)) {
            // Already have this data. Detector may have been declared twice?
            return false;
        }

        // Write data to result dictionary.
        std::vector<double> det_coords;
        det_coords.reserve(fill_data.size());
        for (size_t k = 0; k < fill_data.size(); k++) {
            det_coords.push_back(fill_data[k]);
            if (k < coord_shift.size()) {
                det_coords[k] += coord_shift[k];
            }
        }
        out[fill_index] = std::move(det_coords);

        // Advance the iterator past values that have been written in.
        // If the end has been reached, we're done.
        while (out.contains(*iter_desired_detector_index)) {
            ++iter_desired_detector_index;
            if (iter_desired_detector_index == included_detector_indices.end()) {
                return true;
            }
        }

        return false;
    };

    for (const auto &op : dem.instructions) {
        if (op.type == DemInstructionType::DEM_SHIFT_DETECTORS) {
            vec_pad_add_mul(coord_shift, op.arg_data);
            detector_offset += op.target_data[0].data;
            while (*iter_desired_detector_index < detector_offset) {
                // Shifting past an index proves that it will never be given data.
                // So set the coordinate data to the empty list.
                if (fill_in_data(*iter_desired_detector_index, {})) {
                    return true;
                }
            }

        } else if (op.type == DemInstructionType::DEM_DETECTOR) {
            for (const auto &t : op.target_data) {
                if (fill_in_data(t.data + detector_offset, op.arg_data)) {
                    return true;
                }
            }

        } else if (op.type == DemInstructionType::DEM_REPEAT_BLOCK) {
            const auto &block = op.repeat_block_body(dem);
            uint64_t reps = op.repeat_block_rep_count();

            // TODO: Finish in time proportional to len(instructions) + len(desired) instead of len(execution).
            for (uint64_t k = 0; k < reps; k++) {
                if (get_detector_coordinates_helper(
                        block,
                        included_detector_indices,
                        iter_desired_detector_index,
                        coord_shift,
                        detector_offset,
                        out,
                        false)) {
                    return true;
                }
            }
        }
    }

    // If we've reached the end of the detector error model, then all remaining
    // values should be given empty data.
    if (top && out.size() < included_detector_indices.size()) {
        uint64_t n = dem.count_detectors();
        while (*iter_desired_detector_index < n) {
            if (fill_in_data(*iter_desired_detector_index, {})) {
                return true;
            }
        }
    }

    return false;
}

std::map<uint64_t, std::vector<double>> DetectorErrorModel::get_detector_coordinates(
    const std::set<uint64_t> &included_detector_indices) const {
    std::map<uint64_t, std::vector<double>> out;
    uint64_t detector_offset = 0;
    std::set<uint64_t>::const_iterator iter = included_detector_indices.begin();
    std::vector<double> coord_shift;
    get_detector_coordinates_helper(*this, included_detector_indices, iter, coord_shift, detector_offset, out, true);

    if (iter != included_detector_indices.end()) {
        std::stringstream msg;
        msg << "Detector index " << *iter << " is too big. The detector error model has ";
        msg << count_detectors() << " detectors)";
        throw std::invalid_argument(msg.str());
    }

    return out;
}

DetectorErrorModel DetectorErrorModel::without_tags() const {
    DetectorErrorModel result;
    for (DemInstruction inst : instructions) {
        if (inst.type == DemInstructionType::DEM_REPEAT_BLOCK) {
            result.append_repeat_block(
                inst.repeat_block_rep_count(),
                inst.repeat_block_body(*this).without_tags(),
                "");
        } else {
            inst.tag = "";
            result.append_dem_instruction(inst);
        }
    }
    return result;
}
