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

#include "stim/io/measure_record_batch.h"
#include "stim/io/measure_record_batch_writer.h"
#include "stim/util_bot/probability_util.h"

namespace stim {

template <size_t W>
MeasureRecordBatch<W>::MeasureRecordBatch(size_t num_shots, size_t max_lookback)
    : num_shots(num_shots),
      max_lookback(max_lookback),
      unwritten(0),
      stored(0),
      written(0),
      shot_mask(num_shots),
      storage(1, num_shots) {
    for (size_t k = 0; k < num_shots; k++) {
        shot_mask[k] = true;
    }
}

template <size_t W>
void MeasureRecordBatch<W>::reserve_space_for_results(size_t count) {
    if (stored + count > storage.num_major_bits_padded()) {
        simd_bit_table<W> new_storage((stored + count) * 2, storage.num_minor_bits_padded());
        new_storage.data.word_range_ref(0, storage.data.num_simd_words) = storage.data;
        storage = std::move(new_storage);
    }
}

template <size_t W>
void MeasureRecordBatch<W>::reserve_noisy_space_for_results(const CircuitInstruction &inst, std::mt19937_64 &rng) {
    size_t count = inst.targets.size();
    reserve_space_for_results(count);
    float p = inst.args.empty() ? 0 : inst.args[0];
    biased_randomize_bits(p, storage[stored].u64, storage[stored + count].u64, rng);
}

template <size_t W>
void MeasureRecordBatch<W>::xor_record_reserved_result(simd_bits_range_ref<W> result) {
    storage[stored] ^= result;
    storage[stored] &= shot_mask;
    stored++;
    unwritten++;
}

template <size_t W>
void MeasureRecordBatch<W>::record_result(simd_bits_range_ref<W> result) {
    reserve_space_for_results(1);
    storage[stored] = result;
    storage[stored] &= shot_mask;
    stored++;
    unwritten++;
}

template <size_t W>
simd_bits_range_ref<W> MeasureRecordBatch<W>::record_zero_result_to_edit() {
    reserve_space_for_results(1);
    storage[stored].clear();
    stored++;
    unwritten++;
    return storage[stored - 1];
}

template <size_t W>
simd_bits_range_ref<W> MeasureRecordBatch<W>::lookback(size_t lookback) const {
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

template <size_t W>
void MeasureRecordBatch<W>::mark_all_as_written() {
    unwritten = 0;
    size_t m = max_lookback;
    if ((stored >> 1) > m) {
        memcpy(storage.data.u8, storage[stored - m].u8, m * storage.num_minor_u8_padded());
        stored = m;
    }
}

template <size_t W>
void MeasureRecordBatch<W>::intermediate_write_unwritten_results_to(
    MeasureRecordBatchWriter &writer, simd_bits_range_ref<W> ref_sample) {
    constexpr size_t WRITE_SIZE = 256;
    while (unwritten >= WRITE_SIZE) {
        auto slice = storage.slice_maj(stored - unwritten, stored - unwritten + WRITE_SIZE);
        for (size_t k = 0; k < WRITE_SIZE; k++) {
            size_t j = written + k;
            if (j < ref_sample.num_bits_padded() && ref_sample[j]) {
                slice[k] ^= shot_mask;
            }
        }
        writer.batch_write_bytes<W>(slice, WRITE_SIZE >> 6);
        unwritten -= WRITE_SIZE;
        written += WRITE_SIZE;
    }

    size_t m = std::max(max_lookback, unwritten);
    if ((stored >> 1) > m) {
        memcpy(storage.data.u8, storage[stored - m].u8, m * storage.num_minor_u8_padded());
        stored = m;
    }
}

template <size_t W>
void MeasureRecordBatch<W>::final_write_unwritten_results_to(
    MeasureRecordBatchWriter &writer, simd_bits_range_ref<W> ref_sample) {
    size_t n = stored;
    for (size_t k = n - unwritten; k < n; k++) {
        bool invert = written < ref_sample.num_bits_padded() && ref_sample[written];
        if (invert) {
            storage[k] ^= shot_mask;
        }
        writer.batch_write_bit<W>(storage[k]);
        if (invert) {
            storage[k] ^= shot_mask;
        }
        written++;
    }
    unwritten = 0;
    writer.write_end();
}

template <size_t W>
void MeasureRecordBatch<W>::clear() {
    stored = 0;
    unwritten = 0;
}

template <size_t W>
void MeasureRecordBatch<W>::destructive_resize(size_t new_num_shots, size_t new_max_lookback) {
    unwritten = 0;
    stored = 0;
    written = 0;
    max_lookback = new_max_lookback;
    if (new_num_shots != num_shots) {
        num_shots = new_num_shots;
        shot_mask = simd_bits<W>(num_shots);
        for (size_t k = 0; k < num_shots; k++) {
            shot_mask[k] = true;
        }
        storage.destructive_resize(1, num_shots);
    }
}

}  // namespace stim
