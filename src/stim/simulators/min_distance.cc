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
using namespace stim::impl_min_distance;

constexpr uint64_t NO_NODE_INDEX = UINT64_MAX;

std::string DemAdjEdge::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}
bool DemAdjEdge::operator==(const DemAdjEdge &other) const {
    return opposite_node_index == other.opposite_node_index &&
           crossing_observable_mask == other.crossing_observable_mask;
}
bool DemAdjEdge::operator!=(const DemAdjEdge &other) const {
    return !(*this == other);
}
bool DemAdjNode::operator==(const DemAdjNode &other) const {
    return edges == other.edges;
}
bool DemAdjNode::operator!=(const DemAdjNode &other) const {
    return !(*this == other);
}
std::string DemAdjNode::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}
std::string DemAdjGraph::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}
std::string DemAdjGraphSearchState::str() const {
    std::stringstream result;
    result << *this;
    return result.str();
}

std::ostream &stim::impl_min_distance::operator<<(std::ostream &out, const DemAdjEdge &v) {
    if (v.opposite_node_index == NO_NODE_INDEX) {
        out << "[boundary]";
    } else {
        out << "D" << v.opposite_node_index;
    }
    size_t obs_id = 0;
    uint64_t m = v.crossing_observable_mask;
    while (m) {
        if (m & 1) {
            out << " L" << obs_id;
        }
        m >>= 1;
        obs_id++;
    }
    return out;
}

std::ostream &stim::impl_min_distance::operator<<(std::ostream &out, const DemAdjNode &v) {
    for (const auto &e : v.edges) {
        out << "    " << e << "\n";
    }
    return out;
}

