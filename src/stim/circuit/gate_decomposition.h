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

#ifndef _STIM_GATE_DECOMPOSITION_H
#define _STIM_GATE_DECOMPOSITION_H

#include "stim/circuit/circuit_instruction.h"
#include "stim/circuit/gate_data.h"
#include "stim/circuit/gate_target.h"
#include "stim/mem/simd_bits.h"

namespace stim {

/// Decomposes MPP operations into sequences of simpler operations with the same effect.
///
/// The idea is that an instruction like
///
///     MPP X0*Z1*Y2 X3*X4 Y0*Y1*Y2*Y3*Y4
///
/// can be decomposed into a sequence of instructions like
///
///     H_XZ 0 3 4
///     H_YZ 2
///     CX 1 0 2 0 4 3
///     M 0 3
///     CX 1 0 2 0 4 3
///     H_YZ 2
///     H_XZ 0 3 4
///
///     H_YZ 0 1 2 3 4
///     CX 1 0 2 0 3 0 4 0
///     M 0
///     CX 1 0 2 0 3 0 4 0
///     H_YZ 0 1 2 3 4
///
/// This is tedious to do, so this method does it for you.
///
/// Args:
///     mpp_op: The operation to decompose.
///     num_qubits: The number of qubits in the system. All targets must be less than this.
///     callback: The method told each chunk of the decomposition.
void decompose_mpp_operation(
    const CircuitInstruction &mpp_op,
    size_t num_qubits,
    const std::function<void(
        const CircuitInstruction &h_xz,
        const CircuitInstruction &h_yz,
        const CircuitInstruction &cnot,
        const CircuitInstruction &meas)> &callback);

/// Finds contiguous segments where the first target of each pair is used once.
///
/// This is used when decomposing operations like MXX into CX and MX. The CX
/// gates can overlap on their targets, but the measurements can't overlap with
/// each other and the measurements can't overlap with the controls of the CX
/// gates.
///
/// The idea is that an instruction like
///
///     MXX 0 1 0 2 3 5 4 5 3 4
///
/// can be decomposed into a sequence of instructions like
///
///     CX 0 1
///     MX 0
///     CX 0 1
///
///     CX 0 2 3 5 4 5
///     MX 0 3 4
///     CX 0 2 3 5 4 5
///
///     CX 3 4
///     MX 3
///     CX 3 4
///
/// Args:
///     num_qubits: The number of qubits in the system. All targets in the circuit
///         instruction must be less than this.
///     inst: The circuit instruction to decompose.
///     callback: The method called with each decomposed segment.
void decompose_pair_instruction_into_segments_with_single_use_controls(
    const CircuitInstruction &inst, size_t num_qubits, const std::function<void(CircuitInstruction)> &callback);

}  // namespace stim

#endif
