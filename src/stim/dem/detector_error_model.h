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

#ifndef _STIM_DEM_DETECTOR_ERROR_MODEL_H
#define _STIM_DEM_DETECTOR_ERROR_MODEL_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "stim/circuit/circuit.h"
#include "stim/mem/monotonic_buffer.h"

namespace stim {

enum DemInstructionType : uint8_t {
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
};

struct DemInstruction {
    ConstPointerRange<double> arg_data;
    ConstPointerRange<DemTarget> target_data;
    DemInstructionType type;

    bool operator<(const DemInstruction &other) const;
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

    DetectorErrorModel &operator*=(size_t repetitions);
    DetectorErrorModel operator*(size_t repetitions) const;
    DetectorErrorModel operator+(const DetectorErrorModel &other) const;
    DetectorErrorModel &operator+=(const DetectorErrorModel &other);

    void append_dem_instruction(const DemInstruction &instruction);
    void append_error_instruction(double probability, ConstPointerRange<DemTarget> targets);
    void append_shift_detectors_instruction(ConstPointerRange<double> coord_shift, uint64_t detector_shift);
    void append_detector_instruction(ConstPointerRange<double> coords, DemTarget target);
    void append_logical_observable_instruction(DemTarget target);
    void append_repeat_block(uint64_t repeat_count, DetectorErrorModel &&body);
    void append_repeat_block(uint64_t repeat_count, const DetectorErrorModel &body);

    /// Grows the detector error model using operations from a string.
    void append_from_text(const char *text);
    /// Grows the detector error model using operations from a file.
    ///
    /// Args:
    ///     file: The opened file to read from.
    ///     stop_asap: When set to true, the reading process stops after the next instruction or block is read. This is
    ///         potentially useful for interactive/streaming usage, where errors are being processed on the fly.
    void append_from_file(FILE *file, bool stop_asap = false);
    /// Parses a detector error model from a file.
    static DetectorErrorModel from_file(FILE *file);

    bool operator==(const DetectorErrorModel &other) const;
    bool operator!=(const DetectorErrorModel &other) const;
    bool approx_equals(const DetectorErrorModel &other, double atol) const;
    std::string str() const;

    uint64_t total_detector_shift() const;
    uint64_t count_detectors() const;
    uint64_t count_observables() const;
    uint64_t count_errors() const;

    std::pair<uint64_t, std::vector<double>> final_detector_and_coord_shift() const;
    std::map<uint64_t, std::vector<double>> get_detector_coordinates(
        const std::set<uint64_t> &included_detector_indices) const;

    void clear();

   private:
    template <typename CALLBACK>
    void iter_flatten_error_instructions_helper(const CALLBACK &callback, uint64_t &detector_shift) const {
        std::vector<DemTarget> translate_buf;
        for (const auto &op : instructions) {
            switch (op.type) {
                case DEM_ERROR:
                    translate_buf.clear();
                    translate_buf.insert(translate_buf.end(), op.target_data.begin(), op.target_data.end());
                    for (auto &t : translate_buf) {
                        t.shift_if_detector_id((int64_t)detector_shift);
                    }
                    callback(DemInstruction{op.arg_data, translate_buf, op.type});
                    break;
                case DEM_REPEAT_BLOCK: {
                    const auto &block = blocks[op.target_data[1].data];
                    auto reps = op.target_data[0].data;
                    for (uint64_t k = 0; k < reps; k++) {
                        block.iter_flatten_error_instructions_helper(callback, detector_shift);
                    }
                    break;
                }
                case DEM_SHIFT_DETECTORS:
                    detector_shift += op.target_data[0].data;
                    break;
                case DEM_DETECTOR:
                case DEM_LOGICAL_OBSERVABLE:
                    break;
                default:
                    throw std::invalid_argument("Unrecognized DEM instruction type: " + op.str());
            }
        }
    }

   public:
    /// Iterates through the error model, invoking the given callback on each found error mechanism.
    ///
    /// Automatically flattens `repeat` blocks into repeated instructions.
    /// Automatically folds `shift_detectors` instructions into adjusted indices of later error instructions.
    ///
    /// Args:
    ///     callback: A function that takes a DemInstruction and returns no value. This function will be invoked once
    ///         for each error instruction. The DemInstruction's arg_data will contain a single value (the error's
    ///         probability) and the absolute targets of the error.
    template <typename CALLBACK>
    void iter_flatten_error_instructions(const CALLBACK &callback) const {
        uint64_t offset = 0;
        iter_flatten_error_instructions_helper(callback, offset);
    }

    /// Gets a python-style slice of the error model's instructions.
    DetectorErrorModel py_get_slice(int64_t start, int64_t step, int64_t slice_length) const;

    /// Rounds error probabilities to a given number of digits.
    DetectorErrorModel rounded(uint8_t digits) const;

    /// Returns an equivalent detector error model with no repeat blocks or detector_shift instructions.
    DetectorErrorModel flattened() const;
};

void print_detector_error_model(std::ostream &out, const DetectorErrorModel &v, size_t indent);
std::ostream &operator<<(std::ostream &out, const DemInstructionType &type);
std::ostream &operator<<(std::ostream &out, const DetectorErrorModel &v);
std::ostream &operator<<(std::ostream &out, const DemTarget &v);
std::ostream &operator<<(std::ostream &out, const DemInstruction &v);

}  // namespace stim

#endif
