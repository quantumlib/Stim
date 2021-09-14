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

#ifndef _STIM_MEM_BIT_REF_H
#define _STIM_MEM_BIT_REF_H

#include <cstddef>
#include <cstdint>

namespace stim {

/// A reference to a bit within a byte.
///
/// Conceptually behaves the same as a `bool &`, as opposed to a `bool *`. For example, the `=` operator overwrites the
/// contents of the bit being referenced instead of changing which bit is pointed to.
///
/// This should behave essentially identically to the weird bit references that come out of a `std::vector<bool>`.
struct bit_ref {
    uint8_t *byte;
    uint8_t bit_index;

    /// Construct a bit_ref from a pointer and a bit offset.
    /// The offset can be larger than a word.
    /// Automatically canonicalized so that the offset is less than 8.
    bit_ref(void *base, size_t offset);

    /// Copy assignment.
    inline bit_ref &operator=(bool value) {
        *byte &= ~((uint8_t)1 << bit_index);
        *byte |= (uint8_t)value << bit_index;
        return *this;
    }
    /// Copy assignment.
    inline bit_ref &operator=(const bit_ref &value) {
        *this = (bool)value;
        return *this;
    }
    /// Xor assignment.
    inline bit_ref &operator^=(bool value) {
        *byte ^= (uint8_t)value << bit_index;
        return *this;
    }
    /// Bitwise-and assignment.
    inline bit_ref &operator&=(bool value) {
        *byte &= ((uint8_t)value << bit_index) | ~(1 << bit_index);
        return *this;
    }
    /// Bitwise-or assignment.
    inline bit_ref &operator|=(bool value) {
        *byte |= (uint8_t)value << bit_index;
        return *this;
    }
    /// Swap assignment.
    inline void swap_with(bit_ref other) {
        bool b = (bool)other;
        other = (bool)*this;
        *this = b;
    }

    /// Implicit conversion to bool.
    inline operator bool() const {  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
        return (*byte >> bit_index) & 1;
    }
};

}  // namespace stim

#endif
