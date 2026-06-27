#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>

#include "stim/circuit/circuit_instruction.h"

using namespace stim;

PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.def(
        "test",
        []() -> int {
            std::vector<GateTarget> targets{
                GateTarget::x(0),
                GateTarget::combiner(),
                GateTarget::x(1),
                GateTarget::y(0),
                GateTarget::combiner(),
                GateTarget::y(1),
            };
            CircuitInstruction inst{
                GateType::MPP,
                {},
                targets,
                "",
            };

            size_t num_measure = inst.count_measurement_results();
            if (num_measure != 2) {
                throw std::invalid_argument("WRONG COUNT!");
            }
            return 1;
        });
}
