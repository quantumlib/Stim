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

#include "measure_record_reader.h"

#include <algorithm>

using namespace stim_internal;

std::unique_ptr<MeasureRecordReader> MeasureRecordReader::make(FILE *in, SampleFormat input_format, size_t max_bits) {
    switch (input_format) {
        case SAMPLE_FORMAT_01:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormat01(in, max_bits));
        case SAMPLE_FORMAT_B8:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatB8(in, max_bits));
        case SAMPLE_FORMAT_DETS:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatDets(in, max_bits));
        case SAMPLE_FORMAT_HITS:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatHits(in, max_bits));
        case SAMPLE_FORMAT_PTB64:
            throw std::invalid_argument("SAMPLE_FORMAT_PTB64 incompatible with SingleMeasurementRecord");
        case SAMPLE_FORMAT_R8:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatR8(in, max_bits));
        default:
            throw std::invalid_argument("Sample format not recognized by SingleMeasurementRecord");
    }
}

size_t MeasureRecordReader::read_bytes(PointerRange<uint8_t> data) {
    size_t n = 0;
    for (uint8_t &b : data) {
        b = 0;
        for (size_t k = 0; k < 8; k++) {
            if (is_end_of_record()) {
                return n;
            }
            b |= uint8_t(read_bit()) << k;
            ++n;
        }
    }
    return n;
}

bool MeasureRecordReader::is_end_of_record() {
    return is_end_of_file();
}

char MeasureRecordReader::current_result_type() {
    return 'M';
}

/// 01 format

MeasureRecordReaderFormat01::MeasureRecordReaderFormat01(FILE *in, size_t max_bits) : in(in), payload(getc(in)), max_bits(max_bits) {
}

bool MeasureRecordReaderFormat01::read_bit() {
    // We assume here that in practice bits_returned will never reach SIZE_MAX.
    if (payload == EOF or bits_returned >= max_bits) {
        throw std::out_of_range("Attempt to read past end-of-file");
    }
    if (payload == '\n') {
        throw std::out_of_range("Attempt to read past end-of-record");
    }
    if (payload != '0' and payload != '1') {
        throw std::invalid_argument("Character code " + std::to_string(payload) + " does not encode a bit");
    }

    bool bit = payload == '1';
    payload = getc(in);
    ++bits_returned;
    return bit;
}

bool MeasureRecordReaderFormat01::is_end_of_record() {
    bool eor = payload == EOF or bits_returned >= max_bits or payload == '\n';
    if (payload == '\n') {
        payload = getc(in);
    }
    return eor;
}

bool MeasureRecordReaderFormat01::is_end_of_file() {
    return payload == EOF or bits_returned >= max_bits;
}

/// B8 format

MeasureRecordReaderFormatB8::MeasureRecordReaderFormatB8(FILE *in, size_t max_bits) : in(in), max_bits(max_bits) {
}

size_t MeasureRecordReaderFormatB8::read_bytes(PointerRange<uint8_t> data) {
    if (bits_returned >= max_bits) return 0;
    if (bits_available == 0) {
        size_t k = std::min<size_t>(data.ptr_end - data.ptr_start, (max_bits - bits_returned + 7) / 8);
        k = 8 * fread(data.ptr_start, sizeof(uint8_t), k, in);
        bits_returned += k;
        return k;
    }
    return MeasureRecordReader::read_bytes(data);
}

bool MeasureRecordReaderFormatB8::read_bit() {
    if (bits_available == 0) {
        payload = getc(in);
        if (payload != EOF) bits_available = 8;
    }
    if (payload == EOF or bits_returned >= max_bits) {
        throw std::out_of_range("Attempt to read past end-of-file");
    }
    bool b = payload & 1;
    payload >>= 1;
    --bits_available;
    ++bits_returned;
    return b;
}

bool MeasureRecordReaderFormatB8::is_end_of_file() {
    if (bits_returned >= max_bits) return true;
    if (bits_available != 0) return false;
    payload = getc(in);
    if (payload != EOF) {
        bits_available = 8;
        return false;
    }
    return true;
}

/// Hits format

MeasureRecordReaderFormatHits::MeasureRecordReaderFormatHits(FILE *in, size_t max_bits) : in(in), max_bits(max_bits) {
    update_next_hit();
}

bool MeasureRecordReaderFormatHits::read_bit() {
    if (bits_returned >= max_bits) throw std::out_of_range("Attempt to read past end-of-file");
    if (bits_returned > next_hit) update_next_hit();

    bool b = bits_returned == next_hit;
    ++bits_returned;
    return b;
}

