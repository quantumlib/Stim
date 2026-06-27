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

#ifndef _STIM_SEARCH_GRAPHLIKE_ALGO_H
#define _STIM_SEARCH_GRAPHLIKE_ALGO_H

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
DetectorErrorModel shortest_graphlike_undetectable_logical_error(
    const DetectorErrorModel &model, bool ignore_ungraphlike_errors);

}  // namespace stim

#endif
