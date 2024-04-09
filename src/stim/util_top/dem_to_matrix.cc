#include "stim/util_top/dem_to_matrix.h"

using namespace stim;

std::map<SparseXorVec<DemTarget>, double> stim::dem_to_map(const DetectorErrorModel &dem) {
    std::map<SparseXorVec<DemTarget>, double> result;
    SparseXorVec<DemTarget> buf;
    dem.iter_flatten_error_instructions([&](DemInstruction instruction) {
        if (instruction.type != DemInstructionType::DEM_ERROR) {
            return;
        }
        buf.sorted_items.clear();
        for (DemTarget t: instruction.target_data) {
            if (t.is_observable_id() || t.is_relative_detector_id()) {
                buf.sorted_items.push_back(t);
            }
        }
        size_t kept = xor_sort<DemTarget>(buf.sorted_items);
        buf.sorted_items.resize(kept);
        auto q = instruction.arg_data[0];
        auto &p = result[buf];
        p = p * (1 - q) + q * (1 - p);
    });
    return result;
}
