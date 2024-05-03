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

#ifndef _STIM_SEARCH_GRAPHLIKE_NODE_H
#define _STIM_SEARCH_GRAPHLIKE_NODE_H

#include <vector>

#include "stim/search/graphlike/edge.h"

namespace stim {

namespace impl_search_graphlike {

constexpr uint64_t NO_NODE_INDEX = UINT64_MAX;

/// Adjacency list representation of a detector from a detector error model.
/// Only includes graphlike edges.
struct Node {
    std::vector<Edge> edges;
    std::string str() const;
    bool operator==(const Node &other) const;
    bool operator!=(const Node &other) const;
};
std::ostream &operator<<(std::ostream &out, const Node &v);

}  // namespace impl_search_graphlike
}  // namespace stim

#endif
