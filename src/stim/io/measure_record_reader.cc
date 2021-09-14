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

#include "stim/io/measure_record_reader.h"

#include <algorithm>

using namespace stim;

// Returns true if keyword is found at current position. Returns false if EOF is found at current
// position. Throws otherwise. Uses next output variable for the next character or EOF.
bool maybe_consume_keyword(FILE *in, const std::string &keyword, int &next) {
    next = getc(in);
    if (next == EOF) {
        return false;
    }

    for (char c : keyword) {
        if (c != next) {
            throw std::runtime_error("Failed to find expected string \"" + keyword + "\"");
        }
        next = getc(in);
    }

    return true;
}

// Returns true if an integer value is found at current position. Returns false otherwise.
// Uses two output variables: value to return the integer value read and next for the next
// character or EOF.
bool read_uint64(FILE *in, uint64_t &value, int &next) {
    next = getc(in);
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

size_t MeasureRecordReader::read_records_into(simd_bit_table &out, bool major_index_is_shot_index, size_t max_shots) {
    if (!major_index_is_shot_index) {
        simd_bit_table buf(out.num_minor_bits_padded(), out.num_major_bits_padded());
        size_t r = read_records_into(buf, true, max_shots);
        buf.transpose_into(out);
        return r;
    }

    size_t rec = 0;
    max_shots = std::min(max_shots, out.num_major_bits_padded());
    while (rec < max_shots) {
        size_t n = read_bytes({out[rec].u8, out[rec].u8 + out[rec].num_u8_padded()});
        if (n == 0) {
            break;
        }
        if (!is_end_of_record()) {
            throw std::invalid_argument("Failed to read data. A shot contained more bits than expected.");
        }
        rec++;
        if (!next_record()) {
            break;
        }
    }
    return rec;
}

std::unique_ptr<MeasureRecordReader> MeasureRecordReader::make(
    FILE *in,
    SampleFormat input_format,
    size_t n_measurements,
    size_t n_detection_events,
    size_t n_logical_observables) {
    if (input_format != SAMPLE_FORMAT_DETS && n_detection_events != 0) {
        throw std::invalid_argument("Only DETS format support detection event records");
    }
    if (input_format != SAMPLE_FORMAT_DETS && n_logical_observables != 0) {
        throw std::invalid_argument("Only DETS format support logical observable records");
    }

    switch (input_format) {
        case SAMPLE_FORMAT_01:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormat01(in, n_measurements));
        case SAMPLE_FORMAT_B8:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatB8(in, n_measurements));
        case SAMPLE_FORMAT_DETS:
            return std::unique_ptr<MeasureRecordReader>(
                new MeasureRecordReaderFormatDets(in, n_measurements, n_detection_events, n_logical_observables));
        case SAMPLE_FORMAT_HITS:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatHits(in, n_measurements));
        case SAMPLE_FORMAT_PTB64:
            throw std::invalid_argument("SAMPLE_FORMAT_PTB64 incompatible with SingleMeasurementRecord");
        case SAMPLE_FORMAT_R8:
            return std::unique_ptr<MeasureRecordReader>(new MeasureRecordReaderFormatR8(in, n_measurements));
        default:
            throw std::invalid_argument("Sample format not recognized by SingleMeasurementRecord");
    }
}

size_t MeasureRecordReader::read_bytes(PointerRange<uint8_t> data) {
    if (is_end_of_record()) {
        return 0;
    }
    char result_type = current_result_type();
    size_t n = 0;
    for (uint8_t &b : data) {
        b = 0;
        for (size_t k = 0; k < 8; k++) {
            b |= uint8_t(read_bit()) << k;
            ++n;
            if (is_end_of_record() || current_result_type() != result_type) {
                return n;
            }
        }
    }
    return n;
}

char MeasureRecordReader::current_result_type() {
    return 'M';
}

/// 01 format

MeasureRecordReaderFormat01::MeasureRecordReaderFormat01(FILE *in, size_t bits_per_record)
    : in(in), payload(getc(in)), bits_per_record(bits_per_record) {
}

bool MeasureRecordReaderFormat01::read_bit() {
    if (payload == EOF) {
        throw std::out_of_range("Attempt to read past end-of-file");
    }
    if (payload == '\n' || position >= bits_per_record) {
        throw std::out_of_range("Attempt to read past end-of-record");
    }
    if (payload != '0' && payload != '1') {
        throw std::runtime_error("Expected '0' or '1' because input format was specified as '01'");
    }

    bool bit = payload == '1';
    payload = getc(in);
    ++position;
    return bit;
}

bool MeasureRecordReaderFormat01::next_record() {
    while (payload != EOF && payload != '\n') {
        payload = getc(in);
        if (position++ > bits_per_record) {
            throw std::runtime_error(
                "Line was too long for input file in 01 format. Expected " + std::to_string(bits_per_record) +
                " characters but got " + std::to_string(position));
        }
    }

    position = 0;
    payload = getc(in);
    return payload != EOF;
}

bool MeasureRecordReaderFormat01::is_end_of_record() {
    return payload == EOF || payload == '\n' || position >= bits_per_record;
}

