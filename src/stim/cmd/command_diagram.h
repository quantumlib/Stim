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

#ifndef _STIM_CMD_COMMAND_DIAGRAM_H
#define _STIM_CMD_COMMAND_DIAGRAM_H

#include "stim/util_bot/arg_parse.h"

namespace stim {

int command_diagram(int argc, const char **argv);
SubCommandHelp command_diagram_help();

}  // namespace stim

#endif
