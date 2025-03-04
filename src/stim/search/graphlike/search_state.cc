// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/search/graphlike/search_state.h"

#include <algorithm>
#include <map>

#include "stim/search/graphlike/node.h"

using namespace stim;
using namespace stim::impl_search_graphlike;

std::string SearchState::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}

SearchState::SearchState(size_t num_observables)
    : det_active(NO_NODE_INDEX), det_held(NO_NODE_INDEX), obs_mask(num_observables) {
}
SearchState::SearchState(uint64_t det_active, uint64_t det_held, simd_bits<64> obs_mask)
    : det_active(det_active), det_held(det_held), obs_mask(std::move(obs_mask)) {
}

bool SearchState::is_undetected() const {
    return det_active == det_held;
}

SearchState SearchState::canonical() const {
    if (det_active < det_held) {
        return SearchState{det_active, det_held, obs_mask};
    } else if (det_active > det_held) {
        return SearchState{det_held, det_active, obs_mask};
    } else {
        return SearchState{NO_NODE_INDEX, NO_NODE_INDEX, obs_mask};
    }
}

void SearchState::append_transition_as_error_instruction_to(const SearchState &other, DetectorErrorModel &out) const {
    // Extract detector indices while cancelling duplicates.
    std::array<uint64_t, 5> nodes{det_active, det_held, other.det_active, other.det_held, NO_NODE_INDEX};
    std::sort(nodes.begin(), nodes.end());
    for (size_t k = 0; k < 4; k++) {
        if (nodes[k] == nodes[k + 1]) {
            k++;
        } else {
            out.target_buf.append_tail(DemTarget::relative_detector_id(nodes[k]));
        }
    }

    // Extract logical observable indices.
    auto dif_mask = obs_mask ^ other.obs_mask;
    for (size_t k = 0; k < dif_mask.num_bits_padded(); k++) {
        if (dif_mask[k]) {
            out.target_buf.append_tail(DemTarget::observable_id(k));
        }
    }

    out.arg_buf.append_tail(1);

    out.instructions.push_back(DemInstruction{
        .arg_data=out.arg_buf.commit_tail(),
        .target_data=out.target_buf.commit_tail(),
        .tag="",
        .type=DemInstructionType::DEM_ERROR,
    });
}

bool SearchState::operator==(const SearchState &other) const {
    SearchState a = canonical();
    SearchState b = other.canonical();
    return a.det_active == b.det_active && a.det_held == b.det_held && a.obs_mask == b.obs_mask;
}
bool SearchState::operator!=(const SearchState &other) const {
    return !(*this == other);
}

bool SearchState::operator<(const SearchState &other) const {
    SearchState a = canonical();
    SearchState b = other.canonical();
    if (a.det_active != b.det_active) {
        return a.det_active < b.det_active;
    }
    if (a.det_held != b.det_held) {
        return a.det_held < b.det_held;
    }
    return a.obs_mask < b.obs_mask;
}

std::ostream &stim::impl_search_graphlike::operator<<(std::ostream &out, const SearchState &v) {
    if (v.is_undetected()) {
        out << "[no symptoms] ";
    } else {
        if (v.det_active != NO_NODE_INDEX) {
            out << "D" << v.det_active << " ";
        }
        if (v.det_held != NO_NODE_INDEX) {
            out << "D" << v.det_held << " ";
        }
    }

    for (size_t k = 0; k < v.obs_mask.num_bits_padded(); k++) {
        if (v.obs_mask[k]) {
            out << "L" << k << " ";
        }
    }

    return out;
}