/// B8 format

MeasureRecordReaderFormatB8::MeasureRecordReaderFormatB8(FILE *in, size_t bits_per_record)
    : in(in), bits_per_record(bits_per_record) {
}

size_t MeasureRecordReaderFormatB8::read_bytes(PointerRange<uint8_t> data) {
    if (position >= bits_per_record) {
        return 0;
    }

    if (bits_available > 0) {
        return MeasureRecordReader::read_bytes(data);
    }

    size_t n_bits = std::min<size_t>(8 * data.size(), bits_per_record - position);
    size_t n_bytes = (n_bits + 7) / 8;
    n_bytes = fread(data.ptr_start, sizeof(uint8_t), n_bytes, in);
    n_bits = std::min<size_t>(8 * n_bytes, n_bits);
    position += n_bits;
    return n_bits;
}

bool MeasureRecordReaderFormatB8::read_bit() {
    if (position >= bits_per_record) {
        throw std::out_of_range("Attempt to read past end-of-record");
    }

    maybe_update_payload();

    if (payload == EOF) {
        throw std::out_of_range("Attempt to read past end-of-file");
    }

    bool b = payload & 1;
    payload >>= 1;
    --bits_available;
    ++position;
    return b;
}

bool MeasureRecordReaderFormatB8::next_record() {
    while (!is_end_of_record()) {
        read_bit();
    }
    position = 0;
    int i = fgetc(in);
    if (i == EOF) {
        return false;
    }
    ungetc(i, in);
    return true;
}

bool MeasureRecordReaderFormatB8::is_end_of_record() {
    maybe_update_payload();
    return (bits_available == 0 && payload == EOF) || position >= bits_per_record;
}

void MeasureRecordReaderFormatB8::maybe_update_payload() {
    if (bits_available > 0) {
        return;
    }
    payload = getc(in);
    if (payload != EOF) {
        bits_available = 8;
    }
}

/// Hits format

MeasureRecordReaderFormatHits::MeasureRecordReaderFormatHits(FILE *in, size_t bits_per_record)
    : in(in), bits_per_record(bits_per_record) {
    update_next_hit();
}

bool MeasureRecordReaderFormatHits::read_bit() {
    if (position >= bits_per_record) {
        throw std::out_of_range("Attempt to read past end-of-record");
    }
    if ((no_next_hit || position > next_hit) && separator == ',') {
        update_next_hit();
    }
    if (no_next_hit) {
        position++;
        return false;
    }
    return next_hit == position++;
}

bool MeasureRecordReaderFormatHits::next_record() {
    while (separator == ',') {
        update_next_hit();
    }
    no_next_hit = true;
    next_hit = 0;
    position = 0;
    update_next_hit();
    return separator != EOF;
}

bool MeasureRecordReaderFormatHits::is_end_of_record() {
    return position >= bits_per_record;
}

void MeasureRecordReaderFormatHits::update_next_hit() {
    no_next_hit = true;
    if (!read_uint64(in, next_hit, separator)) {
        if (separator != '\n' && separator != EOF) {
            throw std::runtime_error("Unexpected character " + std::to_string(separator));
        }
        return;
    }
    no_next_hit = false;
    if (separator != ',' && separator != '\n') {
        throw std::runtime_error("Invalid separator character " + std::to_string(separator));
    }
    if (next_hit < position) {
        throw std::runtime_error(
            "New hit " + std::to_string(next_hit) + " is in the past of " + std::to_string(position));
    }
    if (next_hit >= bits_per_record) {
        throw std::runtime_error(
            "New hit " + std::to_string(next_hit) + " is outside record size " + std::to_string(bits_per_record));
    }
}

/// R8 format

MeasureRecordReaderFormatR8::MeasureRecordReaderFormatR8(FILE *in, size_t bits_per_record)
    : in(in), bits_per_record(bits_per_record) {
}

