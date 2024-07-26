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

#ifndef _STIM_MEM_SPAN_REF_H
#define _STIM_MEM_SPAN_REF_H

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace stim {

/// Exposes a collection-like interface to a range of memory delineated by start/end pointers.
///
/// A major difference between the semantics of this class and the std::span class added in C++20 is that this class
/// defines equality and ordering operators that depend on *what is pointed to* instead of the value of the pointers
/// themselves. Two range refs aren't equal because they have the same pointers, they're equal because they point
/// to ranges that have the same contents. In other words, this class really acts more like a *reference* than like a
/// pointer.
template <typename T>
struct SpanRef {
    T *ptr_start;
    T *ptr_end;
    SpanRef() : ptr_start(nullptr), ptr_end(nullptr) {
    }
    SpanRef(T *begin, T *end) : ptr_start(begin), ptr_end(end) {
    }

    // Implicit conversions.
    SpanRef(T *singleton) : ptr_start(singleton), ptr_end(singleton + 1) {
    }
    SpanRef(const SpanRef<typename std::remove_const<T>::type> &other)
        : ptr_start(other.ptr_start), ptr_end(other.ptr_end) {
    }
    SpanRef(std::span<T> &items) : ptr_start(items.data()), ptr_end(items.data() + items.size()) {
    }
    SpanRef(const std::span<typename std::remove_const<T>::type> &items)
        : ptr_start(items.data()), ptr_end(items.data() + items.size()) {
    }
    SpanRef(std::vector<T> &items) : ptr_start(items.data()), ptr_end(items.data() + items.size()) {
    }
    SpanRef(const std::vector<typename std::remove_const<T>::type> &items)
        : ptr_start(items.data()), ptr_end(items.data() + items.size()) {
    }
    template <size_t K>
    SpanRef(std::array<T, K> &items) : ptr_start(items.data()), ptr_end(items.data() + items.size()) {
    }
    template <size_t K>
    SpanRef(const std::array<typename std::remove_const<T>::type, K> &items)
        : ptr_start(items.data()), ptr_end(items.data() + items.size()) {
    }

    operator std::span<T>() {
        return {ptr_start, ptr_end};
    }
    operator std::span<const T>() const {
        return {ptr_start, ptr_end};
    }

    SpanRef sub(size_t start_offset, size_t end_offset) const {
        return SpanRef<T>(ptr_start + start_offset, ptr_start + end_offset);
    }

    size_t size() const {
        return ptr_end - ptr_start;
    }
    const T *begin() const {
        return ptr_start;
    }
    const T *end() const {
        return ptr_end;
    }
    const T &back() const {
        return *(ptr_end - 1);
    }
    const T &front() const {
        return *ptr_start;
    }
    bool empty() const {
        return ptr_end == ptr_start;
    }
    T *begin() {
        return ptr_start;
    }
    T *end() {
        return ptr_end;
    }
    T &back() {
        return *(ptr_end - 1);
    }
    T &front() {
        return *ptr_start;
    }
    const T &operator[](size_t index) const {
        return ptr_start[index];
    }
    T &operator[](size_t index) {
        return ptr_start[index];
    }

    bool operator==(const SpanRef<const T> &other) const {
        size_t n = size();
        if (n != other.size()) {
            return false;
        }
        for (size_t k = 0; k < n; k++) {
            if (ptr_start[k] != other[k]) {
                return false;
            }
        }
        return true;
    }
    bool operator==(const SpanRef<typename std::remove_const<T>::type> &other) const {
        return SpanRef<const T>(ptr_start, ptr_end) == SpanRef<const T>(other.ptr_start, other.ptr_end);
    }
    bool operator!=(const SpanRef<const T> &other) const {
        return !(*this == other);
    }
    bool operator!=(const SpanRef<typename std::remove_const<T>::type> &other) const {
        return !(*this == other);
    }

    std::string str() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

    /// Lexicographic ordering.
    bool operator<(const SpanRef<const T> &other) const {
        auto n = std::min(size(), other.size());
        for (size_t k = 0; k < n; k++) {
            if ((*this)[k] != other[k]) {
                return (*this)[k] < other[k];
            }
        }
        return size() < other.size();
    }
    bool operator<(const SpanRef<typename std::remove_const<T>::type> &other) const {
        return SpanRef<const T>(ptr_start, ptr_end) < SpanRef<const T>(other.ptr_start, other.ptr_end);
    }
};

template <typename T>
std::ostream &operator<<(std::ostream &out, const stim::SpanRef<T> &v) {
    bool first = false;
    out << "SpanRef{";
    for (const auto &item : v) {
        if (!first) {
            first = true;
        } else {
            out << ", ";
        }
        out << item;
    }
    out << "}";
    return out;
}

}  // namespace stim

#endif