bool MeasureRecordReaderFormatHits::is_end_of_file() {
    return bits_returned >= max_bits;
}

void MeasureRecordReaderFormatHits::update_next_hit() {
    int status = fscanf(in, "%zu,", &next_hit);
    if (status == EOF) return;
    if (status != 1) throw std::runtime_error("Failed to parse input");
    if (next_hit < bits_returned) {
        throw std::invalid_argument("New hit " + std::to_string(next_hit) + " is in the past of " + std::to_string(bits_returned));
    }
}

/// R8 format

MeasureRecordReaderFormatR8::MeasureRecordReaderFormatR8(FILE *in, size_t max_bits) : in(in), max_bits(max_bits) {
    update_run_length();
    run_length_1s = 0;
}

size_t MeasureRecordReaderFormatR8::read_bytes(PointerRange<uint8_t> data) {
    if (bits_returned >= max_bits) return 0;
    size_t n = 0;
    for (uint8_t &b : data) {
        if (run_length_0s >= generated_0s + 8 and max_bits >= bits_returned + 8) {
            b = 0;
            generated_0s += 8;
            bits_returned += 8;
            n += 8;
            continue;
        }
        b = 0;
        for (size_t k = 0; k < 8; k++) {
            if (is_end_of_record()) {
                return n;
            }
            b |= uint8_t(read_bit()) << k;
            ++n;
        }
    }
    return n;
}

bool MeasureRecordReaderFormatR8::read_bit() {
    if (bits_returned >= max_bits) throw std::out_of_range("Attempt to read past end-of-file");
    if (generated_1s < run_length_1s) {
        ++generated_1s;
        ++bits_returned;
        return true;
    }
    if (generated_0s < run_length_0s) {
        ++generated_0s;
        ++bits_returned;
        return false;
    }
    if (!update_run_length()) {
        throw std::out_of_range("Attempt to read past end-of-file");
    } else {
        ++generated_1s;
        ++bits_returned;
        return true;
    }
}

bool MeasureRecordReaderFormatR8::is_end_of_file() {
    if (bits_returned >= max_bits) return true;
    if (generated_0s < run_length_0s) return false;
    if (generated_1s < run_length_1s) return false;
    return !update_run_length();
}

bool MeasureRecordReaderFormatR8::update_run_length() {
    size_t new_run_length_0s = 0;
    int r = getc(in);
    if (r == EOF) return false;
    while (r == 0xFF) {
        new_run_length_0s += 0xFF;
        r = getc(in);
    }
    if (r > 0) new_run_length_0s += r;
    run_length_0s = new_run_length_0s;
    run_length_1s = 1;
    generated_0s = 0;
    generated_1s = 0;
    return true;
}

/// DETS format

MeasureRecordReaderFormatDets::MeasureRecordReaderFormatDets(FILE *in, size_t max_bits) : in(in), max_bits(max_bits) {
    fscanf(in, "shot ");
    read_next_shot();
}

bool MeasureRecordReaderFormatDets::read_bit() {
    if (bits_returned >= max_bits) throw std::out_of_range("Attempt to read past end-of-file");
    if (position > next_shot) read_next_shot();

    bool b = position == next_shot;
    ++bits_returned;
    ++position;
    return b;
}

bool MeasureRecordReaderFormatDets::is_end_of_record() {
    if (position <= next_shot) return false;
    bool eor = bits_returned >= max_bits or separator == '\n';
    if (separator == '\n') {
        fscanf(in, "shot ");
    }
    return eor;
}

bool MeasureRecordReaderFormatDets::is_end_of_file() {
    return bits_returned >= max_bits or feof(in);
}

char MeasureRecordReaderFormatDets::current_result_type() {
    return result_type;
}

void MeasureRecordReaderFormatDets::read_next_shot() {
    char next_result_type;
    int status = fscanf(in, "%c%zu%c", &next_result_type, &next_shot, &separator);
    if (status == EOF) {
        bits_returned = max_bits;
        result_type = next_result_type;
        return;
    }
    if (status != 3) {
        throw std::runtime_error("Failed to parse input");
    }
    if (separator != ' ' and separator != '\n') {
        throw std::invalid_argument("Unexpected separator: [" + std::to_string(separator) + "]");
    }
    if (next_result_type != result_type) {
        position = 0;
        result_type = next_result_type;
    }
    if (next_shot < position) {
        throw std::invalid_argument("New shot " + std::to_string(next_shot) + " is in the past of " + std::to_string(bits_returned));
    }
}
