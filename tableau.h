#ifndef TABLEAU_H
#define TABLEAU_H

#include <iostream>
#include <unordered_map>
#include <immintrin.h>
#include "simd_util.h"
#include "pauli_string.h"

struct TableauQubit {
    PauliString x;
    PauliString z;

    PauliString eval_y() const;
};

struct Tableau {
    std::vector<TableauQubit> qubits;
    bool operator==(const Tableau &other) const;
    bool operator!=(const Tableau &other) const;
    std::string str() const;

    /// Creates a Tableau representing the identity operation.
    static Tableau identity(size_t num_qubits);

    /// Creates a Tableau representing a single qubit gate.
    ///
    /// All observables specified using the string format accepted by `PauliString::from_str`.
    /// For example: "-X" or "+Y".
    ///
    /// Args:
    ///    x: The output-side observable that the input-side X observable gets mapped to.
    ///    z: The output-side observable that the input-side Y observable gets mapped to.
    static Tableau gate1(const char *x,
                         const char *z);

    /// Creates a Tableau representing a two qubit gate.
    ///
    /// All observables specified using the string format accepted by `PauliString::from_str`.
    /// For example: "-IX" or "+YZ".
    ///
    /// Args:
    ///    x1: The output-side observable that the input-side XI observable gets mapped to.
    ///    z1: The output-side observable that the input-side YI observable gets mapped to.
    ///    x2: The output-side observable that the input-side IX observable gets mapped to.
    ///    z2: The output-side observable that the input-side IY observable gets mapped to.
    static Tableau gate2(const char *x1,
                         const char *z1,
                         const char *x2,
                         const char *z2);

    /// Returns the result of applying the tableau to the given Pauli string.
    ///
    /// Args:
    ///     p: The input-side Pauli string.
    ///
    /// Returns:
    ///     The output-side Pauli string.
    ///     Algebraically: $c p c^{-1}$ where $c$ is the tableau's Clifford operation.
    PauliString operator()(const PauliString &p) const;

    /// Returns the result of applying the tableau to `gathered_input.scatter(scattered_indices)`.
    PauliString scatter_eval(const PauliString &gathered_input, const std::vector<size_t> &scattered_indices) const;

    /// Applies the Tableau inplace to a subset of a Pauli string.
    void apply_within(PauliString &target, const std::vector<size_t> &target_qubits) const;

    /// Appends a smaller operation into this tableau's operation.
    ///
    /// The new value T' of this tableau will equal the composition T o P = PT where T is the old
    /// value of this tableau and P is the operation to append.
    ///
    /// Args:
    ///     operation: The smaller operation to append into this tableau.
    ///     target_qubits: The qubits being acted on by `operation`.
    void inplace_scatter_append(const Tableau &operation, const std::vector<size_t> &target_qubits);

    /// Prepends a smaller operation into this tableau's operation.
    ///
    /// The new value T' of this tableau will equal the composition P o T = TP where T is the old
    /// value of this tableau and P is the operation to append.
    ///
    /// Args:
    ///     operation: The smaller operation to prepend into this tableau.
    ///     target_qubits: The qubits being acted on by `operation`.
    void inplace_scatter_prepend(const Tableau &operation, const std::vector<size_t> &target_qubits);
};

std::ostream &operator<<(std::ostream &out, const Tableau &ps);

/// Tableaus for common gates, keyed by name.
extern const std::unordered_map<std::string, const Tableau> GATE_TABLEAUS;

#endif