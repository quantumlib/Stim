#ifndef TABLEAU_H
#define TABLEAU_H

#include <iostream>
#include <immintrin.h>
#include "simd_util.h"
#include "pauli_string.h"

struct TableauQubit {
    PauliString x;
    PauliString y;
};

struct Tableau {
    std::vector<TableauQubit> qubits;

    static Tableau identity(size_t size);
    static Tableau gate1(const char *x,
                         const char *y);
    static Tableau gate2(const char *x1,
                         const char *y1,
                         const char *x2,
                         const char *y2);

    std::string str() const;

    /// Returns the result of applying the tableau to `gathered_input.scatter(scattered_indices)`.
    PauliString scatter_eval(const PauliString &gathered_input, const size_t *scattered_indices) const;

    /// Applies the Tableau inplace to a subset of a Pauli string.
    void apply_within(PauliString &target, const size_t *target_qubits) const;

    void inplace_append(const Tableau &after, const size_t *target_qubits);
    void inplace_prepend(const Tableau &operation, const size_t *target_qubits);
};

std::ostream &operator<<(std::ostream &out, const Tableau &ps);

extern const std::map<std::string, const Tableau> GATE_TABLEAUS;

#endif