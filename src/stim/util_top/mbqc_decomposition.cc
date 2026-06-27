#include "stim/util_top/mbqc_decomposition.h"

using namespace stim;

const char *stim::mbqc_decomposition(GateType gate) {
    switch (gate) {
        case GateType::NOT_A_GATE:
        case GateType::DETECTOR:
        case GateType::OBSERVABLE_INCLUDE:
        case GateType::TICK:
        case GateType::QUBIT_COORDS:
        case GateType::SHIFT_COORDS:
        case GateType::REPEAT:
        case GateType::MPAD:
        case GateType::DEPOLARIZE1:
        case GateType::DEPOLARIZE2:
        case GateType::X_ERROR:
        case GateType::Y_ERROR:
        case GateType::Z_ERROR:
        case GateType::I_ERROR:
        case GateType::II_ERROR:
        case GateType::PAULI_CHANNEL_1:
        case GateType::PAULI_CHANNEL_2:
        case GateType::E:
        case GateType::ELSE_CORRELATED_ERROR:
        case GateType::HERALDED_ERASE:
        case GateType::HERALDED_PAULI_CHANNEL_1:
            return nullptr;
        case GateType::MX:
            return R"CIRCUIT(
MX 0
            )CIRCUIT";
        case GateType::MY:
            return R"CIRCUIT(
MY 0
            )CIRCUIT";
        case GateType::M:
            return R"CIRCUIT(
MZ 0
            )CIRCUIT";
        case GateType::MRX:
            return R"CIRCUIT(
MX 0
CZ rec[-1] 0
            )CIRCUIT";
        case GateType::MRY:
            return R"CIRCUIT(
MY 0
CX rec[-1] 0
            )CIRCUIT";
        case GateType::MR:
            return R"CIRCUIT(
MZ 0
CX rec[-1] 0
            )CIRCUIT";
        case GateType::RX:
            return R"CIRCUIT(
MX 0
CZ rec[-1] 0
            )CIRCUIT";
        case GateType::RY:
            return R"CIRCUIT(
MY 0
CX rec[-1] 0
            )CIRCUIT";
        case GateType::R:
            return R"CIRCUIT(
MZ 0
CX rec[-1] 0
            )CIRCUIT";
        case GateType::XCX:
            return R"CIRCUIT(
MX 2
MZZ 0 2
MY 2
MXX 0 2
MZ 2
MXX 1 2
MZZ 0 2
MX 2
MZ 2
MXX 0 2
MY 2
MZZ 0 2
MX 2
CX rec[-11] 0 rec[-10] 1 rec[-8] 0 rec[-6] 0 rec[-3] 0 rec[-13] 1 rec[-12] 1 rec[-7] 1
CY rec[-10] 0 rec[-9] 0 rec[-5] 0 rec[-4] 0
CZ rec[-13] 0 rec[-12] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::XCY:
            return R"CIRCUIT(
MY 2
MXX 1 2
MZ 2
MXX 0 2
MZZ 1 2
MX 2
MZ 2
MXX 1 2
MY 2
X 0
CX rec[-9] 0 rec[-9] 1 rec[-8] 0 rec[-8] 1 rec[-5] 0 rec[-7] 1 rec[-3] 1 rec[-2] 1 rec[-1] 1
CY rec[-6] 1 rec[-4] 1
            )CIRCUIT";
        case GateType::XCZ:
            return R"CIRCUIT(
MZ 2
MXX 0 2
MZZ 1 2
MX 2
CX rec[-4] 0 rec[-2] 0
CZ rec[-3] 1 rec[-1] 1
            )CIRCUIT";
        case GateType::YCX:
            return R"CIRCUIT(
MY 2
MXX 0 2
MZ 2
MXX 1 2
MZZ 0 2
MX 2
MZ 2
MXX 0 2
MY 2
X 1
CX rec[-9] 0 rec[-9] 1 rec[-8] 0 rec[-8] 1 rec[-7] 0 rec[-3] 0 rec[-2] 0 rec[-1] 0 rec[-5] 1
CY rec[-6] 0 rec[-4] 0
            )CIRCUIT";
        case GateType::YCY:
            return R"CIRCUIT(
