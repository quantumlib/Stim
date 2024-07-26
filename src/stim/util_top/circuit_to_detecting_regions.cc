#include "stim/util_top/circuit_to_detecting_regions.h"

#include "stim/simulators/sparse_rev_frame_tracker.h"

using namespace stim;

std::map<DemTarget, std::map<uint64_t, FlexPauliString>> stim::circuit_to_detecting_regions(
    const Circuit &circuit,
    std::set<DemTarget> included_targets,
    std::set<uint64_t> included_ticks,
    bool ignore_anticommutation_errors) {
    CircuitStats stats = circuit.compute_stats();
    uint64_t tick_index = stats.num_ticks;
    SparseUnsignedRevFrameTracker tracker(
        stats.num_qubits, stats.num_measurements, stats.num_detectors, !ignore_anticommutation_errors);
    std::map<DemTarget, std::map<uint64_t, FlexPauliString>> result;
    circuit.for_each_operation_reverse([&](const CircuitInstruction &inst) {
        if (inst.gate_type == GateType::TICK) {
            tick_index -= 1;
            if (included_ticks.contains(tick_index)) {
                for (size_t q = 0; q < stats.num_qubits; q++) {
                    for (auto target : tracker.xs[q]) {
                        if (included_targets.contains(target)) {
                            auto &m = result[target];
                            if (!m.contains(tick_index)) {
                                m.insert({tick_index, FlexPauliString(stats.num_qubits)});
                            }
                            m.at(tick_index).value.xs[q] ^= 1;
                        }
                    }
                    for (auto target : tracker.zs[q]) {
                        if (included_targets.contains(target)) {
                            auto &m = result[target];
                            if (!m.contains(tick_index)) {
                                m.insert({tick_index, FlexPauliString(stats.num_qubits)});
                            }
                            m.at(tick_index).value.zs[q] ^= 1;
                        }
                    }
                }
            }
        }
        tracker.undo_gate(inst);
    });
    return result;
}
