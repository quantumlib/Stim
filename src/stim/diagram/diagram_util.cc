#include "diagram_util.h"

using namespace stim;
using namespace stim_draw_internal;

std::pair<std::string, std::string> stim_draw_internal::two_qubit_gate_pieces(const std::string &name, bool keep_it_short) {
    std::pair<std::string, std::string> result;
    if (name == "CX") {
        result = {"Z_CONTROL", "X_CONTROL"};
    } else if (name == "CY") {
        result = {"Z_CONTROL", "Y_CONTROL"};
    } else if (name == "CZ") {
        result = {"Z_CONTROL", "Z_CONTROL"};
    } else if (name == "XCX") {
        result = {"X_CONTROL", "X_CONTROL"};
    } else if (name == "XCY") {
        result = {"X_CONTROL", "Y_CONTROL"};
    } else if (name == "XCZ") {
        result = {"X_CONTROL", "Z_CONTROL"};
    } else if (name == "YCX") {
        result = {"Y_CONTROL", "X_CONTROL"};
    } else if (name == "YCY") {
        result = {"Y_CONTROL", "Y_CONTROL"};
    } else if (name == "YCZ") {
        result = {"Y_CONTROL", "Z_CONTROL"};
    } else {
        return {name, name};
    }
    if (keep_it_short) {
        return {result.first.substr(0, 1), result.second.substr(0, 1)};
    }
    return result;
}
