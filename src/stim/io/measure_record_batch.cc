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

#include "stim/io/measure_record_batch.h"

#include <algorithm>

#include "stim/io/measure_record_batch_writer.h"
#include "stim/probability_util.h"

using namespace stim;

MeasureRecordBatch::MeasureRecordBatch(size_t num_shots, size_t max_lookback)
    : max_lookback(max_lookback), unwritten(0), stored(0), written(0), shot_mask(num_shots), storage(1, num_shots) {
    for (size_t k = 0; k < num_shots; k++) {
        shot_mask[k] = true;
    }
}

void MeasureRecordBatch::reserve_space_for_results(size_t count) {
    if (stored + count > storage.num_major_bits_padded()) {
        simd_bit_table<MAX_BITWORD_WIDTH> new_storage((stored + count) * 2, storage.num_minor_bits_padded());
        new_storage.data.word_range_ref(0, storage.data.num_simd_words) = storage.data;
        storage = std::move(new_storage);
    }
}

void MeasureRecordBatch::reserve_noisy_space_for_results(const OperationData &target_data, std::mt19937_64 &rng) {
    size_t count = target_data.targets.size();
    reserve_space_for_results(count);
    float p = target_data.args.empty() ? 0 : target_data.args[0];
    biased_randomize_bits(p, storage[stored].u64, storage[stored + count].u64, rng);
}

void MeasureRecordBatch::xor_record_reserved_result(simd_bits_range_ref<MAX_BITWORD_WIDTH> result) {
    storage[stored] ^= result;
    storage[stored] &= shot_mask;
    stored++;
    unwritten++;
}

void MeasureRecordBatch::record_result(simd_bits_range_ref<MAX_BITWORD_WIDTH> result) {
    reserve_space_for_results(1);
    storage[stored] = result;
    storage[stored] &= shot_mask;
    stored++;
    unwritten++;
}

simd_bits_range_ref<MAX_BITWORD_WIDTH> MeasureRecordBatch::lookback(size_t lookback) const {
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

void MeasureRecordBatch::mark_all_as_written() {
    unwritten = 0;
    size_t m = max_lookback;
    if ((stored >> 1) > m) {
        memcpy(storage.data.u8, storage[stored - m].u8, m * storage.num_minor_u8_padded());
        stored = m;
    }
}

void MeasureRecordBatch::intermediate_write_unwritten_results_to(
    MeasureRecordBatchWriter &writer, simd_bits_range_ref<MAX_BITWORD_WIDTH> ref_sample) {
    while (unwritten >= 1024) {
        auto slice = storage.slice_maj(stored - unwritten, stored - unwritten + 1024);
        for (size_t k = 0; k < 1024; k++) {
            size_t j = written + k;
            if (j < ref_sample.num_bits_padded() && ref_sample[j]) {
                slice[k] ^= shot_mask;
            }
        }
        writer.batch_write_bytes(slice, 1024 >> 6);
        unwritten -= 1024;
        written += 1024;
    }

    size_t m = std::max(max_lookback, unwritten);
    if ((stored >> 1) > m) {
        memcpy(storage.data.u8, storage[stored - m].u8, m * storage.num_minor_u8_padded());
        stored = m;
    }
}

void MeasureRecordBatch::final_write_unwritten_results_to(
    MeasureRecordBatchWriter &writer, simd_bits_range_ref<MAX_BITWORD_WIDTH> ref_sample) {
    size_t n = stored;
    for (size_t k = n - unwritten; k < n; k++) {
        bool invert = written < ref_sample.num_bits_padded() && ref_sample[written];
        if (invert) {
            storage[k] ^= shot_mask;
        }
        writer.batch_write_bit(storage[k]);
        if (invert) {
            storage[k] ^= shot_mask;
        }
        written++;
    }
    unwritten = 0;
    writer.write_end();
}

void MeasureRecordBatch::clear() {
    stored = 0;
    unwritten = 0;
}
