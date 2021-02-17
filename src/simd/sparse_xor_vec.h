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

#ifndef SPARSE_XOR_TABLE_H
#define SPARSE_XOR_TABLE_H

#include <cstdint>
#include <sstream>
#include <vector>

/// Merge sorts the elements of two sorted buffers into an output buffer while cancelling out duplicate items.
///
/// \param p1: Pointer to the first input buffer.
/// \param n1: Number of items in the first input buffer.
/// \param p2: Pointer to the second input buffer.
/// \param n2: Number of items in the second input buffer.
/// \param out: Pointer to the output buffer. The output buffer must have a size of at least n1+n2.
/// \return: The (exclusive) end pointer of the written part of the output buffer.
template <typename T>
inline T *xor_merge_sorted_items_into(const T *p1, size_t n1, const T *p2, size_t n2, T *out) {
    // Interleave sorted src and dst into a sorted work buffer.
    auto *end_dst = p2 + n2;
    auto *end_src = p1 + n1;
    while (p1 != end_src) {
        if (p2 == end_dst || *p1 < *p2) {
            *out++ = *p1++;
        } else if (*p2 < *p1) {
            *out++ = *p2++;
        } else {
            // Same value in both lists. Cancels itself out.
            p1++;
            p2++;
        }
    }
    while (p2 != end_dst) {
        *out++ = *p2++;
    }
    return out;
}

// HACK: not templated for compatibility with C++11.
static std::vector<uint32_t> _shared_buf;

/// A sparse set of integers that supports efficient xoring (computing the symmetric difference).
template <typename T>
struct SparseXorVec {
   private:
    inline void inplace_xor_helper(const T *src_ptr, size_t src_size) {
        size_t max = size() + src_size;
        if (_shared_buf.size() < max) {
            _shared_buf.resize(2 * max);
        }
        auto end = xor_merge_sorted_items_into<T>(begin(), size(), src_ptr, src_size, _shared_buf.data());
        vec.clear();
        vec.insert(vec.begin(), _shared_buf.data(), end);
    }
    inline SparseXorVec xor_helper(const T *src_ptr, size_t src_size) const {
        SparseXorVec result;
        result.vec.resize(size() + src_size);
        auto n = xor_merge_sorted_items_into<T>(begin(), size(), src_ptr, src_size, result.begin()) - result.begin();
        result.vec.erase(result.vec.begin() + n, result.vec.end());
        return result;
    }

   public:
    std::vector<T> vec;

    SparseXorVec &operator^=(const T &other) {
        inplace_xor_helper(&other, 1);
        return *this;
    }

    SparseXorVec &operator^=(const SparseXorVec &other) {
        inplace_xor_helper(other.begin(), other.size());
        return *this;
    }

    SparseXorVec operator^(const SparseXorVec &other) const {
        return xor_helper(other.begin(), other.size());
    }

    SparseXorVec operator^(const T &other) const {
        return xor_helper(&other, 1);
    }

    inline size_t size() const {
        return vec.size();
    }

    inline T *begin() {
        return vec.data();
    }

    inline T *end() {
        return vec.data() + size();
    }

    inline const T *begin() const {
        return vec.data();
    }

    inline const T *end() const {
        return vec.data() + size();
    }

    bool operator==(const SparseXorVec &other) const {
        return vec == other.vec;
    }

    bool operator!=(const SparseXorVec &other) const {
        return vec != other.vec;
    }

    bool operator<(const SparseXorVec &other) const {
        auto n = std::min(size(), other.size());
        for (size_t k = 0; k < n; k++) {
            if (vec[k] != other.vec[k]) {
                return vec[k] < other.vec[k];
            }
        }
        return size() < other.size();
    }

    std::string str() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
};

template <typename T>
std::ostream &operator<<(std::ostream &out, const SparseXorVec<T> &v) {
    out << "SparseXorVec{";
    bool first = true;
    for (auto &e : v) {
        if (!first) {
            out << ", ";
        }
        first = false;
        out << e;
    }
    out << "}";
    return out;
}

#endif
