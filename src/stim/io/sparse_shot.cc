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
#include <stim/str_util.h>

using namespace stim;

SparseShot::SparseShot() : hits(), obs_mask(0) {
}
SparseShot::SparseShot(std::vector<uint64_t> hits, uint32_t obs_mask) : hits(hits), obs_mask(obs_mask) {
}

void SparseShot::clear() {
    hits.clear();
    obs_mask = 0;
}

bool SparseShot::operator==(const SparseShot &other) const {
    return hits == other.hits && obs_mask == other.obs_mask;
}

bool SparseShot::operator!=(const SparseShot &other) const {
    return !(*this == other);
}

std::string SparseShot::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::ostream &stim::operator<<(std::ostream &out, const SparseShot &v) {
    return out << "SparseShot{{" << comma_sep(v.hits) << "}, " << v.obs_mask << "}";
}
