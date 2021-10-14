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

bool stim::read_uint64(FILE *in, uint64_t &value, int &next, bool include_next) {
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

MeasureRecordReader::MeasureRecordReader(size_t num_measurements, size_t num_detectors, size_t num_observables)
    : num_measurements(num_measurements), num_detectors(num_detectors), num_observables(num_observables) {
}

size_t MeasureRecordReader::read_records_into(simd_bit_table &out, bool major_index_is_shot_index, size_t max_shots) {
    if (!major_index_is_shot_index) {
        simd_bit_table buf(out.num_minor_bits_padded(), out.num_major_bits_padded());
        size_t r = read_records_into(buf, true, max_shots);
        buf.transpose_into(out);
        return r;
    }

    size_t num_read = 0;
    max_shots = std::min(max_shots, out.num_major_bits_padded());
    while (num_read < max_shots && start_and_read_entire_record(out[num_read])) {
        num_read++;
    }
    return num_read;
}

std::unique_ptr<MeasureRecordReader> MeasureRecordReader::make(
    FILE *in, SampleFormat input_format, size_t num_measurements, size_t num_detectors, size_t num_observables) {
    switch (input_format) {
        case SAMPLE_FORMAT_01:
            return std::unique_ptr<MeasureRecordReader>(
                new MeasureRecordReaderFormat01(in, num_measurements, num_detectors, num_observables));
        case SAMPLE_FORMAT_B8:
            return std::unique_ptr<MeasureRecordReader>(
                new MeasureRecordReaderFormatB8(in, num_measurements, num_detectors, num_observables));
        case SAMPLE_FORMAT_DETS:
            return std::unique_ptr<MeasureRecordReader>(
                new MeasureRecordReaderFormatDets(in, num_measurements, num_detectors, num_observables));
        case SAMPLE_FORMAT_HITS:
            return std::unique_ptr<MeasureRecordReader>(
                new MeasureRecordReaderFormatHits(in, num_measurements, num_detectors, num_observables));
        case SAMPLE_FORMAT_PTB64:
            throw std::invalid_argument("SAMPLE_FORMAT_PTB64 incompatible with SingleMeasurementRecord");
        case SAMPLE_FORMAT_R8:
            return std::unique_ptr<MeasureRecordReader>(
                new MeasureRecordReaderFormatR8(in, num_measurements, num_detectors, num_observables));
        default:
            throw std::invalid_argument("Sample format not recognized by SingleMeasurementRecord");
    }
}

size_t MeasureRecordReader::read_bits_into_bytes(PointerRange<uint8_t> out_buffer) {
    size_t n = 0;
    for (uint8_t &b : out_buffer) {
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

size_t MeasureRecordReader::bits_per_record() const {
    return num_measurements + num_detectors + num_observables;
}

void MeasureRecordReader::move_obs_in_shots_to_mask_assuming_sorted(SparseShot &shot) {
    if (num_observables > 32) {
        throw std::invalid_argument("More than 32 observables. Can't read into SparseShot struct.");
    }

    size_t nd = num_measurements + num_detectors;
    size_t n = nd + num_observables;
    shot.obs_mask = 0;
    while (!shot.hits.empty()) {
        auto top = shot.hits.back();
        if (top < nd) {
            break;
        }
        if (top >= n) {
            throw std::invalid_argument("Hit index from data is too large.");
        }
        shot.hits.pop_back();
        shot.obs_mask ^= 1 << (top - nd);
    }
}

/// 01 format

MeasureRecordReaderFormat01::MeasureRecordReaderFormat01(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader(num_measurements, num_detectors, num_observables),
      in(in),
      payload('\n'),
      position(bits_per_record()) {
}

bool MeasureRecordReaderFormat01::start_and_read_entire_record(simd_bits_range_ref dirty_out_buffer) {
    return start_and_read_entire_record_helper(
        [&](size_t k) {
            dirty_out_buffer[k] = false;
        },
        [&](size_t k) {
            dirty_out_buffer[k] = true;
        });
}

bool MeasureRecordReaderFormat01::start_and_read_entire_record(SparseShot &cleared_out) {
    bool result = start_and_read_entire_record_helper(
        [&](size_t k) {
        },
        [&](size_t k) {
            cleared_out.hits.push_back((uint64_t)k);
        });
    move_obs_in_shots_to_mask_assuming_sorted(cleared_out);
    return result;
}

bool MeasureRecordReaderFormat01::read_bit() {
    if (payload == EOF) {
        throw std::out_of_range("Attempt to read past end-of-file");
    }
    if (payload == '\n' || position >= bits_per_record()) {
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
        if (position++ > bits_per_record()) {
            throw std::runtime_error(
                "Line was too long for input file in 01 format. Expected " + std::to_string(bits_per_record()) +
                " characters but got " + std::to_string(position));
        }
    }

    return start_record();
}

bool MeasureRecordReaderFormat01::start_record() {
    payload = getc(in);
    position = 0;
    return payload != EOF;
}

bool MeasureRecordReaderFormat01::is_end_of_record() {
    bool payload_ended = (payload == EOF || payload == '\n');
    bool expected_end = position >= bits_per_record();
    if (payload_ended && !expected_end) {
        throw std::invalid_argument("Record data (in 01 format) ended early, before expected length.");
    }
    if (!payload_ended && expected_end) {
        throw std::invalid_argument("Record data (in 01 format) did not end by the expected length.");
    }
    return payload_ended;
}

/// B8 format

MeasureRecordReaderFormatB8::MeasureRecordReaderFormatB8(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader(num_measurements, num_detectors, num_observables),
      in(in),
      payload(0),
      bits_available(0),
      position(bits_per_record()) {
}

bool MeasureRecordReaderFormatB8::start_and_read_entire_record(simd_bits_range_ref dirty_out_buffer) {
    return start_and_read_entire_record_helper([&](size_t byte_index, uint8_t byte) {
        dirty_out_buffer.u8[byte_index] = (uint8_t)byte;
    });
}

bool MeasureRecordReaderFormatB8::start_and_read_entire_record(SparseShot &cleared_out) {
    bool result = start_and_read_entire_record_helper([&](size_t byte_index, uint8_t byte) {
        size_t bit_offset = byte_index << 3;
        for (size_t r = 0; r < 8; r++) {
            if (byte & (1 << r)) {
                cleared_out.hits.push_back(bit_offset + r);
            }
        }
    });
    move_obs_in_shots_to_mask_assuming_sorted(cleared_out);
    return result;
}

size_t MeasureRecordReaderFormatB8::read_bits_into_bytes(PointerRange<uint8_t> out_buffer) {
    if (out_buffer.empty()) {
        return 0;
    }
    if (position >= bits_per_record()) {
        return 0;
    }

    if (bits_available & 7) {
        // Partial read from before still had trailing bits.
        return MeasureRecordReader::read_bits_into_bytes(out_buffer);
    }

    size_t total_read = 0;
    if (bits_available) {
        *out_buffer.ptr_start = payload & 0xFF;
        out_buffer.ptr_start++;
        bits_available = 0;
        position += 8;
        total_read += 8;
    }

    size_t n_bits = std::min<size_t>(8 * out_buffer.size(), bits_per_record() - position);
    size_t n_bytes = (n_bits + 7) / 8;
    n_bytes = fread(out_buffer.ptr_start, sizeof(uint8_t), n_bytes, in);
    n_bits = std::min<size_t>(8 * n_bytes, n_bits);
    position += n_bits;
    total_read += n_bits;

    return total_read;
}

bool MeasureRecordReaderFormatB8::read_bit() {
    if (position >= bits_per_record()) {
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
    return start_record();
}

bool MeasureRecordReaderFormatB8::start_record() {
    position = 0;
    bits_available = 0;
    payload = 0;
    maybe_update_payload();
    return payload != EOF;
}

bool MeasureRecordReaderFormatB8::is_end_of_record() {
    return position >= bits_per_record();
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

MeasureRecordReaderFormatHits::MeasureRecordReaderFormatHits(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader(num_measurements, num_detectors, num_observables),
      in(in),
      buffer(bits_per_record()),
      position_in_buffer(bits_per_record()) {
}

bool MeasureRecordReaderFormatHits::start_and_read_entire_record(simd_bits_range_ref dirty_out_buffer) {
    dirty_out_buffer.prefix_ref(bits_per_record()).clear();
    return start_and_read_entire_record_helper([&](size_t bit_index) {
        dirty_out_buffer[bit_index] = true;
    });
}

bool MeasureRecordReaderFormatHits::start_and_read_entire_record(SparseShot &cleared_out) {
    size_t m = bits_per_record();
    size_t nmd = num_measurements + num_detectors;
    return start_and_read_entire_record_helper([&](size_t bit_index) {
        if (bit_index >= m) {
            throw std::invalid_argument("hit index is too large.");
        }
        if (bit_index < nmd) {
            cleared_out.hits.push_back(bit_index);
        } else {
            cleared_out.obs_mask ^= 1 << (bit_index - nmd);
        }
    });
}

bool MeasureRecordReaderFormatHits::read_bit() {
    if (position_in_buffer >= bits_per_record()) {
        throw std::invalid_argument("Read past end of buffer.");
    }
    return buffer[position_in_buffer++];
}

bool MeasureRecordReaderFormatHits::next_record() {
    return start_record();
}

bool MeasureRecordReaderFormatHits::start_record() {
    int c = getc(in);
    if (c == EOF) {
        return false;
    }
    buffer.clear();
    position_in_buffer = 0;
    bool is_first = true;
    while (c != '\n') {
        uint64_t value;
        if (!read_uint64(in, value, c, is_first)) {
            throw std::runtime_error(
                "Integer didn't start immediately at start of line or after comma in 'hits' format.");
        }
        if (c != ',' && c != '\n') {
            throw std::runtime_error(
                "'hits' format requires  integers to be followed by a comma or newline, but got a '" +
                std::to_string(c) + "'.");
        }
        if (value >= bits_per_record()) {
            throw std::runtime_error(
                "Bits per record is " + std::to_string(bits_per_record()) + " but got a hit value " +
                std::to_string(value) + ".");
        }
        buffer[value] ^= true;
        is_first = false;
    }
    return true;
}

bool MeasureRecordReaderFormatHits::is_end_of_record() {
    return position_in_buffer >= bits_per_record();
}

/// R8 format

MeasureRecordReaderFormatR8::MeasureRecordReaderFormatR8(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader(num_measurements, num_detectors, num_observables), in(in) {
}

size_t MeasureRecordReaderFormatR8::read_bits_into_bytes(PointerRange<uint8_t> out_buffer) {
    size_t n = 0;
    for (uint8_t &b : out_buffer) {
        b = 0;
        if (buffered_0s >= 8) {
            position += 8;
            buffered_0s -= 8;
            n += 8;
            continue;
        }
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
    if (!buffered_0s && !buffered_1s) {
        bool read_any = maybe_buffer_data();
        assert(read_any);
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

bool MeasureRecordReaderFormatR8::start_and_read_entire_record(simd_bits_range_ref dirty_out_buffer) {
    dirty_out_buffer.prefix_ref(bits_per_record()).clear();
    return start_and_read_entire_record_helper([&](size_t bit_index) {
        dirty_out_buffer[bit_index] = 1;
    });
}

bool MeasureRecordReaderFormatR8::start_and_read_entire_record(SparseShot &cleared_out) {
    bool result = start_and_read_entire_record_helper([&](size_t bit_index) {
        cleared_out.hits.push_back(bit_index);
    });
    move_obs_in_shots_to_mask_assuming_sorted(cleared_out);
    return result;
}
bool MeasureRecordReaderFormatR8::next_record() {
    while (!is_end_of_record()) {
        read_bit();
    }
    return start_record();
}

bool MeasureRecordReaderFormatR8::start_record() {
    position = 0;
    have_seen_terminal_1 = false;
    return maybe_buffer_data();
}

bool MeasureRecordReaderFormatR8::is_end_of_record() {
    return position == bits_per_record() && have_seen_terminal_1;
}

bool MeasureRecordReaderFormatR8::maybe_buffer_data() {
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
            if (buffered_0s == 0 && position == 0) {
                return false;  // No next record.
            }
            throw std::invalid_argument("r8 data ended on a continuation (a 0xFF byte) which is not allowed.");
        }
        buffered_0s += r;
    } while (r == 0xFF);
    buffered_1s = 1;

    // Check if the 1 is the one at end of data.
    size_t total_data = position + buffered_0s + buffered_1s;
    if (total_data == bits_per_record()) {
        int t = getc(in);
        if (t == EOF) {
            throw std::invalid_argument(
                "r8 data ended too early. "
                "The extracted data ended in a 1, but there was no corresponding 0x00 terminator byte for the expected "
                "'fake encoded 1 just after the end of the data' before the input ended.");
        } else if (t != 0) {
            throw std::invalid_argument(
                "r8 data ended too early. "
                "The extracted data ended in a 1, but there was no corresponding 0x00 terminator byte for the expected "
                "'fake encoded 1 just after the end of the data' before any additional data.");
        }
        have_seen_terminal_1 = true;
    } else if (total_data == bits_per_record() + 1) {
        have_seen_terminal_1 = true;
        buffered_1s = 0;
    } else if (total_data > bits_per_record() + 1) {
        throw std::invalid_argument("r8 data encoded a jump past the expected end of encoded data.");
    }
    return true;
}

/// DETS format

bool MeasureRecordReaderFormatDets::start_and_read_entire_record(simd_bits_range_ref dirty_out_buffer) {
    dirty_out_buffer.prefix_ref(bits_per_record()).clear();
    return start_and_read_entire_record_helper([&](size_t bit_index) {
        dirty_out_buffer[bit_index] = true;
    });
}

bool MeasureRecordReaderFormatDets::start_and_read_entire_record(SparseShot &cleared_out) {
    size_t obs_start = num_measurements + num_detectors;
    return start_and_read_entire_record_helper([&](size_t bit_index) {
        if (bit_index < obs_start) {
            cleared_out.hits.push_back(bit_index);
        } else {
            cleared_out.obs_mask ^= 1 << (bit_index - obs_start);
        }
    });
}

MeasureRecordReaderFormatDets::MeasureRecordReaderFormatDets(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader(num_measurements, num_detectors, num_observables),
      in(in),
      buffer(num_measurements + num_detectors + num_observables),
      position_in_buffer(num_measurements + num_detectors + num_observables) {
}

bool MeasureRecordReaderFormatDets::read_bit() {
    if (position_in_buffer >= bits_per_record()) {
        throw std::invalid_argument("Read past end of buffer.");
    }
    return buffer[position_in_buffer++];
}

bool MeasureRecordReaderFormatDets::next_record() {
    return start_record();
}

bool MeasureRecordReaderFormatDets::start_record() {
    int c;
    if (!maybe_consume_keyword(in, "shot", c)) {
        return false;
    }
    buffer.clear();
    position_in_buffer = 0;
    while (true) {
        bool had_spacing = c == ' ';
        while (c == ' ') {
            c = getc(in);
        }
        if (c == '\n' || c == EOF) {
            break;
        }
        if (!had_spacing) {
            throw std::invalid_argument("DETS values must be separated by spaces.");
        }
        size_t offset;
        size_t size;
        char prefix = c;
        if (prefix == 'M') {
            offset = 0;
            size = num_measurements;
        } else if (prefix == 'D') {
            offset = num_measurements;
            size = num_detectors;
        } else if (prefix == 'L') {
            offset = num_measurements + num_detectors;
            size = num_observables;
        } else {
            throw std::invalid_argument("Unrecognized DETS prefix: '" + std::to_string(c) + "'");
        }
        uint64_t number;
        if (!read_uint64(in, number, c)) {
            throw std::invalid_argument("DETS prefix '" + std::to_string(c) + "' wasn't not followed by an integer.");
        }
        if (number >= size) {
            throw std::invalid_argument(
                "Got '" + std::to_string(c) + std::to_string(number) + "' but expected num values of that type is " +
                std::to_string(size) + ".");
        }
        buffer[offset + number] ^= true;
    }
    return true;
}

bool MeasureRecordReaderFormatDets::is_end_of_record() {
    return position_in_buffer == bits_per_record();
}
