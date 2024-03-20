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

#include <cstddef>
#include <cstdint>

namespace stim {

inline uint8_t floor_lg2(size_t value) {
    uint8_t result = 0;
    while (value > 1) {
        result += 1;
        value >>= 1;
    }
    return result;
}

inline uint8_t is_power_of_2(size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

inline size_t first_set_bit(size_t value, size_t min_result) {
    size_t t = min_result;
    value >>= min_result;
//    assert(value);
    while (!(value & 1)) {
        value >>= 1;
        t += 1;
    }
    return t;
}

}  // namespace stim

#endif
