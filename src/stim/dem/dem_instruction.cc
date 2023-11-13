#include "stim/dem/dem_instruction.h"

#include <cmath>

#include "stim/dem/detector_error_model.h"
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

bool DemInstruction::operator<(const DemInstruction &other) const {
    if (type != other.type) {
        return type < other.type;
    }
    if (target_data != other.target_data) {
        return target_data < other.target_data;
    }
    return arg_data < other.arg_data;
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
        case DemInstructionType::DEM_ERROR:
            out << "error";
            break;
        case DemInstructionType::DEM_DETECTOR:
            out << "detector";
            break;
        case DemInstructionType::DEM_LOGICAL_OBSERVABLE:
            out << "logical_observable";
            break;
        case DemInstructionType::DEM_SHIFT_DETECTORS:
            out << "shift_detectors";
            break;
        case DemInstructionType::DEM_REPEAT_BLOCK:
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
    if (op.type == DemInstructionType::DEM_SHIFT_DETECTORS || op.type == DemInstructionType::DEM_REPEAT_BLOCK) {
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
        case DemInstructionType::DEM_ERROR:
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
        case DemInstructionType::DEM_SHIFT_DETECTORS:
            if (target_data.size() != 1) {
                throw std::invalid_argument(
                    "'shift_detectors' instruction takes 1 target, but got " + std::to_string(target_data.size()) +
                    " targets.");
            }
            break;
        case DemInstructionType::DEM_DETECTOR:
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
        case DemInstructionType::DEM_LOGICAL_OBSERVABLE:
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
        case DemInstructionType::DEM_REPEAT_BLOCK:
            // Handled elsewhere.
            break;
        default:
            throw std::invalid_argument("Unknown instruction type.");
    }
}

uint64_t DemInstruction::repeat_block_rep_count() const {
    assert(target_data.size() > 0);
    return target_data[0].data;
}

const DetectorErrorModel &DemInstruction::repeat_block_body(const DetectorErrorModel &host) const {
    assert(target_data.size() == 2);
    auto b = target_data[1].data;
    assert(b < host.blocks.size());
    return host.blocks[b];
}

DetectorErrorModel &DemInstruction::repeat_block_body(DetectorErrorModel &host) const {
    assert(target_data.size() == 2);
    auto b = target_data[1].data;
    assert(b < host.blocks.size());
    return host.blocks[b];
}
