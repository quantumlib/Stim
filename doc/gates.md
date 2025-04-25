# Gates supported by Stim

- Pauli Gates
    - [I](#I)
    - [X](#X)
    - [Y](#Y)
    - [Z](#Z)
- Single Qubit Clifford Gates
    - [C_NXYZ](#C_NXYZ)
    - [C_NZYX](#C_NZYX)
    - [C_XNYZ](#C_XNYZ)
    - [C_XYNZ](#C_XYNZ)
    - [C_XYZ](#C_XYZ)
    - [C_ZNYX](#C_ZNYX)
    - [C_ZYNX](#C_ZYNX)
    - [C_ZYX](#C_ZYX)
    - [H](#H)
    - [H_NXY](#H_NXY)
    - [H_NXZ](#H_NXZ)
    - [H_NYZ](#H_NYZ)
    - [H_XY](#H_XY)
    - [H_XZ](#H_XZ)
    - [H_YZ](#H_YZ)
    - [S](#S)
    - [SQRT_X](#SQRT_X)
    - [SQRT_X_DAG](#SQRT_X_DAG)
    - [SQRT_Y](#SQRT_Y)
    - [SQRT_Y_DAG](#SQRT_Y_DAG)
    - [SQRT_Z](#SQRT_Z)
    - [SQRT_Z_DAG](#SQRT_Z_DAG)
    - [S_DAG](#S_DAG)
- Two Qubit Clifford Gates
    - [CNOT](#CNOT)
    - [CX](#CX)
    - [CXSWAP](#CXSWAP)
    - [CY](#CY)
    - [CZ](#CZ)
    - [CZSWAP](#CZSWAP)
    - [II](#II)
    - [ISWAP](#ISWAP)
    - [ISWAP_DAG](#ISWAP_DAG)
    - [SQRT_XX](#SQRT_XX)
    - [SQRT_XX_DAG](#SQRT_XX_DAG)
    - [SQRT_YY](#SQRT_YY)
    - [SQRT_YY_DAG](#SQRT_YY_DAG)
    - [SQRT_ZZ](#SQRT_ZZ)
    - [SQRT_ZZ_DAG](#SQRT_ZZ_DAG)
    - [SWAP](#SWAP)
    - [SWAPCX](#SWAPCX)
    - [SWAPCZ](#SWAPCZ)
    - [XCX](#XCX)
    - [XCY](#XCY)
    - [XCZ](#XCZ)
    - [YCX](#YCX)
    - [YCY](#YCY)
    - [YCZ](#YCZ)
    - [ZCX](#ZCX)
    - [ZCY](#ZCY)
    - [ZCZ](#ZCZ)
- Noise Channels
    - [CORRELATED_ERROR](#CORRELATED_ERROR)
    - [DEPOLARIZE1](#DEPOLARIZE1)
    - [DEPOLARIZE2](#DEPOLARIZE2)
    - [E](#E)
    - [ELSE_CORRELATED_ERROR](#ELSE_CORRELATED_ERROR)
    - [HERALDED_ERASE](#HERALDED_ERASE)
    - [HERALDED_PAULI_CHANNEL_1](#HERALDED_PAULI_CHANNEL_1)
    - [II_ERROR](#II_ERROR)
    - [I_ERROR](#I_ERROR)
    - [PAULI_CHANNEL_1](#PAULI_CHANNEL_1)
    - [PAULI_CHANNEL_2](#PAULI_CHANNEL_2)
    - [X_ERROR](#X_ERROR)
    - [Y_ERROR](#Y_ERROR)
    - [Z_ERROR](#Z_ERROR)
- Collapsing Gates
    - [M](#M)
    - [MR](#MR)
    - [MRX](#MRX)
    - [MRY](#MRY)
    - [MRZ](#MRZ)
    - [MX](#MX)
    - [MY](#MY)
    - [MZ](#MZ)
    - [R](#R)
    - [RX](#RX)
    - [RY](#RY)
    - [RZ](#RZ)
- Pair Measurement Gates
    - [MXX](#MXX)
    - [MYY](#MYY)
    - [MZZ](#MZZ)
- Generalized Pauli Product Gates
    - [MPP](#MPP)
    - [SPP](#SPP)
    - [SPP_DAG](#SPP_DAG)
- Control Flow
    - [REPEAT](#REPEAT)
- Annotations
    - [DETECTOR](#DETECTOR)
    - [MPAD](#MPAD)
    - [OBSERVABLE_INCLUDE](#OBSERVABLE_INCLUDE)
    - [QUBIT_COORDS](#QUBIT_COORDS)
    - [SHIFT_COORDS](#SHIFT_COORDS)
    - [TICK](#TICK)

## Pauli Gates

<a name="I"></a>
### The 'I' Gate

The identity gate.
Does nothing to the target qubits.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to do nothing to.

Example:

    I 5
    I 42
    I 5 42
    
Stabilizer Generators:

    X -> X
    Z -> Z
    
Bloch Rotation (axis angle):

    Axis: +X
    Angle: 0°
    
Bloch Rotation (Euler angles):

      theta = 0°
        phi = 0°
     lambda = 0°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(0°) * RotZ(0°)
    unitary = I * I * I

Unitary Matrix:

    [+1  ,     ]
    [    , +1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `I 0`
    # (no operations)
    
    # (The decomposition is empty because this gate has no effect.)
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `I 0` (but affects the measurement record and an ancilla qubit)
                
    # (The decomposition is empty because this gate has no effect.)
    

<a name="X"></a>
### The 'X' Gate

The Pauli X gate.
The bit flip gate.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    X 5
    X 42
    X 5 42
    
Stabilizer Generators:

    X -> X
    Z -> -Z
    
Bloch Rotation (axis angle):

    Axis: +X
    Angle: 180°
    
Bloch Rotation (Euler angles):

      theta = 180°
        phi = 0°
     lambda = 180°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(180°) * RotZ(180°)
    unitary = I * Y * Z

Unitary Matrix:

    [    , +1  ]
    [+1  ,     ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `X 0`
    H 0
    S 0
    S 0
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `X 0` (but affects the measurement record and an ancilla qubit)
    X 0
                
    # (The decomposition is trivial because this gate is in the target gate set.)
    

<a name="Y"></a>
### The 'Y' Gate

The Pauli Y gate.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    Y 5
    Y 42
    Y 5 42
    
Stabilizer Generators:

    X -> -X
    Z -> -Z
    
Bloch Rotation (axis angle):

    Axis: +Y
    Angle: 180°
    
Bloch Rotation (Euler angles):

      theta = 180°
        phi = 0°
     lambda = 0°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(180°) * RotZ(0°)
    unitary = I * Y * I

Unitary Matrix:

    [    ,   -i]
    [  +i,     ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `Y 0`
    S 0
    S 0
    H 0
    S 0
    S 0
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `Y 0` (but affects the measurement record and an ancilla qubit)
    Y 0
                
    # (The decomposition is trivial because this gate is in the target gate set.)
    

<a name="Z"></a>
### The 'Z' Gate

The Pauli Z gate.
The phase flip gate.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    Z 5
    Z 42
    Z 5 42
    
Stabilizer Generators:

    X -> -X
    Z -> Z
    
Bloch Rotation (axis angle):

    Axis: +Z
    Angle: 180°
    
Bloch Rotation (Euler angles):

      theta = 0°
        phi = 0°
     lambda = 180°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(0°) * RotZ(180°)
    unitary = I * I * Z

Unitary Matrix:

    [+1  ,     ]
    [    , -1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `Z 0`
    S 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `Z 0` (but affects the measurement record and an ancilla qubit)
    Z 0
                
    # (The decomposition is trivial because this gate is in the target gate set.)
    

## Single Qubit Clifford Gates

<a name="C_NXYZ"></a>
### The 'C_NXYZ' Gate

Performs the period-3 cycle -X -> Y -> Z -> -X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    C_NXYZ 5
    C_NXYZ 42
    C_NXYZ 5 42
    
Stabilizer Generators:

    X -> -Y
    Z -> -X
    
Bloch Rotation (axis angle):

    Axis: -X+Y+Z
    Angle: -120°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 180°
     lambda = 90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(180°) * RotY(90°) * RotZ(90°)
    unitary = Z * SQRT_Y * S

Unitary Matrix:

    [+1+i, +1-i]
    [-1-i, +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `C_NXYZ 0`
    S 0
    S 0
    S 0
    H 0
    S 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `C_NXYZ 0` (but affects the measurement record and an ancilla qubit)
    MZ 1
    MXX 0 1
    MY 1
    MZZ 0 1
    MX 1
    Z 0
    CX rec[-3] 0
    CY rec[-5] 0 rec[-4] 0
    CZ rec[-2] 0 rec[-1] 0
                

<a name="C_NZYX"></a>
### The 'C_NZYX' Gate

Performs the period-3 cycle X -> -Z -> Y -> X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    C_NZYX 5
    C_NZYX 42
    C_NZYX 5 42
    
Stabilizer Generators:

    X -> -Z
    Z -> -Y
    
Bloch Rotation (axis angle):

    Axis: +X+Y-Z
    Angle: 120°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = -90°
     lambda = 0°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(-90°) * RotY(90°) * RotZ(0°)
    unitary = S_DAG * SQRT_Y * I

Unitary Matrix:

    [+1+i, -1-i]
    [+1-i, +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `C_NZYX 0`
    S 0
    S 0
    H 0
    S 0
    S 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `C_NZYX 0` (but affects the measurement record and an ancilla qubit)
    MX 1
    MZZ 0 1
    MY 1
    MXX 0 1
    MZ 1
    X 0
    CX rec[-2] 0 rec[-1] 0
    CY rec[-5] 0 rec[-4] 0
    CZ rec[-3] 0
                

<a name="C_XNYZ"></a>
### The 'C_XNYZ' Gate

Performs the period-3 cycle X -> -Y -> Z -> X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    C_XNYZ 5
    C_XNYZ 42
    C_XNYZ 5 42
    
Stabilizer Generators:

    X -> -Y
    Z -> X
    
Bloch Rotation (axis angle):

    Axis: +X-Y+Z
    Angle: -120°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 0°
     lambda = -90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(90°) * RotZ(-90°)
    unitary = I * SQRT_Y * S_DAG

Unitary Matrix:

    [+1+i, -1+i]
    [+1+i, +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `C_XNYZ 0`
    S 0
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `C_XNYZ 0` (but affects the measurement record and an ancilla qubit)
    MZ 1
    MXX 0 1
    MY 1
    MZZ 0 1
    MX 1
    X 0
    CX rec[-3] 0
    CY rec[-5] 0 rec[-4] 0
    CZ rec[-2] 0 rec[-1] 0
                

<a name="C_XYNZ"></a>
### The 'C_XYNZ' Gate

Performs the period-3 cycle X -> Y -> -Z -> X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    C_XYNZ 5
    C_XYNZ 42
    C_XYNZ 5 42
    
Stabilizer Generators:

    X -> Y
    Z -> -X
    
Bloch Rotation (axis angle):

    Axis: +X+Y-Z
    Angle: -120°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 180°
     lambda = -90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(180°) * RotY(90°) * RotZ(-90°)
    unitary = Z * SQRT_Y * S_DAG

Unitary Matrix:

    [+1-i, +1+i]
    [-1+i, +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `C_XYNZ 0`
    S 0
    H 0
    S 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `C_XYNZ 0` (but affects the measurement record and an ancilla qubit)
    MZ 1
    MXX 0 1
    MY 1
    MZZ 0 1
    MX 1
    Y 0
    CX rec[-3] 0
    CY rec[-5] 0 rec[-4] 0
    CZ rec[-2] 0 rec[-1] 0
                

<a name="C_XYZ"></a>
### The 'C_XYZ' Gate

Right handed period 3 axis cycling gate, sending X -> Y -> Z -> X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    C_XYZ 5
    C_XYZ 42
    C_XYZ 5 42
    
Stabilizer Generators:

    X -> Y
    Z -> X
    
Bloch Rotation (axis angle):

    Axis: +X+Y+Z
    Angle: 120°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 0°
     lambda = 90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(90°) * RotZ(90°)
    unitary = I * SQRT_Y * S

Unitary Matrix:

    [+1-i, -1-i]
    [+1-i, +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `C_XYZ 0`
    S 0
    S 0
    S 0
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `C_XYZ 0` (but affects the measurement record and an ancilla qubit)
    MZ 1
    MXX 0 1
    MY 1
    MZZ 0 1
    MX 1
    CX rec[-3] 0
    CY rec[-5] 0 rec[-4] 0
    CZ rec[-2] 0 rec[-1] 0
                

<a name="C_ZNYX"></a>
### The 'C_ZNYX' Gate

Performs the period-3 cycle X -> Z -> -Y -> X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    C_ZNYX 5
    C_ZNYX 42
    C_ZNYX 5 42
    
Stabilizer Generators:

    X -> Z
    Z -> -Y
    
Bloch Rotation (axis angle):

    Axis: +X-Y+Z
    Angle: 120°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = -90°
     lambda = 180°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(-90°) * RotY(90°) * RotZ(180°)
    unitary = S_DAG * SQRT_Y * Z

Unitary Matrix:

    [+1-i, +1-i]
    [-1-i, +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `C_ZNYX 0`
    H 0
    S 0
    S 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `C_ZNYX 0` (but affects the measurement record and an ancilla qubit)
    MX 1
    MZZ 0 1
    MY 1
    MXX 0 1
    MZ 1
    Z 0
    CX rec[-2] 0 rec[-1] 0
    CY rec[-5] 0 rec[-4] 0
    CZ rec[-3] 0
                

<a name="C_ZYNX"></a>
### The 'C_ZYNX' Gate

Performs the period-3 cycle -X -> Z -> Y -> -X.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    C_ZYNX 5
    C_ZYNX 42
    C_ZYNX 5 42
    
Stabilizer Generators:

    X -> -Z
    Z -> Y
    
Bloch Rotation (axis angle):

    Axis: -X+Y+Z
    Angle: 120°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 90°
     lambda = 0°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(90°) * RotY(90°) * RotZ(0°)
    unitary = S * SQRT_Y * I

Unitary Matrix:

    [+1-i, -1+i]
    [+1+i, +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `C_ZYNX 0`
    S 0
    S 0
    H 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `C_ZYNX 0` (but affects the measurement record and an ancilla qubit)
    MX 1
    MZZ 0 1
    MY 1
    MXX 0 1
    MZ 1
    Y 0
    CX rec[-2] 0 rec[-1] 0
    CY rec[-5] 0 rec[-4] 0
    CZ rec[-3] 0
                

<a name="C_ZYX"></a>
### The 'C_ZYX' Gate

Left handed period 3 axis cycling gate, sending Z -> Y -> X -> Z.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    C_ZYX 5
    C_ZYX 42
    C_ZYX 5 42
    
Stabilizer Generators:

    X -> Z
    Z -> Y
    
Bloch Rotation (axis angle):

    Axis: +X+Y+Z
    Angle: -120°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 90°
     lambda = 180°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(90°) * RotY(90°) * RotZ(180°)
    unitary = S * SQRT_Y * Z

Unitary Matrix:

    [+1+i, +1+i]
    [-1+i, +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `C_ZYX 0`
    H 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `C_ZYX 0` (but affects the measurement record and an ancilla qubit)
    MX 1
    MZZ 0 1
    MY 1
    MXX 0 1
    MZ 1
    CX rec[-2] 0 rec[-1] 0
    CY rec[-5] 0 rec[-4] 0
    CZ rec[-3] 0
                

<a name="H"></a>
### The 'H' Gate

Alternate name: <a name="H_XZ"></a>`H_XZ`

The Hadamard gate.
Swaps the X and Z axes.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    H 5
    H 42
    H 5 42
    
Stabilizer Generators:

    X -> Z
    Z -> X
    
Bloch Rotation (axis angle):

    Axis: +X+Z
    Angle: 180°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 0°
     lambda = 180°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(90°) * RotZ(180°)
    unitary = I * SQRT_Y * Z

Unitary Matrix:

    [+1  , +1  ]
    [+1  , -1  ] / sqrt(2)
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `H 0`
    H 0
    
    # (The decomposition is trivial because this gate is in the target gate set.)
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `H 0` (but affects the measurement record and an ancilla qubit)
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
                

<a name="H_NXY"></a>
### The 'H_NXY' Gate

A variant of the Hadamard gate that swaps the -X and +Y axes.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    H_NXY 5
    H_NXY 42
    H_NXY 5 42
    
Stabilizer Generators:

    X -> -Y
    Z -> -Z
    
Bloch Rotation (axis angle):

    Axis: +X-Y
    Angle: 180°
    
Bloch Rotation (Euler angles):

      theta = 180°
        phi = 0°
     lambda = -90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(180°) * RotZ(-90°)
    unitary = I * Y * S_DAG

Unitary Matrix:

    [    , +1+i]
    [+1-i,     ] / sqrt(2)
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `H_NXY 0`
    S 0
    H 0
    S 0
    S 0
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `H_NXY 0` (but affects the measurement record and an ancilla qubit)
    MX 1
    MZZ 0 1
    MY 1
    Y 0
    CZ rec[-3] 0 rec[-2] 0 rec[-1] 0
                

<a name="H_NXZ"></a>
### The 'H_NXZ' Gate

A variant of the Hadamard gate that swaps the -X and +Z axes.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    H_NXZ 5
    H_NXZ 42
    H_NXZ 5 42
    
Stabilizer Generators:

    X -> -Z
    Z -> -X
    
Bloch Rotation (axis angle):

    Axis: +X-Z
    Angle: 180°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 180°
     lambda = 0°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(180°) * RotY(90°) * RotZ(0°)
    unitary = Z * SQRT_Y * I

Unitary Matrix:

    [-1  , +1  ]
    [+1  , +1  ] / sqrt(2)
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `H_NXZ 0`
    S 0
    S 0
    H 0
    S 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `H_NXZ 0` (but affects the measurement record and an ancilla qubit)
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
                

<a name="H_NYZ"></a>
### The 'H_NYZ' Gate

A variant of the Hadamard gate that swaps the -Y and +Z axes.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    H_NYZ 5
    H_NYZ 42
    H_NYZ 5 42
    
Stabilizer Generators:

    X -> -X
    Z -> -Y
    
Bloch Rotation (axis angle):

    Axis: +Y-Z
    Angle: 180°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = -90°
     lambda = -90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(-90°) * RotY(90°) * RotZ(-90°)
    unitary = S_DAG * SQRT_Y * S_DAG

Unitary Matrix:

    [-1  ,   -i]
    [  +i, +1  ] / sqrt(2)
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `H_NYZ 0`
    S 0
    S 0
    H 0
    S 0
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `H_NYZ 0` (but affects the measurement record and an ancilla qubit)
    MZ 1
    MXX 0 1
    MY 1
    Y 0
    CX rec[-3] 0 rec[-2] 0 rec[-1] 0
                

<a name="H_XY"></a>
### The 'H_XY' Gate

A variant of the Hadamard gate that swaps the X and Y axes (instead of X and Z).

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    H_XY 5
    H_XY 42
    H_XY 5 42
    
Stabilizer Generators:

    X -> Y
    Z -> -Z
    
Bloch Rotation (axis angle):

    Axis: +X+Y
    Angle: 180°
    
Bloch Rotation (Euler angles):

      theta = 180°
        phi = 0°
     lambda = 90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(180°) * RotZ(90°)
    unitary = I * Y * S

Unitary Matrix:

    [    , +1-i]
    [+1+i,     ] / sqrt(2)
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `H_XY 0`
    H 0
    S 0
    S 0
    H 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `H_XY 0` (but affects the measurement record and an ancilla qubit)
    MX 1
    MZZ 0 1
    MY 1
    X 0
    CZ rec[-3] 0 rec[-2] 0 rec[-1] 0
                

<a name="H_YZ"></a>
### The 'H_YZ' Gate

A variant of the Hadamard gate that swaps the Y and Z axes (instead of X and Z).

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    H_YZ 5
    H_YZ 42
    H_YZ 5 42
    
Stabilizer Generators:

    X -> -X
    Z -> Y
    
Bloch Rotation (axis angle):

    Axis: +Y+Z
    Angle: 180°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 90°
     lambda = 90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(90°) * RotY(90°) * RotZ(90°)
    unitary = S * SQRT_Y * S

Unitary Matrix:

    [+1  ,   -i]
    [  +i, -1  ] / sqrt(2)
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `H_YZ 0`
    H 0
    S 0
    H 0
    S 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `H_YZ 0` (but affects the measurement record and an ancilla qubit)
    MZ 1
    MXX 0 1
    MY 1
    Z 0
    CX rec[-3] 0 rec[-2] 0 rec[-1] 0
                

<a name="S"></a>
### The 'S' Gate

Alternate name: <a name="SQRT_Z"></a>`SQRT_Z`

Principal square root of Z gate.
Phases the amplitude of |1> by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    S 5
    S 42
    S 5 42
    
Stabilizer Generators:

    X -> Y
    Z -> Z
    
Bloch Rotation (axis angle):

    Axis: +Z
    Angle: 90°
    
Bloch Rotation (Euler angles):

      theta = 0°
        phi = 0°
     lambda = 90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(0°) * RotZ(90°)
    unitary = I * I * S

Unitary Matrix:

    [+1  ,     ]
    [    ,   +i]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `S 0`
    S 0
    
    # (The decomposition is trivial because this gate is in the target gate set.)
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `S 0` (but affects the measurement record and an ancilla qubit)
    MY 1
    MZZ 0 1
    MX 1
    CZ rec[-3] 0 rec[-2] 0 rec[-1] 0
                

<a name="SQRT_X"></a>
### The 'SQRT_X' Gate

Principal square root of X gate.
Phases the amplitude of |-> by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    SQRT_X 5
    SQRT_X 42
    SQRT_X 5 42
    
Stabilizer Generators:

    X -> X
    Z -> -Y
    
Bloch Rotation (axis angle):

    Axis: +X
    Angle: 90°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = -90°
     lambda = 90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(-90°) * RotY(90°) * RotZ(90°)
    unitary = S_DAG * SQRT_Y * S

Unitary Matrix:

    [+1+i, +1-i]
    [+1-i, +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_X 0`
    H 0
    S 0
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SQRT_X 0` (but affects the measurement record and an ancilla qubit)
    MZ 1
    MXX 0 1
    MY 1
    CX rec[-3] 0 rec[-2] 0 rec[-1] 0
                

<a name="SQRT_X_DAG"></a>
### The 'SQRT_X_DAG' Gate

Adjoint of the principal square root of X gate.
Phases the amplitude of |-> by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    SQRT_X_DAG 5
    SQRT_X_DAG 42
    SQRT_X_DAG 5 42
    
Stabilizer Generators:

    X -> X
    Z -> Y
    
Bloch Rotation (axis angle):

    Axis: +X
    Angle: -90°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 90°
     lambda = -90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(90°) * RotY(90°) * RotZ(-90°)
    unitary = S * SQRT_Y * S_DAG

Unitary Matrix:

    [+1-i, +1+i]
    [+1+i, +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_X_DAG 0`
    S 0
    H 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SQRT_X_DAG 0` (but affects the measurement record and an ancilla qubit)
    MY 1
    MXX 0 1
    MZ 1
    CX rec[-3] 0 rec[-2] 0 rec[-1] 0
                

<a name="SQRT_Y"></a>
### The 'SQRT_Y' Gate

Principal square root of Y gate.
Phases the amplitude of |-i> by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    SQRT_Y 5
    SQRT_Y 42
    SQRT_Y 5 42
    
Stabilizer Generators:

    X -> -Z
    Z -> X
    
Bloch Rotation (axis angle):

    Axis: +Y
    Angle: 90°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 0°
     lambda = 0°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(90°) * RotZ(0°)
    unitary = I * SQRT_Y * I

Unitary Matrix:

    [+1+i, -1-i]
    [+1+i, +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_Y 0`
    S 0
    S 0
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SQRT_Y 0` (but affects the measurement record and an ancilla qubit)
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
                

<a name="SQRT_Y_DAG"></a>
### The 'SQRT_Y_DAG' Gate

Adjoint of the principal square root of Y gate.
Phases the amplitude of |-i> by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    SQRT_Y_DAG 5
    SQRT_Y_DAG 42
    SQRT_Y_DAG 5 42
    
Stabilizer Generators:

    X -> Z
    Z -> -X
    
Bloch Rotation (axis angle):

    Axis: +Y
    Angle: -90°
    
Bloch Rotation (Euler angles):

      theta = 90°
        phi = 180°
     lambda = 180°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(180°) * RotY(90°) * RotZ(180°)
    unitary = Z * SQRT_Y * Z

Unitary Matrix:

    [+1-i, +1-i]
    [-1+i, +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_Y_DAG 0`
    H 0
    S 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SQRT_Y_DAG 0` (but affects the measurement record and an ancilla qubit)
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
                

<a name="S_DAG"></a>
### The 'S_DAG' Gate

Alternate name: <a name="SQRT_Z_DAG"></a>`SQRT_Z_DAG`

Adjoint of the principal square root of Z gate.
Phases the amplitude of |1> by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubits to operate on.

Example:

    S_DAG 5
    S_DAG 42
    S_DAG 5 42
    
Stabilizer Generators:

    X -> -Y
    Z -> Z
    
Bloch Rotation (axis angle):

    Axis: +Z
    Angle: -90°
    
Bloch Rotation (Euler angles):

      theta = 0°
        phi = 0°
     lambda = -90°
    unitary = RotZ(phi) * RotY(theta) * RotZ(lambda)
    unitary = RotZ(0°) * RotY(0°) * RotZ(-90°)
    unitary = I * I * S_DAG

Unitary Matrix:

    [+1  ,     ]
    [    ,   -i]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `S_DAG 0`
    S 0
    S 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `S_DAG 0` (but affects the measurement record and an ancilla qubit)
    MX 1
    MZZ 0 1
    MY 1
    CZ rec[-3] 0 rec[-2] 0 rec[-1] 0
                

## Two Qubit Clifford Gates

<a name="CX"></a>
### The 'CX' Gate

Alternate name: <a name="CNOT"></a>`CNOT`

Alternate name: <a name="ZCX"></a>`ZCX`

The Z-controlled X gate.
Applies an X gate to the target if the control is in the |1> state.
Equivalently: negates the amplitude of the |1>|-> state.
The first qubit is called the control, and the second qubit is the target.

To perform a classically controlled X, replace the control with a `rec`
target like rec[-2].

To perform an I or X gate as configured by sweep data, replace the
control with a `sweep` target like sweep[3].

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    # Bit flip qubit 5 controlled by qubit 2.
    CX 2 5

    # Perform CX 2 5 then CX 4 2.
    CX 2 5 4 2

    # Bit flip qubit 6 if the most recent measurement result was TRUE.
    CX rec[-1] 6

    # Bit flip qubits 7 and 8 conditioned on sweep configuration data.
    CX sweep[5] 7 sweep[5] 8
Stabilizer Generators:

    X_ -> XX
    Z_ -> Z_
    _X -> _X
    _Z -> ZZ
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,     ,     , +1  ]
    [    ,     , +1  ,     ]
    [    , +1  ,     ,     ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `CX 0 1`
    CNOT 0 1
    
    # (The decomposition is trivial because this gate is in the target gate set.)
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `CX 0 1` (but affects the measurement record and an ancilla qubit)
    MX 2
    MZZ 0 2
    MXX 1 2
    MZ 2
    CX rec[-3] 1 rec[-1] 1
    CZ rec[-4] 0 rec[-2] 0
                

<a name="CXSWAP"></a>
### The 'CXSWAP' Gate

A combination CX-then-SWAP gate.
This gate is kak-equivalent to the iswap gate, but preserves X/Z noise bias.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    CXSWAP 5 6
    CXSWAP 42 43
    CXSWAP 5 6 42 43
    
Stabilizer Generators:

    X_ -> XX
    Z_ -> _Z
    _X -> X_
    _Z -> ZZ
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,     , +1  ,     ]
    [    ,     ,     , +1  ]
    [    , +1  ,     ,     ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `CXSWAP 0 1`
    CNOT 1 0
    CNOT 0 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `CXSWAP 0 1` (but affects the measurement record and an ancilla qubit)
    MZ 2
    MXX 0 2
    MZZ 1 2
    MX 2
    MZZ 0 2
    MXX 1 2
    MZ 2
    CX rec[-7] 0 rec[-7] 1 rec[-5] 0 rec[-5] 1 rec[-3] 1 rec[-1] 1
    CZ rec[-6] 0 rec[-6] 1 rec[-2] 0 rec[-4] 1
                

<a name="CY"></a>
### The 'CY' Gate

Alternate name: <a name="ZCY"></a>`ZCY`

The Z-controlled Y gate.
Applies a Y gate to the target if the control is in the |1> state.
Equivalently: negates the amplitude of the |1>|-i> state.
The first qubit is the control, and the second qubit is the target.

To perform a classically controlled Y, replace the control with a `rec`
target like rec[-2].

To perform an I or Y gate as configured by sweep data, replace the
control with a `sweep` target like sweep[3].

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    # Apply Y to qubit 5 controlled by qubit 2.
    CY 2 5

    # Perform CY 2 5 then CX 4 2.
    CY 2 5 4 2

    # Apply Y to qubit 6 if the most recent measurement result was TRUE.
    CY rec[-1] 6

    # Apply Y to qubits 7 and 8 conditioned on sweep configuration data.
    CY sweep[5] 7 sweep[5] 8
Stabilizer Generators:

    X_ -> XY
    Z_ -> Z_
    _X -> ZX
    _Z -> ZZ
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,     ,     ,   -i]
    [    ,     , +1  ,     ]
    [    ,   +i,     ,     ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `CY 0 1`
    S 1
    S 1
    S 1
    CNOT 0 1
    S 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `CY 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="CZ"></a>
### The 'CZ' Gate

Alternate name: <a name="ZCZ"></a>`ZCZ`

The Z-controlled Z gate.
Applies a Z gate to the target if the control is in the |1> state.
Equivalently: negates the amplitude of the |1>|1> state.
The first qubit is called the control, and the second qubit is the target.

To perform a classically controlled Z, replace either qubit with a `rec`
target like rec[-2].

To perform an I or Z gate as configured by sweep data, replace either qubit
with a `sweep` target like sweep[3].

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    # Apply Z to qubit 5 controlled by qubit 2.
    CZ 2 5

    # Perform CZ 2 5 then CZ 4 2.
    CZ 2 5 4 2

    # Apply Z to qubit 6 if the most recent measurement result was TRUE.
    CZ rec[-1] 6

    # Apply Z to qubit 7 if the 3rd most recent measurement result was TRUE.
    CZ 7 rec[-3]

    # Apply Z to qubits 7 and 8 conditioned on sweep configuration data.
    CZ sweep[5] 7 8 sweep[5]
Stabilizer Generators:

    X_ -> XZ
    Z_ -> Z_
    _X -> ZX
    _Z -> _Z
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    , +1  ,     ,     ]
    [    ,     , +1  ,     ]
    [    ,     ,     , -1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `CZ 0 1`
    H 1
    CNOT 0 1
    H 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `CZ 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="CZSWAP"></a>
### The 'CZSWAP' Gate

Alternate name: <a name="SWAPCZ"></a>`SWAPCZ`

A combination CZ-and-SWAP gate.
This gate is kak-equivalent to the iswap gate.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    CZSWAP 5 6
    CZSWAP 42 43
    CZSWAP 5 6 42 43
    
Stabilizer Generators:

    X_ -> ZX
    Z_ -> _Z
    _X -> XZ
    _Z -> Z_
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,     , +1  ,     ]
    [    , +1  ,     ,     ]
    [    ,     ,     , -1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `CZSWAP 0 1`
    H 0
    CX 0 1
    CX 1 0
    H 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `CZSWAP 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="II"></a>
### The 'II' Gate

A two-qubit identity gate.

Twice as much doing-nothing as the I gate! This gate only exists because it
can be useful as a communication mechanism for systems built on top of stim.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Examples:

    II 0 1

    R 0
    II[ACTUALLY_A_LEAKAGE_ISWAP] 0 1
    R 0
    CX 1 0
Stabilizer Generators:

    X_ -> X_
    Z_ -> Z_
    _X -> _X
    _Z -> _Z
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    , +1  ,     ,     ]
    [    ,     , +1  ,     ]
    [    ,     ,     , +1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `II 0 1`
    
    # (The decomposition is empty because this gate has no effect.)
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `II 0 1` (but affects the measurement record and an ancilla qubit)
    # (The decomposition is empty because this gate has no effect.)
    

<a name="ISWAP"></a>
### The 'ISWAP' Gate

Swaps two qubits and phases the -1 eigenspace of the ZZ observable by i.
Equivalent to `SWAP` then `CZ` then `S` on both targets.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    ISWAP 5 6
    ISWAP 42 43
    ISWAP 5 6 42 43
    
Stabilizer Generators:

    X_ -> ZY
    Z_ -> _Z
    _X -> YZ
    _Z -> Z_
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,     ,   +i,     ]
    [    ,   +i,     ,     ]
    [    ,     ,     , +1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `ISWAP 0 1`
    H 0
    CNOT 0 1
    CNOT 1 0
    H 1
    S 1
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `ISWAP 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="ISWAP_DAG"></a>
### The 'ISWAP_DAG' Gate

Swaps two qubits and phases the -1 eigenspace of the ZZ observable by -i.
Equivalent to `SWAP` then `CZ` then `S_DAG` on both targets.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    ISWAP_DAG 5 6
    ISWAP_DAG 42 43
    ISWAP_DAG 5 6 42 43
    
Stabilizer Generators:

    X_ -> -ZY
    Z_ -> _Z
    _X -> -YZ
    _Z -> Z_
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,     ,   -i,     ]
    [    ,   -i,     ,     ]
    [    ,     ,     , +1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `ISWAP_DAG 0 1`
    S 0
    S 0
    S 0
    S 1
    S 1
    S 1
    H 1
    CNOT 1 0
    CNOT 0 1
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `ISWAP_DAG 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="SQRT_XX"></a>
### The 'SQRT_XX' Gate

Phases the -1 eigenspace of the XX observable by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    SQRT_XX 5 6
    SQRT_XX 42 43
    SQRT_XX 5 6 42 43
    
Stabilizer Generators:

    X_ -> X_
    Z_ -> -YX
    _X -> _X
    _Z -> -XY
    
Unitary Matrix (little endian):

    [+1+i,     ,     , +1-i]
    [    , +1+i, +1-i,     ]
    [    , +1-i, +1+i,     ]
    [+1-i,     ,     , +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_XX 0 1`
    H 0
    CNOT 0 1
    H 1
    S 0
    S 1
    H 0
    H 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SQRT_XX 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="SQRT_XX_DAG"></a>
### The 'SQRT_XX_DAG' Gate

Phases the -1 eigenspace of the XX observable by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    SQRT_XX_DAG 5 6
    SQRT_XX_DAG 42 43
    SQRT_XX_DAG 5 6 42 43
    
Stabilizer Generators:

    X_ -> X_
    Z_ -> YX
    _X -> _X
    _Z -> XY
    
Unitary Matrix (little endian):

    [+1-i,     ,     , +1+i]
    [    , +1-i, +1+i,     ]
    [    , +1+i, +1-i,     ]
    [+1+i,     ,     , +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_XX_DAG 0 1`
    H 0
    CNOT 0 1
    H 1
    S 0
    S 0
    S 0
    S 1
    S 1
    S 1
    H 0
    H 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SQRT_XX_DAG 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="SQRT_YY"></a>
### The 'SQRT_YY' Gate

Phases the -1 eigenspace of the YY observable by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    SQRT_YY 5 6
    SQRT_YY 42 43
    SQRT_YY 5 6 42 43
    
Stabilizer Generators:

    X_ -> -ZY
    Z_ -> XY
    _X -> -YZ
    _Z -> YX
    
Unitary Matrix (little endian):

    [+1+i,     ,     , -1+i]
    [    , +1+i, +1-i,     ]
    [    , +1-i, +1+i,     ]
    [-1+i,     ,     , +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_YY 0 1`
    S 0
    S 0
    S 0
    S 1
    S 1
    S 1
    H 0
    CNOT 0 1
    H 1
    S 0
    S 1
    H 0
    H 1
    S 0
    S 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SQRT_YY 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="SQRT_YY_DAG"></a>
### The 'SQRT_YY_DAG' Gate

Phases the -1 eigenspace of the YY observable by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    SQRT_YY_DAG 5 6
    SQRT_YY_DAG 42 43
    SQRT_YY_DAG 5 6 42 43
    
Stabilizer Generators:

    X_ -> ZY
    Z_ -> -XY
    _X -> YZ
    _Z -> -YX
    
Unitary Matrix (little endian):

    [+1-i,     ,     , -1-i]
    [    , +1-i, +1+i,     ]
    [    , +1+i, +1-i,     ]
    [-1-i,     ,     , +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_YY_DAG 0 1`
    S 0
    S 0
    S 0
    S 1
    H 0
    CNOT 0 1
    H 1
    S 0
    S 1
    H 0
    H 1
    S 0
    S 1
    S 1
    S 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SQRT_YY_DAG 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="SQRT_ZZ"></a>
### The 'SQRT_ZZ' Gate

Phases the -1 eigenspace of the ZZ observable by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    SQRT_ZZ 5 6
    SQRT_ZZ 42 43
    SQRT_ZZ 5 6 42 43
    
Stabilizer Generators:

    X_ -> YZ
    Z_ -> Z_
    _X -> ZY
    _Z -> _Z
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,   +i,     ,     ]
    [    ,     ,   +i,     ]
    [    ,     ,     , +1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_ZZ 0 1`
    H 1
    CNOT 0 1
    H 1
    S 0
    S 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SQRT_ZZ 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="SQRT_ZZ_DAG"></a>
### The 'SQRT_ZZ_DAG' Gate

Phases the -1 eigenspace of the ZZ observable by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    SQRT_ZZ_DAG 5 6
    SQRT_ZZ_DAG 42 43
    SQRT_ZZ_DAG 5 6 42 43
    
Stabilizer Generators:

    X_ -> -YZ
    Z_ -> Z_
    _X -> -ZY
    _Z -> _Z
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,   -i,     ,     ]
    [    ,     ,   -i,     ]
    [    ,     ,     , +1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_ZZ_DAG 0 1`
    H 1
    CNOT 0 1
    H 1
    S 0
    S 0
    S 0
    S 1
    S 1
    S 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SQRT_ZZ_DAG 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="SWAP"></a>
### The 'SWAP' Gate

Swaps two qubits.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    SWAP 5 6
    SWAP 42 43
    SWAP 5 6 42 43
    
Stabilizer Generators:

    X_ -> _X
    Z_ -> _Z
    _X -> X_
    _Z -> Z_
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,     , +1  ,     ]
    [    , +1  ,     ,     ]
    [    ,     ,     , +1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SWAP 0 1`
    CNOT 0 1
    CNOT 1 0
    CNOT 0 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SWAP 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="SWAPCX"></a>
### The 'SWAPCX' Gate

A combination SWAP-then-CX gate.
This gate is kak-equivalent to the iswap gate, but preserves X/Z noise bias.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    SWAPCX 5 6
    SWAPCX 42 43
    SWAPCX 5 6 42 43
    
Stabilizer Generators:

    X_ -> _X
    Z_ -> ZZ
    _X -> XX
    _Z -> Z_
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,     ,     , +1  ]
    [    , +1  ,     ,     ]
    [    ,     , +1  ,     ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SWAPCX 0 1`
    CNOT 0 1
    CNOT 1 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SWAPCX 0 1` (but affects the measurement record and an ancilla qubit)
    MX 2
    MZZ 0 2
    MXX 1 2
    MZ 2
    MXX 0 2
    MZZ 1 2
    MX 2
    CX rec[-6] 0 rec[-6] 1 rec[-2] 0 rec[-4] 1
    CZ rec[-7] 0 rec[-7] 1 rec[-5] 0 rec[-5] 1 rec[-3] 1 rec[-1] 1
                

<a name="XCX"></a>
### The 'XCX' Gate

The X-controlled X gate.
First qubit is the control, second qubit is the target.

Applies an X gate to the target if the control is in the |-> state.

Negates the amplitude of the |->|-> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    XCX 5 6
    XCX 42 43
    XCX 5 6 42 43
    
Stabilizer Generators:

    X_ -> X_
    Z_ -> ZX
    _X -> _X
    _Z -> XZ
    
Unitary Matrix (little endian):

    [+1  , +1  , +1  , -1  ]
    [+1  , +1  , -1  , +1  ]
    [+1  , -1  , +1  , +1  ]
    [-1  , +1  , +1  , +1  ] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `XCX 0 1`
    H 0
    CNOT 0 1
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `XCX 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="XCY"></a>
### The 'XCY' Gate

The X-controlled Y gate.
First qubit is the control, second qubit is the target.

Applies a Y gate to the target if the control is in the |-> state.

Negates the amplitude of the |->|-i> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    XCY 5 6
    XCY 42 43
    XCY 5 6 42 43
    
Stabilizer Generators:

    X_ -> X_
    Z_ -> ZY
    _X -> XX
    _Z -> XZ
    
Unitary Matrix (little endian):

    [+1  , +1  ,   -i,   +i]
    [+1  , +1  ,   +i,   -i]
    [  +i,   -i, +1  , +1  ]
    [  -i,   +i, +1  , +1  ] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `XCY 0 1`
    H 0
    S 1
    S 1
    S 1
    CNOT 0 1
    H 0
    S 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `XCY 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="XCZ"></a>
### The 'XCZ' Gate

The X-controlled Z gate.
Applies a Z gate to the target if the control is in the |-> state.
Equivalently: negates the amplitude of the |->|1> state.
Same as a CX gate, but with reversed qubit order.
The first qubit is the control, and the second qubit is the target.

To perform a classically controlled X, replace the Z target with a `rec`
target like rec[-2].

To perform an I or X gate as configured by sweep data, replace the
Z target with a `sweep` target like sweep[3].

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    # Bit flip qubit 5 controlled by qubit 2.
    XCZ 5 2

    # Perform CX 2 5 then CX 4 2.
    XCZ 5 2 2 4

    # Bit flip qubit 6 if the most recent measurement result was TRUE.
    XCZ 6 rec[-1]

    # Bit flip qubits 7 and 8 conditioned on sweep configuration data.
    XCZ 7 sweep[5] 8 sweep[5]
Stabilizer Generators:

    X_ -> X_
    Z_ -> ZZ
    _X -> XX
    _Z -> _Z
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    , +1  ,     ,     ]
    [    ,     ,     , +1  ]
    [    ,     , +1  ,     ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `XCZ 0 1`
    CNOT 1 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `XCZ 0 1` (but affects the measurement record and an ancilla qubit)
    MZ 2
    MXX 0 2
    MZZ 1 2
    MX 2
    CX rec[-4] 0 rec[-2] 0
    CZ rec[-3] 1 rec[-1] 1
                

<a name="YCX"></a>
### The 'YCX' Gate

The Y-controlled X gate.
First qubit is the control, second qubit is the target.

Applies an X gate to the target if the control is in the |-i> state.

Negates the amplitude of the |-i>|-> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    YCX 5 6
    YCX 42 43
    YCX 5 6 42 43
    
Stabilizer Generators:

    X_ -> XX
    Z_ -> ZX
    _X -> _X
    _Z -> YZ
    
Unitary Matrix (little endian):

    [+1  ,   -i, +1  ,   +i]
    [  +i, +1  ,   -i, +1  ]
    [+1  ,   +i, +1  ,   -i]
    [  -i, +1  ,   +i, +1  ] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `YCX 0 1`
    S 0
    S 0
    S 0
    H 1
    CNOT 1 0
    S 0
    H 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `YCX 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="YCY"></a>
### The 'YCY' Gate

The Y-controlled Y gate.
First qubit is the control, second qubit is the target.

Applies a Y gate to the target if the control is in the |-i> state.

Negates the amplitude of the |-i>|-i> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    YCY 5 6
    YCY 42 43
    YCY 5 6 42 43
    
Stabilizer Generators:

    X_ -> XY
    Z_ -> ZY
    _X -> YX
    _Z -> YZ
    
Unitary Matrix (little endian):

    [+1  ,   -i,   -i, +1  ]
    [  +i, +1  , -1  ,   -i]
    [  +i, -1  , +1  ,   -i]
    [+1  ,   +i,   +i, +1  ] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `YCY 0 1`
    S 0
    S 0
    S 0
    S 1
    S 1
    S 1
    H 0
    CNOT 0 1
    H 0
    S 0
    S 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `YCY 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="YCZ"></a>
### The 'YCZ' Gate

The Y-controlled Z gate.
Applies a Z gate to the target if the control is in the |-i> state.
Equivalently: negates the amplitude of the |-i>|1> state.
Same as a CY gate, but with reversed qubit order.
The first qubit is called the control, and the second qubit is the target.

To perform a classically controlled Y, replace the Z target with a `rec`
target like rec[-2].

To perform an I or Y gate as configured by sweep data, replace the
Z target with a `sweep` target like sweep[3].

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    # Apply Y to qubit 5 controlled by qubit 2.
    YCZ 5 2

    # Perform CY 2 5 then CY 4 2.
    YCZ 5 2 2 4

    # Apply Y to qubit 6 if the most recent measurement result was TRUE.
    YCZ 6 rec[-1]

    # Apply Y to qubits 7 and 8 conditioned on sweep configuration data.
    YCZ 7 sweep[5] 8 sweep[5]
Stabilizer Generators:

    X_ -> XZ
    Z_ -> ZZ
    _X -> YX
    _Z -> _Z
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    , +1  ,     ,     ]
    [    ,     ,     ,   -i]
    [    ,     ,   +i,     ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `YCZ 0 1`
    S 0
    S 0
    S 0
    CNOT 1 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `YCZ 0 1` (but affects the measurement record and an ancilla qubit)
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
                

## Noise Channels

<a name="DEPOLARIZE1"></a>
### The 'DEPOLARIZE1' Instruction

The single qubit depolarizing channel.

Applies a single-qubit depolarizing error with the given probability.
When a single-qubit depolarizing error is applied, a random Pauli
error (except for I) is chosen and applied. Note that this means
maximal mixing occurs when the probability parameter is set to 75%,
rather than at 100%.

Applies a randomly chosen Pauli with a given probability.

Parens Arguments:

    A single float (p) specifying the depolarization strength.

Targets:

    Qubits to apply single-qubit depolarizing noise to.

Pauli Mixture:

    1-p: I
    p/3: X
    p/3: Y
    p/3: Z

Examples:

    # Apply 1-qubit depolarization to qubit 0 using p=1%
    DEPOLARIZE1(0.01) 0

    # Apply 1-qubit depolarization to qubit 2
    # Separately apply 1-qubit depolarization to qubits 3 and 5
    DEPOLARIZE1(0.01) 2 3 5

    # Maximally mix qubits 0 through 2
    DEPOLARIZE1(0.75) 0 1 2

<a name="DEPOLARIZE2"></a>
### The 'DEPOLARIZE2' Instruction

The two qubit depolarizing channel.

Applies a two-qubit depolarizing error with the given probability.
When a two-qubit depolarizing error is applied, a random pair of Pauli
errors (except for II) is chosen and applied. Note that this means
maximal mixing occurs when the probability parameter is set to 93.75%,
rather than at 100%.

Parens Arguments:

    A single float (p) specifying the depolarization strength.

Targets:

    Qubit pairs to apply two-qubit depolarizing noise to.

Pauli Mixture:

     1-p: II
    p/15: IX
    p/15: IY
    p/15: IZ
    p/15: XI
    p/15: XX
    p/15: XY
    p/15: XZ
    p/15: YI
    p/15: YX
    p/15: YY
    p/15: YZ
    p/15: ZI
    p/15: ZX
    p/15: ZY
    p/15: ZZ

Examples:

    # Apply 2-qubit depolarization to qubit 0 and qubit 1 using p=1%
    DEPOLARIZE2(0.01) 0 1

    # Apply 2-qubit depolarization to qubit 2 and qubit 3
    # Separately apply 2-qubit depolarization to qubit 5 and qubit 7
    DEPOLARIZE2(0.01) 2 3 5 7

    # Maximally mix qubits 0 through 3
    DEPOLARIZE2(0.9375) 0 1 2 3

<a name="E"></a>
### The 'E' Instruction

Alternate name: <a name="CORRELATED_ERROR"></a>`CORRELATED_ERROR`

Probabilistically applies a Pauli product error with a given probability.
Sets the "correlated error occurred flag" to true if the error occurred.
Otherwise sets the flag to false.

See also: `ELSE_CORRELATED_ERROR`.

Parens Arguments:

    A single float specifying the probability of applying the Paulis making up the error.

Targets:

    Pauli targets specifying the Paulis to apply when the error occurs.
    Note that, for backwards compatibility reasons, the targets are not combined using combiners (`*`).
    They are implicitly all combined.

Example:

    # With 60% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
    CORRELATED_ERROR(0.2) X1 Y2
    ELSE_CORRELATED_ERROR(0.25) Z2 Z3
    ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3

<a name="ELSE_CORRELATED_ERROR"></a>
### The 'ELSE_CORRELATED_ERROR' Instruction

Probabilistically applies a Pauli product error with a given probability, unless the "correlated error occurred flag" is set.
If the error occurs, sets the "correlated error occurred flag" to true.
Otherwise leaves the flag alone.

Note: when converting a circuit into a detector error model, every `ELSE_CORRELATED_ERROR` instruction must be preceded by
an ELSE_CORRELATED_ERROR instruction or an E instruction. In other words, ELSE_CORRELATED_ERROR instructions should appear
in contiguous chunks started by a CORRELATED_ERROR.

See also: `CORRELATED_ERROR`.

Parens Arguments:

    A single float specifying the probability of applying the Paulis making up the error, conditioned on the "correlated
    error occurred flag" being False.

Targets:

    Pauli targets specifying the Paulis to apply when the error occurs.
    Note that, for backwards compatibility reasons, the targets are not combined using combiners (`*`).
    They are implicitly all combined.

Example:

    # With 60% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
    CORRELATED_ERROR(0.2) X1 Y2
    ELSE_CORRELATED_ERROR(0.25) Z2 Z3
    ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3

<a name="HERALDED_ERASE"></a>
### The 'HERALDED_ERASE' Instruction

The heralded erasure noise channel.

Whether or not this noise channel fires is recorded into the measurement
record. When it doesn't fire, nothing happens to the target qubit and a
0 is recorded. When it does fire, a 1 is recorded and the target qubit
is erased to the maximally mixed state by applying X_ERROR(0.5) and
Z_ERROR(0.5).

CAUTION: when converting a circuit with this error into a detector
error model, this channel is split into multiple potential effects.
In the context of a DEM, these effects are considered independent.
This is an approximation, because independent effects can be combined.
The effect of this approximation, assuming a detector is declared
on the herald, is that it appears this detector can be cancelled out
by two of the (originally disjoint) heralded effects firing together.
Sampling from the DEM instead of the circuit can thus produce unheralded
errors, even if the circuit noise model only contains heralded errors.
These issues occur with probability p^2, where p is the probability of a
heralded error, since two effects that came from the same heralded error
must occur together to cancel out the herald detector. This also means
a decoder configured using the DEM will think there's a chance of unheralded
errors even if the circuit the DEM came from only uses heralded errors.

Parens Arguments:

    A single float (p) specifying the chance of the noise firing.

Targets:

    Qubits to apply single-qubit depolarizing noise to. Each target
    is operated on independently.

Pauli Mixture:

    1-p: record 0, apply I
    p/4: record 1, apply I
    p/4: record 1, apply X
    p/4: record 1, apply Y
    p/4: record 1, apply Z

Examples:

    # Erase qubit 0 with probability 1%
    HERALDED_ERASE(0.01) 0
    # Declare a flag detector based on the erasure
    DETECTOR rec[-1]

    # Erase qubit 2 with 2% probability
    # Separately, erase qubit 3 with 2% probability
    HERALDED_ERASE(0.02) 2 3

    # Do an XXXX measurement
    MPP X2*X3*X5*X7
    # Apply partially-heralded noise to the two qubits
    HERALDED_ERASE(0.01) 2 3 5 7
    DEPOLARIZE1(0.0001) 2 3 5 7
    # Repeat the XXXX measurement
    MPP X2*X3*X5*X7
    # Declare a detector comparing the two XXXX measurements
    DETECTOR rec[-1] rec[-6]
    # Declare flag detectors based on the erasures
    DETECTOR rec[-2]
    DETECTOR rec[-3]
    DETECTOR rec[-4]
    DETECTOR rec[-5]

<a name="HERALDED_PAULI_CHANNEL_1"></a>
### The 'HERALDED_PAULI_CHANNEL_1' Instruction

A heralded error channel that applies biased noise.

This error records a bit into the measurement record, indicating whether
or not the herald fired. How likely it is that the herald fires, and the
corresponding chance of each possible error effect (I, X, Y, or Z) are
configured by the parens arguments of the instruction.

CAUTION: when converting a circuit with this error into a detector
error model, this channel is split into multiple potential effects.
In the context of a DEM, these effects are considered independent.
This is an approximation, because independent effects can be combined.
The effect of this approximation, assuming a detector is declared
on the herald, is that it appears this detector can be cancelled out
by two of the (originally disjoint) heralded effects firing together.
Sampling from the DEM instead of the circuit can thus produce unheralded
errors, even if the circuit noise model only contains heralded errors.
These issues occur with probability p^2, where p is the probability of a
heralded error, since two effects that came from the same heralded error
must occur together to cancel out the herald detector. This also means
a decoder configured using the DEM will think there's a chance of unheralded
errors even if the circuit the DEM came from only uses heralded errors.

Parens Arguments:

    This instruction takes four arguments (pi, px, py, pz). The
    arguments are disjoint probabilities, specifying the chances
    of heralding with various effects.

    pi is the chance of heralding with no effect (a false positive).
    px is the chance of heralding with an X error.
    py is the chance of heralding with a Y error.
    pz is the chance of heralding with a Z error.

Targets:

    Qubits to apply heralded biased noise to.

Pauli Mixture:

    1-pi-px-py-pz: record 0, apply I
               pi: record 1, apply I
               px: record 1, apply X
               py: record 1, apply Y
               pz: record 1, apply Z

Examples:

    # With 10% probability perform a phase flip of qubit 0.
    HERALDED_PAULI_CHANNEL_1(0, 0, 0, 0.1) 0
    DETECTOR rec[-1]  # Include the herald in detectors available to the decoder

    # With 20% probability perform a heralded dephasing of qubit 0.
    HERALDED_PAULI_CHANNEL_1(0.1, 0, 0, 0.1) 0
    DETECTOR rec[-1]

    # Subject a Bell Pair to heralded noise.
    MXX 0 1
    MZZ 0 1
    HERALDED_PAULI_CHANNEL_1(0.01, 0.02, 0.03, 0.04) 0 1
    MXX 0 1
    MZZ 0 1
    DETECTOR rec[-1] rec[-5]  # Did ZZ stabilizer change?
    DETECTOR rec[-2] rec[-6]  # Did XX stabilizer change?
    DETECTOR rec[-3]    # Did the herald on qubit 1 fire?
    DETECTOR rec[-4]    # Did the herald on qubit 0 fire?

<a name="II_ERROR"></a>
### The 'II_ERROR' Instruction

Applies a two-qubit identity with a given probability.

This gate has no effect. It only exists because it can be useful as a
communication mechanism for systems built on top of stim.

Parens Arguments:

    A list of disjoint probabilities summing to at most 1.

    The probabilities have no effect on stim simulations or error analysis, but may be
    interpreted in arbitrary ways by external tools.

Targets:

    Qubits to apply identity noise to.

Pauli Mixture:

    *: II

Examples:

    # does nothing
    II_ERROR 0 1

    # does nothing with probability 0.1, else does nothing
    II_ERROR(0.1) 0 1

    # checks for you that the targets are two-qubit pairs
    II_ERROR[TWO_QUBIT_LEAKAGE_NOISE_FOR_AN_ADVANCED_SIMULATOR:0.1] 0 2 4 6

    # checks for you that the disjoint probabilities in the arguments are legal
    II_ERROR[MULTIPLE_TWO_QUBIT_NOISE_MECHANISMS](0.1, 0.2) 0 2 4 6


<a name="I_ERROR"></a>
### The 'I_ERROR' Instruction

Applies an identity with a given probability.

This gate has no effect. It only exists because it can be useful as a
communication mechanism for systems built on top of stim.

Parens Arguments:

    A list of disjoint probabilities summing to at most 1.

    The probabilities have no effect on stim simulations or error analysis, but may be
    interpreted in arbitrary ways by external tools.

Targets:

    Qubits to apply identity noise to.

Pauli Mixture:

     *: I

Examples:

    # does nothing
    I_ERROR 0

    # does nothing with probability 0.1, else does nothing
    I_ERROR(0.1) 0

    # doesn't require a probability argument
    I_ERROR[LEAKAGE_NOISE_FOR_AN_ADVANCED_SIMULATOR:0.1] 0 2 4

    # checks for you that the disjoint probabilities in the arguments are legal
    I_ERROR[MULTIPLE_NOISE_MECHANISMS](0.1, 0.2) 0 2 4

<a name="PAULI_CHANNEL_1"></a>
### The 'PAULI_CHANNEL_1' Instruction

A single qubit Pauli error channel with explicitly specified probabilities for each case.

Parens Arguments:

    Three floats specifying disjoint Pauli case probabilities.
    px: Disjoint probability of applying an X error.
    py: Disjoint probability of applying a Y error.
    pz: Disjoint probability of applying a Z error.

Targets:

    Qubits to apply the custom noise channel to.

Example:

    # Sample errors from the distribution 10% X, 15% Y, 20% Z, 55% I.
    # Apply independently to qubits 1, 2, 4.
    PAULI_CHANNEL_1(0.1, 0.15, 0.2) 1 2 4

Pauli Mixture:

    1-px-py-pz: I
    px: X
    py: Y
    pz: Z

<a name="PAULI_CHANNEL_2"></a>
### The 'PAULI_CHANNEL_2' Instruction

A two qubit Pauli error channel with explicitly specified probabilities for each case.

Parens Arguments:

    Fifteen floats specifying the disjoint probabilities of each possible Pauli pair
    that can occur (except for the non-error double identity case).
    The disjoint probability arguments are (in order):

    1. pix: Probability of applying an IX operation.
    2. piy: Probability of applying an IY operation.
    3. piz: Probability of applying an IZ operation.
    4. pxi: Probability of applying an XI operation.
    5. pxx: Probability of applying an XX operation.
    6. pxy: Probability of applying an XY operation.
    7. pxz: Probability of applying an XZ operation.
    8. pyi: Probability of applying a YI operation.
    9. pyx: Probability of applying a YX operation.
    10. pyy: Probability of applying a YY operation.
    11. pyz: Probability of applying a YZ operation.
    12. pzi: Probability of applying a ZI operation.
    13. pzx: Probability of applying a ZX operation.
    14. pzy: Probability of applying a ZY operation.
    15. pzz: Probability of applying a ZZ operation.

Targets:

    Pairs of qubits to apply the custom noise channel to.
    There must be an even number of targets.

Example:

    # Sample errors from the distribution 10% XX, 20% YZ, 70% II.
    # Apply independently to qubit pairs (1,2), (5,6), and (8,3)
    PAULI_CHANNEL_2(0,0,0, 0.1,0,0,0, 0,0,0,0.2, 0,0,0,0) 1 2 5 6 8 3

Pauli Mixture:

    1-pix-piy-piz-pxi-pxx-pxy-pxz-pyi-pyx-pyy-pyz-pzi-pzx-pzy-pzz: II
    pix: IX
    piy: IY
    piz: IZ
    pxi: XI
    pxx: XX
    pxy: XY
    pxz: XZ
    pyi: YI
    pyx: YX
    pyy: YY
    pyz: YZ
    pzi: ZI
    pzx: ZX
    pzy: ZY
    pzz: ZZ

<a name="X_ERROR"></a>
### The 'X_ERROR' Instruction

Applies a Pauli X with a given probability.

Parens Arguments:

    A single float specifying the probability of applying an X operation.

Targets:

    Qubits to apply bit flip noise to.

Pauli Mixture:

    1-p: I
     p : X

Example:

    X_ERROR(0.001) 5
    X_ERROR(0.001) 42
    X_ERROR(0.001) 5 42
    

<a name="Y_ERROR"></a>
### The 'Y_ERROR' Instruction

Applies a Pauli Y with a given probability.

Parens Arguments:

    A single float specifying the probability of applying a Y operation.

Targets:

    Qubits to apply Y flip noise to.

Pauli Mixture:

    1-p: I
     p : Y

Example:

    Y_ERROR(0.001) 5
    Y_ERROR(0.001) 42
    Y_ERROR(0.001) 5 42
    

<a name="Z_ERROR"></a>
### The 'Z_ERROR' Instruction

Applies a Pauli Z with a given probability.

Parens Arguments:

    A single float specifying the probability of applying a Z operation.

Targets:

    Qubits to apply phase flip noise to.

Pauli Mixture:

    1-p: I
     p : Z

Example:

    Z_ERROR(0.001) 5
    Z_ERROR(0.001) 42
    Z_ERROR(0.001) 5 42
    

## Collapsing Gates

<a name="M"></a>
### The 'M' Instruction

Alternate name: <a name="MZ"></a>`MZ`

Z-basis measurement.
Projects each target qubit into `|0>` or `|1>` and reports its value (false=`|0>`, true=`|1>`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the Z basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Z basis, and append the result into the measurement record.
    M 5

    # 'MZ' is the same as 'M'. This also measures qubit 5 in the Z basis.
    MZ 5

    # Measure qubit 5 in the Z basis, and append the INVERSE of its result into the measurement record.
    MZ !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MZ(0.01) 5

    # Measure multiple qubits in the Z basis, putting 3 bits into the measurement record.
    MZ 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MZ(0.02) 2 3 5
Stabilizer Generators:

    Z -> rec[-1]
    Z -> Z
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `M 0`
    M 0
    
    # (The decomposition is trivial because this gate is in the target gate set.)
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `M 0` (but affects the measurement record and an ancilla qubit)
    MZ 0
                
    # (The decomposition is trivial because this gate is in the target gate set.)
    

<a name="MR"></a>
### The 'MR' Instruction

Alternate name: <a name="MRZ"></a>`MRZ`

Z-basis demolition measurement (optionally noisy).
Projects each target qubit into `|0>` or `|1>`, reports its value (false=`|0>`, true=`|1>`), then resets to `|0>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the Z basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Z basis, reset it to the |0> state, append the measurement result into the measurement record.
    MRZ 5

    # MR is also a Z-basis demolition measurement.
    MR 5

    # Demolition measure qubit 5 in the Z basis, but append the INVERSE of its result into the measurement record.
    MRZ !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRZ(0.01) 5

    # Demolition measure multiple qubits in the Z basis, putting 3 bits into the measurement record.
    MRZ 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRZ(0.02) 2 3 5
Stabilizer Generators:

    Z -> rec[-1]
    1 -> Z
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MR 0`
    M 0
    R 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `MR 0` (but affects the measurement record and an ancilla qubit)
    MZ 0
    CX rec[-1] 0
                

<a name="MRX"></a>
### The 'MRX' Instruction

X-basis demolition measurement (optionally noisy).
Projects each target qubit into `|+>` or `|->`, reports its value (false=`|+>`, true=`|->`), then resets to `|+>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the X basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the X basis, reset it to the |+> state, append the measurement result into the measurement record.
    MRX 5

    # Demolition measure qubit 5 in the X basis, but append the INVERSE of its result into the measurement record.
    MRX !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRX(0.01) 5

    # Demolition measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MRX 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRX(0.02) 2 3 5
Stabilizer Generators:

    X -> rec[-1]
    1 -> X
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MRX 0`
    H 0
    M 0
    R 0
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `MRX 0` (but affects the measurement record and an ancilla qubit)
    MX 0
    CZ rec[-1] 0
                

<a name="MRY"></a>
### The 'MRY' Instruction

Y-basis demolition measurement (optionally noisy).
Projects each target qubit into `|i>` or `|-i>`, reports its value (false=`|i>`, true=`|-i>`), then resets to `|i>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the Y basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Y basis, reset it to the |i> state, append the measurement result into the measurement record.
    MRY 5

    # Demolition measure qubit 5 in the Y basis, but append the INVERSE of its result into the measurement record.
    MRY !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRY(0.01) 5

    # Demolition measure multiple qubits in the Y basis, putting 3 bits into the measurement record.
    MRY 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRY(0.02) 2 3 5
Stabilizer Generators:

    Y -> rec[-1]
    1 -> Y
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MRY 0`
    S 0
    S 0
    S 0
    H 0
    M 0
    R 0
    H 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `MRY 0` (but affects the measurement record and an ancilla qubit)
    MY 0
    CX rec[-1] 0
                

<a name="MX"></a>
### The 'MX' Instruction

X-basis measurement.
Projects each target qubit into `|+>` or `|->` and reports its value (false=`|+>`, true=`|->`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the X basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the X basis, and append the result into the measurement record.
    MX 5

    # Measure qubit 5 in the X basis, and append the INVERSE of its result into the measurement record.
    MX !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MX(0.01) 5

    # Measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MX 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MX(0.02) 2 3 5
Stabilizer Generators:

    X -> rec[-1]
    X -> X
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MX 0`
    H 0
    M 0
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `MX 0` (but affects the measurement record and an ancilla qubit)
    MX 0
                
    # (The decomposition is trivial because this gate is in the target gate set.)
    

<a name="MY"></a>
### The 'MY' Instruction

Y-basis measurement.
Projects each target qubit into `|i>` or `|-i>` and reports its value (false=`|i>`, true=`|-i>`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the Y basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Y basis, and append the result into the measurement record.
    MY 5

    # Measure qubit 5 in the Y basis, and append the INVERSE of its result into the measurement record.
    MY !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MY(0.01) 5

    # Measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MY 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MY(0.02) 2 3 5
Stabilizer Generators:

    Y -> rec[-1]
    Y -> Y
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MY 0`
    S 0
    S 0
    S 0
    H 0
    M 0
    H 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `MY 0` (but affects the measurement record and an ancilla qubit)
    MY 0
                
    # (The decomposition is trivial because this gate is in the target gate set.)
    

<a name="R"></a>
### The 'R' Instruction

Alternate name: <a name="RZ"></a>`RZ`

Z-basis reset.
Forces each target qubit into the `|0>` state by silently measuring it in the Z basis and applying an `X` gate if it ended up in the `|1>` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the Z basis.

Examples:

    # Reset qubit 5 into the |0> state.
    RZ 5

    # R means the same thing as RZ.
    R 5

    # Reset multiple qubits into the |0> state.
    RZ 2 3 5
Stabilizer Generators:

    1 -> Z
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `R 0`
    R 0
    
    # (The decomposition is trivial because this gate is in the target gate set.)
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `R 0` (but affects the measurement record and an ancilla qubit)
    MZ 0
    CX rec[-1] 0
                

<a name="RX"></a>
### The 'RX' Instruction

X-basis reset.
Forces each target qubit into the `|+>` state by silently measuring it in the X basis and applying a `Z` gate if it ended up in the `|->` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the X basis.

Examples:

    # Reset qubit 5 into the |+> state.
    RX 5

    # Reset multiple qubits into the |+> state.
    RX 2 3 5
Stabilizer Generators:

    1 -> X
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `RX 0`
    R 0
    H 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `RX 0` (but affects the measurement record and an ancilla qubit)
    MX 0
    CZ rec[-1] 0
                

<a name="RY"></a>
### The 'RY' Instruction

Y-basis reset.
Forces each target qubit into the `|i>` state by silently measuring it in the Y basis and applying an `X` gate if it ended up in the `|-i>` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the Y basis.

Examples:

    # Reset qubit 5 into the |i> state.
    RY 5

    # Reset multiple qubits into the |i> state.
    RY 2 3 5
Stabilizer Generators:

    1 -> Y
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `RY 0`
    R 0
    H 0
    S 0
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `RY 0` (but affects the measurement record and an ancilla qubit)
    MY 0
    CX rec[-1] 0
                

## Pair Measurement Gates

<a name="MXX"></a>
### The 'MXX' Instruction

Two-qubit X basis parity measurement.

This operation measures whether pairs of qubits are in the {|++>,|-->} subspace or in the
{|+->,|-+>} subspace of the two qubit state space. |+> and |-> are the +1 and -1
eigenvectors of the X operator.

If the qubits were in the {|++>,|-->} subspace, False is appended to the measurement record.
If the qubits were in the {|+->,|-+>} subspace, True is appended to the measurement record.
Inverting one of the qubit targets inverts the result.

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The pairs of qubits to measure in the X basis.

    This operation accepts inverted qubit targets (like `!5` instead of `5`). Inverted
    targets flip the measurement result.

Examples:

    # Measure the +XX observable of qubit 1 vs qubit 2.
    MXX 1 2

    # Measure the -XX observable of qubit 1 vs qubit 2.
    MXX !1 2

    # Do a noisy measurement of the +XX observable of qubit 2 vs qubit 3.
    # The result recorded to the measurement record will be flipped 1% of the time.
    MXX(0.01) 2 3

    # Measure the +XX observable qubit 1 vs qubit 2, and also qubit 8 vs qubit 9
    MXX 1 2 8 9

    # Perform multiple noisy measurements.
    # Each measurement has an independent 2% chance of being recorded wrong.
    MXX(0.02) 2 3 5 7 11 19 17 4
Stabilizer Generators:

    X_ -> X_
    _X -> _X
    ZZ -> ZZ
    XX -> rec[-1]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MXX 0 1`
    CX 0 1
    H 0
    M 0
    H 0
    CX 0 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `MXX 0 1` (but affects the measurement record and an ancilla qubit)
    MXX 0 1
                
    # (The decomposition is trivial because this gate is in the target gate set.)
    

<a name="MYY"></a>
### The 'MYY' Instruction

Two-qubit Y basis parity measurement.

This operation measures whether pairs of qubits are in the {|ii>,|jj>} subspace or in the
{|ij>,|ji>} subspace of the two qubit state space. |i> and |j> are the +1 and -1
eigenvectors of the Y operator.

If the qubits were in the {|ii>,|jj>} subspace, False is appended to the measurement record.
If the qubits were in the {|ij>,|ji>} subspace, True is appended to the measurement record.
Inverting one of the qubit targets inverts the result.

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The pairs of qubits to measure in the Y basis.

    This operation accepts inverted qubit targets (like `!5` instead of `5`). Inverted
    targets flip the measurement result.

Examples:

    # Measure the +YY observable of qubit 1 vs qubit 2.
    MYY 1 2

    # Measure the -YY observable of qubit 1 vs qubit 2.
    MYY !1 2

    # Do a noisy measurement of the +YY observable of qubit 2 vs qubit 3.
    # The result recorded to the measurement record will be flipped 1% of the time.
    MYY(0.01) 2 3

    # Measure the +YY observable qubit 1 vs qubit 2, and also qubit 8 vs qubit 9
    MYY 1 2 8 9

    # Perform multiple noisy measurements.
    # Each measurement has an independent 2% chance of being recorded wrong.
    MYY(0.02) 2 3 5 7 11 19 17 4
Stabilizer Generators:

    XX -> XX
    Y_ -> Y_
    _Y -> _Y
    YY -> rec[-1]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MYY 0 1`
    S 0 1
    CX 0 1
    H 0
    M 0
    S 1 1
    H 0
    CX 0 1
    S 0 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `MYY 0 1` (but affects the measurement record and an ancilla qubit)
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
                

<a name="MZZ"></a>
### The 'MZZ' Instruction

Two-qubit Z basis parity measurement.

This operation measures whether pairs of qubits are in the {|00>,|11>} subspace or in the
{|01>,|10>} subspace of the two qubit state space. |0> and |1> are the +1 and -1
eigenvectors of the Z operator.

If the qubits were in the {|00>,|11>} subspace, False is appended to the measurement record.
If the qubits were in the {|01>,|10>} subspace, True is appended to the measurement record.
Inverting one of the qubit targets inverts the result.

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The pairs of qubits to measure in the Z basis.

    This operation accepts inverted qubit targets (like `!5` instead of `5`). Inverted
    targets flip the measurement result.

Examples:

    # Measure the +ZZ observable of qubit 1 vs qubit 2.
    MZZ 1 2

    # Measure the -ZZ observable of qubit 1 vs qubit 2.
    MZZ !1 2

    # Do a noisy measurement of the +ZZ observable of qubit 2 vs qubit 3.
    # The result recorded to the measurement record will be flipped 1% of the time.
    MZZ(0.01) 2 3

    # Measure the +ZZ observable qubit 1 vs qubit 2, and also qubit 8 vs qubit 9
    MZZ 1 2 8 9

    # Perform multiple noisy measurements.
    # Each measurement has an independent 2% chance of being recorded wrong.
    MZZ(0.02) 2 3 5 7 11 19 17 4
Stabilizer Generators:

    XX -> XX
    Z_ -> Z_
    _Z -> _Z
    ZZ -> rec[-1]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MZZ 0 1`
    CX 0 1
    M 1
    CX 0 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `MZZ 0 1` (but affects the measurement record and an ancilla qubit)
    MZZ 0 1
                
    # (The decomposition is trivial because this gate is in the target gate set.)
    

## Generalized Pauli Product Gates

<a name="MPP"></a>
### The 'MPP' Instruction

Measures general pauli product operators, like X1*Y2*Z3.

Parens Arguments:

    An optional failure probability.
    If no argument is given, all measurements are perfect.
    If one argument is given, it's the chance of reporting measurement results incorrectly.

Targets:

    A series of Pauli products to measure.

    Each Pauli product is a series of Pauli targets (like `X1`, `Y2`, or `Z3`) separated by
    combiners (`*`). Each Pauli term can be inverted (like `!Y2` instead of `Y2`). A negated
    product will record the opposite measurement result.

    Note that, although you can write down instructions that measure anti-Hermitian products,
    like `MPP X1*Z1`, doing this will cause exceptions when you simulate or analyze the
    circuit since measuring an anti-Hermitian operator doesn't have well defined semantics.

    Using overly-complicated Hermitian products, like saying `MPP X1*Y1*Y2*Z2` instead of
    `MPP !Z1*X2`, is technically allowed. But probably not a great idea since tools consuming
    the circuit may have assumed that each qubit would appear at most once in each product.

Examples:

    # Measure the two-body +X1*Y2 observable.
    MPP X1*Y2

    # Measure the one-body -Z5 observable.
    MPP !Z5

    # Measure the two-body +X1*Y2 observable and also the three-body -Z3*Z4*Z5 observable.
    MPP X1*Y2 !Z3*Z4*Z5

    # Noisily measure +Z1+Z2 and +X1*X2 (independently flip each reported result 0.1% of the time).
    MPP(0.001) Z1*Z2 X1*X2

Stabilizer Generators (for `MPP X0*Y1*Z2 X3*X4`):

    XYZ__ -> rec[-2]
    ___XX -> rec[-1]
    X____ -> X____
    _Y___ -> _Y___
    __Z__ -> __Z__
    ___X_ -> ___X_
    ____X -> ____X
    ZZ___ -> ZZ___
    _XX__ -> _XX__
    ___ZZ -> ___ZZ
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MPP X0*Y1*Z2 X3*X4`
    S 1 1 1
    H 0 1 3 4
    CX 2 0 1 0 4 3
    M 0 3
    CX 2 0 1 0 4 3
    H 0 1 3 4
    S 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `MPP X0*Y1*Z2 X3*X4` (but affects the measurement record and an ancilla qubit)
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
                

<a name="SPP"></a>
### The 'SPP' Gate

The generalized S gate. Phases the -1 eigenspace of Pauli product observables by i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    A series of Pauli products to phase.

    Each Pauli product is a series of Pauli targets (like `X1`, `Y2`, or `Z3`) separated by
    combiners (`*`). Each Pauli term can be inverted (like `!Y2` instead of `Y2`), to negate
    the product.

    Note that, although you can write down instructions that phase anti-Hermitian products,
    like `SPP X1*Z1`, doing this will cause exceptions when you simulate or analyze the
    circuit since phasing an anti-Hermitian operator doesn't have well defined semantics.

    Using overly-complicated Hermitian products, like saying `SPP X1*Y1*Y2*Z2` instead of
    `SPP !Z1*X2`, is technically allowed. But probably not a great idea since tools consuming
    the circuit may have assumed that each qubit would appear at most once in each product.

Examples:

    # Perform an S gate on qubit 1.
    SPP Z1

    # Perform a SQRT_X gate on qubit 1.
    SPP X1

    # Perform a SQRT_X_DAG gate on qubit 1.
    SPP !X1

    # Perform a SQRT_XX gate between qubit 1 and qubit 2.
    SPP X1*X2

    # Perform a SQRT_YY gate between qubit 1 and 2, and a SQRT_ZZ_DAG between qubit 3 and 4.
    SPP Y1*Y2 !Z1*Z2

    # Phase the -1 eigenspace of -X1*Y2*Z3 by i.
    SPP !X1*Y2*Z3

Stabilizer Generators (for `SPP X0*Y1*Z2`):

    X__ -> X__
    Z__ -> -YYZ
    _X_ -> -XZZ
    _Z_ -> XXZ
    __X -> XYY
    __Z -> __Z
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SPP X0*Y1*Z2`
    CX 2 1
    CX 1 0
    S 1
    S 1
    H 1
    CX 1 0
    CX 2 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SPP X0*Y1*Z2` (but affects the measurement record and an ancilla qubit)
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
                

<a name="SPP_DAG"></a>
### The 'SPP_DAG' Gate

The generalized S_DAG gate. Phases the -1 eigenspace of Pauli product observables by -i.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    A series of Pauli products to phase.

    Each Pauli product is a series of Pauli targets (like `X1`, `Y2`, or `Z3`) separated by
    combiners (`*`). Each Pauli term can be inverted (like `!Y2` instead of `Y2`), to negate
    the product.

    Note that, although you can write down instructions that phase anti-Hermitian products,
    like `SPP X1*Z1`, doing this will cause exceptions when you simulate or analyze the
    circuit since phasing an anti-Hermitian operator doesn't have well defined semantics.

    Using overly-complicated Hermitian products, like saying `SPP X1*Y1*Y2*Z2` instead of
    `SPP !Z1*X2`, is technically allowed. But probably not a great idea since tools consuming
    the circuit may have assumed that each qubit would appear at most once in each product.

Examples:

    # Perform an S_DAG gate on qubit 1.
    SPP_DAG Z1

    # Perform a SQRT_X_DAG gate on qubit 1.
    SPP_DAG X1

    # Perform a SQRT_X gate on qubit 1.
    SPP_DAG !X1

    # Perform a SQRT_XX_DAG gate between qubit 1 and qubit 2.
    SPP_DAG X1*X2

    # Perform a SQRT_YY_DAG gate between qubit 1 and 2, and a SQRT_ZZ between qubit 3 and 4.
    SPP_DAG Y1*Y2 !Z1*Z2

    # Phase the -1 eigenspace of -X1*Y2*Z3 by -i.
    SPP_DAG !X1*Y2*Z3

Stabilizer Generators (for `SPP_DAG X0*Y1*Z2`):

    X__ -> X__
    Z__ -> YYZ
    _X_ -> XZZ
    _Z_ -> -XXZ
    __X -> -XYY
    __Z -> __Z
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SPP_DAG X0*Y1*Z2`
    CX 2 1
    CX 1 0
    H 1
    S 1
    S 1
    CX 1 0
    CX 2 1
    
MBQC Decomposition (into MX, MY, MZ, MXX, MZZ, and Pauli feedback):

    # The following circuit performs `SPP_DAG X0*Y1*Z2` (but affects the measurement record and an ancilla qubit)
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
                

## Control Flow

<a name="REPEAT"></a>
### The 'REPEAT' Instruction

Repeats the instructions in its body N times.

Currently, repetition counts of 0 are not allowed because they create corner cases with ambiguous resolutions.
For example, if a logical observable is only given measurements inside a repeat block with a repetition count of 0, it's
ambiguous whether the output of sampling the logical observables includes a bit for that logical observable.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    A positive integer in [1, 10^18] specifying the number of repetitions.

Example:

    REPEAT 2 {
        CNOT 0 1
        CNOT 2 1
        M 1
    }
    REPEAT 10000000 {
        CNOT 0 1
        CNOT 2 1
        M 1
        DETECTOR rec[-1] rec[-3]
    }

## Annotations

<a name="DETECTOR"></a>
### The 'DETECTOR' Instruction

Annotates that a set of measurements can be used to detect errors, because the set's parity should be deterministic.

Note that it is not necessary to say whether the measurement set's parity is even or odd; all that matters is that the
parity should be *consistent* when running the circuit and omitting all noisy operations. Note that, for example, this
means that even though `X` and `X_ERROR(1)` have equivalent effects on the measurements making up a detector, they have
differing effects on the detector (because `X` is intended, determining the expected value, and `X_ERROR` is noise,
causing deviations from the expected value).

Detectors are ignored when sampling measurements, but produce results when sampling detection events. In detector
sampling mode, each detector produces a result bit (where 0 means "measurement set had expected parity" and 1 means
"measurement set had incorrect parity"). When converting a circuit into a detector error model, errors are grouped based
on the detectors they flip (the "symptoms" of the error) and the observables they flip (the "frame changes" of the
error).

It is permitted, though not recommended, for the measurement set given to a `DETECTOR` instruction to have inconsistent
parity. When a detector's measurement set is inconsistent, the detector is called a "gauge detector" and the expected
parity of the measurement set is chosen arbitrarily (in an implementation-defined way). Some circuit analysis tools
(such as the circuit-to-detector-error-model conversion) will by default refuse to process circuits containing gauge
detectors. Gauge detectors produce random results when sampling detection events, though these results will be
appropriately correlated with other gauge detectors. For example, if `DETECTOR rec[-1]` and `DETECTOR rec[-2]` are gauge
detectors but `DETECTOR rec[-1] rec[-2]` is not, then under noiseless execution the two gauge detectors would either
always produce the same result or always produce opposite results.

Detectors can specify coordinates using their parens arguments. Coordinates have no effect on simulations, but can be
useful to tools consuming the circuit. For example, a tool drawing how the detectors in a circuit relate to each other
can use the coordinates as hints for where to place the detectors in the drawing.

Parens Arguments:

    Optional.
    Coordinate metadata, relative to the current coordinate offset accumulated from `SHIFT_COORDS` instructions.
    Can be any number of coordinates from 1 to 16.
    There is no required convention for which coordinate is which.

Targets:

    The measurement records to XOR together to get the deterministic-under-noiseless-execution parity.

Example:

    R 0
    X_ERROR(0.1) 0
    M 0  # This measurement is always False under noiseless execution.
    # Annotate that most recent measurement should be deterministic.
    DETECTOR rec[-1]

    R 0
    X 0
    X_ERROR(0.1) 0
    M 0  # This measurement is always True under noiseless execution.
    # Annotate that most recent measurement should be deterministic.
    DETECTOR rec[-1]

    R 0 1
    H 0
    CNOT 0 1
    DEPOLARIZE2(0.001) 0 1
    M 0 1  # These two measurements are always equal under noiseless execution.
    # Annotate that the parity of the previous two measurements should be consistent.
    DETECTOR rec[-1] rec[-2]

    # A series of trivial detectors with hinted coordinates along the diagonal line Y = 2X + 3.
    REPEAT 100 {
        R 0
        M 0
        SHIFT_COORDS(1, 2)
        DETECTOR(0, 3) rec[-1]
    }

<a name="MPAD"></a>
### The 'MPAD' Instruction

Pads the measurement record with the listed measurement results.

This can be useful for ensuring measurements are aligned to word boundaries, or that the
number of measurement bits produced per circuit layer is always the same even if the number
of measured qubits varies.

Parens Arguments:

    If no parens argument is given, the padding bits are recorded perfectly.
    If one parens argument is given, the padding bits are recorded noisily.
    The argument is the probability of recording the wrong result.

Targets:

    Each target is a measurement result to add.
    Targets should be the value 0 or the value 1.

Examples:

    # Append a False result to the measurement record.
    MPAD 0

    # Append a True result to the measurement record.
    MPAD 1

    # Append a series of results to the measurement record.
    MPAD 0 0 1 0 1

<a name="OBSERVABLE_INCLUDE"></a>
### The 'OBSERVABLE_INCLUDE' Instruction

Adds measurement records to a specified logical observable.

A potential point of confusion here is that Stim's notion of a logical observable is nothing more than a set of
measurements, potentially spanning across the entire circuit, that together produce a deterministic result. It's more
akin to the "boundary of a parity sheet" in a topological spacetime diagram than it is to the notion of a qubit
observable. For example, consider a surface code memory experiment that initializes a logical |0>, preserves the state
noise, and eventually performs a logical Z basis measurement. The circuit representing this experiment would use
`OBSERVABLE_INCLUDE` instructions to specifying which physical measurements within the logical Z basis measurement
should be XOR'd together to get the logical measurement result. This effectively identifies the logical Z observable.
But the circuit would *not* declare an X observable, because the X observable is not deterministic in a Z basis memory
experiment; it has no corresponding deterministic measurement set.

Logical observables are ignored when sampling measurements, but can produce results (if requested) when sampling
detection events. In detector sampling mode, each observable can produce a result bit (where 0 means "measurement set
had expected parity" and 1 means "measurement set had incorrect parity"). When converting a circuit into a detector
error model, errors are grouped based on the detectors they flip (the "symptoms" of the error) and the observables they
flip (the "frame changes" of the error).

Another potential point of confusion is that when sampling logical measurement results, as part of sampling detection
events in the circuit, the reported results are not measurements of the logical observable but rather whether those
measurement results *were flipped*. This has significant simulation speed benefits, and also makes it so that it is not
necessary to say whether the logical measurement result is supposed to be False or True. Note that, for example, this
means that even though `X` and `X_ERROR(1)` have equivalent effects on the measurements making up an observable, they
have differing effects on the reported value of an observable when sampling detection events (because `X` is intended,
determining the expected value, and `X_ERROR` is noise, causing deviations from the expected value).

It is not recommended for the measurement set of an observable to have inconsistent parity. For example, the
circuit-to-detector-error-model conversion will refuse to operate on circuits containing such observables.

In addition to targeting measurements, observables can target Pauli operators. This has no effect when running the
quantum computation, but is used when configuring the decoder. For example, when performing a logical Z initialization,
it allows a logical X operator to be introduced (by marking its Pauli terms) despite the fact that it anticommutes
with the initialization. In practice, when physically sampling a circuit or simulating sampling its measurements and
then computing the observables from the measurements, these Pauli terms are effectively ignored. However, they affect
detection event simulations and affect whether the observable is included in errors in the detector error model. This
makes it easier to benchmark all observables of a code, without having to introduce noiseless qubits entangled with the
logical qubit to avoid the testing of the X observable anticommuting with the testing of the Z observable.

Parens Arguments:

    A non-negative integer specifying the index of the logical observable to add the measurement records to.

Targets:

    The measurement records to add to the specified observable.

Example:

    R 0 1
    H 0
    CNOT 0 1
    M 0 1
    # Observable 0 is the parity of the previous two measurements.
    OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]

    R 0 1
    H 0
    CNOT 0 1
    M 0 1
    # Observable 1 is the parity of the previous measurement...
    OBSERVABLE_INCLUDE(1) rec[-1]
    # ...and the one before that.
    OBSERVABLE_INCLUDE(1) rec[-2]

    # Unphysically tracking two anticommuting observables of a 2x2 surface code.
    QUBIT_COORDS(0, 0) 0
    QUBIT_COORDS(1, 0) 1
    QUBIT_COORDS(0, 1) 2
    QUBIT_COORDS(1, 1) 3
    OBSERVABLE_INCLUDE(0) X0 X1
    OBSERVABLE_INCLUDE(1) Z0 Z2
    MPP X0*X1*X2*X3 Z0*Z1 Z2*Z3
    DEPOLARIZE1(0.001) 0 1 2 3
    MPP X0*X1*X2*X3 Z0*Z1 Z2*Z3
    DETECTOR rec[-1] rec[-4]
    DETECTOR rec[-2] rec[-5]
    DETECTOR rec[-3] rec[-6]
    OBSERVABLE_INCLUDE(0) X0 X1
    OBSERVABLE_INCLUDE(1) Z0 Z2

<a name="QUBIT_COORDS"></a>
### The 'QUBIT_COORDS' Instruction

Annotates the location of a qubit.

Coordinates are not required and have no effect on simulations, but can be useful to tools consuming the circuit. For
example, a tool drawing the circuit  can use the coordinates as hints for where to place the qubits in the drawing.
`stimcirq` uses `QUBIT_COORDS` instructions to preserve `cirq.LineQubit` and `cirq.GridQubit` coordinates when
converting between stim circuits and cirq circuits

A qubit's coordinates can be specified multiple times, with the intended interpretation being that the qubit is at the
location of the most recent assignment. For example, this could be used to indicate a simulated qubit is iteratively
playing the role of many physical qubits.

Parens Arguments:

    Optional.
    The latest coordinates of the qubit, relative to accumulated offsets from `SHIFT_COORDS` instructions.
    Can be any number of coordinates from 1 to 16.
    There is no required convention for which coordinate is which.

Targets:

    The qubit or qubits the coordinates apply to.

Example:

    # Annotate that qubits 0 to 3 are at the corners of a square.
    QUBIT_COORDS(0, 0) 0
    QUBIT_COORDS(0, 1) 1
    QUBIT_COORDS(1, 0) 2
    QUBIT_COORDS(1, 1) 3

<a name="SHIFT_COORDS"></a>
### The 'SHIFT_COORDS' Instruction

Accumulates offsets that affect qubit coordinates and detector coordinates.

Note: when qubit/detector coordinates use fewer dimensions than SHIFT_COORDS, the offsets from the additional dimensions
are ignored (i.e. not specifying a dimension is different from specifying it to be 0).

See also: `QUBIT_COORDS`, `DETECTOR`.

Parens Arguments:

    Offsets to add into the current coordinate offset.
    Can be any number of coordinate offsets from 1 to 16.
    There is no required convention for which coordinate is which.

Targets:

    This instruction takes no targets.

Example:

    SHIFT_COORDS(500.5)
    QUBIT_COORDS(1510) 0  # Actually at 2010.5
    SHIFT_COORDS(1500)
    QUBIT_COORDS(11) 1    # Actually at 2011.5
    QUBIT_COORDS(10.5) 2  # Actually at 2011.0

    # Declare some detectors with coordinates along a diagonal line.
    REPEAT 1000 {
        CNOT 0 2
        CNOT 1 2
        MR 2
        DETECTOR(10.5, 0) rec[-1] rec[-2]  # Actually at (2011.0, iteration_count).
        SHIFT_COORDS(0, 1)  # Advance 2nd coordinate to track loop iterations.
    }

<a name="TICK"></a>
### The 'TICK' Instruction

Annotates the end of a layer of gates, or that time is advancing.

This instruction is not necessary, it has no effect on simulations, but it can be used by tools that are transforming or
visualizing the circuit. For example, a tool that adds noise to a circuit may include cross-talk terms that require
knowing whether or not operations are happening in the same time step or not.

TICK instructions are added, and checked for, by `stimcirq` in order to preserve the moment structure of cirq circuits
converted between stim circuits and cirq circuits.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    This instruction takes no targets.

Example:

    # First time step.
    H 0
    CZ 1 2
    TICK

    # Second time step.
    H 1
    TICK

    # Empty time step.
    TICK

