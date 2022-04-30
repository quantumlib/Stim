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

#include "stim/search/graphlike/graph.h"

#include <algorithm>
#include <map>
#include <queue>

using namespace stim;
using namespace stim::impl_search_graphlike;

std::string Graph::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}

void Graph::add_outward_edge(size_t src, uint64_t dst, uint64_t obs_mask) {
    assert(src < nodes.size());
    auto &node = nodes[src];

    // Don't add duplicate edges.
    // Note: the neighbor list is expected to be short, so we do a linear scan instead of e.g. a binary search.
    for (const auto &e : node.edges) {
        if (e.opposite_node_index == dst && e.crossing_observable_mask == obs_mask) {
            return;
        }
    }

    node.edges.push_back({dst, obs_mask});
}

void Graph::add_edges_from_targets_with_no_separators(
    ConstPointerRange<DemTarget> targets, bool ignore_ungraphlike_errors) {
    FixedCapVector<uint64_t, 2> detectors;
    uint64_t obs_mask = 0;

    // Collect detectors and observables.
    for (const auto &t : targets) {
        if (t.is_relative_detector_id()) {
            if (detectors.size() == 2) {
                if (ignore_ungraphlike_errors) {
                    return;
                }
                throw std::invalid_argument(
                    "The detector error model contained a non-graphlike error mechanism.\n"
                    "You can ignore such errors using `ignore_ungraphlike_errors`.\n"
                    "You can use `decompose_errors` when converting a circuit into a model "
                    "to ensure no such errors are present.\n");
            }
            detectors.push_back(t.raw_id());
        } else if (t.is_observable_id()) {
            obs_mask ^= 1ULL << t.raw_id();
        }
    }

    // Add edges between detector nodes.
    if (detectors.size() == 1) {
        add_outward_edge(detectors[0], NO_NODE_INDEX, obs_mask);
    } else if (detectors.size() == 2) {
        add_outward_edge(detectors[0], detectors[1], obs_mask);
        add_outward_edge(detectors[1], detectors[0], obs_mask);
    } else if (detectors.empty() && obs_mask && distance_1_error_mask == 0) {
        distance_1_error_mask = obs_mask;
    }
}

void Graph::add_edges_from_separable_targets(ConstPointerRange<DemTarget> targets, bool ignore_ungraphlike_errors) {
    const DemTarget *prev = targets.begin();
    const DemTarget *cur = targets.begin();
    while (true) {
        if (cur == targets.end() || cur->is_separator()) {
            add_edges_from_targets_with_no_separators({prev, cur}, ignore_ungraphlike_errors);
            prev = cur + 1;
        }
        if (cur == targets.end()) {
            break;
        }
        cur++;
    }
}

Graph Graph::from_dem(const DetectorErrorModel &model, bool ignore_ungraphlike_errors) {
    if (model.count_observables() > 64) {
        throw std::invalid_argument(
            "NotImplemented: shortest_graphlike_undetectable_logical_error with more than 64 observables.");
    }

    Graph result(model.count_detectors());
    model.iter_flatten_error_instructions([&](const DemInstruction &e) {
        if (e.arg_data[0] != 0) {
            result.add_edges_from_separable_targets(e.target_data, ignore_ungraphlike_errors);
        }
    });
    return result;
}
bool Graph::operator==(const Graph &other) const {
    return nodes == other.nodes && distance_1_error_mask == other.distance_1_error_mask;
}
bool Graph::operator!=(const Graph &other) const {
    return !(*this == other);
}

Graph::Graph(size_t node_count) : nodes(node_count), distance_1_error_mask(0) {
}

Graph::Graph(std::vector<Node> nodes, uint64_t distance_1_error_mask)
    : nodes(std::move(nodes)), distance_1_error_mask(distance_1_error_mask) {
}

std::ostream &stim::impl_search_graphlike::operator<<(std::ostream &out, const Graph &v) {
    for (size_t k = 0; k < v.nodes.size(); k++) {
        out << k << ":\n" << v.nodes[k];
    }
    return out;
}
