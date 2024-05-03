#ifndef _STIM_DIAGRAM_GATE_DATA_3d_H
#define _STIM_DIAGRAM_GATE_DATA_3d_H

#include "stim/diagram/gltf.h"

namespace stim_draw_internal {

std::map<std::string_view, std::shared_ptr<GltfMesh>> make_gate_primitives();

}

#endif
