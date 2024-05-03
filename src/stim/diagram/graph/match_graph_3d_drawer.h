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

#ifndef _STIM_DIAGRAM_GRAPH_MATCH_GRAPH_DRAWER_H
#define _STIM_DIAGRAM_GRAPH_MATCH_GRAPH_DRAWER_H

#include <iostream>

#include "stim/dem/detector_error_model.h"
#include "stim/diagram/basic_3d_diagram.h"
#include "stim/diagram/circuit_timeline_helper.h"

namespace stim_draw_internal {

Basic3dDiagram dem_match_graph_to_basic_3d_diagram(const stim::DetectorErrorModel &dem);

}  // namespace stim_draw_internal

#endif
