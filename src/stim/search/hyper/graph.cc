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

#include "stim/search/hyper/graph.h"

#include <algorithm>
#include <map>
#include <queue>

using namespace stim;
using namespace stim::impl_search_hyper;

std::string Graph::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}

void Graph::add_edge_from_dem_targets(
    ConstPointerRange<DemTarget> targets, size_t dont_explore_edges_with_degree_above) {
    Edge edge{{}, 0};
    for (const auto &t : targets) {
        if (t.is_relative_detector_id()) {
            edge.nodes.xor_item(t.val());
        } else if (t.is_observable_id()) {
            edge.crossing_observable_mask ^= 1 << t.val();
        }
    }
    if (edge.nodes.size() > dont_explore_edges_with_degree_above) {
        return;
    }
    if (edge.nodes.empty() && edge.crossing_observable_mask) {
        distance_1_error_mask = edge.crossing_observable_mask;
    }
    for (const auto &n : edge.nodes) {
        nodes[n].edges.push_back(edge);
    }
}

Graph Graph::from_dem(const DetectorErrorModel &model, size_t dont_explore_edges_with_degree_above) {
    if (model.count_observables() > 64) {
        throw std::invalid_argument("NotImplemented: detector error model has more than 64 observables.");
    }

    Graph result(model.count_detectors());
    model.iter_flatten_error_instructions([&](const DemInstruction &e) {
        if (e.arg_data[0] != 0) {
            result.add_edge_from_dem_targets(e.target_data, dont_explore_edges_with_degree_above);
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

std::ostream &stim::impl_search_hyper::operator<<(std::ostream &out, const Graph &v) {
    for (size_t k = 0; k < v.nodes.size(); k++) {
        out << k << ":\n" << v.nodes[k];
    }
    return out;
}
