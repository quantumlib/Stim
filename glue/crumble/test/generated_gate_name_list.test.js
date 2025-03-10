const KNOWN_GATE_NAMES_FROM_STIM = `
I
X
Y
Z
C_NXYZ
C_NZYX
C_XNYZ
C_XYNZ
C_XYZ
C_ZNYX
C_ZYNX
C_ZYX
H
H_NXY
H_NXZ
H_NYZ
H_XY
H_XZ
H_YZ
S
SQRT_X
SQRT_X_DAG
SQRT_Y
SQRT_Y_DAG
SQRT_Z
SQRT_Z_DAG
S_DAG
CNOT
CX
CXSWAP
CY
CZ
CZSWAP
II
ISWAP
ISWAP_DAG
SQRT_XX
SQRT_XX_DAG
SQRT_YY
SQRT_YY_DAG
SQRT_ZZ
SQRT_ZZ_DAG
SWAP
SWAPCX
SWAPCZ
XCX
XCY
XCZ
YCX
YCY
YCZ
ZCX
ZCY
ZCZ
CORRELATED_ERROR
DEPOLARIZE1
DEPOLARIZE2
E
ELSE_CORRELATED_ERROR
HERALDED_ERASE
HERALDED_PAULI_CHANNEL_1
II_ERROR
I_ERROR
PAULI_CHANNEL_1
PAULI_CHANNEL_2
X_ERROR
Y_ERROR
Z_ERROR
M
MR
MRX
MRY
MRZ
MX
MY
MZ
R
RX
RY
RZ
MXX
MYY
MZZ
MPP
SPP
SPP_DAG
REPEAT
DETECTOR
MPAD
OBSERVABLE_INCLUDE
QUBIT_COORDS
SHIFT_COORDS
TICK
`

export {KNOWN_GATE_NAMES_FROM_STIM};
