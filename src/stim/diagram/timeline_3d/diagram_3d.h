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

#include "stim/circuit/gate_data.h"
#include "stim/circuit/circuit.h"
#include "stim/diagram/gltf.h"
#include "stim/mem/pointer_range.h"

namespace stim_draw_internal {

struct DiagramGate3DPiece {
    std::string gate_piece;
    Coord<3> center;
};

struct Diagram3D {
    std::vector<DiagramGate3DPiece> gates;
    std::vector<Coord<3>> line_data;
    std::vector<Coord<3>> red_line_data;

    static Diagram3D from_circuit(const stim::Circuit &circuit);
};

float min_distance(stim::ConstPointerRange<Coord<2>> points);
GltfScene scene_from_circuit(const stim::Circuit &circuit);

}

#endif
