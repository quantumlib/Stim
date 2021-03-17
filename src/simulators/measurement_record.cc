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

#include <cassert>
#include <algorithm>
#include "measurement_record.h"

void SingleResultWriter::set_result_type(char result_type) {
}
void SingleResultWriter::write_bytes(ConstPointerRange<uint8_t> data) {
    for (uint8_t b : data) {
        for (size_t k = 0; k < 8; k++) {
            write_bit((b >> k) & 1);
        }
    }
}

ResultWriterFormat01::ResultWriterFormat01(FILE *out) : out(out) {
}
void ResultWriterFormat01::write_bit(bool b) {
    putc('0' + b, out);
}
void ResultWriterFormat01::write_end() {
    putc('\n', out);
}

ResultWriterFormatB8::ResultWriterFormatB8(FILE *out) : out(out) {
}
void ResultWriterFormatB8::write_bytes(ConstPointerRange<uint8_t> data) {
    if (count == 0) {
        fwrite(data.ptr_start, sizeof(uint8_t), data.ptr_end - data.ptr_start, out);
    } else {
        SingleResultWriter::write_bytes(data);
    }
}
void ResultWriterFormatB8::write_bit(bool b) {
    payload |= uint8_t{b} << count;
    count++;
    if (count == 8) {
        putc(payload, out);
        count = 0;
        payload = 0;
    }
}
void ResultWriterFormatB8::write_end() {
    if (count > 0) {
        putc(payload, out);
        count = 0;
        payload = 0;
    }
}

ResultWriterFormatHits::ResultWriterFormatHits(FILE *out) : out(out) {
}
void ResultWriterFormatHits::write_bytes(ConstPointerRange<uint8_t> data) {
    for (uint8_t b : data) {
        if (!b) {
            position += 8;
        } else {
            for (size_t k = 0; k < 8; k++) {
                write_bit((b >> k) & 1);
            }
        }
    }
}
void ResultWriterFormatHits::write_bit(bool b) {
    if (b) {
        if (first) {
            first = false;
        } else {
            putc(',', out);
        }
        fprintf(out, "%lld", (unsigned long long)(position));
    }
    position++;
}
void ResultWriterFormatHits::write_end() {
    putc('\n', out);
    position = 0;
    first = true;
}

ResultWriterFormatR8::ResultWriterFormatR8(FILE *out) : out(out) {
}
void ResultWriterFormatR8::write_bytes(ConstPointerRange<uint8_t> data) {
    for (uint8_t b : data) {
        if (!b) {
            run_length += 8;
            if (run_length >= 0xFF) {
                putc(0xFF, out);
                run_length -= 0xFF;
            }
        } else {
            for (size_t k = 0; k < 8; k++) {
                write_bit((b >> k) & 1);
            }
        }
    }
}
void ResultWriterFormatR8::write_bit(bool b) {
    if (b) {
        putc(run_length, out);
        run_length = 0;
    } else {
        run_length++;
        if (run_length == 255) {
            putc(run_length, out);
            run_length = 0;
        }
    }
}
void ResultWriterFormatR8::write_end() {
    putc(run_length, out);
    run_length = 0;
}

ResultWriterFormatDets::ResultWriterFormatDets(FILE *out) : out(out) {
    fprintf(out, "shot");
}

void ResultWriterFormatDets::set_result_type(char new_result_type) {
    result_type = new_result_type;
    position = 0;
}

void ResultWriterFormatDets::write_bytes(ConstPointerRange<uint8_t> data) {
    for (uint8_t b : data) {
        if (!b) {
            position += 8;
        } else {
            for (size_t k = 0; k < 8; k++) {
                write_bit((b >> k) & 1);
            }
        }
    }
}
void ResultWriterFormatDets::write_bit(bool b) {
    if (b) {
        putc(' ', out);
        putc(result_type, out);
        fprintf(out, "%lld", (unsigned long long)(position));
    }
    position++;
}
void ResultWriterFormatDets::write_end() {
    putc('\n', out);
    position = 0;
}

