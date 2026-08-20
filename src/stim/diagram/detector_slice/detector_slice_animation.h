#ifndef _STIM_DIAGRAM_DETECTOR_SLICE_DETECTOR_SLICE_ANIMATION_H
#define _STIM_DIAGRAM_DETECTOR_SLICE_DETECTOR_SLICE_ANIMATION_H

#include <cstdint>
#include <string>

#include "stim/circuit/circuit.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"

namespace stim_draw_internal {

std::string make_detector_slice_animation_html(
    const stim::Circuit &circuit,
    uint64_t tick_slice_start,
    uint64_t tick_slice_num,
    stim::SpanRef<const CoordFilter> det_coord_filter);

}  // namespace stim_draw_internal

#endif
