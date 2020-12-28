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
};

std::ostream &operator<<(std::ostream &out, const Tableau &ps);

extern const std::map<std::string, const Tableau> GATE_TABLEAUS;

#endif