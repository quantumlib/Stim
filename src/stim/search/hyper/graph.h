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

#ifndef _STIM_SEARCH_HYPER_GRAPH_H
#define _STIM_SEARCH_HYPER_GRAPH_H

#include "stim/dem/detector_error_model.h"
#include "stim/search/hyper/node.h"

namespace stim {

namespace impl_search_hyper {

struct Graph {
    std::vector<Node> nodes;
    uint64_t distance_1_error_mask;

    explicit Graph(size_t node_count);
    Graph(std::vector<Node> nodes, uint64_t distance_1_error_mask);

    void add_edge_from_dem_targets(ConstPointerRange<DemTarget> targets, size_t dont_explore_edges_with_degree_above);
    static Graph from_dem(const DetectorErrorModel &model, size_t dont_explore_edges_with_degree_above);
    bool operator==(const Graph &other) const;
    bool operator!=(const Graph &other) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const Graph &v);

}  // namespace impl_search_hyper
}  // namespace stim

#endif
