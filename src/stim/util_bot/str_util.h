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

#ifndef _STIM_STR_UTIL_H
#define _STIM_STR_UTIL_H

#include <ostream>
#include <sstream>
#include <string>

namespace stim {
/// Wraps an iterable object so that its values are printed with comma separators.
template <typename TIter>
struct CommaSep;

/// A wrapper indicating a range of values should be printed with comma separators.
template <typename TIter>
struct CommaSep {
    const TIter &iter;
    const char *sep;
    std::string str() const {
        std::stringstream out;
        out << *this;
        return out.str();
    }
};

template <typename TIter>
CommaSep<TIter> comma_sep(const TIter &v, const char *sep = ", ") {
    return CommaSep<TIter>{v, sep};
}

template <typename TIter>
std::ostream &operator<<(std::ostream &out, const CommaSep<TIter> &v) {
    bool first = true;
    for (const auto &t : v.iter) {
        if (first) {
            first = false;
        } else {
            out << v.sep;
        }
        out << t;
    }
    return out;
}

}  // namespace stim

#endif