MX 2
MZZ 1 2
MY 2
MXX 0 2
MZ 2
MXX 1 2
MZZ 0 2
MX 2
MZ 2
MXX 0 2
MY 2
MZZ 1 2
MX 2
Y 1
CX rec[-10] 0 rec[-9] 0 rec[-5] 0 rec[-4] 0 rec[-3] 0 rec[-11] 1
CY rec[-13] 0 rec[-12] 0 rec[-10] 1 rec[-8] 0 rec[-6] 0 rec[-7] 1
CZ rec[-13] 1 rec[-12] 1 rec[-11] 0 rec[-3] 1 rec[-2] 1 rec[-1] 1
            )CIRCUIT";
        case GateType::YCZ:
            return R"CIRCUIT(
MX 2
MZZ 0 2
MY 2
MZ 2
MXX 0 2
MZZ 1 2
MX 2
MZZ 0 2
MY 2
Z 0
CY rec[-6] 0 rec[-4] 0
CZ rec[-9] 0 rec[-9] 1 rec[-8] 0 rec[-8] 1 rec[-7] 0 rec[-7] 1 rec[-3] 0 rec[-3] 1 rec[-2] 0 rec[-1] 0 rec[-5] 1
            )CIRCUIT";
        case GateType::CX:
            return R"CIRCUIT(
MX 2
MZZ 0 2
MXX 1 2
MZ 2
CX rec[-3] 1 rec[-1] 1
CZ rec[-4] 0 rec[-2] 0
            )CIRCUIT";
        case GateType::CY:
            return R"CIRCUIT(
MY 2
MZZ 1 2
MX 2
MZZ 0 2
MXX 1 2
MZ 2
MX 2
MZZ 1 2
MY 2
Z 0
CY rec[-6] 1 rec[-4] 1
CZ rec[-9] 0 rec[-9] 1 rec[-8] 0 rec[-8] 1 rec[-5] 0 rec[-7] 1 rec[-3] 1 rec[-2] 1 rec[-1] 1
            )CIRCUIT";
        case GateType::CZ:
            return R"CIRCUIT(
MZ 2
MXX 0 2
MY 2
MZZ 0 2
MX 2
MZZ 1 2
MXX 0 2
MZ 2
MX 2
MZZ 0 2
MY 2
MXX 0 2
MZ 2
CX rec[-13] 0 rec[-12] 0 rec[-2] 0 rec[-1] 0
CY rec[-10] 0 rec[-9] 0 rec[-5] 0 rec[-4] 0
CZ rec[-11] 0 rec[-10] 1 rec[-8] 0 rec[-6] 0 rec[-3] 0 rec[-13] 1 rec[-12] 1 rec[-7] 1
            )CIRCUIT";
        case GateType::H:
            return R"CIRCUIT(
MX 1
MZZ 0 1
MY 1
MXX 0 1
MZ 1
MX 1
MZZ 0 1
MY 1
CX rec[-8] 0 rec[-7] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-6] 0 rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::H_XY:
            return R"CIRCUIT(
MX 1
MZZ 0 1
MY 1
X 0
CZ rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::H_YZ:
            return R"CIRCUIT(
MZ 1
MXX 0 1
MY 1
Z 0
CX rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::H_NXY:
            return R"CIRCUIT(
MX 1
MZZ 0 1
MY 1
Y 0
CZ rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::H_NXZ:
            return R"CIRCUIT(
MX 1
MZZ 0 1
MY 1
MXX 0 1
MZ 1
MX 1
MZZ 0 1
MY 1
Y 0
CX rec[-8] 0 rec[-7] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-6] 0 rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::H_NYZ:
            return R"CIRCUIT(
MZ 1
MXX 0 1
MY 1
Y 0
CX rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::I:
            return R"CIRCUIT(
            )CIRCUIT";
        case GateType::X:
            return R"CIRCUIT(
X 0
            )CIRCUIT";
        case GateType::Y:
            return R"CIRCUIT(
Y 0
            )CIRCUIT";
        case GateType::Z:
            return R"CIRCUIT(
