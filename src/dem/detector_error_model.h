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

#ifndef STIM_DETECTOR_ERROR_MODEL_H
#define STIM_DETECTOR_ERROR_MODEL_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../circuit/circuit.h"
#include "../simd/monotonic_buffer.h"

namespace stim_internal {

enum DemInstructionType : uint8_t {
    DEM_ERROR,
    DEM_SHIFT_DETECTORS,
    DEM_DETECTOR,
    DEM_LOGICAL_OBSERVABLE,
    DEM_REPEAT_BLOCK,
};

struct DemTarget {
    uint64_t data;

    static DemTarget observable_id(uint32_t id);
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
};

struct DemInstruction {
    ConstPointerRange<double> arg_data;
    ConstPointerRange<DemTarget> target_data;
    DemInstructionType type;

    bool operator==(const DemInstruction &other) const;
    bool operator!=(const DemInstruction &other) const;
    bool approx_equals(const DemInstruction &other, double atol) const;
    std::string str() const;

    void validate() const;
};

struct DetectorErrorModel {
    MonotonicBuffer<double> arg_buf;
    MonotonicBuffer<DemTarget> target_buf;
    std::vector<DemInstruction> instructions;
    std::vector<DetectorErrorModel> blocks;

    /// Constructs an empty detector error model.
    DetectorErrorModel();
    /// Parses a detector error model from the given text.
    DetectorErrorModel(const char *text);

    /// Copy constructor.
    DetectorErrorModel(const DetectorErrorModel &other);
    /// Move constructor.
    DetectorErrorModel(DetectorErrorModel &&other) noexcept;
    /// Copy assignment.
    DetectorErrorModel &operator=(const DetectorErrorModel &other);
    /// Move assignment.
    DetectorErrorModel &operator=(DetectorErrorModel &&other) noexcept;

    void append_error_instruction(double probability, ConstPointerRange<DemTarget> targets);
    void append_shift_detectors_instruction(ConstPointerRange<double> coord_shift, uint64_t detector_shift);
    void append_detector_instruction(ConstPointerRange<double> coords, DemTarget target);
    void append_logical_observable_instruction(DemTarget target);
    void append_repeat_block(uint64_t repeat_count, DetectorErrorModel &&body);
    void append_repeat_block(uint64_t repeat_count, const DetectorErrorModel &body);
    void append_from_text(const char *text);

    bool operator==(const DetectorErrorModel &other) const;
    bool operator!=(const DetectorErrorModel &other) const;
    bool approx_equals(const DetectorErrorModel &other, double atol) const;
    std::string str() const;

    uint64_t total_detector_shift() const;
    uint64_t count_detectors() const;
    uint64_t count_observables() const;

    void clear();
};

std::ostream &operator<<(std::ostream &out, const DemInstructionType &type);
std::ostream &operator<<(std::ostream &out, const DetectorErrorModel &v);
std::ostream &operator<<(std::ostream &out, const DemTarget &v);
std::ostream &operator<<(std::ostream &out, const DemInstruction &v);

}  // namespace stim_internal

#endif
