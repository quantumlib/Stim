#include <pybind11/pybind11.h>

#include <cstdint>

void repro() {
    uint32_t targets[6]{
        0,
        27,
        0,
        0,
        27,
        0
    };
    uint64_t n = 6;
    for (size_t k = 0; k < 6; k++) {
        if (targets[k] == 27) {
            n -= 2;
        }
    }
    if (n != 2) {
        throw std::invalid_argument("WRONG COUNT!");
    }
}

PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.def("test", &repro);
}
