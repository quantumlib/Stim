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

#ifndef _STIM_IO_SPARSE_SHOT_H
#define _STIM_IO_SPARSE_SHOT_H

#include <string>
#include <vector>

namespace stim {

struct SparseShot {
    /// Indices of non-zero bits.
    std::vector<uint64_t> hits;

    /// When reading detection event data with observables appended, the observables go into this mask.
    /// The observable with index k goes into the 1<<k bit.
    uint32_t obs_mask;

    SparseShot();
    SparseShot(std::vector<uint64_t> hits, uint32_t obs_mask = 0);

    void clear();
    bool operator==(const SparseShot &other) const;
    bool operator!=(const SparseShot &other) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const SparseShot &v);

}  // namespace stim

#endif
