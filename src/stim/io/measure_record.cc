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

#include "stim/io/measure_record.h"

#include <algorithm>

#include "stim/io/measure_record_writer.h"

using namespace stim;

MeasureRecord::MeasureRecord(size_t max_lookback) : max_lookback(max_lookback), unwritten(0) {
}

void MeasureRecord::write_unwritten_results_to(MeasureRecordWriter &writer) {
    size_t n = storage.size();
    for (size_t k = n - unwritten; k < n; k++) {
        writer.write_bit(storage[k]);
    }
    unwritten = 0;
    if ((storage.size() >> 1) > max_lookback) {
        storage.erase(storage.begin(), storage.end() - max_lookback);
    }
}

bool MeasureRecord::lookback(size_t lookback) const {
    if (lookback > storage.size()) {
        throw std::out_of_range("Referred to a measurement record before the beginning of time.");
    }
    if (lookback == 0) {
        throw std::out_of_range("Lookback must be non-zero.");
    }
    if (lookback > max_lookback) {
        throw std::out_of_range("Referred to a measurement record past the lookback limit.");
    }
    return *(storage.end() - lookback);
}

void MeasureRecord::record_result(bool result) {
    storage.push_back(result);
    unwritten++;
}

void MeasureRecord::record_results(const std::vector<bool> &results) {
    storage.insert(storage.end(), results.begin(), results.end());
    unwritten += results.size();
}

void MeasureRecord::clear() {
    unwritten = 0;
    storage.clear();
}

void MeasureRecord::discard_results_past_max_lookback() {
    if (storage.size() > max_lookback) {
        storage.erase(storage.begin(), storage.end() - max_lookback);
    }
    if (unwritten > max_lookback) {
        unwritten = max_lookback;
    }
}
