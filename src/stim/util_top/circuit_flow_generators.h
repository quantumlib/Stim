#ifndef _STIM_UTIL_TOP_CIRCUIT_FLOW_GENERATORS_H
#define _STIM_UTIL_TOP_CIRCUIT_FLOW_GENERATORS_H

#include "stim/circuit/circuit.h"

namespace stim {

/// Finds a basis of flows for the circuit.
template <size_t W>
std::vector<Flow<W>> circuit_flow_generators(const Circuit &circuit);

template <size_t W>
struct CircuitFlowGeneratorSolver {
    std::vector<Flow<W>> table;
    size_t num_qubits;
    size_t num_measurements;
    size_t num_measurements_in_past;
    std::vector<Flow<W>> measurements_only_table;
    std::vector<size_t> buf_for_rows_with;
    std::vector<int32_t> buf_for_xor_merge;
    std::vector<GateTarget> buf_targets;

    CircuitFlowGeneratorSolver(CircuitStats stats);
    Flow<W> &add_row();
    void add_1q_measure_terms(CircuitInstruction inst, bool x, bool z);
    void add_2q_measure_terms(CircuitInstruction inst, bool x, bool z);
    void remove_single_qubit_reset_terms(CircuitInstruction inst);
    void handle_anticommutations(std::span<const size_t> anticommutation_set);
    void check_for_2q_anticommutations(CircuitInstruction inst, bool x, bool z);
    void check_for_1q_anticommutations(CircuitInstruction inst, bool x, bool z);
    void mult_row_into(size_t src_row, size_t dst_row);
    void undo_mrb(CircuitInstruction inst, bool x, bool z);
    void undo_mb(CircuitInstruction inst, bool x, bool z);
    void undo_rb(CircuitInstruction inst, bool x, bool z);
    void undo_2q_m(CircuitInstruction inst, bool x, bool z);
    void undo_instruction(CircuitInstruction inst);
    void elimination_step(std::span<const size_t> elimination_set, size_t &num_eliminated);
    void canonicalize_over_qubits();
    void final_canonicalize_into_table();
    void undo_feedback_capable_instruction(CircuitInstruction inst, bool x, bool z);
    std::span<const size_t> rows_anticommuting_with(uint32_t q, bool x, bool z);

    template <typename PREDICATE>
    std::span<const size_t> rows_with(PREDICATE predicate);
};

}  // namespace stim

#include "stim/util_top/circuit_flow_generators.inl"

#endif
