#ifndef TABLEAU_SIMULATOR_JS_H
#define TABLEAU_SIMULATOR_JS_H

#include <cstddef>
#include <cstdint>
#include <emscripten/val.h>
#include "../../src/simulators/tableau_simulator.h"
#include "circuit.js.h"
#include "pauli_string.js.h"
#include "tableau.js.h"

struct ExposedTableauSimulator {
    TableauSimulator sim;
    ExposedTableauSimulator();
    ExposedTableauSimulator copy() const;
    bool measure(size_t target);
    emscripten::val measure_kickback(size_t target);
    ExposedTableau current_inverse_tableau() const;
    void set_inverse_tableau(const ExposedTableau &tableau);
    void do_circuit(const ExposedCircuit &circuit);
    void do_pauli_string(const ExposedPauliString &pauli_string);
    void do_tableau(const ExposedTableau &tableau, const emscripten::val &targets);
    void X(uint32_t target);
    void Y(uint32_t target);
    void Z(uint32_t target);
    void H(uint32_t target);
    void CNOT(uint32_t control, uint32_t target);
    void CY(uint32_t control, uint32_t target);
    void CZ(uint32_t control, uint32_t target);
};

void emscripten_bind_tableau_simulator();

#endif
