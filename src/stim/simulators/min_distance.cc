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

#include "stim/simulators/min_distance.h"

#include <algorithm>
#include <map>
#include <queue>

using namespace stim;

constexpr uint64_t NO_NODE_INDEX = UINT64_MAX;

struct DemAdjEdge {
    uint64_t opposite_node_index;
    uint64_t crossing_observable_mask;

    std::string str() const {
        std::stringstream result;
        if (opposite_node_index == NO_NODE_INDEX) {
            result << "[boundary]";
        } else {
            result << opposite_node_index;
        }
        size_t obs_id = 0;
        uint64_t m = crossing_observable_mask;
        while (m) {
            if (m & 1) {
                result << " L" << m;
            }
            m >>= 1;
            obs_id++;
        }
        return result.str();
    }
};

struct DemAdjNode {
    std::vector<DemAdjEdge> edges;
};

struct DemAdjGraph {
    std::vector<DemAdjNode> nodes;
    uint64_t distance_1_error_mask = 0;

    void add_outward_edge(size_t src, uint64_t dst, uint64_t obs_mask) {
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

    void add_edges_from_targets_with_no_separators(ConstPointerRange<DemTarget> targets, bool ignore_ungraphlike_errors) {
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
        } else if (detectors.size() == 0 && obs_mask && distance_1_error_mask == 0) {
            distance_1_error_mask = obs_mask;
        }
    }

    void add_edges_from_separable_targets(ConstPointerRange<DemTarget> targets, bool ignore_ungraphlike_errors) {
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

    static DemAdjGraph from_dem(const DetectorErrorModel &model, bool ignore_ungraphlike_errors) {
        DemAdjGraph result;
        if (model.count_observables() > 64) {
            throw std::invalid_argument("NotImplemented: shortest_graphlike_undetectable_logical_error with more than 64 observables.");
        }
        result.nodes.resize(model.count_detectors());
        model.iter_flatten_error_instructions([&](const DemInstruction &e){
            if (e.arg_data[0] != 0) {
                result.add_edges_from_separable_targets(e.target_data, ignore_ungraphlike_errors);
            }
        });
        return result;
    }

    std::string str() const {
        std::stringstream result;
        for (size_t k = 0; k < nodes.size(); k++) {
            result << k << ":\n";
            for (const auto &e : nodes[k].edges) {
                result << "    " << e.str() << "\n";
            }
        }
        return result.str();
    }
};

struct DemAdjGraphSearchState {
    uint64_t node1;
    uint64_t node2;
    uint64_t obs_mask;

    DemAdjGraphSearchState() : node1(NO_NODE_INDEX), node2(NO_NODE_INDEX), obs_mask(0) {
    }
    DemAdjGraphSearchState(uint64_t init_node1, uint64_t init_node2, uint64_t obs_mask) : obs_mask(obs_mask) {
        if (init_node1 < init_node2) {
            node1 = init_node1;
            node2 = init_node2;
        } else if (init_node1 > init_node2) {
            node1 = init_node2;
            node2 = init_node1;
        } else {
            node1 = NO_NODE_INDEX;
            node2 = NO_NODE_INDEX;
        }
    }

    void append_transition_as_error_instruction_to(const DemAdjGraphSearchState &other, DetectorErrorModel &out) {
        // Extract detector indices while cancelling duplicates.
        std::array<uint64_t, 5> nodes{node1, node2, other.node1, other.node2, NO_NODE_INDEX};
        std::sort(nodes.begin(), nodes.end());
        for (size_t k = 0; k < 4; k++) {
            if (nodes[k] == nodes[k + 1]) {
                k++;
            } else {
                out.target_buf.append_tail(DemTarget::relative_detector_id(nodes[k]));
            }
        }

        // Extract logical observable indices.
        uint64_t dif_mask = obs_mask ^ other.obs_mask;
        size_t obs_id = 0;
        while (dif_mask) {
            if (dif_mask & 1) {
                out.target_buf.append_tail(DemTarget::observable_id(obs_id));
            }
            dif_mask >>= 1;
            obs_id++;
        }

        out.arg_buf.append_tail(1);

        out.instructions.push_back(DemInstruction{out.arg_buf.commit_tail(), out.target_buf.commit_tail(), DEM_ERROR});
    }

    bool operator==(const DemAdjGraphSearchState &other) const {
        return node1 == other.node1 && node2 == other.node2 && obs_mask == other.obs_mask;
    }

    bool operator<(const DemAdjGraphSearchState &other) const {
        if (node1 != other.node1) {
            return node1 < other.node1;
        }
        if (node2 != other.node2) {
            return node2 < other.node2;
        }
        return obs_mask < other.obs_mask;
    }

    std::string str() const {
        std::stringstream result;
        if (node1 != NO_NODE_INDEX) {
            result << "D" << node1;
            if (node2 != NO_NODE_INDEX) {
                result << " D" << node2;
            }
        } else {
            result << "[no symptoms]";
        }

        uint64_t dif_mask = obs_mask;
        size_t obs_id = 0;
        while (dif_mask) {
            if (dif_mask & 1) {
                result << " L" << obs_id;
            }
            dif_mask >>= 1;
            obs_id++;
        }

        return result.str();
    }
};

DetectorErrorModel backtrack_path(
        const std::map<DemAdjGraphSearchState, DemAdjGraphSearchState> &back_map,
        DemAdjGraphSearchState final_state) {

    DetectorErrorModel out;
    auto cur_state = final_state;
    while (true) {
        auto prev_state = back_map.at(cur_state);
        cur_state.append_transition_as_error_instruction_to(prev_state, out);
        if (prev_state.node1 == NO_NODE_INDEX) {
            break;
        }
        cur_state = prev_state;
    }
    std::sort(out.instructions.begin(), out.instructions.end());
    return out;
}

DetectorErrorModel stim::shortest_graphlike_undetectable_logical_error(const DetectorErrorModel &model,
                                                                       bool ignore_ungraphlike_errors) {
    DemAdjGraph graph = DemAdjGraph::from_dem(model, ignore_ungraphlike_errors);

    if (graph.distance_1_error_mask) {
        DetectorErrorModel out;
        DemAdjGraphSearchState s1(NO_NODE_INDEX, NO_NODE_INDEX, graph.distance_1_error_mask);
        s1.append_transition_as_error_instruction_to({}, out);
        return out;
    }

    std::queue<DemAdjGraphSearchState> queue;
    std::map<DemAdjGraphSearchState, DemAdjGraphSearchState> back_map;

    // Search starts from any and all edges crossing an observable.
    for (size_t node1 = 0; node1 < graph.nodes.size(); node1++) {
        for (const auto &e : graph.nodes[node1].edges) {
            size_t node2 = e.opposite_node_index;
            if (node1 < node2 && e.crossing_observable_mask) {
                DemAdjGraphSearchState start{node1, node2, e.crossing_observable_mask};
                queue.push(start);
                back_map.emplace(start, DemAdjGraphSearchState());
            }
        }
    }

    for (; !queue.empty(); queue.pop()) {
        DemAdjGraphSearchState cur = queue.front();
        for (const auto &e : graph.nodes[cur.node1].edges) {
            DemAdjGraphSearchState next(e.opposite_node_index, cur.node2, e.crossing_observable_mask ^ cur.obs_mask);
            if (!back_map.emplace(next, cur).second) {
                continue;
            }
            if (next.node1 == NO_NODE_INDEX) {
                if (next.obs_mask) {
                    return backtrack_path(back_map, next);
                }
            } else {
                queue.push(next);
            }
        }
    }

    throw std::invalid_argument("Failed to find any graphlike logical errors.");
}
