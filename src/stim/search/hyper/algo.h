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

#ifndef _STIM_SEARCH_HYPER_ALGO_H
#define _STIM_SEARCH_HYPER_ALGO_H

#include <cstdint>

#include "stim/dem/detector_error_model.h"

namespace stim {

/// Finds a list of errors from the model that form an undetectable logical error.
///
/// Note that this search considers hyper errors, and so if it is not truncated it has exponential
/// runtime. It is important to carefully consider how you are truncating the space to trade off
/// cost versus quality of result.
///
/// Beware that the choice of logical observable can interact with the truncation options. Using
/// different observables can change whether or not the search succeeds, even if those observables
/// are equal modulo the stabilizers of the code. This is because the edges crossing logical
/// observables are used as starting points for the search, and starting from different places along
/// a path will result in different numbers of symptoms in intermediate states as the search
/// progresses. For example, if the logical observable is next to a boundary, then the starting
/// edges are likely boundary edges (degree 1) with 'room to grow', whereas if the observable was
/// running through the bulk then the starting edges will have degree at least 2.
///
/// To perform a graphlike search use:
///     dont_explore_detection_event_sets_with_size_above = 2
///     dont_explore_edges_with_degree_above = 2
///     dont_explore_edges_increasing_symptom_degree = [anything]
///
/// Args:
///     model: The detector error model to search for undetectable errors.
///     dont_explore_detection_event_sets_with_size_above: Truncates the search space by refusing to
///         cross an edge (i.e. add an error) when doing so would produce an intermediate state that
///         has more detection events than this limit.
///     dont_explore_edges_with_degree_above: Truncates the search space by refusing to consider
///         errors that cause a lot of detection events. For example, you may only want to consider
///         graphlike errors which have two or fewer detection events.
///     dont_explore_edges_increasing_symptom_degree: Truncates the search space by refusing to
///         cross an edge (i.e. add an error) when doing so would produce an intermediate state that
///         has more detection events that the previous intermediate state. This massively improves
///         the efficiency of the search because instead of, for example, exploring all n^4 possible
///         detection event sets with 4 symptoms, the search will attempt to cancel out symptoms one
///         by one.
///
/// Returns:
///     A detector error model containing only the error mechanisms that cause the undetectable
///     logical error. The error mechanisms will have their probabilities set to 1 (indicating that
///     they are necessary) and will not suggest a decomposition.
DetectorErrorModel find_undetectable_logical_error(
    const DetectorErrorModel &model,
    size_t dont_explore_detection_event_sets_with_size_above,
    size_t dont_explore_edges_with_degree_above,
    bool dont_explore_edges_increasing_symptom_degree);

}  // namespace stim

#endif
