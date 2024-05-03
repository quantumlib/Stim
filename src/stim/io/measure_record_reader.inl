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

#include <algorithm>

#include "stim/io/measure_record_reader.h"

namespace stim {

template <size_t W>
MeasureRecordReader<W>::MeasureRecordReader(size_t num_measurements, size_t num_detectors, size_t num_observables)
    : num_measurements(num_measurements), num_detectors(num_detectors), num_observables(num_observables) {
}

template <size_t W>
size_t MeasureRecordReader<W>::read_records_into(
    simd_bit_table<W> &out, bool major_index_is_shot_index, size_t max_shots) {
    if (!major_index_is_shot_index) {
        simd_bit_table<W> buf(out.num_minor_bits_padded(), out.num_major_bits_padded());
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

template <size_t W>
std::unique_ptr<MeasureRecordReader<W>> MeasureRecordReader<W>::make(
    FILE *in, SampleFormat input_format, size_t num_measurements, size_t num_detectors, size_t num_observables) {
    switch (input_format) {
        case SampleFormat::SAMPLE_FORMAT_01:
            return std::make_unique<MeasureRecordReaderFormat01<W>>(
                in, num_measurements, num_detectors, num_observables);
        case SampleFormat::SAMPLE_FORMAT_B8:
            return std::make_unique<MeasureRecordReaderFormatB8<W>>(
                in, num_measurements, num_detectors, num_observables);
        case SampleFormat::SAMPLE_FORMAT_DETS:
            return std::make_unique<MeasureRecordReaderFormatDets<W>>(
                in, num_measurements, num_detectors, num_observables);
        case SampleFormat::SAMPLE_FORMAT_HITS:
            return std::make_unique<MeasureRecordReaderFormatHits<W>>(
                in, num_measurements, num_detectors, num_observables);
        case SampleFormat::SAMPLE_FORMAT_PTB64:
            return std::make_unique<MeasureRecordReaderFormatPTB64<W>>(
                in, num_measurements, num_detectors, num_observables);
        case SampleFormat::SAMPLE_FORMAT_R8:
            return std::make_unique<MeasureRecordReaderFormatR8<W>>(
                in, num_measurements, num_detectors, num_observables);
        default:
            throw std::invalid_argument("Sample format not recognized by MeasurementRecordReader");
    }
}

template <size_t W>
size_t MeasureRecordReader<W>::bits_per_record() const {
    return num_measurements + num_detectors + num_observables;
}

template <size_t W>
void MeasureRecordReader<W>::move_obs_in_shots_to_mask_assuming_sorted(SparseShot &shot) {
    if (num_observables > 32) {
        throw std::invalid_argument("More than 32 observables. Can't read into SparseShot struct.");
    }

    size_t nd = num_measurements + num_detectors;
    size_t n = nd + num_observables;
    shot.obs_mask.clear();
    while (!shot.hits.empty()) {
        auto top = shot.hits.back();
        if (top < nd) {
            break;
        }
        if (top >= n) {
            throw std::invalid_argument("Hit index from data is too large.");
        }
        shot.hits.pop_back();
        shot.obs_mask[top - nd] ^= true;
    }
}

template <size_t W>
size_t MeasureRecordReader<W>::read_into_table_with_major_shot_index(simd_bit_table<W> &out_table, size_t max_shots) {
    size_t read_shots = 0;
    while (read_shots < max_shots && start_and_read_entire_record(out_table[read_shots])) {
        read_shots++;
    }
    return read_shots;
}

/// 01 format

template <size_t W>
MeasureRecordReaderFormat01<W>::MeasureRecordReaderFormat01(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader<W>(num_measurements, num_detectors, num_observables), in(in) {
}

template <size_t W>
bool MeasureRecordReaderFormat01<W>::start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) {
    return start_and_read_entire_record_helper(
        [&](size_t k) {
            dirty_out_buffer[k] = false;
        },
        [&](size_t k) {
            dirty_out_buffer[k] = true;
        });
}

template <size_t W>
bool MeasureRecordReaderFormat01<W>::start_and_read_entire_record(SparseShot &cleared_out) {
    if (cleared_out.obs_mask.num_bits_padded() < this->num_observables) {
        cleared_out.obs_mask = simd_bits<64>(this->num_observables);
    }
    bool result = start_and_read_entire_record_helper(
        [&](size_t k) {
        },
        [&](size_t k) {
            cleared_out.hits.push_back((uint64_t)k);
        });
    this->move_obs_in_shots_to_mask_assuming_sorted(cleared_out);
    return result;
}

template <size_t W>
bool MeasureRecordReaderFormat01<W>::expects_empty_serialized_data_for_each_shot() const {
    return false;
}

template <size_t W>
size_t MeasureRecordReaderFormat01<W>::read_into_table_with_minor_shot_index(
    simd_bit_table<W> &out_table, size_t max_shots) {
    size_t read_shots = 0;
    while (read_shots < max_shots) {
        bool more = start_and_read_entire_record_helper(
            [&](size_t k) {
                out_table[k][read_shots] &= 0;
            },
            [&](size_t k) {
                out_table[k][read_shots] |= 1;
            });
        if (!more) {
            break;
        }
        read_shots++;
    }
    return read_shots;
}

template <size_t W>
template <typename SAW0, typename SAW1>
bool MeasureRecordReaderFormat01<W>::start_and_read_entire_record_helper(SAW0 saw0, SAW1 saw1) {
    size_t n = this->bits_per_record();
    for (size_t k = 0; k < n; k++) {
        int b = getc(in);
        switch (b) {
            case '0':
                saw0(k);
                break;
            case '1':
                saw1(k);
                break;
            case EOF:
                if (k == 0) {
                    return false;
                }
                [[fallthrough]];
            case '\r':
                [[fallthrough]];
            case '\n':
                throw std::invalid_argument(
                    "01 data ended in middle of record at byte position " + std::to_string(k) +
                    ".\nExpected bits per record was " + std::to_string(n) + ".");
            default:
                throw std::invalid_argument("Unexpected character in 01 format data: '" + std::to_string(b) + "'.");
        }
    }
    int last = getc(in);
    if (n == 0 && last == EOF) {
        return false;
    }
    if (last == '\r') {
        last = getc(in);
    }
    if (last != '\n') {
        throw std::invalid_argument(
            "01 data didn't end with a newline after the expected data length of '" + std::to_string(n) + "'.");
    }
    return true;
}

/// B8 format

template <size_t W>
MeasureRecordReaderFormatB8<W>::MeasureRecordReaderFormatB8(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader<W>(num_measurements, num_detectors, num_observables), in(in) {
}

template <size_t W>
bool MeasureRecordReaderFormatB8<W>::start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) {
    size_t n = this->bits_per_record();
    size_t nb = (n + 7) >> 3;
    size_t nr = fread(dirty_out_buffer.u8, 1, nb, in);
    if (nr == 0) {
        return false;
    }
    if (nr != nb) {
        throw std::invalid_argument(
            "b8 data ended in middle of record at byte position " + std::to_string(nr) +
            ".\n"
            "Expected bytes per record was " +
            std::to_string(nb) + " (" + std::to_string(n) + " bits padded).");
    }
    return true;
}

template <size_t W>
size_t MeasureRecordReaderFormatB8<W>::read_into_table_with_minor_shot_index(
    simd_bit_table<W> &out_table, size_t max_shots) {
    size_t n = this->bits_per_record();
    if (n == 0) {
        return 0;  // Ambiguous when the data ends. Stop as early as possible.
    }
    for (size_t read_shots = 0; read_shots < max_shots; read_shots++) {
        for (size_t bit = 0; bit < n; bit += 8) {
            int c = getc(in);
            if (c == EOF) {
                if (bit == 0) {
                    return read_shots;
                }
                throw std::invalid_argument("b8 data ended in middle of record.");
            }
            for (size_t b = 0; b < 8 && bit + b < n; b++) {
                out_table[bit + b][read_shots] = ((c >> b) & 1) != 0;
            }
        }
    }
    return max_shots;
}

template <size_t W>
bool MeasureRecordReaderFormatB8<W>::start_and_read_entire_record(SparseShot &cleared_out) {
    if (cleared_out.obs_mask.num_bits_padded() < this->num_observables) {
        cleared_out.obs_mask = simd_bits<64>(this->num_observables);
    }
    size_t n = this->bits_per_record();
    size_t nb = (n + 7) >> 3;
    if (n == 0) {
        return 0;  // Ambiguous when the data ends. Stop as early as possible.
    }
    for (size_t k = 0; k < nb; k++) {
        int b = getc(in);
        if (b == EOF) {
            if (k == 0) {
                return false;
            }
            throw std::invalid_argument(
                "b8 data ended in middle of record at byte position " + std::to_string(k) +
                ".\n"
                "Expected bytes per record was " +
                std::to_string(nb) + " (" + std::to_string(n) + " bits padded).");
        }

        size_t bit_offset = k << 3;
        for (size_t r = 0; r < 8; r++) {
            if (b & (1 << r)) {
                cleared_out.hits.push_back(bit_offset + r);
            }
        }
    }
    this->move_obs_in_shots_to_mask_assuming_sorted(cleared_out);
    return true;
}

template <size_t W>
bool MeasureRecordReaderFormatB8<W>::expects_empty_serialized_data_for_each_shot() const {
    return this->bits_per_record() == 0;
}

/// Hits format

template <size_t W>
MeasureRecordReaderFormatHits<W>::MeasureRecordReaderFormatHits(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader<W>(num_measurements, num_detectors, num_observables), in(in) {
}

template <size_t W>
bool MeasureRecordReaderFormatHits<W>::start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) {
    size_t m = this->bits_per_record();
    dirty_out_buffer.prefix_ref(m).clear();
    return start_and_read_entire_record_helper([&](size_t bit_index) {
        if (bit_index >= m) {
            throw std::invalid_argument("hit index is too large.");
        }
        dirty_out_buffer[bit_index] ^= true;
    });
}

template <size_t W>
bool MeasureRecordReaderFormatHits<W>::start_and_read_entire_record(SparseShot &cleared_out) {
    if (cleared_out.obs_mask.num_bits_padded() < this->num_observables) {
        cleared_out.obs_mask = simd_bits<64>(this->num_observables);
    }
    size_t m = this->bits_per_record();
    size_t nmd = this->num_measurements + this->num_detectors;
    return start_and_read_entire_record_helper([&](size_t bit_index) {
        if (bit_index >= m) {
            throw std::invalid_argument("hit index is too large.");
        }
        if (bit_index < nmd) {
            cleared_out.hits.push_back(bit_index);
        } else {
            cleared_out.obs_mask[bit_index - nmd] ^= true;
        }
    });
}

template <size_t W>
bool MeasureRecordReaderFormatHits<W>::expects_empty_serialized_data_for_each_shot() const {
    return false;
}

template <size_t W>
size_t MeasureRecordReaderFormatHits<W>::read_into_table_with_minor_shot_index(
    simd_bit_table<W> &out_table, size_t max_shots) {
    size_t read_shots = 0;
    out_table.clear();
    while (read_shots < max_shots) {
        bool more = start_and_read_entire_record_helper([&](size_t bit_index) {
            out_table[bit_index][read_shots] |= 1;
        });
        if (!more) {
            break;
        }
        read_shots++;
    }
    return read_shots;
}

template <size_t W>
template <typename HANDLE_HIT>
bool MeasureRecordReaderFormatHits<W>::start_and_read_entire_record_helper(HANDLE_HIT handle_hit) {
    bool first = true;
    while (true) {
        int next_char;
        uint64_t value;
        if (!read_uint64(in, value, next_char, false)) {
            if (first && next_char == EOF) {
                return false;
            }
            if (first && next_char == '\r') {
                next_char = getc(in);
            }
            if (first && next_char == '\n') {
                return true;
            }
            throw std::invalid_argument("HITS data wasn't comma-separated integers terminated by a newline.");
        }
        handle_hit((size_t)value);
        first = false;
        if (next_char == '\r') {
            next_char = getc(in);
            if (next_char == '\n') {
                return true;
            }
        } else if (next_char == '\n') {
            return true;
        }
        if (next_char != ',') {
            throw std::invalid_argument("HITS data wasn't comma-separated integers terminated by a newline.");
        }
    }
}

/// R8 format

template <size_t W>
MeasureRecordReaderFormatR8<W>::MeasureRecordReaderFormatR8(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader<W>(num_measurements, num_detectors, num_observables), in(in) {
}

template <size_t W>
bool MeasureRecordReaderFormatR8<W>::start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) {
    dirty_out_buffer.prefix_ref(this->bits_per_record()).clear();
    return start_and_read_entire_record_helper([&](size_t bit_index) {
        dirty_out_buffer[bit_index] = 1;
    });
}

template <size_t W>
bool MeasureRecordReaderFormatR8<W>::start_and_read_entire_record(SparseShot &cleared_out) {
    if (cleared_out.obs_mask.num_bits_padded() < this->num_observables) {
        cleared_out.obs_mask = simd_bits<64>(this->num_observables);
    }
    bool result = start_and_read_entire_record_helper([&](size_t bit_index) {
        cleared_out.hits.push_back(bit_index);
    });
    this->move_obs_in_shots_to_mask_assuming_sorted(cleared_out);
    return result;
}

template <size_t W>
bool MeasureRecordReaderFormatR8<W>::expects_empty_serialized_data_for_each_shot() const {
    return false;
}

template <size_t W>
size_t MeasureRecordReaderFormatR8<W>::read_into_table_with_minor_shot_index(
    simd_bit_table<W> &out_table, size_t max_shots) {
    size_t read_shots = 0;
    out_table.clear();
    while (read_shots < max_shots) {
        bool more = start_and_read_entire_record_helper([&](size_t bit_index) {
            out_table[bit_index][read_shots] |= 1;
        });
        if (!more) {
            break;
        }
        read_shots++;
    }
    return read_shots;
}

template <size_t W>
template <typename HANDLE_HIT>
bool MeasureRecordReaderFormatR8<W>::start_and_read_entire_record_helper(HANDLE_HIT handle_hit) {
    int next_char = getc(in);
    if (next_char == EOF) {
        return false;
    }

    size_t n = this->bits_per_record();
    size_t pos = 0;
    while (true) {
        pos += next_char;
        if (next_char != 255) {
            if (pos < n) {
                handle_hit(pos);
                pos++;
            } else if (pos == n) {
                return true;
            } else {
                throw std::invalid_argument(
                    "r8 data jumped past expected end of encoded data. Expected to decode " +
                    std::to_string(this->bits_per_record()) + " bits.");
            }
        }
        next_char = getc(in);
        if (next_char == EOF) {
            throw std::invalid_argument(
                "End of file before end of r8 data. Expected to decode " + std::to_string(this->bits_per_record()) +
                " bits.");
        }
    }
}

/// DETS format

template <size_t W>
bool MeasureRecordReaderFormatDets<W>::start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) {
    dirty_out_buffer.prefix_ref(this->bits_per_record()).clear();
    return start_and_read_entire_record_helper([&](size_t bit_index) {
        dirty_out_buffer[bit_index] = true;
    });
}

template <size_t W>
bool MeasureRecordReaderFormatDets<W>::start_and_read_entire_record(SparseShot &cleared_out) {
    if (cleared_out.obs_mask.num_bits_padded() < this->num_observables) {
        cleared_out.obs_mask = simd_bits<64>(this->num_observables);
    }
    size_t obs_start = this->num_measurements + this->num_detectors;
    return start_and_read_entire_record_helper([&](size_t bit_index) {
        if (bit_index < obs_start) {
            cleared_out.hits.push_back(bit_index);
        } else {
            cleared_out.obs_mask[bit_index - obs_start] ^= true;
        }
    });
}

template <size_t W>
MeasureRecordReaderFormatDets<W>::MeasureRecordReaderFormatDets(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader<W>(num_measurements, num_detectors, num_observables), in(in) {
}

template <size_t W>
bool MeasureRecordReaderFormatDets<W>::expects_empty_serialized_data_for_each_shot() const {
    return false;
}

template <size_t W>
size_t MeasureRecordReaderFormatDets<W>::read_into_table_with_minor_shot_index(
    simd_bit_table<W> &out_table, size_t max_shots) {
    size_t read_shots = 0;
    out_table.clear();
    while (read_shots < max_shots) {
        bool more = start_and_read_entire_record_helper([&](size_t bit_index) {
            out_table[bit_index][read_shots] |= 1;
        });
        if (!more) {
            break;
        }
        read_shots++;
    }
    return read_shots;
}

template <size_t W>
template <typename HANDLE_HIT>
bool MeasureRecordReaderFormatDets<W>::start_and_read_entire_record_helper(HANDLE_HIT handle_hit) {
    // Read "shot" prefix, or notice end of data. Ignore indentation and spacing.
    while (true) {
        int next_char = getc(in);
        if (next_char == ' ' || next_char == '\n' || next_char == '\r' || next_char == '\t') {
            continue;
        }
        if (next_char == EOF) {
            return false;
        }
        if (next_char != 's' || getc(in) != 'h' || getc(in) != 'o' || getc(in) != 't') {
            throw std::invalid_argument("DETS data didn't start with 'shot'");
        }
        break;
    }

    // Read prefixed integers until end of line.
    int next_char = getc(in);
    while (true) {
        if (next_char == '\r') {
            next_char = getc(in);
        }
        if (next_char == '\n' || next_char == EOF) {
            return true;
        }
        if (next_char != ' ') {
            throw std::invalid_argument("DETS data wasn't single-space-separated with no trailing spaces.");
        }
        next_char = getc(in);
        uint64_t offset;
        uint64_t length;
        if (next_char == 'M') {
            offset = 0;
            length = this->num_measurements;
        } else if (next_char == 'D') {
            offset = this->num_measurements;
            length = this->num_detectors;
        } else if (next_char == 'L') {
            offset = this->num_measurements + this->num_detectors;
            length = this->num_observables;
        } else {
            throw std::invalid_argument(
                "Unrecognized DETS prefix. Expected M or D or L not '" + std::to_string(next_char) + "'");
        }
        char prefix = next_char;

        uint64_t value;
        if (!read_uint64(in, value, next_char, false)) {
            throw std::invalid_argument("DETS data had a value prefix (M or D or L) not followed by an integer.");
        }
        if (value >= length) {
            std::stringstream msg;
            msg << "DETS data had a value larger than expected. ";
            msg << "Got " << prefix << value << " but expected length of " << prefix << " space to be " << length
                << ".";
            throw std::invalid_argument(msg.str());
        }
        handle_hit((size_t)(offset + value));
    }
}

/// PTB64 format

template <size_t W>
MeasureRecordReaderFormatPTB64<W>::MeasureRecordReaderFormatPTB64(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader<W>(num_measurements, num_detectors, num_observables),
      in(in),
      buf(0),
      num_unread_shots_in_buf(0) {
}

template <size_t W>
bool MeasureRecordReaderFormatPTB64<W>::load_cache() {
    size_t n = this->bits_per_record();
    size_t expected_buf_bits = (n + 63) / 64 * 64 * 64;
    if (buf.num_bits_padded() < expected_buf_bits) {
        buf = simd_bits<W>(expected_buf_bits);
    }

    size_t nb = this->bits_per_record() * (64 / 8);
    size_t nr = fread(buf.u8, 1, nb, in);
    if (nr == 0) {
        num_unread_shots_in_buf = 0;
        return false;
    }
    if (nr != nb) {
        throw std::invalid_argument(
            "ptb64 data ended in middle of 64 record group at byte position " + std::to_string(nr) +
            ".\n"
            "Expected bytes per 64 records was " +
            std::to_string(nb) + " (" + std::to_string(n) + " bits padded).");
    }

    // Convert from bit interleaving to uint64_t interleaving.
    for (size_t k = 0; k + 63 < n; k += 64) {
        inplace_transpose_64x64(buf.u64 + k, 1);
    }

    num_unread_shots_in_buf = 64;
    return true;
}

template <size_t W>
bool MeasureRecordReaderFormatPTB64<W>::start_and_read_entire_record(simd_bits_range_ref<W> dirty_out_buffer) {
    if (num_unread_shots_in_buf == 0) {
        load_cache();
    }
    if (num_unread_shots_in_buf == 0) {
        return false;
    }

    size_t offset = 64 - num_unread_shots_in_buf;
    size_t n = this->bits_per_record();
    size_t n64 = n / 64;
    for (size_t k = 0; k < n64; k++) {
        dirty_out_buffer.u64[k] = buf.u64[k * 64 + offset];
    }
    for (size_t k = n64 * 64; k < n; k++) {
        dirty_out_buffer[k] = buf[k * 64 + offset];
    }
    num_unread_shots_in_buf -= 1;
    return true;
}

template <size_t W>
bool MeasureRecordReaderFormatPTB64<W>::start_and_read_entire_record(SparseShot &cleared_out) {
    if (cleared_out.obs_mask.num_bits_padded() < this->num_observables) {
        cleared_out.obs_mask = simd_bits<64>(this->num_observables);
    }
    if (num_unread_shots_in_buf == 0) {
        load_cache();
    }
    if (num_unread_shots_in_buf == 0) {
        return false;
    }

    size_t offset = 64 - num_unread_shots_in_buf;
    size_t n = this->bits_per_record();
    size_t n64 = n / 64;
    for (size_t k = 0; k < n64; k++) {
        uint64_t v = buf.u64[k * 64 + offset];
        if (v) {
            for (size_t k2 = 0; k2 < 64; k2++) {
                if ((v >> k2) & 1) {
                    cleared_out.hits.push_back(k * 64 + k2);
                }
            }
        }
    }
    for (size_t k = n64 * 64; k < n; k++) {
        if (buf[k * 64 + offset]) {
            cleared_out.hits.push_back(k);
        }
    }
    num_unread_shots_in_buf -= 1;
    this->move_obs_in_shots_to_mask_assuming_sorted(cleared_out);
    return true;
}

template <size_t W>
bool MeasureRecordReaderFormatPTB64<W>::expects_empty_serialized_data_for_each_shot() const {
    return this->bits_per_record() == 0;
}

template <size_t W>
size_t MeasureRecordReaderFormatPTB64<W>::read_into_table_with_minor_shot_index(
    simd_bit_table<W> &out_table, size_t max_shots) {
    size_t n = this->bits_per_record();
    if (n == 0) {
        return 0;  // Ambiguous when the data ends. Stop as early as possible.
    }
    if (max_shots % 64 != 0) {
        throw std::invalid_argument("max_shots must be a multiple of 64 when using PTB64 format");
    }
    for (size_t shots_read = 0; shots_read < max_shots; shots_read += 64) {
        for (size_t bit = 0; bit < n; bit++) {
            size_t read = fread(&out_table[bit].u64[shots_read >> 6], 1, sizeof(uint64_t), in);
            if (read != sizeof(uint64_t)) {
                if (read == 0 && bit == 0) {
                    // End of file at a shot boundary.
                    return shots_read;
                } else {
                    // Fragmented file.
                    throw std::invalid_argument("File ended in the middle of a ptb64 record.");
                }
            }
        }
    }
    return max_shots;
}

template <size_t W>
size_t MeasureRecordReaderFormatPTB64<W>::read_into_table_with_major_shot_index(
    simd_bit_table<W> &out_table, size_t max_shots) {
    size_t n = this->bits_per_record();
    if (n == 0) {
        return 0;  // Ambiguous when the data ends. Stop as early as possible.
    }
    uint64_t buffer[64];
    assert(max_shots % 64 == 0);
    for (size_t shot = 0; shot < max_shots; shot += 64) {
        for (size_t bit = 0; bit < n; bit += 64) {
            for (size_t b = 0; b < 64; b++) {
                if (bit + b >= n) {
                    buffer[b] = 0;
                } else {
                    size_t read = fread(&buffer[b], 1, sizeof(uint64_t), in);
                    if (read != sizeof(uint64_t)) {
                        if (read == 0 && bit == 0 && b == 0) {
                            // End of file at a shot boundary.
                            return shot;
                        } else {
                            // Fragmented file.
                            throw std::invalid_argument("File ended in the middle of a ptb64 record.");
                        }
                    }
                }
            }
            inplace_transpose_64x64(buffer, 1);
            for (size_t s = 0; s < 64; s++) {
                out_table[shot + s].u64[bit >> 6] = buffer[s];
            }
        }
    }
    return max_shots;
}

template <size_t W>
size_t read_file_data_into_shot_table(
    FILE *in,
    size_t max_shots,
    size_t num_bits_per_shot,
    SampleFormat format,
    char dets_char,
    simd_bit_table<W> &out_table,
    bool shots_is_major_index_of_out_table) {
    auto reader = MeasureRecordReader<W>::make(
        in,
        format,
        dets_char == 'M' ? num_bits_per_shot : 0,
        dets_char == 'D' ? num_bits_per_shot : 0,
        dets_char == 'L' ? num_bits_per_shot : 0);
    if (shots_is_major_index_of_out_table) {
        return reader->read_into_table_with_major_shot_index(out_table, max_shots);
    } else {
        return reader->read_into_table_with_minor_shot_index(out_table, max_shots);
    }
}

}  // namespace stim
