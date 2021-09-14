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

#ifndef _STIM_IO_MEASURE_RECORD_H
#define _STIM_IO_MEASURE_RECORD_H

#include <cstddef>
#include <cstdint>

#include "stim/io/measure_record_writer.h"

namespace stim {

/// Stores a historical record of measurement results that can be looked up and written to the external world.
///
/// Results that have been written and are further back than `max_lookback` may be discarded from memory.
struct MeasureRecord {
    /// How far back into the measurement record a circuit being simulated may look.
    /// Results younger than this cannot be discarded.
    size_t max_lookback;
    /// How many results have been recorded but not yet written to the external world.
    /// Results younger than this cannot be discarded.
    size_t unwritten;
    /// The actual recorded results.
    std::vector<bool> storage;
    /// Creates an empty measurement record.
    MeasureRecord(size_t max_lookback = SIZE_MAX);
    /// Forces all unwritten results to be written via the given writer.
    ///
    /// After the results are written, older measurements now eligible to be discarded may be removed from memory.
    void write_unwritten_results_to(MeasureRecordWriter &writer);
    /// Returns a measurement result from the record.
    ///
    /// Args:
    ///     lookback: How far back the measurement is. lookback=1 is the latest measurement, 2 the second latest, etc.
    bool lookback(size_t lookback) const;
    /// Appends a measurement to the record.
    void record_result(bool result);
};

}  // namespace stim

#endif
