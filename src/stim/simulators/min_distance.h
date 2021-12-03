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

#ifndef _STIM_SIMULATORS_MIN_DISTANCE_H
#define _STIM_SIMULATORS_MIN_DISTANCE_H

#include "stim/dem/detector_error_model.h"

namespace stim {

/// Finds a list of graphlike errors from the model that form an undetectable logical error.
///
/// An error is graphlike if it has at most 2 symptoms (detector indices).
///
/// The components of composite errors like D0 ^ D1 D2 are included in the set of graphlike errors being considered when
/// trying to find an undetectable error (even if that specific graphlike error component doesn't occur on its own
/// outside a composite error anywhere in the model).
///
/// Args:
///     model: The detector error model to search for undetectable errors.
///     ignore_ungraphlike_errors: Determines whether or not error components with more than 2 symptoms should raise an
///         exception, or just be ignored as if they weren't there.
///
/// Returns:
///     A detector error model containing only the error mechanisms that cause the undetectable logical error.
///     Note that the error mechanisms will have their probabilities set to 1 (indicating they are necessary).
DetectorErrorModel shortest_graphlike_undetectable_logical_error(const DetectorErrorModel &model,
                                                                 bool ignore_ungraphlike_errors);

namespace impl_min_distance {

struct DemAdjEdge {
    uint64_t opposite_node_index;
    uint64_t crossing_observable_mask;
    std::string str() const;
    bool operator==(const DemAdjEdge &other) const;
    bool operator!=(const DemAdjEdge &other) const;
};
std::ostream &operator<<(std::ostream &out, const DemAdjEdge &v);

struct DemAdjNode {
    std::vector<DemAdjEdge> edges;
    std::string str() const;
    bool operator==(const DemAdjNode &other) const;
    bool operator!=(const DemAdjNode &other) const;
};
std::ostream &operator<<(std::ostream &out, const DemAdjNode &v);

struct DemAdjGraph {
    std::vector<DemAdjNode> nodes;
    uint64_t distance_1_error_mask;

    explicit DemAdjGraph(size_t node_count);
    DemAdjGraph(std::vector<DemAdjNode> nodes, uint64_t distance_1_error_mask);

    void add_outward_edge(size_t src, uint64_t dst, uint64_t obs_mask);
    void add_edges_from_targets_with_no_separators(ConstPointerRange<DemTarget> targets, bool ignore_ungraphlike_errors);
    void add_edges_from_separable_targets(ConstPointerRange<DemTarget> targets, bool ignore_ungraphlike_errors);
    static DemAdjGraph from_dem(const DetectorErrorModel &model, bool ignore_ungraphlike_errors);
    bool operator==(const DemAdjGraph &other) const;
    bool operator!=(const DemAdjGraph &other) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const DemAdjGraph &v);

struct DemAdjGraphSearchState {
    uint64_t det_active;  // The detection event being moved around in an attempt to remove it (or NO_NODE_INDEX).
    uint64_t det_held;  // The detection event being left in the same place (or NO_NODE_INDEX).
    uint64_t obs_mask;  // The accumulated frame changes from moving the detection events around.

    DemAdjGraphSearchState();
    DemAdjGraphSearchState(uint64_t det_active, uint64_t det_held, uint64_t obs_mask);
    bool is_undetected() const;
    DemAdjGraphSearchState canonical() const;
    void append_transition_as_error_instruction_to(const DemAdjGraphSearchState &other, DetectorErrorModel &out);
    bool operator==(const DemAdjGraphSearchState &other) const;
    bool operator!=(const DemAdjGraphSearchState &other) const;
    bool operator<(const DemAdjGraphSearchState &other) const;
    std::string str() const;
};
std::ostream &operator<<(std::ostream &out, const DemAdjGraphSearchState &v);

}  // namespace impl_min_distance
}  // namespace stim

#endif
