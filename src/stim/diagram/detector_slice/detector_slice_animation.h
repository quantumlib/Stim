#ifndef _STIM_DIAGRAM_DETECTOR_SLICE_DETECTOR_SLICE_ANIMATION_H
#define _STIM_DIAGRAM_DETECTOR_SLICE_DETECTOR_SLICE_ANIMATION_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "stim/circuit/circuit.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"

namespace stim_draw_internal {

struct DetectorSliceAnimationFrame {
    uint64_t tick;
    std::string svg;
    std::map<uint64_t, DetectorSliceSvgRegion> detector_regions;
};

struct DetectorSliceAnimationTransition {
    uint64_t detector_id;
    bool has_source;
    bool has_target;
    bool geometry_changed;
    bool style_changed;
    std::vector<Coord<2>> source_points;
    std::vector<Coord<2>> target_points;
};

std::vector<DetectorSliceAnimationTransition> make_detector_slice_animation_transitions(
    const DetectorSliceAnimationFrame &source, const DetectorSliceAnimationFrame &target);

std::vector<DetectorSliceAnimationFrame> make_detector_slice_animation_frames(
    const stim::Circuit &circuit,
    uint64_t tick_slice_start,
    uint64_t tick_slice_num,
    stim::SpanRef<const CoordFilter> det_coord_filter);

std::string make_detector_slice_animation_html(
    const stim::Circuit &circuit,
    uint64_t tick_slice_start,
    uint64_t tick_slice_num,
    stim::SpanRef<const CoordFilter> det_coord_filter);

}  // namespace stim_draw_internal

#endif
