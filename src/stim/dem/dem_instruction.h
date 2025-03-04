#ifndef _STIM_DEM_DEM_INSTRUCTION_H
#define _STIM_DEM_DEM_INSTRUCTION_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "stim/mem/span_ref.h"

namespace stim {

constexpr uint64_t MAX_OBS = 0xFFFFFFFF;
constexpr uint64_t MAX_DET = (uint64_t{1} << 62) - 1;

enum class DemInstructionType : uint8_t {
    DEM_ERROR,
    DEM_SHIFT_DETECTORS,
    DEM_DETECTOR,
    DEM_LOGICAL_OBSERVABLE,
    DEM_REPEAT_BLOCK,
};

struct DemTarget {
    uint64_t data;

    static DemTarget observable_id(uint64_t id);
    static DemTarget relative_detector_id(uint64_t id);
    static constexpr DemTarget separator() {
        return {UINT64_MAX};
    }
    uint64_t raw_id() const;
    uint64_t val() const;
    bool is_observable_id() const;
    bool is_separator() const;
    bool is_relative_detector_id() const;
    void shift_if_detector_id(int64_t offset);

    bool operator==(const DemTarget &other) const;
    bool operator!=(const DemTarget &other) const;
    bool operator<(const DemTarget &other) const;
    std::string str() const;

    static DemTarget from_text(std::string_view text);
};

struct DetectorErrorModel;
struct DemInstruction {
    SpanRef<const double> arg_data;
    SpanRef<const DemTarget> target_data;
    std::string_view tag;
    DemInstructionType type;

    bool operator<(const DemInstruction &other) const;
    bool operator==(const DemInstruction &other) const;
    bool operator!=(const DemInstruction &other) const;
    bool approx_equals(const DemInstruction &other, double atol) const;
    std::string str() const;

    void validate() const;

    uint64_t repeat_block_rep_count() const;
    const DetectorErrorModel &repeat_block_body(const DetectorErrorModel &host) const;
    DetectorErrorModel &repeat_block_body(DetectorErrorModel &host) const;

    template <typename CALLBACK>
    inline void for_separated_targets(CALLBACK callback) const {
        size_t start = 0;
        do {
            size_t end = start + 1;
            while (end < target_data.size() && !target_data[end].is_separator()) {
                end++;
            }
            std::span<const DemTarget> group = target_data.sub(start, std::min(end, target_data.size()));
            callback(group);
            start = end + 1;
        } while (start < target_data.size());
    }
};

std::ostream &operator<<(std::ostream &out, const DemInstructionType &type);
std::ostream &operator<<(std::ostream &out, const DemTarget &v);
std::ostream &operator<<(std::ostream &out, const DemInstruction &v);

}  // namespace stim

#endif
