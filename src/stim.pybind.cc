#include <pybind11/pybind11.h>

#include <cstdint>
#include <iostream>

uint64_t count_measurement_results(const uint32_t *targets, size_t count) {
    uint64_t n = (uint64_t)count;
    for (size_t k = 0; k < count; k++) {
        if (targets[k] == 27) {
            n -= 2;
        }
    }
    return n;
}

void repro() {
    uint32_t targets[3]{
        0,
        0,
        27,
    };
    if (count_measurement_results(targets, 3) != 1) {
        throw std::invalid_argument("WRONG COUNT!");
    }
}
PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.def("test", &repro);
}
