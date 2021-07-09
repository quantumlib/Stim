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

#ifndef STIM_STR_UTIL_FORWARD_DECLARATION_H
#define STIM_STR_UTIL_FORWARD_DECLARATION_H

#include <sstream>
#include <string>
#include <ostream>

namespace stim_internal {
/// A wrapper indicating a range of values should be printed with comma separators.
template <typename TIter>
struct CommaSep {
    const TIter &iter;
    std::string str() const;
};

template <typename TIter>
CommaSep<TIter> comma_sep(const TIter &v);

}  // namespace stim_internal

template <typename TIter>
std::ostream &operator<<(std::ostream &out, const stim_internal::CommaSep<TIter> &v);

#endif
