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

#ifndef _STIM_CIRCUIT_DRAW_H
#define _STIM_CIRCUIT_DRAW_H

#include <iostream>

#include "stim/circuit/circuit.h"

namespace stim {

std::string draw(const Circuit &circuit);
std::string draw_svg(const Circuit &circuit);
size_t utf8_char_count(const std::string &s);

}  // namespace stim

#endif
