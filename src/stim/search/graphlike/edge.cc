// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/search/graphlike/edge.h"

#include <algorithm>
#include <sstream>

#include "stim/search/graphlike/node.h"

using namespace stim;
using namespace stim::impl_search_graphlike;

std::string Edge::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}
bool Edge::operator==(const Edge &other) const {
    return opposite_node_index == other.opposite_node_index &&
           crossing_observable_mask == other.crossing_observable_mask;
}
bool Edge::operator!=(const Edge &other) const {
    return !(*this == other);
}
std::ostream &stim::impl_search_graphlike::operator<<(std::ostream &out, const Edge &v) {
    if (v.opposite_node_index == NO_NODE_INDEX) {
        out << "[boundary]";
    } else {
        out << "D" << v.opposite_node_index;
    }
    auto m = v.crossing_observable_mask;
    for (size_t k = 0; k < m.num_bits_padded(); k++) {
        if (m[k]) {
            out << " L" << k;
        }
    }
    return out;
}
