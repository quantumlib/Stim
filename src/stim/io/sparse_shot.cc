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

#include "stim/io/sparse_shot.h"

#include <sstream>

#include "stim/util_bot/str_util.h"

using namespace stim;

SparseShot::SparseShot() : hits(), obs_mask(0) {
}
SparseShot::SparseShot(std::vector<uint64_t> hits, simd_bits<64> obs_mask)
    : hits(std::move(hits)), obs_mask(std::move(obs_mask)) {
}

void SparseShot::clear() {
    hits.clear();
    obs_mask.clear();
}

bool SparseShot::operator==(const SparseShot &other) const {
    return hits == other.hits && obs_mask == other.obs_mask;
}

bool SparseShot::operator!=(const SparseShot &other) const {
    return !(*this == other);
}

uint64_t SparseShot::obs_mask_as_u64() const {
    if (obs_mask.num_u64_padded() == 0) {
        return 0;
    }
    return obs_mask.u64[0];
}

std::string SparseShot::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::ostream &stim::operator<<(std::ostream &out, const SparseShot &v) {
    return out << "SparseShot{{" << comma_sep(v.hits) << "}, " << v.obs_mask << "}";
}
