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

#include "vector_view.h"

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
    auto *end1 = p1 + n1;
    auto *end2 = p2 + n2;
    while (p1 != end1) {
        if (p2 == end2 || *p1 < *p2) {
            *out++ = *p1++;
        } else if (*p2 < *p1) {
            *out++ = *p2++;
        } else {
            // Same value in both lists. Cancels itself out.
            p1++;
            p2++;
        }
    }
    while (p2 != end2) {
        *out++ = *p2++;
    }
    return out;
}

// HACK: this should be templated, but it's not in order to have compatibility with C++11.
static std::vector<uint32_t> _shared_buf;

template <typename T>
inline void vector_tail_view_xor_in_place(VectorView<T> &buf, const T *p2, size_t n2) {
    size_t max = buf.size() + n2;
    if (_shared_buf.size() < max) {
        _shared_buf.resize(2 * max);
    }
    auto end = xor_merge_sorted_items_into<T>(buf.begin(), buf.size(), p2, n2, _shared_buf.data());
    buf.length = end - _shared_buf.data();
    buf.vec_ptr->resize(buf.offset);
    buf.vec_ptr->insert(buf.vec_ptr->end(), _shared_buf.data(), _shared_buf.data() + buf.length);
}

template <typename T>
inline void xor_into_vector_tail_view(VectorView<T> &buf, const T *p1, size_t n1, const T *p2, size_t n2) {
    buf.vec_ptr->resize(buf.offset + n1 + n2);
    auto end = xor_merge_sorted_items_into<T>(p1, n1, p2, n2, buf.begin());
    buf.length = end - buf.begin();
    buf.vec_ptr->resize(buf.offset + buf.length);
}

/// A sparse set of integers that supports efficient xoring (computing the symmetric difference).
template <typename T>
struct SparseXorVec {
   private:
    inline SparseXorVec xor_helper(const T *src_ptr, size_t src_size) const {
        SparseXorVec result;
        result.vec.resize(size() + src_size);
        auto n = xor_merge_sorted_items_into<T>(begin(), size(), src_ptr, src_size, result.begin()) - result.begin();
        result.vec.resize(n);
        return result;
    }

   public:
    std::vector<T> vec;

    inline void inplace_xor_helper(const T *src_ptr, size_t src_size) {
        VectorView<uint32_t> view{&vec, 0, vec.size()};
        vector_tail_view_xor_in_place(view, src_ptr, src_size);
    }
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

    bool operator<(const SparseXorVec<T> &other) const {
        return view() < other.view();
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

    std::string str() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

    const VectorView<T> view() const {
        // Temporarily remove const correctness but then immediately restore it.
        return VectorView<T>{(std::vector<uint32_t> *)&vec, 0, vec.size()};
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
