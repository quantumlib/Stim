#include "stim/diagram/lattice_map.h"

using namespace stim;
using namespace stim_draw_internal;

void LatticeMap::set(
    uint64_t index,
    stim::SpanRef<const uint64_t> offsets_per_iteration,
    stim::SpanRef<const uint64_t> iteration_counts,
    uint32_t value) {
    if (offsets_per_iteration.empty()) {
        if (index >= brute_force_data.size()) {
            brute_force_data.resize(2 * index + 10);
        }
        brute_force_data[index] = value;
        return;
    }

    for (uint64_t k = 0; k < iteration_counts[0]; k++) {
        set(index + k * offsets_per_iteration[0],
            {offsets_per_iteration.ptr_start + 1, offsets_per_iteration.ptr_end},
            {iteration_counts.ptr_start + 1, iteration_counts.ptr_end},
            value);
    }
}
uint32_t LatticeMap::get(uint64_t index) {
    if (index >= brute_force_data.size()) {
        return 0;
    }
    return brute_force_data[index];
}
