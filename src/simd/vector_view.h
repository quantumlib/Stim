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

#ifndef VECTOR_VIEW_H
#define VECTOR_VIEW_H

#include <sstream>
#include <vector>

/// A pointer to contiguous data inside a std::vector.
///
/// This is used to avoid copies and allocations in contexts where there is a lot of jagged data. The data is aggregated
/// into one vector, and then referenced using views into that vector. For example, it is used by Circuit to store
/// operation target data.
///
/// A notable distinction between a vector view and a raw pointer with size information is that the vector view does not
/// become invalid when appending to the underlying vector (e.g. due to it reallocating its backing buffer).
template <typename T>
struct VectorView {
    std::vector<T> *vec_ptr;
    size_t offset;
    size_t length;

    inline size_t size() const {
        return length;
    }

    inline T operator[](size_t k) const {
        return (*vec_ptr)[offset + k];
    }

    inline T &operator[](size_t k) {
        return (*vec_ptr)[offset + k];
    }

    T *begin() {
        return vec_ptr->data() + offset;
    }

    T *end() {
        return vec_ptr->data() + offset + length;
    }

    const T *begin() const {
        return vec_ptr->data() + offset;
    }

    const T *end() const {
        return vec_ptr->data() + offset + length;
    }

    bool operator==(const VectorView<T> &other) const {
        if (length != other.length) {
            return false;
        }
        for (size_t k = 0; k < length; k++) {
            if ((*this)[k] != other[k]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const VectorView<T> &other) const {
        return !(*this == other);
    }

    /// Lexicographic ordering.
    bool operator<(const VectorView<T> &other) const {
        auto n = std::min(size(), other.size());
        for (size_t k = 0; k < n; k++) {
            if ((*this)[k] != other[k]) {
                return (*this)[k] < other[k];
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
std::ostream &operator<<(std::ostream &out, const VectorView<T> &v) {
    out << "VectorView{";
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

template <typename T>
struct JaggedDataArena {
    std::vector<T> vec;
    JaggedDataArena() : vec() {
    }
    JaggedDataArena(const JaggedDataArena<T> &) = delete;
    JaggedDataArena(JaggedDataArena<T> &&) noexcept = delete;
    JaggedDataArena<T> &operator=(JaggedDataArena<T> &&) noexcept = delete;
    JaggedDataArena<T> &operator=(const JaggedDataArena<T> &) = delete;

    VectorView<T> view(size_t start, size_t size) {
        return {&vec, start, size};
    }
    VectorView<T> tail_view(size_t start) {
        return view(start, vec.size() - start);
    }
    VectorView<T> inserted(const T *data, size_t size) {
        size_t n = vec.size();
        vec.insert(vec.end(), data, data + size);
        return tail_view(n);
    }
    VectorView<T> inserted(const std::vector<T> &items) {
        return inserted(items.data(), items.size());
    }
    VectorView<T> inserted(const VectorView<T> &items) {
        if (items.vec_ptr == &vec) {
            return items;
        }
        return inserted(items.begin(), items.size());
    }
};

#endif
