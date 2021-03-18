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

#ifndef POINTER_RANGE_H
#define POINTER_RANGE_H

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

/// Delineates mutable memory using an inclusive start pointer and exclusive end pointer.
template <typename T>
struct PointerRange {
    T *ptr_start;
    T *ptr_end;
    PointerRange() : ptr_start(nullptr), ptr_end(nullptr) {
    }
    PointerRange(T *begin, T *end) : ptr_start(begin), ptr_end(end) {
    }
    // Implicit conversions.
    PointerRange(std::vector<T> &items) : ptr_start(items.data()), ptr_end(items.data() + items.size()) {
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
    T *begin() {
        return ptr_start;
    }
    T *end() {
        return ptr_end;
    }
    const T &operator[](size_t index) const {
        return ptr_start[index];
    }
    T &operator[](size_t index) {
        return ptr_start[index];
    }

    bool operator==(const PointerRange<T> &other) const {
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
    bool operator!=(const PointerRange<T> &other) const {
        return !(*this == other);
    }

    std::string str() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

    /// Lexicographic ordering.
    bool operator<(const PointerRange<T> &other) const {
        auto n = std::min(size(), other.size());
        for (size_t k = 0; k < n; k++) {
            if ((*this)[k] != other[k]) {
                return (*this)[k] < other[k];
            }
        }
        return size() < other.size();
    }
};

/// Delineates readable memory using an inclusive start pointer and exclusive end pointer.
template <typename T>
struct ConstPointerRange {
    const T *ptr_start;
    const T *ptr_end;

    ConstPointerRange() : ptr_start(nullptr), ptr_end(nullptr) {
    }
    ConstPointerRange(const T *begin, const T *end) : ptr_start(begin), ptr_end(end) {
    }
    // Implicit conversions.
    ConstPointerRange(PointerRange<T> items) : ptr_start(items.ptr_start), ptr_end(items.ptr_end) {
    }
    ConstPointerRange(const std::vector<T> &items) : ptr_start(items.data()), ptr_end(items.data() + items.size()) {
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
    const T &operator[](size_t index) const {
        return ptr_start[index];
    }

    bool operator==(const ConstPointerRange<T> &other) const {
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
    bool operator!=(const ConstPointerRange<T> &other) const {
        return !(*this == other);
    }

    std::string str() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

    /// Lexicographic ordering.
    bool operator<(const ConstPointerRange<T> &other) const {
        auto n = std::min(size(), other.size());
        for (size_t k = 0; k < n; k++) {
            if ((*this)[k] != other[k]) {
                return (*this)[k] < other[k];
            }
        }
        return size() < other.size();
    }
};

template <typename T>
std::ostream &operator<<(std::ostream &out, ConstPointerRange<T> v) {
    out << "PointerRange{";
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
