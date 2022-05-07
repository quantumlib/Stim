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

#ifndef _STIM_SEARCH_GRAPHLIKE_GRAPH_H
#define _STIM_SEARCH_GRAPHLIKE_GRAPH_H

#include "stim/dem/detector_error_model.h"
#include "stim/search/graphlike/node.h"

namespace stim {

namespace impl_search_graphlike {

struct Graph {
    std::vector<Node> nodes;
    uint64_t distance_1_error_mask;

    explicit Graph(size_t node_count);
    Graph(std::vector<Node> nodes, uint64_t distance_1_error_mask);

    void add_outward_edge(size_t src, uint64_t dst, uint64_t obs_mask);
    void add_edges_from_targets_with_no_separators(
        ConstPointerRange<DemTarget> targets, bool ignore_ungraphlike_errors);
    void add_edges_from_separable_targets(ConstPointerRange<DemTarget> targets, bool ignore_ungraphlike_errors);
    static Graph from_dem(const DetectorErrorModel &model, bool ignore_ungraphlike_errors);
    bool operator==(const Graph &other) const;
    bool operator!=(const Graph &other) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const Graph &v);

}  // namespace impl_search_graphlike
}  // namespace stim

#endif
