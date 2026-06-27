#ifndef _STIM_DIAGRAM_CRUMBLE_H
#define _STIM_DIAGRAM_CRUMBLE_H

#include <iostream>

#include "stim/circuit/circuit.h"

namespace stim_draw_internal {

void write_crumble_html_with_preloaded_circuit(const stim::Circuit &circuit, std::ostream &out);

}  // namespace stim_draw_internal

#endif
