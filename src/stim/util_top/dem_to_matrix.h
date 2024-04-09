#ifndef _STIM_UTIL_TOP_DEM_TO_MATRIX_H
#define _STIM_UTIL_TOP_DEM_TO_MATRIX_H

#include "stim/mem/sparse_xor_vec.h"
#include "stim/dem/detector_error_model.h"

namespace stim {

std::map<SparseXorVec<DemTarget>, double> dem_to_map(const DetectorErrorModel &dem);

/// Sorts the given items, and cancels out duplicates.
/// If an item appears an even number of times in the span, it is removed.
/// If it appears an odd number of times, exactly one instance is kept.
///
/// Args:
///     target: The span of items to xor-sort.
///
/// Returns:
///     The number of kept items. Kept items are moved to the start of the range.
template <typename T>
size_t xor_sort(std::span<T> target) {
    if (target.empty()) {
        return 0;
    }
    std::sort(target.begin(), target.end());
    size_t kept = 0;
    for (size_t k = 0; k < target.size(); k++) {
        if (k + 1 < target.size() && target[k] == target[k + 1]) {
            k += 1;
            continue;
        }
        if (kept < k) {
            target[kept] = std::move(target[k]);
        }
        kept++;
    }

    return kept;
}

}  // namespace stim

#endif
