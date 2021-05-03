#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "circuit.js.h"
#include "pauli_string.js.h"
#include "tableau.js.h"
#include "tableau_simulator.js.h"

EMSCRIPTEN_BINDINGS(stim) {
    emscripten_bind_circuit();
    emscripten_bind_pauli_string();
    emscripten_bind_tableau();
    emscripten_bind_tableau_simulator();
}
