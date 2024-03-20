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

#ifndef _STIM_UTIL_BOT_TWIDDLE_H
#define _STIM_UTIL_BOT_TWIDDLE_H

#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace stim {

inline uint8_t is_power_of_2(size_t value) {
    // Note: would use std::has_single_bit here, but as of March 2024 that method is missing when building with
    // emscripten.
    return std::popcount(value) == 1;
}

inline uint8_t floor_lg2(size_t value) {
    return sizeof(value) * 8 - 1 - std::countl_zero(value);
}

inline size_t first_set_bit(size_t value, size_t min_result) {
    value >>= min_result;
    if (!value) {
        throw std::invalid_argument("No matching set bit.");
    }
    return std::countr_zero(value) + min_result;
}

}  // namespace stim

#endif
