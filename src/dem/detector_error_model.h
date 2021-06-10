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
    DEM_REDUCIBLE_ERROR,
    DEM_TICK,
    DEM_REPEAT_BLOCK,
};

struct DemRelValue {
    uint64_t data;
    uint64_t raw_value() const;
    uint64_t absolute_value(uint64_t t) const;
    bool is_relative() const;
    bool is_unspecified() const;
    bool is_absolute() const;
    static DemRelValue unspecified();
    static DemRelValue relative(uint64_t v);
    static DemRelValue absolute(uint64_t v);
    bool operator==(const DemRelValue &other) const;
    bool operator!=(const DemRelValue &other) const;
    std::string str() const;
};

struct DemRelativeSymptom {
    DemRelValue x;
    DemRelValue y;
    DemRelValue t;

    static DemRelativeSymptom observable_id(uint32_t id);
    static DemRelativeSymptom detector_id(DemRelValue x, DemRelValue y, DemRelValue t);
    static DemRelativeSymptom separator();
    bool is_observable_id() const;
    bool is_separator() const;
    bool is_detector_id() const;

    bool operator==(const DemRelativeSymptom &other) const;
    bool operator!=(const DemRelativeSymptom &other) const;
    std::string str() const;
};

struct DemInstruction {
    double probability;
    PointerRange<DemRelativeSymptom> target_data;
    DemInstructionType type;

    bool operator==(const DemInstruction &other) const;
    bool operator!=(const DemInstruction &other) const;
    bool approx_equals(const DemInstruction &other, double atol) const;
    std::string str() const;
};

struct DetectorErrorModel {
    MonotonicBuffer<DemRelativeSymptom> symptom_buf;
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

    static DetectorErrorModel from_circuit(const Circuit &circuit);

    void append_error(double probability, ConstPointerRange<DemRelativeSymptom> data);
    void append_reducible_error(double probability, ConstPointerRange<DemRelativeSymptom> data);
    void append_tick(uint64_t tick_count);
    void append_repeat_block(uint64_t repeat_count, DetectorErrorModel &&body);
    void append_repeat_block(uint64_t repeat_count, const DetectorErrorModel &body);
    void append_from_text(const char *text);

    bool operator==(const DetectorErrorModel &other) const;
    bool operator!=(const DetectorErrorModel &other) const;
    bool approx_equals(const DetectorErrorModel &other, double atol) const;
    std::string str() const;

    void clear();
};

}  // namespace stim_internal

std::ostream &operator<<(std::ostream &out, const stim_internal::DemRelValue &v);
std::ostream &operator<<(std::ostream &out, const stim_internal::DetectorErrorModel &v);
std::ostream &operator<<(std::ostream &out, const stim_internal::DemRelativeSymptom &v);
std::ostream &operator<<(std::ostream &out, const stim_internal::DemInstruction &v);

#endif
