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

#include "measure_record_writer.h"

#include <algorithm>

std::unique_ptr<MeasureRecordWriter> MeasureRecordWriter::make(FILE *out, SampleFormat output_format) {
    switch (output_format) {
        case SAMPLE_FORMAT_01:
            return std::unique_ptr<MeasureRecordWriter>(new MeasureRecordWriterFormat01(out));
        case SAMPLE_FORMAT_B8:
            return std::unique_ptr<MeasureRecordWriter>(new MeasureRecordWriterFormatB8(out));
        case SAMPLE_FORMAT_DETS:
            return std::unique_ptr<MeasureRecordWriter>(new MeasureRecordFormatDets(out));
        case SAMPLE_FORMAT_HITS:
            return std::unique_ptr<MeasureRecordWriter>(new MeasureRecordWriterFormatHits(out));
        case SAMPLE_FORMAT_PTB64:
            throw std::invalid_argument("SAMPLE_FORMAT_PTB64 incompatible with SingleMeasurementRecord");
        case SAMPLE_FORMAT_R8:
            return std::unique_ptr<MeasureRecordWriter>(new MeasureRecordFormatR8(out));
        default:
            throw std::invalid_argument("Sample format not recognized by SingleMeasurementRecord");
    }
}

void MeasureRecordWriter::begin_result_type(char result_type) {
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

MeasureRecordFormatR8::MeasureRecordFormatR8(FILE *out) : out(out) {
}
void MeasureRecordFormatR8::write_bytes(ConstPointerRange<uint8_t> data) {
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
void MeasureRecordFormatR8::write_bit(bool b) {
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
void MeasureRecordFormatR8::write_end() {
    putc(run_length, out);
    run_length = 0;
}

MeasureRecordFormatDets::MeasureRecordFormatDets(FILE *out) : out(out) {
    fprintf(out, "shot");
}

void MeasureRecordFormatDets::begin_result_type(char new_result_type) {
    result_type = new_result_type;
    position = 0;
}

void MeasureRecordFormatDets::write_bytes(ConstPointerRange<uint8_t> data) {
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
void MeasureRecordFormatDets::write_bit(bool b) {
    if (b) {
        putc(' ', out);
        putc(result_type, out);
        fprintf(out, "%lld", (unsigned long long)(position));
    }
    position++;
}
void MeasureRecordFormatDets::write_end() {
    putc('\n', out);
    position = 0;
}
