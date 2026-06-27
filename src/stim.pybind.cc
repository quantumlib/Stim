#include <pybind11/pybind11.h>

#include <span>
#include <cstdint>
#include <iostream>

struct CircuitInstruction {
    uint32_t gate_type;
    std::span<const uint32_t> targets;
    uint64_t count_measurement_results() const {
        uint64_t n = (uint64_t)targets.size();
        std::cerr << "counting start ... " << n << "\n";
        for (auto e : targets) {
            if (e == 27) {
                n -= 2;
            }
        }
        std::cerr << "count final " << n << "\n";
        return n;
    }
};

struct Circuit {
    std::vector<CircuitInstruction> operations;

    template <typename MAP>
    uint64_t max_operation_property(const MAP &map) const {
        uint64_t n = 0;
        for (const auto &op : operations) {
            if (op.gate_type == 8) {
                // Handled in block case.
                continue;
            }
            n = std::max(n, (uint64_t)map(op));
        }
        return n;
    }
};

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