Z 0
            )CIRCUIT";
        case GateType::C_XYZ:
            return R"CIRCUIT(
MZ 1
MXX 0 1
MY 1
MZZ 0 1
MX 1
CX rec[-3] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::C_ZYX:
            return R"CIRCUIT(
MX 1
MZZ 0 1
MY 1
MXX 0 1
MZ 1
CX rec[-2] 0 rec[-1] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-3] 0
            )CIRCUIT";
        case GateType::C_NXYZ:
            return R"CIRCUIT(
MZ 1
MXX 0 1
MY 1
MZZ 0 1
MX 1
Z 0
CX rec[-3] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::C_XNYZ:
            return R"CIRCUIT(
MZ 1
MXX 0 1
MY 1
MZZ 0 1
MX 1
X 0
CX rec[-3] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::C_XYNZ:
            return R"CIRCUIT(
MZ 1
MXX 0 1
MY 1
MZZ 0 1
MX 1
Y 0
CX rec[-3] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::C_NZYX:
            return R"CIRCUIT(
MX 1
MZZ 0 1
MY 1
MXX 0 1
MZ 1
X 0
CX rec[-2] 0 rec[-1] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-3] 0
            )CIRCUIT";
        case GateType::C_ZNYX:
            return R"CIRCUIT(
MX 1
MZZ 0 1
MY 1
MXX 0 1
MZ 1
Z 0
CX rec[-2] 0 rec[-1] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-3] 0
            )CIRCUIT";
        case GateType::C_ZYNX:
            return R"CIRCUIT(
MX 1
MZZ 0 1
MY 1
MXX 0 1
MZ 1
Y 0
CX rec[-2] 0 rec[-1] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-3] 0
            )CIRCUIT";
        case GateType::SQRT_X:
            return R"CIRCUIT(
MZ 1
MXX 0 1
MY 1
CX rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::SQRT_X_DAG:
            return R"CIRCUIT(
MY 1
MXX 0 1
MZ 1
CX rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::SQRT_Y:
            return R"CIRCUIT(
MX 1
MZZ 0 1
MY 1
MXX 0 1
MZ 1
MX 1
MZZ 0 1
MY 1
X 0
CX rec[-8] 0 rec[-7] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-6] 0 rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::SQRT_Y_DAG:
            return R"CIRCUIT(
MX 1
MZZ 0 1
MY 1
MXX 0 1
MZ 1
MX 1
MZZ 0 1
MY 1
Z 0
CX rec[-8] 0 rec[-7] 0
CY rec[-5] 0 rec[-4] 0
CZ rec[-6] 0 rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::S:
            return R"CIRCUIT(
MY 1
MZZ 0 1
MX 1
CZ rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::S_DAG:
            return R"CIRCUIT(
MX 1
MZZ 0 1
MY 1
CZ rec[-3] 0 rec[-2] 0 rec[-1] 0
            )CIRCUIT";
        case GateType::II:
            return "";
        case GateType::SQRT_XX:
            return R"CIRCUIT(
MX 2
MZZ 0 2
MY 2
MXX 0 2
MZ 2
MXX 1 2
MZZ 0 2
MX 2
MY 2
MXX 1 2
MZ 2
MXX 0 2
MY 2
MZZ 0 2
MX 2
MZ 2
MXX 0 2
MY 2
X 1
CX rec[-18] 1 rec[-17] 1 rec[-16] 0 rec[-13] 0 rec[-11] 0 rec[-6] 0 rec[-3] 0 rec[-2] 0 rec[-1] 0 rec[-15] 1 rec[-12] 1 rec[-10] 1 rec[-9] 1 rec[-8] 1
CY rec[-18] 0 rec[-17] 0 rec[-5] 0 rec[-4] 0
CZ rec[-15] 0 rec[-14] 0 rec[-8] 0 rec[-7] 0
            )CIRCUIT";
        case GateType::SQRT_XX_DAG:
            return R"CIRCUIT(