size_t MeasureRecordReaderFormatR8::read_bytes(PointerRange<uint8_t> data) {
    size_t n = 0;
    for (uint8_t &b : data) {
        b = 0;
        if (buffered_0s >= 8) {
            position += 8;
            buffered_0s -= 8;
            n += 8;
            continue;
        }
        for (size_t k = 0; k < 8; k++) {
            if (!buffered_0s && !buffered_1s && !have_seen_terminal_1) {
                buffer_data();
            }
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
    if (!buffered_0s && !buffered_1s) {
        buffer_data();
    }
    if (buffered_0s) {
        buffered_0s--;
        position++;
        return false;
    } else if (buffered_1s) {
        buffered_1s--;
        position++;
        return true;
    } else {
        throw std::invalid_argument("Read past end-of-record.");
    }
}

bool MeasureRecordReaderFormatR8::next_record() {
    while (!is_end_of_record()) {
        read_bit();
    }
    position = 0;
    have_seen_terminal_1 = false;
    int i = fgetc(in);
    if (i == EOF) {
        return false;
    }
    ungetc(i, in);
    return true;
}

bool MeasureRecordReaderFormatR8::is_end_of_record() {
    return position == bits_per_record && have_seen_terminal_1;
}

void MeasureRecordReaderFormatR8::buffer_data() {
    assert(buffered_0s == 0);
    assert(buffered_1s == 0);
    if (is_end_of_record()) {
        throw std::invalid_argument("Attempted to read past end-of-record.");
    }

    // Count zeroes until a one is found.
    int r;
    do {
        r = getc(in);
        if (r == EOF) {
            throw std::invalid_argument("r8 data hit end-of-file without seeing a 1 encoded at end of data.");
        }
        buffered_0s += r;
    } while (r == 0xFF);
    buffered_1s = 1;

    // Check if the 1 is the one at end of data.
    size_t total_data = position + buffered_0s + buffered_1s;
    if (total_data == bits_per_record) {
        if (getc(in) != 0) {
            throw std::invalid_argument("r8 data ending in an encoded 1 didn't include a 0x00 byte encoding the extra 1 at end of data.");
        }
        have_seen_terminal_1 = true;
    } else if (total_data == bits_per_record + 1) {
        have_seen_terminal_1 = true;
        buffered_1s = 0;
    } else if (total_data > bits_per_record + 1) {
        throw std::invalid_argument("r8 data encoded a jump past the expected end of encoded data.");
    }
}

/// DETS format

MeasureRecordReaderFormatDets::MeasureRecordReaderFormatDets(
    FILE *in, size_t n_measurements, size_t n_detection_events, size_t n_logical_observables)
    : in(in),
      bits_per_m_record(n_measurements),
      bits_per_d_record(n_detection_events),
      bits_per_l_record(n_logical_observables) {
    if (!maybe_consume_keyword(in, "shot", separator)) {
        throw std::runtime_error("Need a \"shot\" to begin record");
    }
    update_next_shot();
    result_type = next_shot_result_type;
}

bool MeasureRecordReaderFormatDets::read_bit() {
    if (position_m >= bits_per_m_record && position_d >= bits_per_d_record && position_l >= bits_per_l_record) {
        throw std::out_of_range("Attempt to read past end-of-record");
    }

    if (position() >= bits_per_record()) {
        result_type = next_shot_result_type;
    }
    bool r = next_shot == position()++;
    if (no_next_shot) {
        r = false;
    }
    if ((no_next_shot || position() > next_shot) && result_type == next_shot_result_type && separator == ' ') {
        update_next_shot();
    }
    return r;
}

bool MeasureRecordReaderFormatDets::next_record() {
    while (separator == ' ') {
        update_next_shot();
    }
    if (!maybe_consume_keyword(in, "shot", separator)) {
        return false;
    }
    no_next_shot = true;
    next_shot = 0;
    position_m = position_d = position_l = 0;
    update_next_shot();
    return true;
}

bool MeasureRecordReaderFormatDets::is_end_of_record() {
    return position_m >= bits_per_m_record && position_d >= bits_per_d_record && position_l >= bits_per_l_record;
}

char MeasureRecordReaderFormatDets::current_result_type() {
    if (position() >= bits_per_record()) {
        return next_shot_result_type;
    }
    return result_type;
}

uint64_t &MeasureRecordReaderFormatDets::bits_per_record() {
    if (result_type == 'M') {
        return bits_per_m_record;
    }
    if (result_type == 'D') {
        return bits_per_d_record;
    }
    if (result_type == 'L') {
        return bits_per_l_record;
    }
    throw std::runtime_error("Unknown result type " + std::to_string(result_type));
}

uint64_t &MeasureRecordReaderFormatDets::position() {
    if (result_type == 'M') {
        return position_m;
    }
    if (result_type == 'D') {
        return position_d;
    }
    if (result_type == 'L') {
        return position_l;
    }
    throw std::runtime_error("Unknown result type " + std::to_string(result_type));
}

void MeasureRecordReaderFormatDets::update_next_shot() {
    no_next_shot = true;

    // Read and validate result type.
    next_shot_result_type = getc(in);
    if (next_shot_result_type == EOF) {
        separator = EOF;
        return;
    }
    if (next_shot_result_type != 'M' && next_shot_result_type != 'D' && next_shot_result_type != 'L') {
        throw std::runtime_error(
            "Unknown result type " + std::to_string(next_shot_result_type) + ", expected M, D or L");
    }

    // Read and validate shot number and separator.
    if (!read_uint64(in, next_shot, separator)) {
        throw std::runtime_error("Failed to parse input");
    }
    if (separator != ' ' && separator != '\n') {
        throw std::runtime_error("Unexpected separator: [" + std::to_string(separator) + "]");
    }
    no_next_shot = false;

    if (next_shot_result_type != result_type) {
        return;
    }
    std::string shot_name = std::string(1, (char)next_shot_result_type) + std::to_string(next_shot);
    if (next_shot < position()) {
        throw std::runtime_error("New shot " + shot_name + " is in the past of its position");
    }
    if (next_shot >= bits_per_record()) {
        throw std::runtime_error(
            "New shot " + shot_name + " is outside record size " + std::to_string(bits_per_record()));
    }
}
