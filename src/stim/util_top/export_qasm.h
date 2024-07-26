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

#ifndef _STIM_CIRCUIT_EXPORT_CIRCUIT_H
#define _STIM_CIRCUIT_EXPORT_CIRCUIT_H

#include "stim/circuit/circuit.h"

namespace stim {

void export_open_qasm(const Circuit &circuit, std::ostream &out, int open_qasm_version, bool skip_dets_and_obs);

}  // namespace stim

#endif
