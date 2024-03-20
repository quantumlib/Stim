#ifndef _STIM_UTIL_TOP_STABILIZERS_VS_AMPLITUDES_H
#define _STIM_UTIL_TOP_STABILIZERS_VS_AMPLITUDES_H

#include "stim/circuit/circuit.h"
#include "stim/stabilizers/tableau.h"

namespace stim {

/// Converts a tableau into a unitary matrix.
template <size_t W>
std::vector<std::vector<std::complex<float>>> tableau_to_unitary(const Tableau<W> &tableau, bool little_endian);

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
