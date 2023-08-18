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

#ifndef _STIM_MEM_BIT_WORD_H
#define _STIM_MEM_BIT_WORD_H

namespace stim {

/// A `bitword` is a bag of bits that can be operated on in a SIMD-esque fashion
/// by individual CPU instructions.
///
/// This template would not have to exist, except that different architectures and
/// operating systems expose different interfaces between native types like
/// uint64_t and intrinsics like __m256i. For example, in some contexts, __m256i
/// values can be operated on by operators (e.g. you can do `a ^= b`) while in
/// other contexts this does not work. The bitword template implementations define
/// a common set of methods required by stim to function, so that the same code
/// can be compiled to use 256 bit registers or 64 bit registers as appropriate.
template <size_t bit_size>
struct bitword;

template <size_t W>
inline bool operator==(const bitword<W> &self, const bitword<W> &other) {
    return self.to_u64_array() == other.to_u64_array();
}

template <size_t W>
inline bool operator<(const bitword<W> &self, const bitword<W> &other) {
    auto v1 = self.to_u64_array();
    auto v2 = other.to_u64_array();
    for (size_t k = 0; k < v1.size(); k++) {
        if (v1[k] != v2[k]) {
            return v1[k] < v2[k];
        }
    }
    return false;
}

template <size_t W>
inline bool operator!=(const bitword<W> &self, const bitword<W> &other) {
    return !(self == other);
}

template <size_t W>
inline bool operator==(const bitword<W> &self, int other) {
    return self == (bitword<W>)other;
}
template <size_t W>
inline bool operator!=(const bitword<W> &self, int other) {
    return self != (bitword<W>)other;
}
template <size_t W>
inline bool operator==(const bitword<W> &self, uint64_t other) {
    return self == (bitword<W>)other;
}
template <size_t W>
inline bool operator!=(const bitword<W> &self, uint64_t other) {
    return self != (bitword<W>)other;
}
template <size_t W>
inline bool operator==(const bitword<W> &self, int64_t other) {
    return self == (bitword<W>)other;
}
template <size_t W>
inline bool operator!=(const bitword<W> &self, int64_t other) {
    return self != (bitword<W>)other;
}

template <size_t W>
std::ostream &operator<<(std::ostream &out, const bitword<W> &v) {
    out << "bitword<" << W << ">{";
    auto u = v.to_u64_array();
    for (size_t k = 0; k < u.size(); k++) {
        for (size_t b = 0; b < 64; b++) {
            if ((b | k) && (b & 7) == 0) {
                out << ' ';
            }
            out << ".1"[(u[k] >> b) & 1];
        }
    }
    out << '}';
    return out;
}

template <size_t W>
inline bitword<W> operator<<(const bitword<W> &self, uint64_t offset) {
    return self.shifted((int)offset);
}

template <size_t W>
inline bitword<W> operator>>(const bitword<W> &self, uint64_t offset) {
    return self.shifted(-(int)offset);
}

template <size_t W>
inline bitword<W> &operator<<=(bitword<W> &self, uint64_t offset) {
    self = (self << offset);
    return self;
}

template <size_t W>
inline bitword<W> &operator>>=(bitword<W> &self, uint64_t offset) {
    self = (self >> offset);
    return self;
}

template <size_t W>
inline bitword<W> operator<<(const bitword<W> &self, int offset) {
    return self.shifted((int)offset);
}

template <size_t W>
inline bitword<W> operator>>(const bitword<W> &self, int offset) {
    return self.shifted(-(int)offset);
}

template <size_t W>
inline bitword<W> &operator<<=(bitword<W> &self, int offset) {
    self = (self << offset);
    return self;
}

template <size_t W>
inline bitword<W> &operator>>=(bitword<W> &self, int offset) {
    self = (self >> offset);
    return self;
}

template <size_t W>
inline bitword<W> operator&(const bitword<W> &self, int mask) {
    return self & bitword<W>(mask);
}
template <size_t W>
inline bitword<W> operator&(const bitword<W> &self, uint64_t mask) {
    return self & bitword<W>(mask);
}
template <size_t W>
inline bitword<W> operator&(const bitword<W> &self, int64_t mask) {
    return self & bitword<W>(mask);
}
template <size_t W>
inline bitword<W> operator|(const bitword<W> &self, int mask) {
    return self | bitword<W>(mask);
}
template <size_t W>
inline bitword<W> operator|(const bitword<W> &self, uint64_t mask) {
    return self | bitword<W>(mask);
}
template <size_t W>
inline bitword<W> operator|(const bitword<W> &self, int64_t mask) {
    return self | bitword<W>(mask);
}
template <size_t W>
inline bitword<W> operator^(const bitword<W> &self, int mask) {
    return self ^ bitword<W>(mask);
}
template <size_t W>
inline bitword<W> operator^(const bitword<W> &self, uint64_t mask) {
    return self ^ bitword<W>(mask);
}
template <size_t W>
inline bitword<W> operator^(const bitword<W> &self, int64_t mask) {
    return self ^ bitword<W>(mask);
}

}  // namespace stim

#endif
