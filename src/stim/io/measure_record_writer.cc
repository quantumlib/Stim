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

#include "stim/io/measure_record_writer.h"

#include <algorithm>

using namespace stim;

std::unique_ptr<MeasureRecordWriter> MeasureRecordWriter::make(FILE *out, SampleFormat output_format) {
    switch (output_format) {
        case SAMPLE_FORMAT_01:
            return std::unique_ptr<MeasureRecordWriter>(new MeasureRecordWriterFormat01(out));
        case SAMPLE_FORMAT_B8:
            return std::unique_ptr<MeasureRecordWriter>(new MeasureRecordWriterFormatB8(out));
        case SAMPLE_FORMAT_DETS:
            return std::unique_ptr<MeasureRecordWriter>(new MeasureRecordWriterFormatDets(out));
        case SAMPLE_FORMAT_HITS:
            return std::unique_ptr<MeasureRecordWriter>(new MeasureRecordWriterFormatHits(out));
        case SAMPLE_FORMAT_PTB64:
            throw std::invalid_argument("SAMPLE_FORMAT_PTB64 incompatible with SingleMeasurementRecord");
        case SAMPLE_FORMAT_R8:
            return std::unique_ptr<MeasureRecordWriter>(new MeasureRecordWriterFormatR8(out));
        default:
            throw std::invalid_argument("Sample format not recognized by SingleMeasurementRecord");
    }
}

void MeasureRecordWriter::begin_result_type(char result_type) {
}

void MeasureRecordWriter::write_bits(uint8_t *data, size_t num_bits) {
    size_t num_bytes = num_bits >> 3;
    write_bytes({data, data + num_bytes});
    for (size_t b = 0; b < (num_bits & 7); b++) {
        write_bit((data[num_bytes] >> b) & 1);
    }
}

void MeasureRecordWriter::write_bytes(ConstPointerRange<uint8_t> data) {
    for (uint8_t b : data) {
        for (size_t k = 0; k < 8; k++) {
            write_bit((b >> k) & 1);
        }
    }
}

MeasureRecordWriterFormat01::MeasureRecordWriterFormat01(FILE *out) : out(out) {
}

void MeasureRecordWriterFormat01::write_bit(bool b) {
    putc('0' + b, out);
}

void MeasureRecordWriterFormat01::write_end() {
    putc('\n', out);
}

MeasureRecordWriterFormatB8::MeasureRecordWriterFormatB8(FILE *out) : out(out) {
}

void MeasureRecordWriterFormatB8::write_bytes(ConstPointerRange<uint8_t> data) {
    if (count == 0) {
        fwrite(data.ptr_start, sizeof(uint8_t), data.ptr_end - data.ptr_start, out);
    } else {
        MeasureRecordWriter::write_bytes(data);
    }
}

void MeasureRecordWriterFormatB8::write_bit(bool b) {
    payload |= uint8_t{b} << count;
    count++;
    if (count == 8) {
        putc(payload, out);
        count = 0;
        payload = 0;
    }
}

void MeasureRecordWriterFormatB8::write_end() {
    if (count > 0) {
        putc(payload, out);
        count = 0;
        payload = 0;
    }
}

MeasureRecordWriterFormatHits::MeasureRecordWriterFormatHits(FILE *out) : out(out) {
}

void MeasureRecordWriterFormatHits::write_bytes(ConstPointerRange<uint8_t> data) {
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

void MeasureRecordWriterFormatHits::write_bit(bool b) {
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

void MeasureRecordWriterFormatHits::write_end() {
    putc('\n', out);
    position = 0;
    first = true;
}

MeasureRecordWriterFormatR8::MeasureRecordWriterFormatR8(FILE *out) : out(out) {
}

void MeasureRecordWriterFormatR8::write_bytes(ConstPointerRange<uint8_t> data) {
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

void MeasureRecordWriterFormatR8::write_bit(bool b) {
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

void MeasureRecordWriterFormatR8::write_end() {
    putc(run_length, out);
    run_length = 0;
}

MeasureRecordWriterFormatDets::MeasureRecordWriterFormatDets(FILE *out) : out(out) {
}

void MeasureRecordWriterFormatDets::begin_result_type(char new_result_type) {
    result_type = new_result_type;
    position = 0;
}

void MeasureRecordWriterFormatDets::write_bytes(ConstPointerRange<uint8_t> data) {
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

void MeasureRecordWriterFormatDets::write_bit(bool b) {
    if (b) {
        if (first) {
            fprintf(out, "shot");
            first = false;
        }
        putc(' ', out);
        putc(result_type, out);
        fprintf(out, "%lld", (unsigned long long)(position));
    }
    position++;
}

void MeasureRecordWriterFormatDets::write_end() {
    if (first) {
        fprintf(out, "shot");
    }
    putc('\n', out);
    position = 0;
    first = true;
}

simd_bit_table stim::transposed_vs_ref(
    size_t num_samples_raw, const simd_bit_table &table, const simd_bits &reference_sample) {
    auto result = table.transposed();
    for (size_t s = 0; s < num_samples_raw; s++) {
        result[s].word_range_ref(0, reference_sample.num_simd_words) ^= reference_sample;
    }
    return result;
}

void stim::write_table_data(
    FILE *out,
    size_t num_shots,
    size_t num_measurements,
    const simd_bits &reference_sample,
    const simd_bit_table &table,
    SampleFormat format,
    char dets_prefix_1,
    char dets_prefix_2,
    size_t dets_prefix_transition) {
    if (format == SAMPLE_FORMAT_PTB64) {
        if (num_shots % 64 != 0) {
            throw std::invalid_argument("shots must be a multiple of 64 to use ptb64 format.");
        }
        auto f64 = num_shots >> 6;
        for (size_t s = 0; s < f64; s++) {
            for (size_t m = 0; m < num_measurements; m++) {
                uint64_t v = table[m].u64[s];
                if (m < reference_sample.num_bits_padded() && reference_sample[m]) {
                    v = ~v;
                }
                fwrite(&v, 1, 64 >> 3, out);
            }
        }
    } else {
        auto result = transposed_vs_ref(num_shots, table, reference_sample);
        if (dets_prefix_transition == 0) {
            dets_prefix_transition = num_measurements;
            dets_prefix_1 = dets_prefix_2;
        } else if (dets_prefix_1 == dets_prefix_2 || dets_prefix_transition >= num_measurements) {
            dets_prefix_transition = num_measurements;
        }
        for (size_t shot = 0; shot < num_shots; shot++) {
            auto w = MeasureRecordWriter::make(out, format);

            w->begin_result_type(dets_prefix_1);
            size_t n8 = dets_prefix_transition >> 3;
            uint8_t *p = result[shot].u8;
            w->write_bytes({p, p + n8});
            size_t m = n8 << 3;
            while (m < dets_prefix_transition) {
                w->write_bit(result[shot][m]);
                m++;
            }

            w->begin_result_type(dets_prefix_2);
            while (m < num_measurements) {
                w->write_bit(result[shot][m]);
                m++;
            }

            w->write_end();
        }
    }
}
