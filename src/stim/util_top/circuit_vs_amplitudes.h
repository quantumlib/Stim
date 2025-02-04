#ifndef _STIM_UTIL_TOP_CIRCUIT_VS_AMPLITUDES_H
#define _STIM_UTIL_TOP_CIRCUIT_VS_AMPLITUDES_H

#include "stim/circuit/circuit.h"

namespace stim {

/// Synthesizes a circuit to generate the given state vector.
///
/// Args:
///     stabilizer_state_vector: The vector of amplitudes to produce using a circuit. Does not need to be a unit vector,
///     but must be non-zero.
///     little_endian: Whether the vector is using little endian or big endian ordering.
///     inverted_circuit: If false, returns a circuit that sends |000...0> to the state vector.
///         If true, returns a circuit that sends the state vector to |000...0> instead of a cir.
///
/// Returns:
///     A circuit that outputs the given state vector (up to global phase).
///
/// Throws:
///     std::invalid_argument: The given state vector cannot be produced by a stabilizer circuit.
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
std::vector<std::complex<float>> circuit_to_output_state_vector(const Circuit &circuit, bool little_endian);

}  // namespace stim

#endif
