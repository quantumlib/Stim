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

#ifndef _STIM_SEARCH_GRAPHLIKE_SEARCH_STATE_H
#define _STIM_SEARCH_GRAPHLIKE_SEARCH_STATE_H

#include "stim/dem/detector_error_model.h"
#include "stim/mem/simd_bits.h"

namespace stim {

namespace impl_search_graphlike {

struct SearchState {
    uint64_t det_active;     // The detection event being moved around in an attempt to remove it (or NO_NODE_INDEX).
    uint64_t det_held;       // The detection event being left in the same place (or NO_NODE_INDEX).
    simd_bits<64> obs_mask;  // The accumulated frame changes from moving the detection events around.

    SearchState() = delete;
    SearchState(size_t num_observables);
    SearchState(uint64_t det_active, uint64_t det_held, simd_bits<64> obs_mask);
    bool is_undetected() const;
    SearchState canonical() const;
    void append_transition_as_error_instruction_to(const SearchState &other, DetectorErrorModel &out) const;
    bool operator==(const SearchState &other) const;
    bool operator!=(const SearchState &other) const;
    bool operator<(const SearchState &other) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const SearchState &v);

inline void hash_combine(size_t &h, uint64_t x) {
    h ^= std::hash<uint64_t>{}(x) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2); // mimic Boost's hash-combine function
}

struct SearchStateHash {
    size_t operator()(const SearchState &s) const {
        SearchState c = s.canonical();
        size_t h = std::hash<uint64_t>{}(c.det_active);
        hash_combine(h, c.det_held);
        for (size_t i = 0; i < c.obs_mask.num_u64_padded(); i++) {
            hash_combine(h, c.obs_mask.u64[i]);
        }
        return h;
    }
};

}  // namespace impl_search_graphlike
}  // namespace stim

#endif
