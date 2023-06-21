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

#ifndef _STIM_SIMULATORS_COUNT_DETERMINED_MEASUREMENTS_H
#define _STIM_SIMULATORS_COUNT_DETERMINED_MEASUREMENTS_H

#include "stim/circuit/circuit.h"

namespace stim {

template <size_t W>
uint64_t count_determined_measurements(const Circuit &circuit);

}  // namespace stim

#include "stim/simulators/count_determined_measurements.inl"

#endif
