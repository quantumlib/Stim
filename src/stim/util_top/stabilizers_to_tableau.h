#ifndef _STIM_STABILIZERS_CONVERSIONS_H
#define _STIM_STABILIZERS_CONVERSIONS_H

#include "stim/circuit/circuit.h"
#include "stim/dem/dem_instruction.h"
#include "stim/stabilizers/flex_pauli_string.h"
#include "stim/stabilizers/tableau.h"

namespace stim {

/// Computes destabilizers for the given stabilizers, and packages into a tableau.
///
/// Args:
///     stabilizers: The desired stabilizers for the tableau. Every stabilizer must have the same number of qubits.
///     allow_redundant: If false, including a redundant stabilizer will result in an error.
///         If true, redundant stabilizers are quietly dropped.
///     allow_underconstrained: If false, the number of independent stabilizers must equal the number of qubits in each
///         stabilizer. If true, the returned result will arbitrarily fill in missing stabilizers.
///     invert: Return the inverse tableau instead of the tableau with the stabilizers as its Z outputs.
///
/// Returns:
///     A tableau containing the given stabilizers, but extended to also include matching stabilizers.
///     The Z outputs of the tableau will be the given stabilizers (skipping any redundant ones).
template <size_t W>
Tableau<W> stabilizers_to_tableau(
    const std::vector<stim::PauliString<W>> &stabilizers,
    bool allow_redundant,
    bool allow_underconstrained,
    bool invert);

}  // namespace stim

#include "stim/util_top/stabilizers_to_tableau.inl"

#endif
