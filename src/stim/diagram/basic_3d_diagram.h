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

#ifndef _STIM_DRAW_3D_DIAGRAM_3D_H
#define _STIM_DRAW_3D_DIAGRAM_3D_H

#include <iostream>

#include "stim/diagram/gltf.h"

namespace stim_draw_internal {

struct Basic3DElement {
    std::string gate_piece;
    Coord<3> center;
};

struct Basic3dDiagram {
    std::vector<Basic3DElement> elements;
    std::vector<Coord<3>> line_data;
    std::vector<Coord<3>> red_line_data;
    std::vector<Coord<3>> blue_line_data;
    std::vector<Coord<3>> purple_line_data;

    GltfScene to_gltf_scene() const;
};

}  // namespace stim_draw_internal

#endif