MX 2
MZZ 0 2
MY 2
MXX 0 2
MZ 2
MXX 1 2
MZZ 0 2
MX 2
MY 2
MXX 1 2
MZ 2
MXX 0 2
MY 2
MZZ 0 2
MX 2
MZ 2
MXX 0 2
MY 2
X 0
CX rec[-18] 1 rec[-17] 1 rec[-16] 0 rec[-13] 0 rec[-11] 0 rec[-6] 0 rec[-3] 0 rec[-2] 0 rec[-1] 0 rec[-15] 1 rec[-12] 1 rec[-10] 1 rec[-9] 1 rec[-8] 1
CY rec[-18] 0 rec[-17] 0 rec[-5] 0 rec[-4] 0
CZ rec[-15] 0 rec[-14] 0 rec[-8] 0 rec[-7] 0
            )CIRCUIT";
        case GateType::SQRT_YY:
            return R"CIRCUIT(
MX 2
MZZ 1 2
MY 2
MXX 0 2
MZ 2
MXX 1 2
MZZ 0 2
MX 2
MZZ 0 2
MY 2
MXX 0 2
MZ 2
MXX 1 2
MY 2
MZZ 1 2
MX 2
X 0
Y 1
CX rec[-16] 1 rec[-15] 1 rec[-14] 0 rec[-6] 0 rec[-5] 0 rec[-3] 1
CY rec[-16] 0 rec[-15] 0 rec[-11] 0 rec[-8] 0 rec[-5] 1 rec[-13] 1 rec[-10] 1 rec[-4] 1
CZ rec[-13] 0 rec[-12] 0 rec[-7] 0 rec[-14] 1 rec[-2] 1 rec[-1] 1
            )CIRCUIT";
        case GateType::SQRT_YY_DAG:
            return R"CIRCUIT(
MX 2
MZZ 1 2
MY 2
MXX 0 2
MZ 2
MXX 1 2
MZZ 0 2
MX 2
MZZ 0 2
MY 2
MXX 0 2
MZ 2
MXX 1 2
MY 2
MZZ 1 2
MX 2
Z 0
CX rec[-16] 1 rec[-15] 1 rec[-14] 0 rec[-6] 0 rec[-5] 0 rec[-3] 1
CY rec[-16] 0 rec[-15] 0 rec[-11] 0 rec[-8] 0 rec[-5] 1 rec[-13] 1 rec[-10] 1 rec[-4] 1
CZ rec[-13] 0 rec[-12] 0 rec[-7] 0 rec[-14] 1 rec[-2] 1 rec[-1] 1
            )CIRCUIT";
        case GateType::SQRT_ZZ:
            return R"CIRCUIT(
MZ 2
MXX 0 2
MY 2
MZZ 0 2
MX 2
MZZ 1 2
MXX 0 2
MZ 2
MY 2
MZZ 1 2
MX 2
MZZ 0 2
MY 2
MXX 0 2
MZ 2
MX 2
MZZ 0 2
MY 2
Z 0
CX rec[-15] 0 rec[-14] 0 rec[-8] 0 rec[-7] 0
CY rec[-18] 0 rec[-17] 0 rec[-5] 0 rec[-4] 0
CZ rec[-18] 1 rec[-17] 1 rec[-16] 0 rec[-13] 0 rec[-11] 0 rec[-6] 0 rec[-3] 0 rec[-2] 0 rec[-1] 0 rec[-15] 1 rec[-12] 1 rec[-10] 1 rec[-9] 1 rec[-8] 1
            )CIRCUIT";
        case GateType::SQRT_ZZ_DAG:
            return R"CIRCUIT(
MZ 2
MXX 0 2
MY 2
MZZ 0 2
MX 2
MZZ 1 2
MXX 0 2
MZ 2
MY 2
MZZ 1 2
MX 2
MZZ 0 2
MY 2
MXX 0 2
MZ 2
MX 2
MZZ 0 2
MY 2
Z 1
CX rec[-15] 0 rec[-14] 0 rec[-8] 0 rec[-7] 0
CY rec[-18] 0 rec[-17] 0 rec[-5] 0 rec[-4] 0
CZ rec[-18] 1 rec[-17] 1 rec[-16] 0 rec[-13] 0 rec[-11] 0 rec[-6] 0 rec[-3] 0 rec[-2] 0 rec[-1] 0 rec[-15] 1 rec[-12] 1 rec[-10] 1 rec[-9] 1 rec[-8] 1
            )CIRCUIT";
        case GateType::SWAP:
            return R"CIRCUIT(
