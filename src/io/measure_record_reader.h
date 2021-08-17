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

#ifndef STIM_RECORD_READER_H
#define STIM_RECORD_READER_H

#include <memory>

#include "../circuit/circuit.h"
#include "../simd/pointer_range.h"
#include "../simd/simd_bit_table.h"

namespace stim_internal {

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
    static std::unique_ptr<MeasureRecordReader> make(FILE *in,
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
    virtual size_t read_bytes(PointerRange<uint8_t> buf);

    /// Advances the reader to the next record (i.e. the next sequence of 0s and 1s). Skips the remainder
    /// of the current record and an end-of-record marker (such as a newline). Returns true if a new record
    /// has been found. Returns false if end of file has been reached.
    virtual bool next_record() = 0;

    /// Returns true when the current record has ended. Beyond this point read_bit() throws an exception
    /// and read_bytes() returns no data. Note that records in file formats HITS and DETS never end.
    virtual bool is_end_of_record() = 0;

    /// Used to obtain the DETS format prefix character (M for measurement, D for detector, L for logical
    /// observable). Readers of other formats always return 'M'.
    virtual char current_result_type();
};

struct MeasureRecordReaderFormat01 : MeasureRecordReader {
    FILE *in;
    int payload;
    size_t position = 0;
    size_t bits_per_record;

    MeasureRecordReaderFormat01(FILE *in, size_t bits_per_record);

    bool read_bit() override;
    bool next_record() override;
    bool is_end_of_record() override;
};

struct MeasureRecordReaderFormatB8 : MeasureRecordReader {
    FILE *in;
    int payload = 0;
    uint8_t bits_available = 0;
    size_t position = 0;
    size_t bits_per_record;

    MeasureRecordReaderFormatB8(FILE *in, size_t bits_per_record);

    size_t read_bytes(PointerRange<uint8_t> data) override;
    bool read_bit() override;
    bool next_record() override;
    bool is_end_of_record() override;

  private:
    void maybe_update_payload();
};

struct MeasureRecordReaderFormatHits : MeasureRecordReader {
    FILE *in;
    int separator;
    bool no_next_hit = true;
    size_t next_hit = 0;
    size_t position = 0;
    size_t bits_per_record;

    MeasureRecordReaderFormatHits(FILE *in, size_t bits_per_record);

    bool read_bit() override;
    bool next_record() override;
    bool is_end_of_record() override;

  private:
    void update_next_hit();
};

struct MeasureRecordReaderFormatR8 : MeasureRecordReader {
    FILE *in;
    size_t run_length_0s = 0;
    size_t run_length_1s = 0;
    size_t generated_0s = 0;
    size_t generated_1s = 1;
    size_t position = 0;
    size_t bits_per_record;

    MeasureRecordReaderFormatR8(FILE *in, size_t bits_per_record);

    size_t read_bytes(PointerRange<uint8_t> data) override;
    bool read_bit() override;
    bool next_record() override;
    bool is_end_of_record() override;

  private:
    bool update_run_length();
};

struct MeasureRecordReaderFormatDets : MeasureRecordReader {
    FILE *in;
    int separator = '\n';
    char result_type = 'M';
    char next_shot_result_type = 'M';
    bool no_next_shot = true;
    size_t next_shot = -1;

    size_t position_m = 0;
    size_t position_d = 0;
    size_t position_l = 0;
    size_t bits_per_m_record;
    size_t bits_per_d_record;
    size_t bits_per_l_record;

    MeasureRecordReaderFormatDets(FILE *in, size_t n_measurements, size_t n_detection_events = 0, size_t n_logical_observables = 0);

    bool read_bit() override;
    bool next_record() override;
    bool is_end_of_record() override;
    char current_result_type() override;

  private:
    size_t& position();
    size_t& bits_per_record();

    void update_next_shot();
};

}  // namespace stim_internal

#endif