void DemAdjGraph::add_outward_edge(size_t src, uint64_t dst, uint64_t obs_mask) {
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

void DemAdjGraph::add_edges_from_targets_with_no_separators(
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

void DemAdjGraph::add_edges_from_separable_targets(
    ConstPointerRange<DemTarget> targets, bool ignore_ungraphlike_errors) {
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

DemAdjGraph DemAdjGraph::from_dem(const DetectorErrorModel &model, bool ignore_ungraphlike_errors) {
    if (model.count_observables() > 64) {
        throw std::invalid_argument(
            "NotImplemented: shortest_graphlike_undetectable_logical_error with more than 64 observables.");
    }

    DemAdjGraph result(model.count_detectors());
    model.iter_flatten_error_instructions([&](const DemInstruction &e) {
        if (e.arg_data[0] != 0) {
            result.add_edges_from_separable_targets(e.target_data, ignore_ungraphlike_errors);
        }
    });
    return result;
}
bool DemAdjGraph::operator==(const DemAdjGraph &other) const {
    return nodes == other.nodes && distance_1_error_mask == other.distance_1_error_mask;
}
bool DemAdjGraph::operator!=(const DemAdjGraph &other) const {
    return !(*this == other);
}

DemAdjGraph::DemAdjGraph(size_t node_count) : nodes(node_count), distance_1_error_mask(0) {
}

DemAdjGraph::DemAdjGraph(std::vector<DemAdjNode> nodes, uint64_t distance_1_error_mask)
    : nodes(std::move(nodes)), distance_1_error_mask(distance_1_error_mask) {
}

std::ostream &stim::impl_min_distance::operator<<(std::ostream &out, const DemAdjGraph &v) {
    for (size_t k = 0; k < v.nodes.size(); k++) {
        out << k << ":\n" << v.nodes[k];
    }
    return out;
}

DemAdjGraphSearchState::DemAdjGraphSearchState() : det_active(NO_NODE_INDEX), det_held(NO_NODE_INDEX), obs_mask(0) {
}
DemAdjGraphSearchState::DemAdjGraphSearchState(uint64_t det_active, uint64_t det_held, uint64_t obs_mask)
    : det_active(det_active), det_held(det_held), obs_mask(obs_mask) {
}

bool DemAdjGraphSearchState::is_undetected() const {
    return det_active == det_held;
}

DemAdjGraphSearchState DemAdjGraphSearchState::canonical() const {
    if (det_active < det_held) {
        return {det_active, det_held, obs_mask};
    } else if (det_active > det_held) {
        return {det_held, det_active, obs_mask};
    } else {
        return {NO_NODE_INDEX, NO_NODE_INDEX, obs_mask};
    }
}

void DemAdjGraphSearchState::append_transition_as_error_instruction_to(
    const DemAdjGraphSearchState &other, DetectorErrorModel &out) {
    // Extract detector indices while cancelling duplicates.
    std::array<uint64_t, 5> nodes{det_active, det_held, other.det_active, other.det_held, NO_NODE_INDEX};
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

bool DemAdjGraphSearchState::operator==(const DemAdjGraphSearchState &other) const {
    DemAdjGraphSearchState a = canonical();
    DemAdjGraphSearchState b = other.canonical();
    return a.det_active == b.det_active && a.det_held == b.det_held && a.obs_mask == b.obs_mask;
}
bool DemAdjGraphSearchState::operator!=(const DemAdjGraphSearchState &other) const {
    return !(*this == other);
}

bool DemAdjGraphSearchState::operator<(const DemAdjGraphSearchState &other) const {
    DemAdjGraphSearchState a = canonical();
    DemAdjGraphSearchState b = other.canonical();
    if (a.det_active != b.det_active) {
        return a.det_active < b.det_active;
    }
    if (a.det_held != b.det_held) {
        return a.det_held < b.det_held;
    }
    return a.obs_mask < b.obs_mask;
}

std::ostream &stim::impl_min_distance::operator<<(std::ostream &out, const DemAdjGraphSearchState &v) {
    if (v.is_undetected()) {
        out << "[no symptoms] ";
    } else {
        if (v.det_active != NO_NODE_INDEX) {
            out << "D" << v.det_active << " ";
        }
        if (v.det_held != NO_NODE_INDEX) {
            out << "D" << v.det_held << " ";
        }
    }

    uint64_t dif_mask = v.obs_mask;
    size_t obs_id = 0;
    while (dif_mask) {
        if (dif_mask & 1) {
            out << "L" << obs_id << " ";
        }
        dif_mask >>= 1;
        obs_id++;
    }

    return out;
}

DetectorErrorModel backtrack_path(
    const std::map<DemAdjGraphSearchState, DemAdjGraphSearchState> &back_map, DemAdjGraphSearchState final_state) {
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
    DemAdjGraph graph = DemAdjGraph::from_dem(model, ignore_ungraphlike_errors);

    if (graph.distance_1_error_mask) {
        DetectorErrorModel out;
        DemAdjGraphSearchState s1(NO_NODE_INDEX, NO_NODE_INDEX, graph.distance_1_error_mask);
        s1.append_transition_as_error_instruction_to({}, out);
        return out;
    }

    std::queue<DemAdjGraphSearchState> queue;
    std::map<DemAdjGraphSearchState, DemAdjGraphSearchState> back_map;
    // Mark the vacuous dead-end state as already seen.
    back_map.emplace(DemAdjGraphSearchState(), DemAdjGraphSearchState());

    // Search starts from any and all edges crossing an observable.
    for (size_t node1 = 0; node1 < graph.nodes.size(); node1++) {
        for (const auto &e : graph.nodes[node1].edges) {
            uint64_t node2 = e.opposite_node_index;
            if (node1 < node2 && e.crossing_observable_mask) {
                DemAdjGraphSearchState start{node1, node2, e.crossing_observable_mask};
                queue.push(start);
                back_map.emplace(start, DemAdjGraphSearchState());
            }
        }
    }

    // Breadth first search for a symptomless state that has a frame change.
    for (; !queue.empty(); queue.pop()) {
        DemAdjGraphSearchState cur = queue.front();
        assert(cur.det_active != NO_NODE_INDEX);
        for (const auto &e : graph.nodes[cur.det_active].edges) {
            DemAdjGraphSearchState next(e.opposite_node_index, cur.det_held, e.crossing_observable_mask ^ cur.obs_mask);
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
