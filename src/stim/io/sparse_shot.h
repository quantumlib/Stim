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

#include "stim/mem/simd_bits.h"

namespace stim {

/// Stores shot data sparsely, as a list of locations of 1 bits in the shot.
///
/// For contrast, dense storage would involve storing one bit for each bit in the shot.
///
/// If this shot is detection event data, the observable data is still stored densely in
/// the obs_mask field.
struct SparseShot {
    /// Indices of non-zero bits.
    std::vector<uint64_t> hits;

    /// When reading detection event data with observables appended, the observables go into this mask.
    /// The observable with index k goes into the 1<<k bit.
    simd_bits<64> obs_mask;

    SparseShot();
    SparseShot(std::vector<uint64_t> hits, simd_bits<64> obs_mask);

    /// Returns a uint64_t containing the first 64 observable bit flips.
    /// Defaults to 0 if there are no observables.
    uint64_t obs_mask_as_u64() const;

    void clear();
    bool operator==(const SparseShot &other) const;
    bool operator!=(const SparseShot &other) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const SparseShot &v);

}  // namespace stim

#endif
