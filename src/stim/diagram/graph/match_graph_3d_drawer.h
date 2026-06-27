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
