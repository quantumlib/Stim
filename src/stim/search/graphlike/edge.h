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

#ifndef _STIM_SEARCH_GRAPHLIKE_EDGE_H
#define _STIM_SEARCH_GRAPHLIKE_EDGE_H

#include <cstdint>
#include <iostream>
#include <string>

namespace stim {

namespace impl_search_graphlike {

/// A graphlike edge from a detector error model.
struct Edge {
    uint64_t opposite_node_index;
    uint64_t crossing_observable_mask;
    std::string str() const;
    bool operator==(const Edge &other) const;
    bool operator!=(const Edge &other) const;
};
std::ostream &operator<<(std::ostream &out, const Edge &v);

}  // namespace impl_search_graphlike
}  // namespace stim

#endif
