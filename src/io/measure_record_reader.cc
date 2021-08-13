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

namespace {

// Returns true if keyword is found at current position.
// Returns false if EOF is found at current position.
// Throws otherwise.
bool maybe_consume_keyword(FILE *in, const std::string& keyword, int &next) {
    next = getc(in);
    if (next == EOF) return false;

    size_t i = 0;
    while (i < keyword.size() && next == (int)keyword[i++]) {
        next = getc(in);
    }

    if (i != keyword.size()) throw std::runtime_error("Failed to find expected string \"" + keyword + "\"");

    return true;
}

// Returns true if an integer value is found at current position.
// Returns false otherwise.
bool read_unsigned_int(FILE* in, long &value, int &next) {
    next = getc(in);
    if (!isdigit(next)) return false;

    value = 0;
    while (isdigit(next)) {
        value *= 10;
        value += next - '0';
        next = getc(in);
    }
    return true;
}

}  // namespace

std::unique_ptr<MeasureRecordReader> MeasureRecordReader::make(FILE *in, SampleFormat input_format) {
    switch (input_format) {
        case SAMPLE_FORMAT_01:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormat01(in));
        case SAMPLE_FORMAT_B8:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatB8(in));
        case SAMPLE_FORMAT_DETS:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatDets(in));
        case SAMPLE_FORMAT_HITS:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatHits(in));
        case SAMPLE_FORMAT_PTB64:
            throw std::invalid_argument("SAMPLE_FORMAT_PTB64 incompatible with SingleMeasurementRecord");
        case SAMPLE_FORMAT_R8:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatR8(in));
        default:
            throw std::invalid_argument("Sample format not recognized by SingleMeasurementRecord");
    }
}

size_t MeasureRecordReader::read_bytes(PointerRange<uint8_t> data) {
    size_t n = 0;
    for (uint8_t &b : data) {
        b = 0;
        for (size_t k = 0; k < 8; k++) {
            b |= uint8_t(read_bit()) << k;
            ++n;
            if (is_end_of_record()) {
                return n;
            }
        }
    }
    return n;
}

bool MeasureRecordReader::next_record() {
    return false;
}

bool MeasureRecordReader::is_end_of_record() {
    return false;
}

char MeasureRecordReader::current_result_type() {
    return 'M';
}

/// 01 format

MeasureRecordReaderFormat01::MeasureRecordReaderFormat01(FILE *in) : in(in), payload(getc(in)) {
}

bool MeasureRecordReaderFormat01::read_bit() {
    if (payload == EOF) {
        throw std::out_of_range("Attempt to read past end-of-file");
    }
    if (payload == '\n') {
        throw std::out_of_range("Attempt to read past end-of-record");
    }
    if (payload != '0' && payload != '1') {
        throw std::runtime_error("Character code " + std::to_string(payload) + " does not encode a bit");
    }

    bool bit = payload == '1';
    payload = getc(in);
    return bit;
}

bool MeasureRecordReaderFormat01::next_record() {
    while (payload != EOF && payload != '\n') payload = getc(in);
    payload = getc(in);
    return payload != EOF;
}

bool MeasureRecordReaderFormat01::is_end_of_record() {
    return payload == EOF || payload == '\n';
}

/// B8 format

MeasureRecordReaderFormatB8::MeasureRecordReaderFormatB8(FILE *in) : in(in) {
}

size_t MeasureRecordReaderFormatB8::read_bytes(PointerRange<uint8_t> data) {
    if (bits_available > 0) return MeasureRecordReader::read_bytes(data);

    size_t k = data.ptr_end - data.ptr_start;
    k = 8 * fread(data.ptr_start, sizeof(uint8_t), k, in);
    return k;
}

bool MeasureRecordReaderFormatB8::read_bit() {
    maybe_update_payload();

    if (payload == EOF) {
        throw std::out_of_range("Attempt to read past end-of-file");
    }

    bool b = payload & 1;
    payload >>= 1;
    --bits_available;
    return b;
}

bool MeasureRecordReaderFormatB8::is_end_of_record() {
    maybe_update_payload();
    return bits_available == 0 && payload == EOF;
}

void MeasureRecordReaderFormatB8::maybe_update_payload() {
    if (bits_available > 0) return;
    payload = getc(in);
    if (payload != EOF) bits_available = 8;
}

