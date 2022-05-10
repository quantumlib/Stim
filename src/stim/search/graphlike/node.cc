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

#include "stim/search/graphlike/node.h"

#include <queue>
#include <sstream>

using namespace stim;
using namespace stim::impl_search_graphlike;

constexpr uint64_t NO_NODE_INDEX = UINT64_MAX;

std::string Node::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}

bool Node::operator==(const Node &other) const {
    return edges == other.edges;
}

bool Node::operator!=(const Node &other) const {
    return !(*this == other);
}

std::ostream &stim::impl_search_graphlike::operator<<(std::ostream &out, const Node &v) {
    for (const auto &e : v.edges) {
        out << "    " << e << "\n";
    }
    return out;
}
