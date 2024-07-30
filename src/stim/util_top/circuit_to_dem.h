#ifndef _STIM_UTIL_TOP_CIRCUIT_TO_DEM_H
#define _STIM_UTIL_TOP_CIRCUIT_TO_DEM_H

#include "stim/simulators/error_analyzer.h"

namespace stim {

struct DemOptions {
    bool decompose_errors = false;
    bool flatten_loops = true;
    bool allow_gauge_detectors = false;
    double approximate_disjoint_errors_threshold = 0;
    bool ignore_decomposition_failures = false;
    bool block_decomposition_from_introducing_remnant_edges = false;
};

inline DetectorErrorModel circuit_to_dem(const Circuit &circuit, DemOptions options = {}) {
    return ErrorAnalyzer::circuit_to_detector_error_model(
        circuit,
        options.decompose_errors,
        !options.flatten_loops,
        options.allow_gauge_detectors,
        options.approximate_disjoint_errors_threshold,
        options.ignore_decomposition_failures,
        options.block_decomposition_from_introducing_remnant_edges);
}

}  // namespace stim

#endif
