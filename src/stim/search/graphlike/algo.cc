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

#include "stim/search/graphlike/algo.h"

#include <algorithm>
#include <map>
#include <queue>

#include "stim/search/graphlike/edge.h"
#include "stim/search/graphlike/graph.h"
#include "stim/search/graphlike/node.h"
#include "stim/search/graphlike/search_state.h"

using namespace stim;
using namespace stim::impl_search_graphlike;

DetectorErrorModel backtrack_path(const std::map<SearchState, SearchState> &back_map, SearchState final_state) {
    DetectorErrorModel out;
    auto cur_state = final_state;
    while (true) {
        auto prev_state = back_map.at(cur_state);
        cur_state.append_transition_as_error_instruction_to(prev_state, out);
        if (prev_state.is_undetected()) {
            break;
        }
        cur_state = prev_state;
    }
    std::sort(out.instructions.begin(), out.instructions.end());
    return out;
}

DetectorErrorModel stim::shortest_graphlike_undetectable_logical_error(
    const DetectorErrorModel &model, bool ignore_ungraphlike_errors) {
    Graph graph = Graph::from_dem(model, ignore_ungraphlike_errors);

    if (graph.distance_1_error_mask) {
        DetectorErrorModel out;
        SearchState s1(NO_NODE_INDEX, NO_NODE_INDEX, graph.distance_1_error_mask);
        s1.append_transition_as_error_instruction_to({}, out);
        return out;
    }

    std::queue<SearchState> queue;
    std::map<SearchState, SearchState> back_map;
    // Mark the vacuous dead-end state as already seen.
    back_map.emplace(SearchState(), SearchState());

    // Search starts from any and all edges crossing an observable.
    for (size_t node1 = 0; node1 < graph.nodes.size(); node1++) {
        for (const auto &e : graph.nodes[node1].edges) {
            uint64_t node2 = e.opposite_node_index;
            if (node1 < node2 && e.crossing_observable_mask) {
                SearchState start{node1, node2, e.crossing_observable_mask};
                queue.push(start);
                back_map.emplace(start, SearchState());
            }
        }
    }

    // Breadth first search for a symptomless state that has a frame change.
    for (; !queue.empty(); queue.pop()) {
        SearchState cur = queue.front();
        assert(cur.det_active != NO_NODE_INDEX);
        for (const auto &e : graph.nodes[cur.det_active].edges) {
            SearchState next(e.opposite_node_index, cur.det_held, e.crossing_observable_mask ^ cur.obs_mask);
            if (!back_map.emplace(next, cur).second) {
                continue;
            }
            if (next.is_undetected()) {
                assert(next.obs_mask);  // Otherwise, it would have already been in back_map.
                return backtrack_path(back_map, next);
            } else {
                if (next.det_active == NO_NODE_INDEX) {
                    // Just resolved one out of two excitations. Move on to the second excitation.
                    std::swap(next.det_active, next.det_held);
                }
                queue.push(next);
            }
        }
    }

    throw std::invalid_argument("Failed to find any graphlike logical errors.");
}
