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

#ifndef _STIM_IO_MEASURE_RECORD_READER_H
#define _STIM_IO_MEASURE_RECORD_READER_H

#include <memory>

#include "stim/circuit/circuit.h"
#include "stim/io/stim_data_formats.h"
#include "stim/mem/pointer_range.h"
#include "stim/mem/simd_bit_table.h"

namespace stim {

/// Handles reading measurement data from the outside world.
///
/// Child classes implement the various input formats. Each file format encodes a certain number of records.
/// Each record is a sequence of 0s and 1s. File formats B8 and R8 encode a single record. File formats 01,
/// HITS and DETS encode any number of records. Record size in bits is fixed for each file and the client
/// must specify it upfront.
struct MeasureRecordReader {
    /// Creates a MeasureRecordReader that reads measurement records in the given format from the given FILE*.
    /// Record size must be specified upfront. The DETS format supports three different types of records
    /// and size of each is specified independently. All other formats support one type of record. It is
    /// an error to specify non-zero size of detection event records or logical observable records unless
    /// the input format is DETS.
    static std::unique_ptr<MeasureRecordReader> make(
        FILE *in,
        SampleFormat input_format,
        size_t n_measurements,
        size_t n_detection_events = 0,
        size_t n_logical_observables = 0);
    virtual ~MeasureRecordReader() = default;

    /// Reads and returns one measurement result. If no result is available, exception is thrown.
    /// If is_end_of_record() just returned false, then a result is available and no exception is thrown.
    virtual bool read_bit() = 0;
    /// Reads multiple measurement results. Returns the number of results read. If no results are available,
    /// zero is returned. Read terminates when data is filled up or when the current record ends. Note that
    /// records encoded in HITS and DETS file formats never end.
    virtual size_t read_bits_into_bytes(PointerRange<uint8_t> out_buffer);

    /// Reads entire records into the given bit table.
    ///
    /// This method must only be called when the reader is at the start of a record.
    ///
    /// Args:
    ///     out: The bit table to write the records into.
    ///         The major axis indexes shots.
    ///         The minor axis indexes results within a shot.
    ///     major_index_is_shot_index: Whether or not the data should be transposed.
    ///     max_shots: Maximum number of shots to read. Automatically clamped down based on the size of `out`.
    ///
    /// Returns:
    ///     The number of records that were read.
    ///     Cannot be larger than the capacity of the output table.
    ///     If this value is 0, there are no more records to read.
    ///
    /// Throws:
    ///     std::invalid_argument:
    ///         The minor axis of the table has a length that's too short to hold an entire record.
    ///         The major axis of the table has length zero.
    ///         The reader is not at the start of a record.
    virtual size_t read_records_into(
        simd_bit_table &out, bool major_index_is_shot_index, size_t max_shots = UINT32_MAX);

    /// Advances the reader to the next record (i.e. the next sequence of 0s and 1s). Skips the remainder
    /// of the current record and an end-of-record marker (such as a newline). Returns true if a new record
    /// has been found. Returns false if end of file has been reached.
    virtual bool next_record() = 0;

    /// Checks if there is another record present. The reader must be between records.
    ///
    /// Returns:
    //      True: there is another record to read and we have begun reading it.
    //      False: there are no more records to read.
    virtual bool start_record() = 0;

    /// Returns true when the current record has ended. Beyond this point read_bit() throws an exception
    /// and read_bits_into_bytes() returns no data. Note that records in file formats HITS and DETS never end.
    virtual bool is_end_of_record() = 0;
};

struct MeasureRecordReaderFormat01 : MeasureRecordReader {
    FILE *in;
    int payload;
    size_t position;
    size_t bits_per_record;

    MeasureRecordReaderFormat01(FILE *in, size_t bits_per_record);

    bool read_bit() override;
    bool next_record() override;
    bool start_record() override;
    bool is_end_of_record() override;
};

struct MeasureRecordReaderFormatB8 : MeasureRecordReader {
    FILE *in;
    size_t bits_per_record;
    int payload;
    uint8_t bits_available;
    size_t position;

    MeasureRecordReaderFormatB8(FILE *in, size_t bits_per_record);

    size_t read_bits_into_bytes(PointerRange<uint8_t> out_buffer) override;
    bool read_bit() override;
    bool next_record() override;
    bool start_record() override;
    bool is_end_of_record() override;

   private:
    void maybe_update_payload();
};

struct MeasureRecordReaderFormatHits : MeasureRecordReader {
    FILE *in;
    uint64_t bits_per_record;
    simd_bits buffer;
    size_t position_in_buffer;

    MeasureRecordReaderFormatHits(FILE *in, size_t bits_per_record);

    bool read_bit() override;
    bool next_record() override;
    bool start_record() override;
    bool is_end_of_record() override;
};

struct MeasureRecordReaderFormatR8 : MeasureRecordReader {
    FILE *in;
    size_t position = 0;
    bool have_seen_terminal_1 = false;
    size_t buffered_0s = 0;
    size_t buffered_1s = 0;
    size_t bits_per_record;

    MeasureRecordReaderFormatR8(FILE *in, size_t bits_per_record);

    size_t read_bits_into_bytes(PointerRange<uint8_t> out_buffer) override;
    bool read_bit() override;
    bool next_record() override;
    bool start_record() override;
    bool is_end_of_record() override;

   private:
    bool maybe_buffer_data();
};

struct MeasureRecordReaderFormatDets : MeasureRecordReader {
    FILE *in;
    simd_bits buffer;
    size_t position_in_buffer;
    uint64_t m_bits_per_record;
    uint64_t d_bits_per_record;
    uint64_t l_bits_per_record;

    MeasureRecordReaderFormatDets(
        FILE *in, size_t n_measurements, size_t n_detection_events = 0, size_t n_logical_observables = 0);

    bool read_bit() override;
    bool next_record() override;
    bool start_record() override;
    bool is_end_of_record() override;
};

}  // namespace stim

#endif
