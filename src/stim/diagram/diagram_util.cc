#include "diagram_util.h"

using namespace stim;
using namespace stim_draw_internal;

std::pair<std::string, std::string> stim_draw_internal::two_qubit_gate_pieces(const std::string &name) {
    std::pair<std::string, std::string> result;
    if (name == "CX" || name == "CNOT" || name == "ZCX") {
        return {"Z", "X"};
    } else if (name == "CY" || name == "ZCY") {
        return {"Z", "Y"};
    } else if (name == "CZ" || name == "ZCZ") {
        return {"Z", "Z"};
    } else if (name == "XCX") {
        return {"X", "X"};
    } else if (name == "XCY") {
        return {"X", "Y"};
    } else if (name == "XCZ") {
        return {"X", "Z"};
    } else if (name == "YCX") {
        return {"Y", "X"};
    } else if (name == "YCY") {
        return {"Y", "Y"};
    } else if (name == "YCZ") {
        return {"Y", "Z"};
    } else {
        return {name, name};
    }
}
