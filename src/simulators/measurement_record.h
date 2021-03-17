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

#ifndef MEASUREMENT_RECORD_H
#define MEASUREMENT_RECORD_H

#include <cassert>
#include <functional>
#include <iostream>
#include <new>
#include <random>
#include <sstream>

#include "../circuit/circuit.h"
#include "../stabilizers/tableau.h"
#include "../stabilizers/tableau_transposed_raii.h"
#include "vector_simulator.h"

struct SingleResultWriter {
    static std::unique_ptr<SingleResultWriter> make(FILE *out, SampleFormat output_format);
    virtual void set_result_type(char result_type);
    virtual void write_bit(bool b) = 0;
    virtual void write_end() = 0;
    virtual void write_bytes(ConstPointerRange<uint8_t> data);
};

struct ResultWriterFormat01 : SingleResultWriter {
    FILE *out;
    ResultWriterFormat01(FILE *out);
    void write_bit(bool b) override;
    void write_end() override;
};

struct ResultWriterFormatB8 : SingleResultWriter {
    FILE *out;
    uint8_t payload = 0;
    uint8_t count = 0;
    ResultWriterFormatB8(FILE *out);
    void write_bytes(ConstPointerRange<uint8_t> data) override;
    void write_bit(bool b) override;
    void write_end() override;
};

struct ResultWriterFormatHits : SingleResultWriter {
    FILE *out;
    uint64_t position = 0;
    bool first = true;

    ResultWriterFormatHits(FILE *out);
    void write_bytes(ConstPointerRange<uint8_t> data) override;
    void write_bit(bool b) override;
    void write_end() override;
};

struct ResultWriterFormatR8 : SingleResultWriter {
    FILE *out;
    uint16_t run_length = 0;

    ResultWriterFormatR8(FILE *out);
    void write_bytes(ConstPointerRange<uint8_t> data) override;
    void write_bit(bool b) override;
    void write_end() override;
};

struct ResultWriterFormatDets : SingleResultWriter {
    FILE *out;
    uint64_t position = 0;
    char result_type = 'M';

    ResultWriterFormatDets(FILE *out);
    void set_result_type(char result_type) override;
    void write_bytes(ConstPointerRange<uint8_t> data) override;
    void write_bit(bool b) override;
    void write_end() override;
};

struct SingleMeasurementRecord {
   private:
    size_t max_lookback;
    size_t unwritten;

   public:
    std::vector<bool> storage;
    SingleMeasurementRecord(size_t max_lookback = SIZE_MAX);
    void write_unwritten_results_to(SingleResultWriter &writer);
    bool lookback(size_t lookback) const;
    void record_result(bool result);
};

struct BatchResultWriter {
    SampleFormat output_format;
    FILE *out;
    std::vector<FILE *> temporary_files;
    std::vector<std::unique_ptr<SingleResultWriter>> writers;

    BatchResultWriter(FILE *out, size_t num_shots, SampleFormat output_format);
    ~BatchResultWriter();
    void set_result_type(char result_type);
    void write_table_batch(simd_bit_table slice, size_t num_major_u64);
    void write_bit_batch(simd_bits_range_ref bits);
    void write_end();
};

struct BatchMeasurementRecord {
    size_t max_lookback;
    size_t unwritten;
    size_t stored;
    size_t written;
    simd_bits shot_mask;
    simd_bit_table storage;
    BatchMeasurementRecord(size_t num_shots, size_t max_lookback, size_t initial_capacity);
    void mark_all_as_written();
    void intermediate_write_unwritten_results_to(BatchResultWriter &writer, simd_bits_range_ref ref_sample);
    void final_write_unwritten_results_to(BatchResultWriter &writer, simd_bits_range_ref ref_sample);
    simd_bits_range_ref lookback(size_t lookback) const;
    void record_result(simd_bits_range_ref result);
    void clear();
};

#endif
