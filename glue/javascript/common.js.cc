#include "common.js.h"

using namespace stim;

uint32_t js_val_to_uint32_t(const emscripten::val &val) {
    double v = val.as<double>();
    double f = floor(v);
    if (v != f || v < 0 || v > UINT32_MAX) {
        throw std::out_of_range("Number isn't a uint32_t: " + std::to_string(v));
    }
    return (uint32_t)f;
}