MZ 2
MXX 0 2
MZZ 1 2
MX 2
MZZ 0 2
MXX 1 2
MZ 2
MXX 0 2
MZZ 1 2
MX 2
CX rec[-6] 0 rec[-6] 1 rec[-2] 0 rec[-10] 1 rec[-8] 1 rec[-4] 1
CZ rec[-9] 0 rec[-5] 0 rec[-5] 1 rec[-7] 1 rec[-3] 1 rec[-1] 1
            )CIRCUIT";
        case GateType::ISWAP:
            return R"CIRCUIT(
MX 2
MZZ 0 2
MY 2
MZZ 1 2
MX 2
MZ 2
MXX 0 2
MY 2
MZZ 0 2
MX 2
MZZ 0 2
MXX 1 2
MZ 2
MXX 0 2
MZZ 1 2
MX 2
MZZ 1 2
MY 2
MXX 1 2
MZ 2
Z 1
CX rec[-10] 0 rec[-6] 0 rec[-15] 1 rec[-14] 1 rec[-2] 1 rec[-1] 1
CY rec[-12] 1 rec[-9] 1 rec[-7] 1 rec[-4] 1
CZ rec[-18] 0 rec[-18] 1 rec[-17] 0 rec[-16] 0 rec[-15] 0 rec[-14] 0 rec[-12] 0 rec[-9] 0 rec[-20] 1 rec[-19] 1 rec[-13] 1 rec[-10] 1 rec[-8] 1 rec[-3] 1
            )CIRCUIT";
        case GateType::CXSWAP:
            return R"CIRCUIT(
MZ 2
MXX 0 2
MZZ 1 2
MX 2
MZZ 0 2
MXX 1 2
MZ 2
CX rec[-7] 0 rec[-7] 1 rec[-5] 0 rec[-5] 1 rec[-3] 1 rec[-1] 1
CZ rec[-6] 0 rec[-6] 1 rec[-2] 0 rec[-4] 1
            )CIRCUIT";
        case GateType::SWAPCX:
            return R"CIRCUIT(
MX 2
MZZ 0 2
MXX 1 2
MZ 2
MXX 0 2
MZZ 1 2
MX 2
CX rec[-6] 0 rec[-6] 1 rec[-2] 0 rec[-4] 1
CZ rec[-7] 0 rec[-7] 1 rec[-5] 0 rec[-5] 1 rec[-3] 1 rec[-1] 1
            )CIRCUIT";
        case GateType::CZSWAP:
            return R"CIRCUIT(
MZ 2
MXX 0 2
MY 2
MZZ 0 2
MX 2
MZZ 0 2
MXX 1 2
MZ 2
MXX 0 2
MZZ 1 2
MX 2
MZZ 1 2
MY 2
MXX 1 2
MZ 2
CX rec[-10] 0 rec[-6] 0 rec[-15] 1 rec[-14] 1 rec[-2] 1 rec[-1] 1
CY rec[-12] 1 rec[-9] 1 rec[-7] 1 rec[-4] 1
CZ rec[-15] 0 rec[-14] 0 rec[-12] 0 rec[-9] 0 rec[-13] 1 rec[-10] 1 rec[-8] 1 rec[-3] 1
            )CIRCUIT";
        case GateType::ISWAP_DAG:
            return R"CIRCUIT(
MX 2
MZZ 0 2
MY 2
MZZ 1 2
MX 2
MZ 2
MXX 0 2
MY 2
MZZ 0 2
MX 2
MZZ 0 2
MXX 1 2
MZ 2
MXX 0 2
MZZ 1 2
MX 2
MZZ 1 2
MY 2
MXX 1 2
MZ 2
Z 0
CX rec[-10] 0 rec[-6] 0 rec[-15] 1 rec[-14] 1 rec[-2] 1 rec[-1] 1
CY rec[-12] 1 rec[-9] 1 rec[-7] 1 rec[-4] 1
CZ rec[-18] 0 rec[-18] 1 rec[-17] 0 rec[-16] 0 rec[-15] 0 rec[-14] 0 rec[-12] 0 rec[-9] 0 rec[-20] 1 rec[-19] 1 rec[-13] 1 rec[-10] 1 rec[-8] 1 rec[-3] 1
            )CIRCUIT";
        case GateType::MXX:
            return R"CIRCUIT(
