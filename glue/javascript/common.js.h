#ifndef STIM_COMMON_JS_H
#define STIM_COMMON_JS_H

#include <emscripten/val.h>

#include "stim/probability_util.h"

std::mt19937_64 &JS_BIND_SHARED_RNG();

template <typename T>
emscripten::val vec_to_js_array(const std::vector<T> &items) {
    emscripten::val result = emscripten::val::array();
    for (size_t k = 0; k < items.size(); k++) {
        result.set(k, items[k]);
    }
    return result;
}

uint32_t js_val_to_uint32_t(const emscripten::val &val);

#endif
