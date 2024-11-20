#ifndef _STIM_UTIL_TOP_EXPORT_CRUMBLE_URL_H
#define _STIM_UTIL_TOP_EXPORT_CRUMBLE_URL_H

#include "stim/circuit/circuit.h"
#include "stim/simulators/matched_error.h"

namespace stim {

std::string export_crumble_url(
    const Circuit &circuit, bool skip_detectors = false, const std::map<int, std::vector<ExplainedError>> &mark = {});

}  // namespace stim

#endif
