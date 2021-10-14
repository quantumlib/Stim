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

#include "sparse_shot.h"
#include "stim/circuit/circuit.h"
#include "stim/io/stim_data_formats.h"
#include "stim/mem/pointer_range.h"
#include "stim/mem/simd_bit_table.h"

namespace stim {

// Returns true if an integer value is found at current position. Returns false otherwise.
// Uses two output variables: value to return the integer value read and next for the next
// character or EOF.
bool read_uint64(FILE *in, uint64_t &value, int &next, bool include_next = false);

/// Handles reading measurement data from the outside world.
///
/// Child classes implement the various input formats. Each file format encodes a certain number of records.
/// Each record is a sequence of 0s and 1s. File formats B8 and R8 encode a single record. File formats 01,
/// HITS and DETS encode any number of records. Record size in bits is fixed for each file and the client
/// must specify it upfront.
struct MeasureRecordReader {
    size_t num_measurements;
    size_t num_detectors;
    size_t num_observables;
    size_t bits_per_record() const;
    MeasureRecordReader(size_t num_measurements, size_t num_detectors, size_t num_observables);

    /// Creates a MeasureRecordReader that reads measurement records in the given format from the given FILE*.
    /// Record size must be specified upfront. The DETS format supports three different types of records
    /// and size of each is specified independently. All other formats support one type of record. It is
    /// an error to specify non-zero size of detection event records or logical observable records unless
    /// the input format is DETS.
    static std::unique_ptr<MeasureRecordReader> make(
        FILE *in,
        SampleFormat input_format,
        size_t num_measurements,
        size_t num_detectors = 0,
        size_t num_observables = 0);
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

    /// Reads an entire record from start to finish, returning False if there are no more records.
    /// The data from the record is bit packed into a simd_bits.
    ///
    /// Args:
    ///     dirty_out_buffer: The simd-compatible buffer to write the data to. The buffer is not required to be zero'd.
    ///
    /// Returns:
    ///     True: The record was read successfully.
    ///     False: End of file. There were no more records. No record was read.
    ///
    /// Throws:
    ///     std::invalid_argument: A record was only partially read.
    virtual bool start_and_read_entire_record(simd_bits_range_ref dirty_out_buffer) = 0;

    /// Reads an entire record from start to finish, returning False if there are no more records.
    /// The data from the record is stored as sparse indices-of-ones data.
    ///
    /// When reading detection event data with observables appended, the observable data goes into the `mask` field
    /// of the output. Note that this method requires that there be at most 32 observables.
    ///
    /// Args:
    ///     cleared_out: A cleared SparseShot struct to write data into.
    ///
    /// Returns:
    ///     True: The record was read successfully.
    ///     False: End of file. There were no more records. No record was read.
    ///
    /// Throws:
    ///     std::invalid_argument: A record was only partially read.
    virtual bool start_and_read_entire_record(SparseShot &cleared_out) = 0;

   protected:
    void move_obs_in_shots_to_mask_assuming_sorted(SparseShot &shot);
};

struct MeasureRecordReaderFormat01 : MeasureRecordReader {
    FILE *in;
    int payload;
    size_t position;