/// Hits format

MeasureRecordReaderFormatHits::MeasureRecordReaderFormatHits(FILE *in) : in(in) {
    update_next_hit();
}

bool MeasureRecordReaderFormatHits::read_bit() {
    if (position > next_hit && separator ==',') update_next_hit();
    return next_hit == position++;
}

bool MeasureRecordReaderFormatHits::next_record() {
    while (separator == ',') update_next_hit();
    next_hit = -1;
    position = 0;
    update_next_hit();
    return separator != EOF;
}

void MeasureRecordReaderFormatHits::update_next_hit() {
    if(!read_unsigned_int(in, next_hit, separator)) {
        if (separator != '\n' && separator != EOF) {
            throw std::runtime_error("Unexpected character " + std::to_string(separator));
        }
        return;
    }
    if (separator != ',' && separator != '\n') {
        throw std::runtime_error("Invalid separator character " + std::to_string(separator));
    }
    if (next_hit < position) {
        throw std::runtime_error("New hit " + std::to_string(next_hit) + " is in the past of " + std::to_string(position));
    }
}

/// R8 format

MeasureRecordReaderFormatR8::MeasureRecordReaderFormatR8(FILE *in) : in(in) {
    update_run_length();
    run_length_1s = 0;
}

size_t MeasureRecordReaderFormatR8::read_bytes(PointerRange<uint8_t> data) {
    size_t n = 0;
    for (uint8_t &b : data) {
        if (run_length_0s >= generated_0s + 8) {
            b = 0;
            generated_0s += 8;
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
    if (generated_1s < run_length_1s) {
        ++generated_1s;
        return true;
    }
    if (generated_0s < run_length_0s) {
        ++generated_0s;
        return false;
    }
    if (!update_run_length()) {
        throw std::out_of_range("Attempt to read past end-of-file");
    } else {
        ++generated_1s;
        return true;
    }
}

bool MeasureRecordReaderFormatR8::is_end_of_record() {
    if (generated_0s < run_length_0s) return false;
    if (generated_1s < run_length_1s) return false;
    return !update_run_length();
}

bool MeasureRecordReaderFormatR8::update_run_length() {
    int r = getc(in);
    if (r == EOF) return false;
    run_length_0s = 0;
    while (r == 0xFF) {
        run_length_0s += 0xFF;
        r = getc(in);
    }
    if (r > 0) run_length_0s += r;
    run_length_1s = 1;
    generated_0s = 0;
    generated_1s = 0;
    return true;
}

/// DETS format

MeasureRecordReaderFormatDets::MeasureRecordReaderFormatDets(FILE *in) : in(in) {
    if (!maybe_consume_keyword(in, "shot", separator)) {
        throw std::runtime_error("Need a \"shot\" to begin record");
    }
    update_next_shot();
}

bool MeasureRecordReaderFormatDets::read_bit() {
    if (position > next_shot && separator == ' ') update_next_shot();
    return next_shot == position++;
}

bool MeasureRecordReaderFormatDets::next_record() {
    while (separator == ' ') update_next_shot();
    if (!maybe_consume_keyword(in, "shot", separator)) return false;
    next_shot = -1;
    position = 0;
    update_next_shot();
    return true;
}

char MeasureRecordReaderFormatDets::current_result_type() {
    return result_type;
}

void MeasureRecordReaderFormatDets::update_next_shot() {
    // Read and validate result type.
    char next_result_type = getc(in);
    if (next_result_type == EOF) {
        separator = EOF;
        return;
    }
    if (next_result_type != 'M' && next_result_type != 'D' && next_result_type != 'L') {
        throw std::runtime_error("Unknown result type " + std::to_string(next_result_type) + ", expected M, D or L");
    }

    // Read and validate shot number and separator.
    if (!read_unsigned_int(in, next_shot, separator)) {
        throw std::runtime_error("Failed to parse input");
    }
    if (separator != ' ' && separator != '\n') {
        throw std::invalid_argument("Unexpected separator: [" + std::to_string(separator) + "]");
    }
    if (next_result_type != result_type) {
        position = 0;
        result_type = next_result_type;
    }
    if (next_shot < position) {
        throw std::invalid_argument("New shot " + std::to_string(next_shot) + " is in the past of " + std::to_string(position));
    }
}
