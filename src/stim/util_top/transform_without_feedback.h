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

#ifndef _STIM_UTIL_TOP_TRANSFORM_WITHOUT_FEEDBACK_H
#define _STIM_UTIL_TOP_TRANSFORM_WITHOUT_FEEDBACK_H

#include <complex>
#include <iostream>
#include <random>
#include <unordered_map>

#include "stim/circuit/circuit.h"
#include "stim/stabilizers/pauli_string.h"

namespace stim {

Circuit circuit_with_inlined_feedback(const Circuit &circuit);

}  // namespace stim

#endif