SingleMeasurementRecord::SingleMeasurementRecord(size_t max_lookback) : max_lookback(max_lookback), unwritten(0) {
}

void SingleMeasurementRecord::write_unwritten_results_to(SingleResultWriter &writer) {
    size_t n = lookback_storage.size();
    for (size_t k = n - unwritten; k < n; k++) {
        writer.write_bit(lookback_storage[k]);
    }
    unwritten = 0;
    if ((lookback_storage.size() >> 1) > max_lookback) {
        lookback_storage.erase(lookback_storage.begin(), lookback_storage.end() - max_lookback);
    }
}

bool SingleMeasurementRecord::lookback(size_t lookback) const {
    if (lookback > lookback_storage.size()) {
        throw std::out_of_range("Referred to a measurement record before the beginning of time.");
    }
    if (lookback == 0) {
        throw std::out_of_range("Lookback must be non-zero.");
    }
    if (lookback > max_lookback) {
        throw std::out_of_range("Referred to a measurement record past the lookback limit.");
    }
    return *(lookback_storage.end() - lookback);
}

void SingleMeasurementRecord::record_result(bool result) {
    lookback_storage.push_back(result);
    unwritten++;
}

std::unique_ptr<SingleResultWriter> SingleResultWriter::make(FILE *out, SampleFormat output_format) {
    switch (output_format) {
        case SAMPLE_FORMAT_01:
            return std::unique_ptr<SingleResultWriter>(new ResultWriterFormat01(out));
        case SAMPLE_FORMAT_B8:
            return std::unique_ptr<SingleResultWriter>(new ResultWriterFormatB8(out));
        case SAMPLE_FORMAT_DETS:
            return std::unique_ptr<SingleResultWriter>(new ResultWriterFormatDets(out));
        case SAMPLE_FORMAT_HITS:
            return std::unique_ptr<SingleResultWriter>(new ResultWriterFormatHits(out));
        case SAMPLE_FORMAT_PTB64:
            throw std::invalid_argument("SAMPLE_FORMAT_PTB64 incompatible with SingleMeasurementRecord");
        case SAMPLE_FORMAT_R8:
            return std::unique_ptr<SingleResultWriter>(new ResultWriterFormatR8(out));
        default:
            throw std::invalid_argument("Sample format not recognized by SingleMeasurementRecord");
    }
}

BatchResultWriter::BatchResultWriter(FILE *out, size_t num_shots, SampleFormat output_format) : output_format(output_format), out(out) {
    auto f = output_format;
    auto s = num_shots;
    if (output_format == SAMPLE_FORMAT_PTB64) {
        f = SAMPLE_FORMAT_B8;
        s += 63;
        s /= 64;
    }
    if (s) {
        writers.push_back(SingleResultWriter::make(out, f));
    }
    for (size_t k = 1; k < s; k++) {
        FILE *file = tmpfile();
        writers.push_back(SingleResultWriter::make(file, f));
        temporary_files.push_back(file);
    }
}

BatchResultWriter::~BatchResultWriter() {
    for (auto &e : temporary_files) {
        fclose(e);
    }
    temporary_files.clear();
}

void BatchResultWriter::set_result_type(char result_type) {
    for (auto &e : writers) {
        e->set_result_type(result_type);
    }
}

void BatchResultWriter::write_table_batch(simd_bit_table slice, size_t num_major_u64) {
    if (output_format == SAMPLE_FORMAT_PTB64) {
        for (size_t k = 0; k < writers.size(); k++) {
            for (size_t w = 0; w < num_major_u64; w++) {
                uint8_t *p = slice.data.u8 + (k * 8) + slice.num_minor_u8_padded() * w;
                writers[k]->write_bytes({p, p + 8});
            }
        }
    } else {
        auto transposed = slice.transposed();
        for (size_t k = 0; k < writers.size(); k++) {
            uint8_t *p = transposed[k].u8;
            writers[k]->write_bytes({p, p + num_major_u64 * 8});
        }
    }
}

