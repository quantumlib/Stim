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

#ifndef _STIM_IO_MEASURE_RECORD_BATCH_WRITER_H
#define _STIM_IO_MEASURE_RECORD_BATCH_WRITER_H

#include "stim/io/measure_record_writer.h"
#include "stim/mem/simd_bit_table.h"

namespace stim {

/// Handles buffering and writing multiple measurement data streams that ultimately need to be concatenated.
struct MeasureRecordBatchWriter {
    SampleFormat output_format;
    FILE *out;
    /// Temporary files used to hold data that will eventually be concatenated onto the main stream.
    std::vector<FILE *> temporary_files;
    /// The individual writers for each incoming stream of measurement results.
    /// The first writer will go directly to `out`, whereas the others go into temporary files.
    std::vector<std::unique_ptr<MeasureRecordWriter>> writers;

    MeasureRecordBatchWriter(FILE *out, size_t num_shots, SampleFormat output_format);
    /// Cleans up temporary files.
    ~MeasureRecordBatchWriter();
    /// See MeasureRecordWriter::begin_result_type.
    void begin_result_type(char result_type);

    /// Writes a separate measurement result to each MeasureRecordWriter.
    ///
    /// Args:
    ///     bits: The measurement results. The bit at offset k is the bit for the writer at offset k.
    void batch_write_bit(simd_bits_range_ref bits);
    /// Writes multiple separate measurement results to each MeasureRecordWriter.
    ///
    /// This method can be called after calling `batch_write_bit`, but for performance reasons it is recommended to not
    /// do this since it can result in the individual writers doing extra work due to not being on byte boundaries.
    ///
    /// Args:
    ///     table: The measurement results.
    ///         The bits at minor offset k, from major offset 0 to major offset 64*num_major_u64, are the bits for the
    ///         writer at offset k.
    ///     num_major_u64: The number of measurement results (divided by 64) for each writer. The actual number of
    ///         results is required to be a multiple of 64 for performance reasons.
    void batch_write_bytes(const simd_bit_table &table, size_t num_major_u64);
    /// Tells each writer to finish up, then concatenates all of their data into the `out` stream and cleans up.
    void write_end();
};

}  // namespace stim

#endif
