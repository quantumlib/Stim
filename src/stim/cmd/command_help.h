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

#ifndef _STIM_CMD_COMMAND_HELP_H
#define _STIM_CMD_COMMAND_HELP_H

#include <map>
#include <string>
#include <vector>

#include "stim/circuit/gate_target.h"
#include "stim/gates/gates.h"

namespace stim {

int command_help(int argc, const char **argv);
std::string help_for(std::string help_key);
std::string clean_doc_string(const char *c, bool allow_too_long = false);

std::vector<GateTarget> gate_decomposition_help_targets_for_gate_type(stim::GateType g);

}  // namespace stim

#endif