void BatchResultWriter::write_bit_batch(simd_bits_range_ref bits) {
    if (output_format == SAMPLE_FORMAT_PTB64) {
        uint8_t *p = bits.u8;
        for (auto & writer : writers) {
            uint8_t *n = p + 8;
            writer->write_bytes({p, n});
            p = n;
        }
    } else {
        for (size_t k = 0; k < writers.size(); k++) {
            writers[k]->write_bit(bits[k]);
        }
    }
}

void BatchResultWriter::write_end() {
    for (auto &writer : writers) {
        writer->write_end();
    }

    for (FILE *file : temporary_files) {
        rewind(file);
        while (true) {
            int c = getc(file);
            if (c == EOF) {
                break;
            }
            putc(c, out);
        }
        fclose(file);
    }
    temporary_files.clear();
}

BatchMeasurementRecord::BatchMeasurementRecord(size_t num_shots, size_t max_lookback, size_t initial_capacity)
    : max_lookback(max_lookback), unwritten(0), stored(0), written(0), shot_mask(num_shots), storage(initial_capacity, num_shots) {
    for (size_t k = 0; k < num_shots; k++) {
        shot_mask[k] = true;
    }
}

void BatchMeasurementRecord::record_result(simd_bits_range_ref result) {
    if (stored >= storage.num_major_bits_padded()) {
        simd_bit_table new_storage(storage.num_major_bits_padded() * 2, storage.num_minor_bits_padded());
        new_storage.data.word_range_ref(0, storage.data.num_simd_words) = storage.data;
        storage = std::move(new_storage);
    }
    storage[stored] = result;
    storage[stored] &= shot_mask;
    stored++;
    unwritten++;
}

simd_bits_range_ref BatchMeasurementRecord::lookback(size_t lookback) const {
    if (lookback > stored) {
        throw std::out_of_range("Referred to a measurement record before the beginning of time.");
    }
    if (lookback == 0) {
        throw std::out_of_range("Lookback must be non-zero.");
    }
    if (lookback > max_lookback) {
        throw std::out_of_range("Referred to a measurement record past the lookback limit.");
    }
    return storage[stored - lookback];
}

void BatchMeasurementRecord::mark_all_as_written() {
    unwritten = 0;
    size_t m = max_lookback;
    if ((stored >> 1) > m) {
        memcpy(storage.data.u8, storage[stored - m].u8, m * storage.num_minor_u8_padded());
        stored = m;
    }
}

void BatchMeasurementRecord::intermediate_write_unwritten_results_to(BatchResultWriter &writer, simd_bits_range_ref ref_sample) {
    while (unwritten >= 1024) {
        auto slice = storage.slice_maj(stored - unwritten, stored - unwritten + 1024);
        for (size_t k = 0; k < 1024; k++) {
            size_t j = written + k;
            if (j < ref_sample.num_bits_padded() && ref_sample[j]) {
                slice[k] ^= shot_mask;
            }
        }
        writer.write_table_batch(slice, 1024 >> 6);
        unwritten -= 1024;
        written += 1024;
    }

    size_t m = std::max(max_lookback, unwritten);
    if ((stored >> 1) > m) {
        memcpy(storage.data.u8, storage[stored - m].u8, m * storage.num_minor_u8_padded());
        stored = m;
    }
}

void BatchMeasurementRecord::final_write_unwritten_results_to(
        BatchResultWriter &writer, simd_bits_range_ref ref_sample) {
    size_t n = stored;
    for (size_t k = n - unwritten; k < n; k++) {
        bool invert = written < ref_sample.num_bits_padded() && ref_sample[written];
        if (invert) {
            storage[k] ^= shot_mask;
        }
        writer.write_bit_batch(storage[k]);
        if (invert) {
            storage[k] ^= shot_mask;
        }
        written++;
    }
    unwritten = 0;
    writer.write_end();
}

void BatchMeasurementRecord::clear() {
    stored = 0;
    unwritten = 0;
}
