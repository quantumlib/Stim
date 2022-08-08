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

#ifndef _STIM_IO_MEASURE_RECORD_BATCH_H
#define _STIM_IO_MEASURE_RECORD_BATCH_H

#include "stim/io/measure_record_batch_writer.h"

namespace stim {

/// Stores a record of multiple measurement streams that can be looked up and written to the external world.
///
/// Results that have been written and are further back than `max_lookback` may be discarded from memory.
struct MeasureRecordBatch {
    /// How far back into the measurement record a circuit being simulated may look.
    /// Results younger than this cannot be discarded.
    size_t max_lookback;
    /// How many results have been recorded but not yet written to the external world.
    /// Results younger than this cannot be discarded.
    size_t unwritten;
    /// How many results are currently stored (from each separate stream).
    size_t stored;
    /// How many results have been written to the external world.
    size_t written;
    /// For performance reasons, measurement data given to store may include non-zero values past the data corresponding
    /// to the number of expected shots. AND-ing the data with this mask fixes the problem.
    simd_bits<MAX_BITWORD_WIDTH> shot_mask;
    /// The 2-dimensional block of bits storing the measurement results from each separate measurement stream.
    /// Major index is measurement index, minor index is shot index.
    simd_bit_table<MAX_BITWORD_WIDTH> storage;

    /// Constructs an empty MeasureRecordBatch configured for the given max_lookback and number of shots.
    MeasureRecordBatch(size_t num_shots, size_t max_lookback);

    /// Allows measurements older than max_lookback to be discarded, even though they weren't written out.
    ///
    /// E.g. this is used during detection event sampling, when what is written is derived detection events.
    void mark_all_as_written();
    /// Hints that measurements can be written to the given writer.
    ///
    /// For performance reasons, they may not be written until a large enough block has been accumulated.
    void intermediate_write_unwritten_results_to(MeasureRecordBatchWriter &writer, simd_bits_range_ref<MAX_BITWORD_WIDTH> ref_sample);
    /// Forces measurements to be written to the given writer, and to tell the writer the measurements are ending.
    void final_write_unwritten_results_to(MeasureRecordBatchWriter &writer, simd_bits_range_ref<MAX_BITWORD_WIDTH> ref_sample);
    /// Looks up a historical batch measurement.
    ///
    /// Returns:
    ///     A reference into the storage table, with the bit at offset k corresponding to the measurement from stream k.
    simd_bits_range_ref<MAX_BITWORD_WIDTH> lookback(size_t lookback) const;
    /// Xors a batch measurement result into pre-reserved noisy storage.
    void xor_record_reserved_result(simd_bits_range_ref<MAX_BITWORD_WIDTH> result);
    /// Appends a batch measurement result into storage.
    void record_result(simd_bits_range_ref<MAX_BITWORD_WIDTH> result);
    /// Reserves space for storing measurement results. Initializes bits to be noisy with the given probability.
    void reserve_noisy_space_for_results(const OperationData &target_data, std::mt19937_64 &rng);
    /// Ensures there is enough space for storing a number of measurement results, without moving memory.
    void reserve_space_for_results(size_t count);
    /// Resets the record to an empty state.
    void clear();
};

}  // namespace stim

#endif
