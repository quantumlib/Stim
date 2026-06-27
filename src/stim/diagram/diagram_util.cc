#include "stim/diagram/diagram_util.h"

using namespace stim;
using namespace stim_draw_internal;

std::pair<std::string_view, std::string_view> stim_draw_internal::two_qubit_gate_pieces(GateType gate_type) {
    if (gate_type == GateType::CX) {
        return {"Z", "X"};
    } else if (gate_type == GateType::CY) {
        return {"Z", "Y"};
    } else if (gate_type == GateType::CZ) {
        return {"Z", "Z"};
    } else if (gate_type == GateType::XCX) {
        return {"X", "X"};
    } else if (gate_type == GateType::XCY) {
        return {"X", "Y"};
    } else if (gate_type == GateType::XCZ) {
        return {"X", "Z"};
    } else if (gate_type == GateType::YCX) {
        return {"Y", "X"};
    } else if (gate_type == GateType::YCY) {
        return {"Y", "Y"};
    } else if (gate_type == GateType::YCZ) {
        return {"Y", "Z"};
    } else if (gate_type == GateType::CXSWAP) {
        return {"ZSWAP", "XSWAP"};
    } else if (gate_type == GateType::CZSWAP) {
        return {"ZSWAP", "ZSWAP"};
    } else if (gate_type == GateType::SWAPCX) {
        return {"XSWAP", "ZSWAP"};
    } else {
        auto name = GATE_DATA[gate_type].name;
        return {name, name};
    }
}

size_t stim_draw_internal::utf8_char_count(std::string_view s) {
    size_t t = 0;
    for (uint8_t c : s) {
        // Continuation bytes start with "10" in binary.
        if ((c & 0xC0) != 0x80) {
            t++;
        }
    }
    return t;
}

void stim_draw_internal::add_coord_summary_to_ss(std::ostream &ss, std::vector<double> vec) {
    bool first = true;
    for (const auto &c : vec) {
        if (first) {
            ss << ":";
        } else {
            ss << "_";
        }
        ss << c;
        first = false;
    }
}