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

#ifndef _STIM_MEM_SPARSE_XOR_VEC_H
#define _STIM_MEM_SPARSE_XOR_VEC_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <sstream>
#include <vector>

#include "stim/mem/monotonic_buffer.h"
#include "stim/str_util.h"

namespace stim {

/// Merges the elements of two sorted buffers into an output buffer while cancelling out duplicate items.
///
/// \param sorted_in1: Pointer range covering the first sorted list.
/// \param sorted_in2: Pointer range covering the second sorted list.
/// \param out: Where to write the output. Must have size of at least sorted_in1.size() + sorted_in2.size().
/// \return: A pointer to the end of the output (one past the last place written).
template <typename T>
inline T *xor_merge_sort(ConstPointerRange<T> sorted_in1, ConstPointerRange<T> sorted_in2, T *out) {
    // Interleave sorted src and dst into a sorted work buffer.
    const T *p1 = sorted_in1.ptr_start;
    const T *p2 = sorted_in2.ptr_start;
    const T *end1 = sorted_in1.ptr_end;
    const T *end2 = sorted_in2.ptr_end;
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

template <typename T>
bool is_subset_of_sorted(ConstPointerRange<T> subset, ConstPointerRange<T> superset) {
    const T *p_sub = subset.ptr_start;
    const T *p_sup = superset.ptr_start;
    const T *end_sub = subset.ptr_end;
    const T *end_sup = superset.ptr_end;
    while (p_sub != end_sub) {
        if (p_sup == end_sup || *p_sub < *p_sup) {
            return false;
        } else if (*p_sup < *p_sub) {
            p_sup++;
        } else {
            // Same value in both lists. Cancels itself out.
            p_sub++;
            p_sup++;
        }
    }
    return true;
}

template <typename T, typename CALLBACK>
inline void xor_merge_sort_temp_buffer_callback(
    ConstPointerRange<T> sorted_items_1, ConstPointerRange<T> sorted_items_2, CALLBACK handler) {
    constexpr size_t STACK_SIZE = 64;
    T data[STACK_SIZE];
    size_t max = sorted_items_1.size() + sorted_items_2.size();
    T *begin = max > STACK_SIZE ? new T[max] : &data[0];
    T *end = xor_merge_sort(sorted_items_1, sorted_items_2, begin);
    handler(ConstPointerRange<T>(begin, end));
    if (max > STACK_SIZE) {
        delete[] begin;
    }
}

template <typename T>
struct SparseXorVec;

template <typename T>
std::ostream &operator<<(std::ostream &out, const SparseXorVec<T> &v);

/// A sparse set of integers that supports efficient xoring (computing the symmetric difference).
template <typename T>
struct SparseXorVec {
   public:
    // Sorted list of entries.
    std::vector<T> sorted_items;

    SparseXorVec() = default;
    SparseXorVec(std::vector<T> &&vec) : sorted_items(std::move(vec)) {
    }

    bool empty() const {
        return sorted_items.empty();
    }

    void set_to_xor_merge_sort(ConstPointerRange<T> sorted_items1, ConstPointerRange<T> sorted_items2) {
        sorted_items.resize(sorted_items1.size() + sorted_items2.size());
        auto written = xor_merge_sort(sorted_items, sorted_items1, sorted_items2);
        sorted_items.resize(written.size());
    }

    bool is_superset_of(ConstPointerRange<T> subset) const {
        return is_subset_of_sorted(subset, (ConstPointerRange<T>)sorted_items);
    }

    bool contains(const T &item) const {
        return std::find(sorted_items.begin(), sorted_items.end(), item) != sorted_items.end();
    }

    void xor_sorted_items(ConstPointerRange<T> sorted) {
        xor_merge_sort_temp_buffer_callback(range(), sorted, [&](ConstPointerRange<T> result) {
            sorted_items.clear();
            sorted_items.insert(sorted_items.end(), result.begin(), result.end());
        });
    }

    void clear() {
        sorted_items.clear();
    }

    void xor_item(const T &item) {
        xor_sorted_items({&item});
    }

    SparseXorVec &operator^=(const SparseXorVec<T> &other) {
        xor_sorted_items(other.range());
        return *this;
    }

    SparseXorVec operator^(const SparseXorVec<T> &other) const {
        SparseXorVec result;
        result.sorted_items.resize(size() + other.size());
        auto n = xor_merge_sort<T>(range(), other.range(), result.begin()) - result.begin();
        result.sorted_items.resize(n);
        return result;
    }

    SparseXorVec operator^(const T &other) const {
        return xor_helper(&other, 1);
    }

    bool operator<(const SparseXorVec<T> &other) const {
        return range() < other.range();
    }

    inline size_t size() const {
        return sorted_items.size();
    }

    inline T *begin() {
        return sorted_items.data();
    }

    inline T *end() {
        return sorted_items.data() + size();
    }

    inline const T *begin() const {
        return sorted_items.data();
    }

    inline const T *end() const {
        return sorted_items.data() + size();
    }

    bool operator==(const SparseXorVec &other) const {
        return sorted_items == other.sorted_items;
    }

    bool operator!=(const SparseXorVec &other) const {
        return sorted_items != other.sorted_items;
    }

    ConstPointerRange<T> range() const {
        return {begin(), end()};
    }

    std::string str() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

    void check_invariants() const {
        for (size_t k = 1; k < sorted_items.size(); k++) {
            if (!(sorted_items[k - 1] < sorted_items[k])) {
                throw std::invalid_argument(str() + " is not unique and sorted.");
            }
        }
    }
};

template <typename T>
std::ostream &operator<<(std::ostream &out, const SparseXorVec<T> &v) {
    out << "SparseXorVec{" << comma_sep(v) << "}";
    return out;
}

}  // namespace stim

#endif