MXX 0 1
            )CIRCUIT";
        case GateType::MYY:
            return R"CIRCUIT(
MX 2
MZZ 0 2
MY 2
MX 2
MZZ 1 2
MY 2
MXX 0 1
MX 2
MZZ 0 2
MY 2
MX 2
MZZ 1 2
MY 2
Z 0 1
CZ rec[-13] 0 rec[-12] 0 rec[-11] 0 rec[-6] 0 rec[-5] 0 rec[-4] 0 rec[-10] 1 rec[-9] 1 rec[-8] 1 rec[-3] 1 rec[-2] 1 rec[-1] 1
            )CIRCUIT";
        case GateType::MZZ:
            return R"CIRCUIT(
MZZ 0 1
            )CIRCUIT";

        case GateType::SPP:
            return R"CIRCUIT(
MY 3
MZZ 1 3
MX 3
MZZ 0 3
MXX 1 3
MZ 3
MX 3
MZZ 1 3
MY 3
MZ 3
MXX 0 3
MY 3
MZZ 0 3
MX 3
MZZ 2 3
MXX 0 3
MZ 3
MX 3
MZZ 0 3
MY 3
MXX 0 3
MZ 3
MXX 0 3
MY 3
MZ 3
MXX 0 3
MY 3
MZZ 0 3
MX 3
MZZ 2 3
MXX 0 3
MZ 3
MX 3
MZZ 0 3
MY 3
MXX 0 3
MZ 3
MY 3
MZZ 1 3
MX 3
MZZ 0 3
MXX 1 3
MZ 3
MX 3
MZZ 1 3
MY 3
X 0
Y 1
Z 2
CX rec[-46] 0 rec[-46] 1 rec[-45] 0 rec[-45] 1 rec[-37] 0 rec[-36] 0 rec[-26] 0 rec[-24] 0 rec[-23] 0 rec[-22] 0 rec[-21] 0 rec[-11] 0 rec[-10] 0
CY rec[-42] 0 rec[-42] 1 rec[-37] 1 rec[-36] 1 rec[-35] 0 rec[-35] 1 rec[-32] 0 rec[-32] 1 rec[-30] 0 rec[-30] 1 rec[-27] 0 rec[-27] 1 rec[-26] 1 rec[-24] 1 rec[-23] 1 rec[-22] 1 rec[-21] 1 rec[-19] 0 rec[-19] 1 rec[-18] 0 rec[-18] 1 rec[-14] 0 rec[-14] 1 rec[-13] 0 rec[-13] 1 rec[-11] 1 rec[-10] 1 rec[-43] 1 rec[-41] 1 rec[-6] 1 rec[-4] 1
CZ rec[-44] 0 rec[-44] 1 rec[-42] 2 rec[-40] 0 rec[-40] 1 rec[-39] 0 rec[-39] 1 rec[-38] 0 rec[-38] 1 rec[-35] 2 rec[-34] 0 rec[-34] 2 rec[-33] 0 rec[-32] 2 rec[-30] 2 rec[-29] 0 rec[-28] 0 rec[-27] 2 rec[-20] 0 rec[-19] 2 rec[-17] 0 rec[-15] 0 rec[-12] 0 rec[-9] 0 rec[-9] 1 rec[-8] 0 rec[-8] 1 rec[-5] 0 rec[-26] 2 rec[-24] 2 rec[-23] 2 rec[-22] 2 rec[-21] 2 rec[-7] 1 rec[-3] 1 rec[-2] 1 rec[-1] 1 rec[-46] 2 rec[-45] 2 rec[-31] 2 rec[-16] 2
            )CIRCUIT";
        case GateType::SPP_DAG:
            return R"CIRCUIT(
