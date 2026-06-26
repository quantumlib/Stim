#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>

#include "stim/circuit/circuit.pybind.h"

#define xstr_literal(s) str_literal(s)
#define str_literal(s) #s

using namespace stim;
using namespace stim_pybind;

PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.attr("__version__") = xstr_literal(VERSION_INFO);
    m.doc() = R"pbdoc(
        Stim: A fast stabilizer circuit library.
    )pbdoc";

    auto c_circuit = pybind_circuit(m);

    pybind_circuit_methods(m, c_circuit);
}
