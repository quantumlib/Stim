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

#include "stim/search/hyper/edge.h"

#include <algorithm>
#include <sstream>

using namespace stim;
using namespace stim::impl_search_hyper;

std::string Edge::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}
bool Edge::operator==(const Edge &other) const {
    return nodes == other.nodes && crossing_observable_mask == other.crossing_observable_mask;
}
bool Edge::operator!=(const Edge &other) const {
    return !(*this == other);
}
std::ostream &stim::impl_search_hyper::operator<<(std::ostream &out, const Edge &v) {
    bool sep = false;
    if (v.nodes.empty()) {
        out << "[silent]";
        sep = true;
    } else if (v.nodes.size() == 1) {
        out << "[boundary]";
        sep = true;
    }
    for (const auto &t : v.nodes) {
        if (sep) {
            out << ' ';
        }
        sep = true;
        out << "D" << t;
    }
    for (size_t k = 0; k < v.crossing_observable_mask.num_bits_padded(); k++) {
        if (v.crossing_observable_mask[k]) {
            if (sep) {
                out << ' ';
            }
            sep = true;
            out << "L" << k;
        }
    }
    return out;
}
