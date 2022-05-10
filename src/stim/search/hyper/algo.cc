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

#include "stim/search/hyper/algo.h"

#include <algorithm>
#include <map>
#include <queue>

#include "stim/search/graphlike/algo.h"
#include "stim/search/hyper/edge.h"
#include "stim/search/hyper/graph.h"
#include "stim/search/hyper/search_state.h"

using namespace stim;
using namespace stim::impl_search_hyper;

DetectorErrorModel backtrack_path(const std::map<SearchState, SearchState> &back_map, SearchState final_state) {
    DetectorErrorModel out;
    SearchState cur_state = std::move(final_state);
    while (true) {
        auto prev_state = back_map.at(cur_state);
        cur_state.append_transition_as_error_instruction_to(prev_state, out);
        if (prev_state.dets.empty()) {
            break;
        }
        cur_state = prev_state;
    }
    std::sort(out.instructions.begin(), out.instructions.end());
    return out;
}

DetectorErrorModel stim::find_undetectable_logical_error(
    const DetectorErrorModel &model,
    size_t dont_explore_detection_event_sets_with_size_above,
    size_t dont_explore_edges_with_degree_above,
    bool dont_explore_edges_increasing_symptom_degree) {
    if (dont_explore_edges_with_degree_above == 2 && dont_explore_detection_event_sets_with_size_above == 2) {
        return stim::shortest_graphlike_undetectable_logical_error(model, true);
    }

    Graph graph = Graph::from_dem(model, dont_explore_edges_with_degree_above);

    if (graph.distance_1_error_mask) {
        DetectorErrorModel out;
        SearchState s1{{}, graph.distance_1_error_mask};
        s1.append_transition_as_error_instruction_to({}, out);
        return out;
    }

    std::queue<SearchState> queue;
    std::map<SearchState, SearchState> back_map;
    // Mark the vacuous dead-end state as already seen.
    back_map.emplace(SearchState(), SearchState());

    // Search starts from any and all edges crossing an observable.
    for (size_t node = 0; node < graph.nodes.size(); node++) {
        for (const auto &e : graph.nodes[node].edges) {
            if (e.crossing_observable_mask && e.nodes.sorted_items[0] == node) {
                SearchState start{e.nodes, e.crossing_observable_mask};
                if (start.dets.size() <= dont_explore_detection_event_sets_with_size_above) {
                    queue.push(start);
                }
                back_map.emplace(start, SearchState());
            }
        }
    }

    // Breadth first search for a symptomless state that has a frame change.
    for (; !queue.empty(); queue.pop()) {
        SearchState cur = queue.front();
        assert(!cur.dets.empty());
        size_t active_node = cur.dets.sorted_items[0];
        for (const auto &e : graph.nodes[active_node].edges) {
            SearchState next{e.nodes ^ cur.dets, e.crossing_observable_mask ^ cur.obs_mask};
            if (next.dets.size() > dont_explore_detection_event_sets_with_size_above) {
                continue;
            }
            if (dont_explore_edges_increasing_symptom_degree && next.dets.size() > cur.dets.size()) {
                continue;
            }
            if (!back_map.emplace(next, cur).second) {
                continue;
            }
            if (next.dets.empty()) {
                assert(next.obs_mask);  // Otherwise, it would have already been in back_map.
                return backtrack_path(back_map, next);
            }
            queue.push(std::move(next));
        }
    }

    throw std::invalid_argument("Failed to find any decaying logical errors.");
}