MY 3
MZZ 1 3
MX 3
MZZ 0 3
MXX 1 3
MZ 3
MX 3
MZZ 1 3
MY 3
MZ 3
MXX 0 3
MY 3
MZZ 0 3
MX 3
MZZ 2 3
MXX 0 3
MZ 3
MX 3
MZZ 0 3
MY 3
MXX 0 3
MZ 3
MXX 0 3
MY 3
MZ 3
MXX 0 3
MY 3
MZZ 0 3
MX 3
MZZ 2 3
MXX 0 3
MZ 3
MX 3
MZZ 0 3
MY 3
MXX 0 3
MZ 3
MY 3
MZZ 1 3
MX 3
MZZ 0 3
MXX 1 3
MZ 3
MX 3
MZZ 1 3
MY 3
CX rec[-46] 0 rec[-46] 1 rec[-45] 0 rec[-45] 1 rec[-37] 0 rec[-36] 0 rec[-26] 0 rec[-24] 0 rec[-23] 0 rec[-22] 0 rec[-21] 0 rec[-11] 0 rec[-10] 0
CY rec[-42] 0 rec[-42] 1 rec[-37] 1 rec[-36] 1 rec[-35] 0 rec[-35] 1 rec[-32] 0 rec[-32] 1 rec[-30] 0 rec[-30] 1 rec[-27] 0 rec[-27] 1 rec[-26] 1 rec[-24] 1 rec[-23] 1 rec[-22] 1 rec[-21] 1 rec[-19] 0 rec[-19] 1 rec[-18] 0 rec[-18] 1 rec[-14] 0 rec[-14] 1 rec[-13] 0 rec[-13] 1 rec[-11] 1 rec[-10] 1 rec[-43] 1 rec[-41] 1 rec[-6] 1 rec[-4] 1
CZ rec[-44] 0 rec[-44] 1 rec[-42] 2 rec[-40] 0 rec[-40] 1 rec[-39] 0 rec[-39] 1 rec[-38] 0 rec[-38] 1 rec[-35] 2 rec[-34] 0 rec[-34] 2 rec[-33] 0 rec[-32] 2 rec[-30] 2 rec[-29] 0 rec[-28] 0 rec[-27] 2 rec[-20] 0 rec[-19] 2 rec[-17] 0 rec[-15] 0 rec[-12] 0 rec[-9] 0 rec[-9] 1 rec[-8] 0 rec[-8] 1 rec[-5] 0 rec[-26] 2 rec[-24] 2 rec[-23] 2 rec[-22] 2 rec[-21] 2 rec[-7] 1 rec[-3] 1 rec[-2] 1 rec[-1] 1 rec[-46] 2 rec[-45] 2 rec[-31] 2 rec[-16] 2
            )CIRCUIT";
        case GateType::MPP:
            return R"CIRCUIT(
MY 5
MZZ 1 5
MX 5
MZZ 0 5
MXX 1 5
MZ 5
MX 5
MZZ 1 5
MY 5
MZ 5
MXX 2 5
MY 5
MZZ 2 5
MX 5
MXX 0 2
MXX 3 4
MX 5
MZZ 2 5
MY 5
MXX 2 5
MZ 5
MY 5
MZZ 1 5
MX 5
MZZ 0 5
MXX 1 5
MZ 5
MX 5
MZZ 1 5
MY 5
CX rec[-21] 2 rec[-20] 2 rec[-11] 2 rec[-10] 2
CY rec[-27] 1 rec[-25] 1 rec[-6] 1 rec[-4] 1 rec[-18] 2 rec[-13] 2
CZ rec[-28] 0 rec[-28] 1 rec[-26] 0 rec[-24] 0 rec[-24] 1 rec[-23] 0 rec[-23] 1 rec[-22] 0 rec[-22] 1 rec[-9] 0 rec[-9] 1 rec[-8] 0 rec[-8] 1 rec[-5] 0 rec[-30] 1 rec[-29] 1 rec[-7] 1 rec[-3] 1 rec[-2] 1 rec[-1] 1 rec[-19] 2 rec[-12] 2
            )CIRCUIT";
        default:
            throw std::invalid_argument("Unhandled gate type " + std::string(GATE_DATA[gate].name));
    }
}
