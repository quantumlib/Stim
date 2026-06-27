#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>

#include "stim/py/base.pybind.h"
#include "stim/py/numpy.pybind.h"
#include "stim/circuit/circuit.h"

using namespace stim;
using namespace stim_pybind;

PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.def(
        "test",
        []() -> int {
            Circuit circuit(R"CIRCUIT(
                MPP X0*X1 Y0*Y1
            )CIRCUIT");
            size_t num_measure = circuit.count_measurements();
            if (num_measure != 2) {
                throw std::invalid_argument("WRONG COUNT!");
            }
            return 1;
        });
}
