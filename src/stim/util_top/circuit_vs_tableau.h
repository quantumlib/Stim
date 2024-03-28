#ifndef _STIM_UTIL_TOP_CIRCUIT_VS_TABLEAU_H
#define _STIM_UTIL_TOP_CIRCUIT_VS_TABLEAU_H

#include "stim/circuit/circuit.h"
#include "stim/stabilizers/tableau.h"

namespace stim {

/// Compiles the given circuit into a tableau.
///
/// Args:
///     circuit: The circuit to compile. Should only contain unitary operations.
///     ignore_noise: If the circuit contains noise channels, ignore them instead of raising an exception.
///     ignore_measurement: If the circuit contains measurements, ignore them instead of raising an exception.
///     ignore_reset: If the circuit contains resets, ignore them instead of raising an exception.
///     inverse: The last step of the implementation is to invert the tableau. Setting this argument
///         to true will skip this inversion, saving time but returning the inverse tableau.
///
/// Returns:
///     A tableau encoding the given circuit's Clifford operation.
template <size_t W>
Tableau<W> circuit_to_tableau(
    const Circuit &circuit, bool ignore_noise, bool ignore_measurement, bool ignore_reset, bool inverse = false);

/// Synthesizes a circuit that implements the given tableau's Clifford operation.
///
/// This method is allowed to output different circuits, from call to call or version
/// to version, for the same input tableau.
///
/// Args:
///     tableau: The tableau to synthesize into a circuit.
///     method: The method to use when synthesizing the circuit. Available values:
///         "elimination": Cancels off-diagonal terms using Gaussian elimination.
///             Gate set: H, S, CX
///             Circuit qubit count: n
///             Circuit operation count: O(n^2)
///             Circuit depth: O(n^2)
///         "graph_state": Prepares the tableau's state using a graph state circuit.
///             Gate set: RX, CZ, H, S, X, Y, Z
///             Circuit qubit count: n
///             Circuit operation count: O(n^2)
///
///             The circuit will be made up of three layers:
///                 1. An RX layer initializing all qubits.
///                 2. A CZ layer coupling the qubits.
///                     an edge in the graph state.)
///                 3. A single qubit rotation layer.
///
///             Note: "graph_state" treats the tableau as a state instead of as a
///             Clifford operation. It will preserve the set of stabilizers, but
///             not the exact choice of generators.
///
/// Returns:
///     The synthesized circuit.
template <size_t W>
Circuit tableau_to_circuit(const Tableau<W> &tableau, std::string_view method);
template <size_t W>
Circuit tableau_to_circuit_graph_method(const Tableau<W> &tableau);
template <size_t W>
Circuit tableau_to_circuit_mpp_method(const Tableau<W> &tableau, bool skip_sign);
template <size_t W>
Circuit tableau_to_circuit_elimination_method(const Tableau<W> &tableau);

}  // namespace stim

#include "stim/util_top/circuit_vs_tableau.inl"

#endif
