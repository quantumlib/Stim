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

#ifndef _STIM_UTIL_TOP_STABILIZERS_VS_AMPLITUDES_H
#define _STIM_UTIL_TOP_STABILIZERS_VS_AMPLITUDES_H

#include "stim/circuit/circuit.h"
#include "stim/stabilizers/tableau.h"

namespace stim {

/// Converts a tableau into a unitary matrix.
template <size_t W>
std::vector<std::vector<std::complex<float>>> tableau_to_unitary(const Tableau<W> &tableau, bool little_endian);

/// Synthesizes a circuit to generate the given state vector.
///
/// Args:
///     stabilizer_state_vector: The vector of amplitudes to produce using a circuit.
///     little_endian: Whether the vector is using little endian or big endian ordering.
///     inverted_circuit: If false, returns a circuit that sends |000...0> to the state vector.
///         If true, returns a circuit that sends the state vector to |000...0> instead of a cir.
///
/// Returns:
///     A circuit that outputs the given state vector (up to global phase).
///
/// Throws:
///     std::invalid_argument: The given state vector cannot be produced by a stabilizer circuit.
template <size_t W>
Circuit stabilizer_state_vector_to_circuit(
    const std::vector<std::complex<float>> &stabilizer_state_vector, bool little_endian);

/// Simulates the given circuit and outputs a state vector.
///
/// Args:
///     circuit: The circuit to simulate. Cannot contain noisy or dissipative operations.
///     little_endian: Whether the returned vector uses little endian or big endian qubit order.
///
/// Returns:
///     The state vector, using the requested endianness.
template <size_t W>
std::vector<std::complex<float>> circuit_to_output_state_vector(const Circuit &circuit, bool little_endian);

/// Converts a unitary matrix into a stabilizer tableau.
///
/// Args:
///     matrix: The unitary matrix to convert. Must correspond to a Clifford.
//      little_endian: Whether the amplitude ordering is little endian or big endian.
///
/// Returns:
///     A tableau implementing the same operation as the unitary matrix (up to global phase).
///
/// Throws:
///     std::invalid_argument: The given unitary matrix isn't a Clifford operation.
template <size_t W>
Tableau<W> unitary_to_tableau(const std::vector<std::vector<std::complex<float>>> &matrix, bool little_endian);

}  // namespace stim

#include "stim/util_top/stabilizers_vs_amplitudes.inl"

#endif
