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

#include "stim/io/sparse_shot.h"
#include "stim/io/stim_data_formats.h"
#include "stim/mem/simd_bit_table.h"
#include "stim/mem/span_ref.h"

namespace stim {

// Returns true if an integer value is found at current position. Returns false otherwise.
// Uses two output variables: value to return the integer value read and next for the next
// character or EOF.
inline bool read_uint64(FILE *in, uint64_t &value, int &next, bool include_next = false) {
    if (!include_next) {
        next = getc(in);
    }
    if (!isdigit(next)) {
        return false;
    }

    value = 0;
    while (isdigit(next)) {
        uint64_t prev_value = value;
        value *= 10;
        value += next - '0';
        if (value < prev_value) {
            throw std::runtime_error("Integer value read from file was too big");
        }
        next = getc(in);
    }
    return true;
}

/// Handles reading measurement data from the outside world.
///
/// Child classes implement the various input formats. Each file format encodes a certain number of records.
/// Each record is a sequence of 0s and 1s. File formats B8 and R8 encode a single record. File formats 01,
/// HITS and DETS encode any number of records. Record size in bits is fixed for each file and the client
/// must specify it upfront.
///
/// The template parameter, W, represents the SIMD width.
template <size_t W>
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
    static std::unique_ptr<MeasureRecordReader<W>> make(
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
    size_t read_records_into(simd_bit_table<W> &out, bool major_index_is_shot_index, size_t max_shots = UINT32_MAX);

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
    virtual bool start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) = 0;

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
    virtual size_t read_into_table_with_major_shot_index(simd_bit_table<W> &out_table, size_t max_shots);

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
    virtual size_t read_into_table_with_minor_shot_index(simd_bit_table<W> &out_table, size_t max_shots) = 0;

   protected:
    void move_obs_in_shots_to_mask_assuming_sorted(SparseShot &shot);
};

template <size_t W>
struct MeasureRecordReaderFormatPTB64 : MeasureRecordReader<W> {
    FILE *in;
    // This buffer stores partially transposed shots.
    // The uint64_t for index k of shot s is stored in the buffer at offset k*64 + s.
    simd_bits<W> buf;
    size_t num_unread_shots_in_buf;

    MeasureRecordReaderFormatPTB64(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_major_shot_index(simd_bit_table<W> &out_table, size_t max_shots) override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<W> &out_table, size_t max_shots) override;

   private:
    bool load_cache();
};

template <size_t W>
struct MeasureRecordReaderFormat01 : MeasureRecordReader<W> {
    FILE *in;

    MeasureRecordReaderFormat01(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<W> &out_table, size_t max_shots) override;

   private:
    template <typename SAW0, typename SAW1>
    bool start_and_read_entire_record_helper(SAW0 saw0, SAW1 saw1);
};

template <size_t W>
struct MeasureRecordReaderFormatB8 : MeasureRecordReader<W> {
    FILE *in;

    MeasureRecordReaderFormatB8(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<W> &out_table, size_t max_shots) override;
};

template <size_t W>
struct MeasureRecordReaderFormatHits : MeasureRecordReader<W> {
    FILE *in;

    MeasureRecordReaderFormatHits(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<W> &out_table, size_t max_shots) override;

   private:
    template <typename HANDLE_HIT>
    bool start_and_read_entire_record_helper(HANDLE_HIT handle_hit);
};

template <size_t W>
struct MeasureRecordReaderFormatR8 : MeasureRecordReader<W> {
    FILE *in;

    MeasureRecordReaderFormatR8(FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables);

    bool start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<W> &out_table, size_t max_shots) override;

   private:
    template <typename HANDLE_HIT>
    bool start_and_read_entire_record_helper(HANDLE_HIT handle_hit);
};

template <size_t W>
struct MeasureRecordReaderFormatDets : MeasureRecordReader<W> {
    FILE *in;

    MeasureRecordReaderFormatDets(
        FILE *in, size_t num_measurements, size_t num_detectors = 0, size_t num_observables = 0);

    bool start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) override;
    bool start_and_read_entire_record(SparseShot &cleared_out) override;
    bool expects_empty_serialized_data_for_each_shot() const override;
    size_t read_into_table_with_minor_shot_index(simd_bit_table<W> &out_table, size_t max_shots) override;

   private:
    template <typename HANDLE_HIT>
    bool start_and_read_entire_record_helper(HANDLE_HIT handle_hit);
};

template <size_t W>
size_t read_file_data_into_shot_table(
    FILE *in,
    size_t max_shots,
    size_t num_bits_per_shot,
    SampleFormat format,
    char dets_char,
    simd_bit_table<W> &out_table,
    bool shots_is_major_index_of_out_table);

}  // namespace stim

#include "stim/io/measure_record_reader.inl"

#endif
