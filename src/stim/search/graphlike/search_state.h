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

namespace stim {

namespace impl_search_graphlike {

struct SearchState {
    uint64_t det_active;  // The detection event being moved around in an attempt to remove it (or NO_NODE_INDEX).
    uint64_t det_held;    // The detection event being left in the same place (or NO_NODE_INDEX).
    uint64_t obs_mask;    // The accumulated frame changes from moving the detection events around.

    SearchState();
    SearchState(uint64_t det_active, uint64_t det_held, uint64_t obs_mask);
    bool is_undetected() const;
    SearchState canonical() const;
    void append_transition_as_error_instruction_to(const SearchState &other, DetectorErrorModel &out) const;
    bool operator==(const SearchState &other) const;
    bool operator!=(const SearchState &other) const;
    bool operator<(const SearchState &other) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const SearchState &v);

}  // namespace impl_search_graphlike
}  // namespace stim

#endif
