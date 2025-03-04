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

#include "stim/search/hyper/search_state.h"

#include <algorithm>

#include "stim/search/hyper/node.h"

using namespace stim;
using namespace stim::impl_search_hyper;

std::string SearchState::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}

void SearchState::append_transition_as_error_instruction_to(const SearchState &other, DetectorErrorModel &out) const {
    // Extract detector indices while cancelling duplicates.
    SparseXorVec<uint64_t> dif = dets ^ other.dets;
    for (const auto &n : dif) {
        out.target_buf.append_tail(DemTarget::relative_detector_id(n));
    }

    // Extract logical observable indices.
    auto dif_mask = obs_mask ^ other.obs_mask;
    for (size_t k = 0; k < dif_mask.num_bits_padded(); k++) {
        if (dif_mask[k]) {
            out.target_buf.append_tail(DemTarget::observable_id(k));
        }
    }

    // Default probability to 1.
    out.arg_buf.append_tail(1);

    out.instructions.push_back(DemInstruction{
        .arg_data=out.arg_buf.commit_tail(),
        .target_data=out.target_buf.commit_tail(),
        .tag="",
        .type=DemInstructionType::DEM_ERROR,
    });
}

bool SearchState::operator==(const SearchState &other) const {
    return dets == other.dets && obs_mask == other.obs_mask;
}
bool SearchState::operator!=(const SearchState &other) const {
    return !(*this == other);
}

bool SearchState::operator<(const SearchState &other) const {
    if (dets != other.dets) {
        return dets < other.dets;
    }
    return obs_mask < other.obs_mask;
}

std::ostream &stim::impl_search_hyper::operator<<(std::ostream &out, const SearchState &v) {
    if (v.dets.empty()) {
        out << "[no symptoms] ";
    } else {
        for (const auto &d : v.dets) {
            out << "D" << d << " ";
        }
    }

    for (size_t k = 0; k < v.obs_mask.num_bits_padded(); k++) {
        if (v.obs_mask[k]) {
            out << "L" << k << " ";
        }
    }

    return out;
}
