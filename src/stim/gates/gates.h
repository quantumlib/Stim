/*
 * Copyright 2021 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STIM_GATES_GATE_DATA_H
#define _STIM_GATES_GATE_DATA_H

#include <array>
#include <cassert>
#include <complex>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace stim {

enum class GateType : uint8_t {
    NOT_A_GATE = 0,
    // Annotations
    DETECTOR,
    OBSERVABLE_INCLUDE,
    TICK,
    QUBIT_COORDS,
    SHIFT_COORDS,
    // Control flow
    REPEAT,
    // Collapsing gates
    MPAD,
    MX,
    MY,
    M,  // alias when parsing: MZ
    MRX,
    MRY,
    MR,  // alias when parsing: MRZ
    RX,
    RY,
    R,  // alias when parsing: RZ
    // Controlled gates
    XCX,
    XCY,
    XCZ,
    YCX,
    YCY,
    YCZ,
    CX,  // alias when parsing: CNOT, ZCX
    CY,  // alias when parsing: ZCY
    CZ,  // alias when parsing: ZCZ
    // Hadamard-like gates
    H,  // alias when parsing: H_XZ
    H_XY,
    H_YZ,
    H_NXY,
    H_NXZ,
    H_NYZ,
    // Noise channels
    DEPOLARIZE1,
    DEPOLARIZE2,
    X_ERROR,
    Y_ERROR,
    Z_ERROR,
    I_ERROR,
    II_ERROR,
    PAULI_CHANNEL_1,
    PAULI_CHANNEL_2,
    E,  // alias when parsing: CORRELATED_ERROR
    ELSE_CORRELATED_ERROR,
    // Heralded noise channels
    HERALDED_ERASE,
    HERALDED_PAULI_CHANNEL_1,
    // Pauli gates
    I,
    X,
    Y,
    Z,
    // Period 3 gates
//    C_XYZ,
//    C_ZYX,
//    C_NXYZ,
//    C_XNYZ,
//    C_XYNZ,
//    C_NZYX,
//    C_ZNYX,
//    C_ZYNX,
    // Period 4 gates
    SQRT_X,
    SQRT_X_DAG,
    SQRT_Y,
    SQRT_Y_DAG,
    S,      // alias when parsing: SQRT_Z
    S_DAG,  // alias when parsing: SQRT_Z_DAG
    // Parity phasing gates.
//    II,
//    SQRT_XX,
//    SQRT_XX_DAG,
//    SQRT_YY,
//    SQRT_YY_DAG,
//    SQRT_ZZ,
//    SQRT_ZZ_DAG,
    // Pauli product gates
    MPP,
//    SPP,
//    SPP_DAG,
    // Swap gates
//    SWAP,
//    ISWAP,
//    CXSWAP,
//    SWAPCX,
//    CZSWAP,
//    ISWAP_DAG,
    // Pair measurement gates
//    MXX,
//    MYY,
//    MZZ,
};

}  // namespace stim

#endif
