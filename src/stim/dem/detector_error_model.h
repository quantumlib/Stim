#ifndef _STIM_DEM_DETECTOR_ERROR_MODEL_H
#define _STIM_DEM_DETECTOR_ERROR_MODEL_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "stim/circuit/circuit.h"
#include "stim/dem/dem_instruction.h"
#include "stim/mem/monotonic_buffer.h"

namespace stim {

struct DetectorErrorModel {
    MonotonicBuffer<double> arg_buf;
    MonotonicBuffer<DemTarget> target_buf;
    MonotonicBuffer<char> tag_buf;
    std::vector<DemInstruction> instructions;
    std::vector<DetectorErrorModel> blocks;

    /// Constructs an empty detector error model.
    DetectorErrorModel();
    /// Parses a detector error model from the given text.
    explicit DetectorErrorModel(std::string_view text);

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
    void append_error_instruction(double probability, SpanRef<const DemTarget> targets, std::string_view tag);
    void append_shift_detectors_instruction(SpanRef<const double> coord_shift, uint64_t detector_shift, std::string_view tag);
    void append_detector_instruction(SpanRef<const double> coords, DemTarget target, std::string_view tag);
    void append_logical_observable_instruction(DemTarget target, std::string_view tag);
    void append_repeat_block(uint64_t repeat_count, DetectorErrorModel &&body, std::string_view tag);
    void append_repeat_block(uint64_t repeat_count, const DetectorErrorModel &body, std::string_view tag);

    /// Grows the detector error model using operations from a string.
    void append_from_text(std::string_view text);
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

    DetectorErrorModel without_tags() const;

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
                case DemInstructionType::DEM_ERROR:
                    translate_buf.clear();
                    translate_buf.insert(translate_buf.end(), op.target_data.begin(), op.target_data.end());
                    for (auto &t : translate_buf) {
                        t.shift_if_detector_id((int64_t)detector_shift);
                    }
                    callback(DemInstruction{op.arg_data, translate_buf, op.tag, op.type});
                    break;
                case DemInstructionType::DEM_REPEAT_BLOCK: {
                    const auto &block = op.repeat_block_body(*this);
                    auto reps = op.repeat_block_rep_count();
                    for (uint64_t k = 0; k < reps; k++) {
                        block.iter_flatten_error_instructions_helper(callback, detector_shift);
                    }
                    break;
                }
                case DemInstructionType::DEM_SHIFT_DETECTORS:
                    detector_shift += op.target_data[0].data;
                    break;
                case DemInstructionType::DEM_DETECTOR:
                case DemInstructionType::DEM_LOGICAL_OBSERVABLE:
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
