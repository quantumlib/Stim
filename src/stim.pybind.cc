#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>

#include "circuit_instruction.h"

using namespace stim;

PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.def(
        "test",
        []() -> int {
            std::vector<uint32_t> targets{
                0,
                27,
                1,
                2,
                27,
                3,
            };
            CircuitInstruction inst{
                33,
                targets,
            };

            size_t num_measure = inst.count_measurement_results();
            if (num_measure != 2) {
                throw std::invalid_argument("WRONG COUNT!");
            }
            return 1;
        });
}
