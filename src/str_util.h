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

#ifndef STIM_STR_UTIL_H
#define STIM_STR_UTIL_H

#include <sstream>
#include <string>
#include <ostream>

#include "str_util_forward_declaration.h"

template <typename TIter>
std::string stim_internal::CommaSep<TIter>::str() const {
    std::stringstream out;
    out << *this;
    return out.str();
}

/// Wraps an iterable object so that its values are printed with comma separators.
template <typename TIter>
stim_internal::CommaSep<TIter> stim_internal::comma_sep(const TIter &v) {
    return CommaSep<TIter>{v};
}

template <typename TIter>
std::ostream &operator<<(std::ostream &out, const stim_internal::CommaSep<TIter> &v) {
    bool first = true;
    for (const auto &t : v.iter) {
        if (first) {
            first = false;
        } else {
            out << ", ";
        }
        out << t;
    }
    return out;
}

#endif
