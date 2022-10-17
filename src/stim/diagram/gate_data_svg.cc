#include "stim/diagram/gate_data_svg.h"

using namespace stim_draw_internal;

std::map<std::string, SvgGateData> SvgGateData::make_gate_data_map() {
    std::map<std::string, SvgGateData> result;

    result.insert({"X", {1, "X", "", "", "white"}});
    result.insert({"Y", {1, "Y", "", "", "white"}});
    result.insert({"Z", {1, "Z", "", "", "white"}});

    result.insert({"H_YZ", {1, "H", "YZ", "", "white"}});
    result.insert({"H", {1, "H", "", "", "white"}});
    result.insert({"H_XY", {1, "H", "XY", "", "white"}});

    result.insert({"SQRT_X", {1, u8"√X", "", "", "white"}});
    result.insert({"SQRT_Y", {1, u8"√Y", "", "", "white"}});
    result.insert({"S", {1, "S", "", "", "white"}});

    result.insert({"SQRT_X_DAG", {1, u8"√X", "", u8"†", "white"}});
    result.insert({"SQRT_Y_DAG", {1, u8"√Y", "", u8"†", "white"}});
    result.insert({"S_DAG", {1, "S", "", u8"†", "white"}});

    result.insert({"MX", {1, "M", "X", "", "white"}});
    result.insert({"MY", {1, "M", "Y", "", "white"}});
    result.insert({"M", {1, "M", "", "", "white"}});

    result.insert({"RX", {1, "R", "X", "", "white"}});
    result.insert({"RY", {1, "R", "Y", "", "white"}});
    result.insert({"R", {1, "R", "", "", "white"}});

    result.insert({"MRX", {1, "MR", "X", "", "white"}});
    result.insert({"MRY", {1, "MR", "Y", "", "white"}});
    result.insert({"MR", {1, "MR", "", "", "white"}});

    result.insert({"X_ERROR", {1, "ERR", "X", "", "pink"}});
    result.insert({"Y_ERROR", {1, "ERR", "Y", "", "pink"}});
    result.insert({"Z_ERROR", {1, "ERR", "Z", "", "pink"}});

    result.insert({"E[X]", {1, "E", "X", "", "pink"}});
    result.insert({"E[Y]", {1, "E", "Y", "", "pink"}});
    result.insert({"E[Z]", {1, "E", "Z", "", "pink"}});

    result.insert({"ELSE_CORRELATED_ERROR[X]", {1, "EE", "X", "", "pink"}});
    result.insert({"ELSE_CORRELATED_ERROR[Y]", {1, "EE", "Y", "", "pink"}});
    result.insert({"ELSE_CORRELATED_ERROR[Z]", {1, "EE", "Z", "", "pink"}});

    result.insert({"MPP[X]", {1, "MPP", "X", "", "white"}});
    result.insert({"MPP[Y]", {1, "MPP", "Y", "", "white"}});
    result.insert({"MPP[Z]", {1, "MPP", "Z", "", "white"}});

    result.insert({"SQRT_XX", {1, u8"√XX", "", "", "white"}});
    result.insert({"SQRT_YY", {1, u8"√YY", "", "", "white"}});
    result.insert({"SQRT_ZZ", {1, u8"√ZZ", "", "", "white"}});

    result.insert({"SQRT_XX_DAG", {1, u8"√XX", "", u8"†", "white"}});
    result.insert({"SQRT_YY_DAG", {1, u8"√YY", "", u8"†", "white"}});
    result.insert({"SQRT_ZZ_DAG", {1, u8"√ZZ", "", u8"†", "white"}});

    result.insert({"I", {1, "I", "", "", "white"}});
    result.insert({"C_XYZ", {1, "C", "XYZ", "", "white"}});
    result.insert({"C_ZYX", {1, "C", "ZYX", "", "white"}});

    result.insert({"DEPOLARIZE1", {1, "DEP", "1", "", "pink"}});
    result.insert({"DEPOLARIZE2", {1, "DEP", "2", "", "pink"}});

    result.insert({"PAULI_CHANNEL_1", {4, "PAULI_CHANNEL_1", "", "", "pink"}});
    result.insert({"PAULI_CHANNEL_2[0]", {16, "PAULI_CHANNEL_2", "0", "", "pink"}});
    result.insert({"PAULI_CHANNEL_2[1]", {16, "PAULI_CHANNEL_2", "1", "", "pink"}});

    return result;
}
