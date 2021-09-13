#include "common.js.h"

using namespace stim;

static bool shared_rng_initialized;
static std::mt19937_64 shared_rng;

std::mt19937_64 &JS_BIND_SHARED_RNG() {
    if (!shared_rng_initialized) {
        shared_rng = externally_seeded_rng();
        shared_rng_initialized = true;
    }
    return shared_rng;
}

uint32_t js_val_to_uint32_t(const emscripten::val &val) {
    double v = val.as<double>();
    double f = floor(v);
    if (v != f || v < 0 || v > UINT32_MAX) {
        throw std::out_of_range("Number isn't a uint32_t: " + std::to_string(v));
    }
    return (uint32_t)f;
}
