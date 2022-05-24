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

#ifndef _STIM_MEM_SIMD_UTIL_H
#define _STIM_MEM_SIMD_UTIL_H

#include <cstddef>
#include <cstdint>

namespace stim {

constexpr uint64_t tile64_helper(uint64_t val, size_t shift) {
    return shift >= 64 ? val : tile64_helper(val | (val << shift), shift << 1);
}

constexpr uint64_t interleave_mask(size_t step) {
    return tile64_helper((uint64_t{1} << step) - 1, step << 1);
}

inline uint64_t spread_bytes_32_to_64(uint32_t v) {
    uint64_t r = (((uint64_t)v << 16) | v) & 0xFFFF0000FFFFULL;
    return ((r << 8) | r) & 0xFF00FF00FF00FFULL;
}

void inplace_transpose_64x64(uint64_t *data);

inline uint8_t popcnt64(uint64_t val) {
    val -= (val >> 1) & 0x5555555555555555ULL;
    val = (val & 0x3333333333333333ULL) + ((val >> 2) & 0x3333333333333333ULL);
    val += val >> 4;
    val &= 0xF0F0F0F0F0F0F0FULL;
    val *= 0x101010101010101ULL;
    val >>= 56;
    return (uint8_t)val;
}

}  // namespace stim

#endif
