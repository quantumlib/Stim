#include <pybind11/pybind11.h>

#include <span>
#include <cstdint>
#include <iostream>

uint64_t count_measurement_results(std::span<const uint32_t> targets) {
    uint64_t n = (uint64_t)targets.size();
    for (uint32_t e : targets) {
        if (e == 27) {
            n -= 2;
        }
    }
    return n;
}

void repro() {
    std::vector<uint32_t> targets{
        0,
        0,
        27,
    };
    if (count_measurement_results(targets) != 1) {
        throw std::invalid_argument("WRONG COUNT!");
    }
}
PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.def("test", &repro);
}
