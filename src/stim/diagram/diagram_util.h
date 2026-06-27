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

#ifndef _STIM_DIAGRAM_DIAGRAM_UTIL_H
#define _STIM_DIAGRAM_DIAGRAM_UTIL_H

#include <iostream>

#include "stim/circuit/circuit.h"

namespace stim_draw_internal {

constexpr const char *X_RED = "#FF4040";
constexpr const char *Y_GREEN = "#59FF7A";
constexpr const char *Z_BLUE = "#4DA6FF";
constexpr const char *EX_PURPLE = "#FF4DDB";
constexpr const char *EY_YELLOW = "#F1FF59";
constexpr const char *EZ_ORANGE = "#FF9500";
constexpr const char *BG_GREY = "#AAAAAA";

size_t utf8_char_count(std::string_view s);

/// Splits a two qubit gate into two end pieces, which can be drawn independently.
std::pair<std::string_view, std::string_view> two_qubit_gate_pieces(stim::GateType gate_type);

/// Adds each element of a vector to the string stream starting with ':', separated by '_'
/// adds nothing if the vector is empty
void add_coord_summary_to_ss(std::ostream &ss, std::vector<double> vec);

}  // namespace stim_draw_internal

#endif
