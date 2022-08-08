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

#include "stim/mem/simd_util.h"

template <uint64_t mask, uint64_t shift>
void inplace_transpose_64x64_pass(uint64_t* data, size_t stride) {
    for (size_t k = 0; k < 64; k++) {
        if (k & shift) {
            continue;
        }
        uint64_t& x = data[stride * k];
        uint64_t& y = data[stride * (k + shift)];
        uint64_t a = x & mask;
        uint64_t b = x & ~mask;
        uint64_t c = y & mask;
        uint64_t d = y & ~mask;
        x = a | (c << shift);
        y = (b >> shift) | d;
    }
}

void stim::inplace_transpose_64x64(uint64_t* data, size_t stride) {
    inplace_transpose_64x64_pass<0x5555555555555555UL, 1>(data, stride);
    inplace_transpose_64x64_pass<0x3333333333333333UL, 2>(data, stride);
    inplace_transpose_64x64_pass<0x0F0F0F0F0F0F0F0FUL, 4>(data, stride);
    inplace_transpose_64x64_pass<0x00FF00FF00FF00FFUL, 8>(data, stride);
    inplace_transpose_64x64_pass<0x0000FFFF0000FFFFUL, 16>(data, stride);
    inplace_transpose_64x64_pass<0x00000000FFFFFFFFUL, 32>(data, stride);
}
