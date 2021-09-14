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

#ifndef _STIM_IO_MEASURE_RECORD_WRITER_H
#define _STIM_IO_MEASURE_RECORD_WRITER_H

#include <memory>

#include "stim/circuit/circuit.h"
#include "stim/io/stim_data_formats.h"
#include "stim/mem/pointer_range.h"
#include "stim/mem/simd_bit_table.h"

namespace stim {

/// Handles writing measurement data to the outside world.
///
/// Child classes implement the various output formats.
struct MeasureRecordWriter {
    /// Creates a MeasureRecordWriter that writes the given format into the given FILE*.
    static std::unique_ptr<MeasureRecordWriter> make(FILE *out, SampleFormat output_format);
    virtual ~MeasureRecordWriter() = default;
    /// Writes (or buffers) one measurement result.
    virtual void write_bit(bool b) = 0;
    /// Writes (or buffers) multiple measurement results.
    virtual void write_bytes(ConstPointerRange<uint8_t> data);
    /// Flushes all buffered measurement results and writes any end-of-record markers that are needed (e.g. a newline).
    virtual void write_end() = 0;
    /// Writes (or buffers) multiple measurement results.
    virtual void write_bits(uint8_t *data, size_t num_bits);
    /// Used to control the DETS format prefix character (M for measurement, D for detector, L for logical observable).
    ///
    /// Setting this is understood to reset the "result index" back to 0 so that e.g. listing logical observables after
    /// detectors results in the first logical observable being L0 instead of L[number-of-detectors].
    virtual void begin_result_type(char result_type);
};

struct MeasureRecordWriterFormat01 : MeasureRecordWriter {
    FILE *out;
    MeasureRecordWriterFormat01(FILE *out);
    void write_bit(bool b) override;
    void write_end() override;
};

struct MeasureRecordWriterFormatB8 : MeasureRecordWriter {
    FILE *out;
    uint8_t payload = 0;
    uint8_t count = 0;
    MeasureRecordWriterFormatB8(FILE *out);
    void write_bytes(ConstPointerRange<uint8_t> data) override;
    void write_bit(bool b) override;
    void write_end() override;
};

struct MeasureRecordWriterFormatHits : MeasureRecordWriter {
    FILE *out;
    uint64_t position = 0;
    bool first = true;

    MeasureRecordWriterFormatHits(FILE *out);
    void write_bytes(ConstPointerRange<uint8_t> data) override;
    void write_bit(bool b) override;
    void write_end() override;
};

struct MeasureRecordWriterFormatR8 : MeasureRecordWriter {
    FILE *out;
    uint16_t run_length = 0;

    MeasureRecordWriterFormatR8(FILE *out);
    void write_bytes(ConstPointerRange<uint8_t> data) override;
    void write_bit(bool b) override;
    void write_end() override;
};

struct MeasureRecordWriterFormatDets : MeasureRecordWriter {
    FILE *out;
    uint64_t position = 0;
    char result_type = 'M';
    bool first = true;

    MeasureRecordWriterFormatDets(FILE *out);
    void begin_result_type(char result_type) override;
    void write_bytes(ConstPointerRange<uint8_t> data) override;
    void write_bit(bool b) override;
    void write_end() override;
};

simd_bit_table transposed_vs_ref(
    size_t num_samples_raw, const simd_bit_table &table, const simd_bits &reference_sample);

void write_table_data(
    FILE *out,
    size_t num_shots,
    size_t num_measurements,
    const simd_bits &reference_sample,
    const simd_bit_table &table,
    SampleFormat format,
    char dets_prefix_1,
    char dets_prefix_2,
    size_t dets_prefix_transition);

}  // namespace stim

#endif
