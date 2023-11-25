/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STIM_SIMULATORS_GRAPH_SIMULATOR_H
#define _STIM_SIMULATORS_GRAPH_SIMULATOR_H

#include "stim/circuit/circuit.h"
#include "stim/mem/simd_bit_table.h"
#include "stim/mem/simd_bits.h"
#include "stim/stabilizers/pauli_string.h"

namespace stim {

struct GraphSimulator {
    // RX applied to each qubit.
    size_t num_qubits;
    // Then CZs applied according to this adjacency matrix.
    simd_bit_table<64> adj;
    // Then Paulis adding sign data.
    PauliString<64> paulis;
    // Then an unsigned Clifford mapping.
    PauliString<64> x2outs;
    PauliString<64> z2outs;
    // Used as temporary workspace.
    std::vector<size_t> buffer;

    explicit GraphSimulator(size_t num_qubits);
    static GraphSimulator random_state(size_t n, std::mt19937_64 &rng);

    Circuit to_circuit(bool to_hs_xyz = false) const;

    void do_circuit(const Circuit &circuit);
    void do_instruction(const CircuitInstruction &instruction);
    void do_complementation(size_t c);
    void verify_invariants() const;
    void inside_do_sqrt_z(size_t q);
    void inside_do_sqrt_x_dag(size_t q);
    std::tuple<bool, bool, bool> after2inside_basis_transform(size_t qubit, bool x, bool z);

    std::string str() const;

   private:
    void output_pauli_layer(Circuit &out, bool to_hs_xyz) const;
    void output_clifford_layer(Circuit &out, bool to_hs_xyz) const;

    void do_1q_gate(GateType gate, size_t qubit);
    void do_2q_unitary_instruction(const CircuitInstruction &inst);
    void do_pauli_interaction(bool x1, bool z1, bool x2, bool z2, size_t qubit1, size_t qubit2);
    void do_gate_by_decomposition(const CircuitInstruction &inst);

    // These operations apply to the state inside of the single qubit gates.
    void inside_do_cz(size_t a, size_t b);
    void inside_do_cx(size_t c, size_t t);
    void inside_do_cy(size_t c, size_t t);
    void inside_do_ycx(size_t q1, size_t q2);
    void inside_do_ycy(size_t q1, size_t q2);
    void inside_do_xcx(size_t q1, size_t q2);
    void inside_do_pauli_interaction(bool x1, bool z1, bool x2, bool z2, size_t q1, size_t q2);
};

std::ostream &operator<<(std::ostream &out, const GraphSimulator &sim);

}  // namespace stim

#endif
