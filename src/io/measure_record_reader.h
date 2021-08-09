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
/// Child classes implement the various input formats.
struct MeasureRecordReader {
    /// Creates a MeasureRecordReader that reads the given number of bits in the given format from the given FILE*.
    /// Number of bits SIZE_MAX indicates that the reader is to determine the appropriate number on its own. Note
    /// that some input formats do not provide any means to infer appropriate stream length. In this case, the
    /// caller should treat the stream as essentially infinite and use other information to determine when to stop
    /// reading from the stream.
    static std::unique_ptr<MeasureRecordReader> make(FILE *in, SampleFormat input_format, size_t max_bits = SIZE_MAX);
    virtual ~MeasureRecordReader() = default;

    /// Reads and returns one measurement result.
    /// If no result is available, exception is thrown.
    /// If is_end_of_record() just returned false, then a result is available and no exception is thrown.
    virtual bool read_bit() = 0;
    /// Reads multiple measurement results. Returns the number of results read.
    /// If no results are available, zero is returned.
    /// Read terminates at end-of-record marker (e.g. a newline), at end-of-file or when data is filled up.
    virtual size_t read_bytes(PointerRange<uint8_t> data);

    /// Returns true when the stream is at enf-of-file or max_bits have already been read.
    virtual bool is_end_of_file() = 0;
    /// Returns true when the stream is at end-of-record marker (e.g. a newline), at end-of-file or max_bits have
    /// been read. If the stream is at end-of-record marker, the function advances the stream to the next position.
    virtual bool is_end_of_record();

    /// Used to obtain the DETS format prefix character (M for measurement, D for detector, L for logical observable).
    /// Readers of other formats always return 'M'.
    virtual char current_result_type();
};

struct MeasureRecordReaderFormat01 : MeasureRecordReader {
    FILE *in;
    int payload;
    size_t bits_returned = 0;
    const size_t max_bits;

    MeasureRecordReaderFormat01(FILE *in, size_t max_bits = SIZE_MAX);

    bool read_bit() override;
    bool is_end_of_record() override;
    bool is_end_of_file() override;
};

struct MeasureRecordReaderFormatB8 : MeasureRecordReader {
    FILE *in;
    int payload = 0;
    uint8_t bits_available = 0;
    size_t bits_returned = 0;
    const size_t max_bits;

    MeasureRecordReaderFormatB8(FILE *in, size_t max_bits = SIZE_MAX);

    size_t read_bytes(PointerRange<uint8_t> data) override;
    bool read_bit() override;
    bool is_end_of_file() override;
};

struct MeasureRecordReaderFormatHits : MeasureRecordReader {
    FILE *in;
    size_t next_hit = 0;
    size_t bits_returned = 0;
    const size_t max_bits;

    MeasureRecordReaderFormatHits(FILE *in, size_t max_bits = SIZE_MAX);

    bool read_bit() override;
    bool is_end_of_file() override;
  private:
    void update_next_hit();
};

struct MeasureRecordReaderFormatR8 : MeasureRecordReader {
    FILE *in;
    size_t run_length_0s = 0;
    size_t run_length_1s = 0;
    size_t generated_0s = 0;
    size_t generated_1s = 1;
    size_t bits_returned = 0;
    const size_t max_bits;

    MeasureRecordReaderFormatR8(FILE *in, size_t max_bits = SIZE_MAX);

    size_t read_bytes(PointerRange<uint8_t> data) override;
    bool read_bit() override;
    bool is_end_of_file() override;
  private:
    bool update_run_length();
};

struct MeasureRecordReaderFormatDets : MeasureRecordReader {
    FILE *in;
    char result_type = 'M';
    char separator;
    size_t next_shot;
    uint64_t position = 0;
    size_t bits_returned = 0;
    const size_t max_bits;

    MeasureRecordReaderFormatDets(FILE *in, size_t max_bits = SIZE_MAX);

    bool read_bit() override;
    bool is_end_of_record() override;
    bool is_end_of_file() override;
    char current_result_type() override;
  private:
    void read_next_shot();
};

}  // namespace stim_internal

#endif
