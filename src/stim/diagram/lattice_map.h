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

#ifndef _STIM_DIAGRAM_TIMELINE_ASCII_LATTICE_MAP_H
#define _STIM_DIAGRAM_TIMELINE_ASCII_LATTICE_MAP_H

#include <iostream>

#include "stim/circuit/circuit.h"

namespace stim_draw_internal {

/// A map that supports writing to sets of indices that form bounded lattices.
struct LatticeMap {
    std::vector<uint32_t> brute_force_data;

    /// Write to a lattice of indices.
    void set(
        uint64_t index,
        stim::SpanRef<const uint64_t> offsets_per_iteration,
        stim::SpanRef<const uint64_t> iteration_counts,
        uint32_t value);
    /// Read the latest value written to an index, defaulting to 0 if no writes yet.
    uint32_t get(uint64_t index);
};

}  // namespace stim_draw_internal

#endif
