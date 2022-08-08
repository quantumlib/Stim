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

size_t MeasureRecordReader::read_records_into(simd_bit_table<MAX_BITWORD_WIDTH> &out, bool major_index_is_shot_index, size_t max_shots) {
    if (!major_index_is_shot_index) {
        simd_bit_table<MAX_BITWORD_WIDTH> buf(out.num_minor_bits_padded(), out.num_major_bits_padded());
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
            return std::unique_ptr<MeasureRecordReader>(
                new MeasureRecordReaderFormatPTB64(in, num_measurements, num_detectors, num_observables));
        case SAMPLE_FORMAT_R8:
            return std::unique_ptr<MeasureRecordReader>(
                new MeasureRecordReaderFormatR8(in, num_measurements, num_detectors, num_observables));
        default:
            throw std::invalid_argument("Sample format not recognized by SingleMeasurementRecord");
    }
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

size_t MeasureRecordReader::read_into_table_with_major_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) {
    size_t read_shots = 0;
    while (read_shots < max_shots && start_and_read_entire_record(out_table[read_shots])) {
        read_shots++;
    }
    return read_shots;
}

/// 01 format

MeasureRecordReaderFormat01::MeasureRecordReaderFormat01(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader(num_measurements, num_detectors, num_observables), in(in) {
}

bool MeasureRecordReaderFormat01::start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) {
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

bool MeasureRecordReaderFormat01::expects_empty_serialized_data_for_each_shot() const {
    return false;
}

size_t MeasureRecordReaderFormat01::read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) {
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

/// B8 format

MeasureRecordReaderFormatB8::MeasureRecordReaderFormatB8(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader(num_measurements, num_detectors, num_observables), in(in) {
}

bool MeasureRecordReaderFormatB8::start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) {
    size_t n = bits_per_record();
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

size_t MeasureRecordReaderFormatB8::read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) {
    size_t n = bits_per_record();
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

bool MeasureRecordReaderFormatB8::start_and_read_entire_record(SparseShot &cleared_out) {
    size_t n = bits_per_record();
    size_t nb = (n + 7) >> 3;
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
    move_obs_in_shots_to_mask_assuming_sorted(cleared_out);
    return true;
}

bool MeasureRecordReaderFormatB8::expects_empty_serialized_data_for_each_shot() const {
    return bits_per_record() == 0;
}

/// Hits format

MeasureRecordReaderFormatHits::MeasureRecordReaderFormatHits(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader(num_measurements, num_detectors, num_observables), in(in) {
}

bool MeasureRecordReaderFormatHits::start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) {
    size_t m = bits_per_record();
    dirty_out_buffer.prefix_ref(bits_per_record()).clear();
    return start_and_read_entire_record_helper([&](size_t bit_index) {
        if (bit_index >= m) {
            throw std::invalid_argument("hit index is too large.");
        }
        dirty_out_buffer[bit_index] ^= true;
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

bool MeasureRecordReaderFormatHits::expects_empty_serialized_data_for_each_shot() const {
    return false;
}

size_t MeasureRecordReaderFormatHits::read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) {
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

/// R8 format

MeasureRecordReaderFormatR8::MeasureRecordReaderFormatR8(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader(num_measurements, num_detectors, num_observables), in(in) {
}

bool MeasureRecordReaderFormatR8::start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) {
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

bool MeasureRecordReaderFormatR8::expects_empty_serialized_data_for_each_shot() const {
    return false;
}

size_t MeasureRecordReaderFormatR8::read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) {
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

/// DETS format

bool MeasureRecordReaderFormatDets::start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) {
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
    : MeasureRecordReader(num_measurements, num_detectors, num_observables), in(in) {
}

bool MeasureRecordReaderFormatDets::expects_empty_serialized_data_for_each_shot() const {
    return false;
}

size_t MeasureRecordReaderFormatDets::read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) {
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

/// PTB64 format

MeasureRecordReaderFormatPTB64::MeasureRecordReaderFormatPTB64(
    FILE *in, size_t num_measurements, size_t num_detectors, size_t num_observables)
    : MeasureRecordReader(num_measurements, num_detectors, num_observables),
      in(in),
      buf(0),
      num_unread_shots_in_buf(0) {
}

bool MeasureRecordReaderFormatPTB64::load_cache() {
    size_t n = bits_per_record();
    size_t expected_buf_bits = (n + 63) / 64 * 64 * 64;
    if (buf.num_bits_padded() < expected_buf_bits) {
        buf = simd_bits<MAX_BITWORD_WIDTH>(expected_buf_bits);
    }

    size_t nb = bits_per_record() * (64 / 8);
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
    for (size_t k = 0; k < n; k += 64) {
        inplace_transpose_64x64(buf.u64 + k, 1);
    }

    num_unread_shots_in_buf = 64;
    return true;
}

bool MeasureRecordReaderFormatPTB64::start_and_read_entire_record(simd_bits_range_ref<MAX_BITWORD_WIDTH> dirty_out_buffer) {
    if (num_unread_shots_in_buf == 0) {
        load_cache();
    }
    if (num_unread_shots_in_buf == 0) {
        return false;
    }

    size_t offset = 64 - num_unread_shots_in_buf;
    size_t n64 = (bits_per_record() + 63) / 64;
    for (size_t k = 0; k < n64; k++) {
        dirty_out_buffer.u64[k] = buf.u64[k * 64 + offset];
    }
    num_unread_shots_in_buf -= 1;
    return true;
}

bool MeasureRecordReaderFormatPTB64::start_and_read_entire_record(SparseShot &cleared_out) {
    if (num_unread_shots_in_buf == 0) {
        load_cache();
    }
    if (num_unread_shots_in_buf == 0) {
        return false;
    }

    size_t offset = 64 - num_unread_shots_in_buf;
    size_t n = bits_per_record();
    size_t n64 = (n + 63) / 64;
    for (size_t k = 0; k < n64; k++) {
        uint64_t v = buf.u64[k * 64 + offset];
        if (v) {
            size_t nb = std::min(size_t{64}, n - k * 64);
            for (size_t b = 0; b < nb; b++) {
                if (v & (uint64_t{1} << b)) {
                    cleared_out.hits.push_back(k * 64 + b);
                }
            }
        }
    }
    num_unread_shots_in_buf -= 1;
    move_obs_in_shots_to_mask_assuming_sorted(cleared_out);
    return true;
}

bool MeasureRecordReaderFormatPTB64::expects_empty_serialized_data_for_each_shot() const {
    return bits_per_record() == 0;
}

size_t MeasureRecordReaderFormatPTB64::read_into_table_with_minor_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) {
    size_t n = bits_per_record();
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

size_t MeasureRecordReaderFormatPTB64::read_into_table_with_major_shot_index(simd_bit_table<MAX_BITWORD_WIDTH> &out_table, size_t max_shots) {
    size_t n = bits_per_record();
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

size_t stim::read_file_data_into_shot_table(
    FILE *in,
    size_t max_shots,
    size_t num_bits_per_shot,
    SampleFormat format,
    char dets_char,
    simd_bit_table<MAX_BITWORD_WIDTH> &out_table,
    bool shots_is_major_index_of_out_table) {
    auto reader = MeasureRecordReader::make(
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
