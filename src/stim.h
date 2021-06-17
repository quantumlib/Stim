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

#ifndef STIM_H
#define STIM_H

#include "simulators/frame_simulator.h"
#include "simulators/tableau_simulator.h"
#include "stabilizers/pauli_string.h"
#include "stabilizers/tableau.h"

namespace stim {
using Circuit = stim_internal::Circuit;
using ErrorAnalyzer = stim_internal::ErrorAnalyzer;
using FrameSimulator = stim_internal::FrameSimulator;
using PauliString = stim_internal::PauliString;
using Tableau = stim_internal::Tableau;
using TableauSimulator = stim_internal::TableauSimulator;
const auto &GATE_DATA = stim_internal::GATE_DATA;
}  // namespace stim

#endif