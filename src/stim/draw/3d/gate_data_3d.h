#ifndef _STIM_DRAW_3D_GATE_DATA_3d_H
#define _STIM_DRAW_3D_GATE_DATA_3d_H

#include "stim/draw/3d/gltf.h"

namespace stim_draw_internal {

std::map<std::string, std::shared_ptr<GltfMesh>> make_gate_primitives();

}

#endif
