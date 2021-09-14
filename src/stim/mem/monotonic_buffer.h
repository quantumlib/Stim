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

#ifndef _STIM_MEM_MONOTONIC_BUFFER_H
#define _STIM_MEM_MONOTONIC_BUFFER_H

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "stim/mem/pointer_range.h"

namespace stim {

/// A memory resource that can efficiently incrementally accumulate data.
///
/// There are three important types of "region" in play: the tail region, the current region, and old regions.
///
/// The tail is for contiguous data being added incrementally into the buffer.
/// When the tail grows beyond the currently available storage, more memory is allocated and the tail is is copied into
/// the new memory so that it can stay contiguous. At any time, the tail can be discarded or committed. Discarding the
/// tail allows the memory it was covering to be re-used when writing the next tail. Committing the tail permanently
/// preserves that data (until the monotonic buffer is cleared or deconstructed) and also guarantees it will no longer
/// move so pointers to it can be stored.
///
/// The current region is a contiguous chunk of memory that the tail is being written into.
/// When the tail grows beyond this region and triggers an allocation, the current region is relabelled as an old region
/// and the newly allocated memory is now the current region. Each subsequent current region will be at least double the
/// size of the previous one.
///
/// The old regions are memory that has been finalized, and will be stored until the buffer is cleared or deconstructed.
template <typename T>
struct MonotonicBuffer {
    /// Contiguous memory that is being appended to, but has not yet been committed.
    PointerRange<T> tail;
    /// The current contiguous memory region with a mix of committed, staged, and unused memory.
    PointerRange<T> cur;
    /// Old contiguous memory regions that have been committed and now need to be kept.
    std::vector<PointerRange<T>> old_areas;

    /// Constructs an empty monotonic buffer.
    MonotonicBuffer() = default;
    /// Constructs an empty monotonic buffer with initial capacity for its current region.
    MonotonicBuffer(size_t reserve) {
        ensure_available(reserve);
    }
    ~MonotonicBuffer() {
        for (auto &v : old_areas) {
            free(v.ptr_start);
        }
        if (cur.ptr_start) {
            free(cur.ptr_start);
        }
        old_areas.clear();
        cur.ptr_start = cur.ptr_end = tail.ptr_start = tail.ptr_end = nullptr;
    }
    MonotonicBuffer(MonotonicBuffer &&other) noexcept
        : tail(other.tail), cur(other.cur), old_areas(std::move(other.old_areas)) {
        other.cur.ptr_start = nullptr;
        other.cur.ptr_end = nullptr;
        other.tail.ptr_start = nullptr;
        other.tail.ptr_end = nullptr;
    }
    MonotonicBuffer(const MonotonicBuffer &other) = delete;
    MonotonicBuffer &operator=(MonotonicBuffer &&other) noexcept {
        (*this).~MonotonicBuffer();
        new (this) MonotonicBuffer(std::move(other));
        return *this;
    }

    /// Invalidates all previous data and resets the class into a clean state.
    ///
    /// Happens to keep the current contiguous memory region and free old regions.
    void clear() {
        for (auto &v : old_areas) {
            free(v.ptr_start);
        }
        old_areas.clear();
        tail.ptr_end = tail.ptr_start = cur.ptr_start;
    }

    /// Returns the size of memory allocated and held by this monotonic buffer (in units of sizeof(T)).
    size_t total_allocated() const {
        size_t result = cur.size();
        for (auto &old : old_areas) {
            result += old.size();
        }
        return result;
    }

    /// Appends and commits data.
    /// Requires the tail to be empty, to avoid bugs where previously staged data is committed.
    PointerRange<T> take_copy(ConstPointerRange<T> data) {
        assert(tail.size() == 0);
        append_tail(data);
        return commit_tail();
    }

    /// Adds a staged data item.
    void append_tail(T item) {
        ensure_available(1);
        *tail.ptr_end = item;
        tail.ptr_end++;
    }

    /// Adds staged data.
    void append_tail(ConstPointerRange<T> data) {
        ensure_available(data.size());
        std::copy(data.begin(), data.end(), tail.ptr_end);
        tail.ptr_end += data.size();
    }

    /// Throws away staged data, so its memory can be re-used.
    void discard_tail() {
        tail.ptr_end = tail.ptr_start;
    }

    /// Changes staged data into committed data that will be kept until the buffer is cleared or deconstructed.
    PointerRange<T> commit_tail() {
        PointerRange<T> result(tail);
        tail.ptr_start = tail.ptr_end;
        return result;
    }

    /// Ensures it is possible to stage at least `min_required` more items without more reallocations.
    void ensure_available(size_t min_required) {
        size_t available = cur.ptr_end - tail.ptr_end;
        if (available >= min_required) {
            return;
        }

        size_t alloc_count = std::max(min_required, cur.size() << 1);
        if (cur.ptr_start != nullptr) {
            old_areas.push_back(cur);
        }
        cur.ptr_start = (T *)malloc(alloc_count * sizeof(T));
        cur.ptr_end = cur.ptr_start + alloc_count;

        // Staged data is not complete yet; keep it contiguous by copying it to the new larger memory region.
        size_t tail_size = tail.size();
        if (tail_size) {
            std::move(tail.ptr_start, tail.ptr_end, cur.ptr_start);
        }

        tail = {cur.ptr_start, cur.ptr_start + tail_size};
    }
};

}  // namespace stim

#endif
