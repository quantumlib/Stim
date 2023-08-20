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

#include <cstddef>
#include <cstdint>

#ifndef _STIM_MEM_SIMD_WORD_H
#define _STIM_MEM_SIMD_WORD_H

#include "stim/mem/bitword_128_sse.h"
#include "stim/mem/bitword_256_avx.h"
#include "stim/mem/bitword_64.h"

namespace stim {
#if __AVX2__
constexpr size_t MAX_BITWORD_WIDTH = 256;
#elif __SSE2__
constexpr size_t MAX_BITWORD_WIDTH = 128;
#else
constexpr size_t MAX_BITWORD_WIDTH = 64;
#endif

template <size_t W>
using simd_word = bitword<W>;

}  // namespace stim

#endif
