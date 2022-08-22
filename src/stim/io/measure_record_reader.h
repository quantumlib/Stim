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

    /// Determines whether or not there is no actual data written for each shot.
    ///
    /// For example, sampling a circuit with no measurements produces no bytes of data
    /// when using the 'b8' format.
    ///
    /// This is important to check for sometimes. For example, instead of getting stuck
    /// in an infinite loop repeatedly reading zero bytes of data and not reaching the
    /// end of the file, code can check for this degenerate code.
    virtual bool expects_empty_serialized_data_for_each_shot() const = 0;

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
        simd_bit_table<MAX_BITWORD_WIDTH> &out, bool major_index_is_shot_index, size_t max_shots = UINT32_MAX);

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
    virtual bool start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) = 0;

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

    /// Reads many records into a shot table.
    ///
    /// Args:
    ///     out_table: The table to write shots into.
    ///         Must have num_minor_bits >= bits_per_shot.
    ///         num_major_bits is max read shots.
    ///     max_shots: Don't read more than this many shots.
    ///         Must be at most the number of shots that can be stored in the table.
    ///
    /// Returns:
    ///     The number of shots that were read.
    virtual size_t read_into_table_with_major_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots);

    /// Reads many records into a shot table.
    ///
    /// Args:
    ///     out_table: The table to write shots into.
    ///         Must have num_major_bits >= bits_per_shot.
    ///         num_minor_bits is max read shots.
    ///     max_shots: Don't read more than this many shots.
    ///         Must be at most the number of shots that can be stored in the table.
    ///
    /// Returns:
    ///     The number of shots that were read.
    virtual size_t read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) = 0;

   protected:
    void move_obs_in_shots_to_mask_assuming_sorted(SparseShot &shot);
};

struct MeasureRecordReaderFormatPTB64 : MeasureRecordReader {
    FILE *in;
    // This buffer stores partially transposed shots.
    // The uint64_t for index k of shot s is stored in the buffer at offset k*64 + s.
    simd_bits<MAX_BITWORD_WIDTH> buf;
    size_t num_unread_shots_in_buf;

    MeasureRecordReaderFormatPTB64(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_major_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) override;

   private:
    bool load_cache();
};

struct MeasureRecordReaderFormat01 : MeasureRecordReader {
    FILE *in;

    MeasureRecordReaderFormat01(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) override;

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
        int last = getc(in);
        if (n == 0 && last == EOF) {
            return false;
        }
        if (last != '\n') {
            throw std::invalid_argument(
                "01 data didn't end with a newline after the expected data length of '" + std::to_string(n) + "'.");
        }
        return true;
    }
};

struct MeasureRecordReaderFormatB8 : MeasureRecordReader {
    FILE *in;

    MeasureRecordReaderFormatB8(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) override;
};

struct MeasureRecordReaderFormatHits : MeasureRecordReader {
    FILE *in;

    MeasureRecordReaderFormatHits(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) override;

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
            handle_hit((size_t)value);
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

    MeasureRecordReaderFormatR8(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) override;

   private:
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

    MeasureRecordReaderFormatDets(
        FILE *in, size_t num_measurements, size_t num_detectors = 0, size_t num_observables = 0);

    bool start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) override;

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
                msg << "DETS data had a value larger than expected. ";
                msg << "Got " << prefix << value << " but expected length of " << prefix << " space to be " << length
                    << ".";
                throw std::invalid_argument(msg.str());
            }
            handle_hit((size_t)(offset + value));
        }
    }
};

size_t read_file_data_into_shot_table(
    FILE *in,
    size_t max_shots,
    size_t num_bits_per_shot,
    SampleFormat format,
    char dets_char,
    simd_bit_table<MAX_BITWORD_WIDTH> &out_table,
    bool shots_is_major_index_of_out_table);

}  // namespace stim

#endif
