#ifndef _STIM_UTIL_TOP_CIRCUIT_INVERSE_QEC_H
#define _STIM_UTIL_TOP_CIRCUIT_INVERSE_QEC_H

#include "stim/circuit/circuit.h"
#include "stim/stabilizers/flow.h"

namespace stim {

template <size_t W>
std::pair<Circuit, std::vector<Flow<W>>> circuit_inverse_qec(
    const Circuit &circuit, std::span<const Flow<W>> flows, bool dont_turn_measurements_into_resets = false);

namespace internal {

struct CircuitFlowReverser {
    CircuitStats stats;
    bool dont_turn_measurements_into_resets;

    SparseUnsignedRevFrameTracker rev;
    simd_bits<64> qubit_workspace;
    size_t num_new_measurements;

    Circuit inverted_circuit;
    std::map<DemTarget, std::string_view> d2tag;
    std::map<DemTarget, std::vector<double>> d2coords;
    std::vector<double> coord_buf;
    std::vector<double> coord_shifts;
    Circuit qubit_coords_circuit;
    std::vector<GateTarget> buf;
    std::map<DemTarget, std::set<size_t>> d2ms;
    std::set<DemTarget> active_terms;
    std::vector<DemTarget> terms_to_erase;

    CircuitFlowReverser(CircuitStats stats, bool dont_turn_measurements_into_resets);

    void recompute_active_terms();
    void do_rp_mrp_instruction(const CircuitInstruction &inst);
    void do_m2r_instruction(const CircuitInstruction &inst);
    void do_measuring_instruction(const CircuitInstruction &inst);
    void do_simple_instruction(const CircuitInstruction &inst);
    void do_feedback_capable_instruction(const CircuitInstruction &inst);
    void flush_detectors_and_observables();

    void do_instruction(const CircuitInstruction &inst);

    template <size_t W>
    void xor_pauli_string_into_tracker_as_target(const PauliString<W> &pauli_string, DemTarget target);

    template <size_t W>
    void xor_flow_ends_into_tracker(std::span<const Flow<W>> flows);

    template <size_t W>
    void xor_flow_measurements_into_tracker(std::span<const Flow<W>> flows);

    template <size_t W>
    void xor_flow_starts_into_tracker(std::span<const Flow<W>> flows);

    template <size_t W>
    void verify_flow_observables_disappeared(std::span<const Flow<W>> flows);

    template <size_t W>
    std::vector<Flow<W>> build_inverted_flows(std::span<const Flow<W>> flows);

    Circuit &&build_and_move_final_inverted_circuit();
};

}  // namespace internal
}  // namespace stim

#include "stim/util_top/circuit_inverse_qec.inl"

#endif
