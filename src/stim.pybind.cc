#include <pybind11/pybind11.h>

#include <cstdint>
#include <iostream>

void repro() {
    int targets[6]{0, 1, 0, 0, 1, 0};
    uint64_t n = 6;
    for (int k = 0; k < 6; k++) {
        if (targets[k]) {
            n -= 2;
        }
    }
    std::cerr << "n=" << n << "\n";
    if (n != 2) {
        throw std::invalid_argument("WRONG COUNT!");
    }
}

PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.def("test", &repro);
}