    MeasureRecordReaderFormat01(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool read_bit() override;
    bool next_record() override;
    bool start_record() override;
    bool is_end_of_record() override;
    bool start_and_read_entire_record(simd_bits_range_ref dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;

   private:
    template <typename SAW0, typename SAW1>
    bool start_and_read_entire_record_helper(SAW0 saw0, SAW1 saw1) {
        size_t n = bits_per_record();
        for (size_t k = 0; k < n; k++) {
            int b = getc(in);
            switch (b) {
                case '0':
                    saw0(k);
                    break;
                case '1':
                    saw1(k);
                    break;
                case EOF:
                    if (k == 0) {
                        return false;
                    }
                    // intentional fall through.
                case '\n':
                    throw std::invalid_argument(
                        "01 data ended in middle of record at byte position " + std::to_string(k) +
                        ".\nExpected bits per record was " + std::to_string(n) + ".");
                default:
                    throw std::invalid_argument("Unexpected character in 01 format data: '" + std::to_string(b) + "'.");
            }
        }
        if (getc(in) != '\n') {
            throw std::invalid_argument(
                "01 data didn't end with a newline after the expected data length of '" + std::to_string(n) + "'.");
        }
        return true;
    }
};

struct MeasureRecordReaderFormatB8 : MeasureRecordReader {
    FILE *in;
    int payload;
    uint8_t bits_available;
    size_t position;

    MeasureRecordReaderFormatB8(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    size_t read_bits_into_bytes(PointerRange<uint8_t> out_buffer) override;
    bool read_bit() override;
    bool next_record() override;
    bool start_record() override;
    bool is_end_of_record() override;
    bool start_and_read_entire_record(simd_bits_range_ref dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;

   private:
    void maybe_update_payload();

    template <typename HANDLE_BYTE>
    bool start_and_read_entire_record_helper(HANDLE_BYTE handle_byte) {
        size_t n = bits_per_record();
        size_t nb = (n + 7) >> 3;
        for (size_t k = 0; k < nb; k++) {
            int b = getc(in);
            if (b == EOF) {
                if (k == 0) {
                    return false;
                }
                throw std::invalid_argument(
                    "b8 data ended in middle of record at byte position " + std::to_string(k) +
                    ".\n"
                    "Expected bytes per record was " +
                    std::to_string(nb) + " (" + std::to_string(n) + " bits padded).");
            }
            handle_byte(k, (uint8_t)b);
        }
        return true;
    }
};

struct MeasureRecordReaderFormatHits : MeasureRecordReader {
    FILE *in;
    simd_bits buffer;
    size_t position_in_buffer;

    MeasureRecordReaderFormatHits(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool read_bit() override;
    bool next_record() override;
    bool start_record() override;
    bool is_end_of_record() override;
    bool start_and_read_entire_record(simd_bits_range_ref dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;

   private:
    template <typename HANDLE_HIT>
    bool start_and_read_entire_record_helper(HANDLE_HIT handle_hit) {
        bool first = true;
        while (true) {
            int next_char;
            uint64_t value;
            if (!read_uint64(in, value, next_char, false)) {
                if (first && next_char == EOF) {
                    return false;
                }
                if (first && next_char == '\n') {
                    return true;
                }
                throw std::invalid_argument("HITS data wasn't comma-separated integers terminated by a newline.");
            }
            handle_hit(value);
            first = false;
            if (next_char == '\n') {
                return true;
            }
            if (next_char != ',') {
                throw std::invalid_argument("HITS data wasn't comma-separated integers terminated by a newline.");
            }
        }
    }
};

struct MeasureRecordReaderFormatR8 : MeasureRecordReader {
    FILE *in;
    size_t position = 0;
    bool have_seen_terminal_1 = false;
    size_t buffered_0s = 0;
    size_t buffered_1s = 0;

    MeasureRecordReaderFormatR8(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    size_t read_bits_into_bytes(PointerRange<uint8_t> out_buffer) override;
    bool read_bit() override;
    bool next_record() override;
    bool start_record() override;
    bool is_end_of_record() override;

    bool start_and_read_entire_record(simd_bits_range_ref dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;

   private:
    bool maybe_buffer_data();
    template <typename HANDLE_HIT>
    bool start_and_read_entire_record_helper(HANDLE_HIT handle_hit) {
        int next_char = getc(in);
        if (next_char == EOF) {
            return false;
        }

        size_t n = bits_per_record();
        size_t pos = 0;
        while (true) {
            pos += next_char;
            if (next_char != 255) {
                if (pos < n) {
                    handle_hit(pos);
                    pos++;
                } else if (pos == n) {
                    return true;
                } else {
                    throw std::invalid_argument(
                        "r8 data jumped past expected end of encoded data. Expected to decode " +
                        std::to_string(bits_per_record()) + " bits.");
                }
            }
            next_char = getc(in);
            if (next_char == EOF) {
                throw std::invalid_argument(
                    "End of file before end of r8 data. Expected to decode " + std::to_string(bits_per_record()) +
                    " bits.");
            }
        }
    }
};

struct MeasureRecordReaderFormatDets : MeasureRecordReader {
    FILE *in;
    simd_bits buffer;
    size_t position_in_buffer;

    MeasureRecordReaderFormatDets(
        FILE *in, size_t num_measurements, size_t num_detectors = 0, size_t num_observables = 0);

    bool read_bit() override;
    bool next_record() override;
    bool start_record() override;
    bool is_end_of_record() override;
    bool start_and_read_entire_record(simd_bits_range_ref dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;

   private:
    template <typename HANDLE_HIT>
    bool start_and_read_entire_record_helper(HANDLE_HIT handle_hit) {
        // Read "shot" prefix, or notice end of data. Ignore indentation and spacing.
        while (true) {
            int next_char = getc(in);
            if (next_char == ' ' || next_char == '\n' || next_char == '\t') {
                continue;
            }
            if (next_char == EOF) {
                return false;
            }
            if (next_char != 's' || getc(in) != 'h' || getc(in) != 'o' || getc(in) != 't') {
                throw std::invalid_argument("DETS data didn't start with 'shot'");
            }
            break;
        }

        // Read prefixed integers until end of line.
        int next_char = getc(in);
        while (true) {
            if (next_char == '\n' || next_char == EOF) {
                return true;
            }
            if (next_char != ' ') {
                throw std::invalid_argument("DETS data wasn't single-space-separated with no trailing spaces.");
            }
            next_char = getc(in);
            uint64_t offset;
            uint64_t length;
            if (next_char == 'M') {
                offset = 0;
                length = num_measurements;
            } else if (next_char == 'D') {
                offset = num_measurements;
                length = num_detectors;
            } else if (next_char == 'L') {
                offset = num_measurements + num_detectors;
                length = num_observables;
            } else {
                throw std::invalid_argument(
                    "Unrecognized DETS prefix. Expected M or D or L not '" + std::to_string(next_char) + "'");
            }
            char prefix = next_char;

            uint64_t value;
            if (!read_uint64(in, value, next_char, false)) {
                throw std::invalid_argument("DETS data had a value prefix (M or D or L) not followed by an integer.");
            }
            if (value >= length) {
                std::stringstream msg;
                msg << "DETS data had a value that larger than expected. ";
                msg << "Got " << prefix << value << " but expected length of " << prefix << " space to be " << length
                    << ".";
                throw std::invalid_argument(msg.str());
            }
            handle_hit(offset + value);
        }
    }
};

}  // namespace stim

#endif
