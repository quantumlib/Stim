# Gates supported by Stim

- Pauli Gates
    - [I](#I)
    - [X](#X)
    - [Y](#Y)
    - [Z](#Z)
- Single Qubit Clifford Gates
    - [C_XYZ](#C_XYZ)
    - [C_ZYX](#C_ZYX)
    - [H](#H)
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
    - [CY](#CY)
    - [CZ](#CZ)
    - [ISWAP](#ISWAP)
    - [ISWAP_DAG](#ISWAP_DAG)
    - [SQRT_XX](#SQRT_XX)
    - [SQRT_XX_DAG](#SQRT_XX_DAG)
    - [SQRT_YY](#SQRT_YY)
    - [SQRT_YY_DAG](#SQRT_YY_DAG)
    - [SQRT_ZZ](#SQRT_ZZ)
    - [SQRT_ZZ_DAG](#SQRT_ZZ_DAG)
    - [SWAP](#SWAP)
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
    - [PAULI_CHANNEL_1](#PAULI_CHANNEL_1)
    - [PAULI_CHANNEL_2](#PAULI_CHANNEL_2)
    - [X_ERROR](#X_ERROR)
    - [Y_ERROR](#Y_ERROR)
    - [Z_ERROR](#Z_ERROR)
- Collapsing Gates
    - [M](#M)
    - [MPP](#MPP)
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
- Control Flow
    - [REPEAT](#REPEAT)
- Annotations
    - [DETECTOR](#DETECTOR)
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

    X -> +X
    Z -> +Z
    
Bloch Rotation:

    Axis: 
    Angle: 0 degrees
    
Unitary Matrix:

    [+1  ,     ]
    [    , +1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `I 0`
    # (no operations)
    

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

    X -> +X
    Z -> -Z
    
Bloch Rotation:

    Axis: +X
    Angle: 180 degrees
    
Unitary Matrix:

    [    , +1  ]
    [+1  ,     ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `X 0`
    H 0
    S 0
    S 0
    H 0
    

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
    
Bloch Rotation:

    Axis: +Y
    Angle: 180 degrees
    
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
    Z -> +Z
    
Bloch Rotation:

    Axis: +Z
    Angle: 180 degrees
    
Unitary Matrix:

    [+1  ,     ]
    [    , -1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `Z 0`
    S 0
    S 0
    

## Single Qubit Clifford Gates

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

    X -> +Y
    Z -> +X
    
Bloch Rotation:

    Axis: +X+Y+Z
    Angle: 120 degrees
    
Unitary Matrix:

    [+1-i, -1-i]
    [+1-i, +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `C_XYZ 0`
    S 0
    S 0
    S 0
    H 0
    

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

    X -> +Z
    Z -> +Y
    
Bloch Rotation:

    Axis: +X+Y+Z
    Angle: -120 degrees
    
Unitary Matrix:

    [+1+i, +1+i]
    [-1+i, +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `C_ZYX 0`
    H 0
    S 0
    

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

    X -> +Z
    Z -> +X
    
Bloch Rotation:

    Axis: +X+Z
    Angle: 180 degrees
    
Unitary Matrix:

    [+1  , +1  ]
    [+1  , -1  ] / sqrt(2)
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `H 0`
    H 0
    
    # (The decomposition is trivial because this gate is in the target gate set.)
    

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

    X -> +Y
    Z -> -Z
    
Bloch Rotation:

    Axis: +X+Y
    Angle: 180 degrees
    
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
    Z -> +Y
    
Bloch Rotation:

    Axis: +Y+Z
    Angle: 180 degrees
    
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

    X -> +Y
    Z -> +Z
    
Bloch Rotation:

    Axis: +Z
    Angle: 90 degrees
    
Unitary Matrix:

    [+1  ,     ]
    [    ,   +i]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `S 0`
    S 0
    
    # (The decomposition is trivial because this gate is in the target gate set.)
    

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

    X -> +X
    Z -> -Y
    
Bloch Rotation:

    Axis: +X
    Angle: 90 degrees
    
Unitary Matrix:

    [+1+i, +1-i]
    [+1-i, +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_X 0`
    H 0
    S 0
    H 0
    

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

    X -> +X
    Z -> +Y
    
Bloch Rotation:

    Axis: +X
    Angle: -90 degrees
    
Unitary Matrix:

    [+1-i, +1+i]
    [+1+i, +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_X_DAG 0`
    S 0
    H 0
    S 0
    

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
    Z -> +X
    
Bloch Rotation:

    Axis: +Y
    Angle: 90 degrees
    
Unitary Matrix:

    [+1+i, -1-i]
    [+1+i, +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_Y 0`
    S 0
    S 0
    H 0
    

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

    X -> +Z
    Z -> -X
    
Bloch Rotation:

    Axis: +Y
    Angle: -90 degrees
    
Unitary Matrix:

    [+1-i, +1-i]
    [-1+i, +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_Y_DAG 0`
    H 0
    S 0
    S 0
    

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
    Z -> +Z
    
Bloch Rotation:

    Axis: +Z
    Angle: -90 degrees
    
Unitary Matrix:

    [+1  ,     ]
    [    ,   -i]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `S_DAG 0`
    S 0
    S 0
    S 0
    

## Two Qubit Clifford Gates

<a name="CX"></a>
### The 'CX' Gate

Alternate name: <a name="ZCX"></a>`ZCX`

Alternate name: <a name="CNOT"></a>`CNOT`

The Z-controlled X gate.
First qubit is the control, second qubit is the target.
The first qubit can be replaced by a measurement record.

Applies an X gate to the target if the control is in the |1> state.

Negates the amplitude of the |1>|-> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    CX 5 6
    CX 42 43
    CX 5 6 42 43
    CX rec[-1] 111
    
Stabilizer Generators:

    X_ -> +XX
    Z_ -> +Z_
    _X -> +_X
    _Z -> +ZZ
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,     ,     , +1  ]
    [    ,     , +1  ,     ]
    [    , +1  ,     ,     ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `CX 0 1`
    CNOT 0 1
    
    # (The decomposition is trivial because this gate is in the target gate set.)
    

<a name="CY"></a>
### The 'CY' Gate

Alternate name: <a name="ZCY"></a>`ZCY`

The Z-controlled Y gate.
First qubit is the control, second qubit is the target.
The first qubit can be replaced by a measurement record.

Applies a Y gate to the target if the control is in the |1> state.

Negates the amplitude of the |1>|-i> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    CY 5 6
    CY 42 43
    CY 5 6 42 43
    CY rec[-1] 111
    
Stabilizer Generators:

    X_ -> +XY
    Z_ -> +Z_
    _X -> +ZX
    _Z -> +ZZ
    
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
    

<a name="CZ"></a>
### The 'CZ' Gate

Alternate name: <a name="ZCZ"></a>`ZCZ`

The Z-controlled Z gate.
First qubit is the control, second qubit is the target.
Either qubit can be replaced by a measurement record.

Applies a Z gate to the target if the control is in the |1> state.

Negates the amplitude of the |1>|1> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    CZ 5 6
    CZ 42 43
    CZ 5 6 42 43
    CZ rec[-1] 111
    CZ 111 rec[-1]
    
Stabilizer Generators:

    X_ -> +XZ
    Z_ -> +Z_
    _X -> +ZX
    _Z -> +_Z
    
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

    X_ -> +ZY
    Z_ -> +_Z
    _X -> +YZ
    _Z -> +Z_
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,     ,   +i,     ]
    [    ,   +i,     ,     ]
    [    ,     ,     , +1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `ISWAP 0 1`
    CNOT 0 1
    S 1
    CNOT 1 0
    CNOT 0 1
    

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
    Z_ -> +_Z
    _X -> -YZ
    _Z -> +Z_
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,     ,   -i,     ]
    [    ,   -i,     ,     ]
    [    ,     ,     , +1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `ISWAP_DAG 0 1`
    CNOT 0 1
    S 1
    S 1
    S 1
    CNOT 1 0
    CNOT 0 1
    

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

    X_ -> +X_
    Z_ -> -YX
    _X -> +_X
    _Z -> -XY
    
Unitary Matrix (little endian):

    [+1+i,     ,     , +1-i]
    [    , +1+i, +1-i,     ]
    [    , +1-i, +1+i,     ]
    [+1-i,     ,     , +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_XX 0 1`
    CNOT 0 1
    H 0
    S 0
    H 0
    CNOT 0 1
    

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

    X_ -> +X_
    Z_ -> +YX
    _X -> +_X
    _Z -> +XY
    
Unitary Matrix (little endian):

    [+1-i,     ,     , +1+i]
    [    , +1-i, +1+i,     ]
    [    , +1+i, +1-i,     ]
    [+1+i,     ,     , +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_XX_DAG 0 1`
    S 0
    CNOT 0 1
    H 0
    S 0
    CNOT 0 1
    

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
    Z_ -> +XY
    _X -> -YZ
    _Z -> +YX
    
Unitary Matrix (little endian):

    [+1+i,     ,     , -1+i]
    [    , +1+i, +1-i,     ]
    [    , +1-i, +1+i,     ]
    [-1+i,     ,     , +1+i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_YY 0 1`
    S 0
    CNOT 1 0
    S 0
    S 0
    H 1
    CNOT 1 0
    S 0
    

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

    X_ -> +ZY
    Z_ -> -XY
    _X -> +YZ
    _Z -> -YX
    
Unitary Matrix (little endian):

    [+1-i,     ,     , -1-i]
    [    , +1-i, +1+i,     ]
    [    , +1+i, +1-i,     ]
    [-1-i,     ,     , +1-i] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_YY_DAG 0 1`
    CNOT 0 1
    S 1
    H 0
    S 0
    H 0
    CNOT 1 0
    CNOT 0 1
    

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

    X_ -> +YZ
    Z_ -> +Z_
    _X -> +ZY
    _Z -> +_Z
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    ,   +i,     ,     ]
    [    ,     ,   +i,     ]
    [    ,     ,     , +1  ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `SQRT_ZZ 0 1`
    CNOT 0 1
    S 1
    CNOT 0 1
    

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
    Z_ -> +Z_
    _X -> -ZY
    _Z -> +_Z
    
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

    X_ -> +_X
    Z_ -> +_Z
    _X -> +X_
    _Z -> +Z_
    
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

    X_ -> +X_
    Z_ -> +ZX
    _X -> +_X
    _Z -> +XZ
    
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

    X_ -> +X_
    Z_ -> +ZY
    _X -> +XX
    _Z -> +XZ
    
Unitary Matrix (little endian):

    [+1  , +1  ,   -i,   +i]
    [+1  , +1  ,   +i,   -i]
    [  +i,   -i, +1  , +1  ]
    [  -i,   +i, +1  , +1  ] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `XCY 0 1`
    CNOT 1 0
    H 0
    S 0
    CNOT 0 1
    H 0
    

<a name="XCZ"></a>
### The 'XCZ' Gate

The X-controlled Z gate.
First qubit is the control, second qubit is the target.
The second qubit can be replaced by a measurement record.

Applies a Z gate to the target if the control is in the |-> state.

Negates the amplitude of the |->|1> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    XCZ 5 6
    XCZ 42 43
    XCZ 5 6 42 43
    XCZ 111 rec[-1]
    
Stabilizer Generators:

    X_ -> +X_
    Z_ -> +ZZ
    _X -> +XX
    _Z -> +_Z
    
Unitary Matrix (little endian):

    [+1  ,     ,     ,     ]
    [    , +1  ,     ,     ]
    [    ,     ,     , +1  ]
    [    ,     , +1  ,     ]
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `XCZ 0 1`
    CNOT 1 0
    

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

    X_ -> +XX
    Z_ -> +ZX
    _X -> +_X
    _Z -> +YZ
    
Unitary Matrix (little endian):

    [+1  ,   -i, +1  ,   +i]
    [  +i, +1  ,   -i, +1  ]
    [+1  ,   +i, +1  ,   -i]
    [  -i, +1  ,   +i, +1  ] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `YCX 0 1`
    CX 0 1
    H 1
    S 1
    CX 1 0
    H 1
    

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

    X_ -> +XY
    Z_ -> +ZY
    _X -> +YX
    _Z -> +YZ
    
Unitary Matrix (little endian):

    [+1  ,   -i,   -i, +1  ]
    [  +i, +1  , -1  ,   -i]
    [  +i, -1  , +1  ,   -i]
    [+1  ,   +i,   +i, +1  ] / 2
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `YCY 0 1`
    H 0
    S 0
    H 0
    CX 0 1
    H 0
    CX 1 0
    S 0
    

<a name="YCZ"></a>
### The 'YCZ' Gate

The Y-controlled Z gate.
First qubit is the control, second qubit is the target.
The second qubit can be replaced by a measurement record.

Applies a Z gate to the target if the control is in the |-i> state.

Negates the amplitude of the |-i>|1> state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    Qubit pairs to operate on.

Example:

    YCZ 5 6
    YCZ 42 43
    YCZ 5 6 42 43
    YCZ 111 rec[-1]
    
Stabilizer Generators:

    X_ -> +XZ
    Z_ -> +ZZ
    _X -> +YX
    _Z -> +_Z
    
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
    

## Noise Channels

<a name="DEPOLARIZE1"></a>
### The 'DEPOLARIZE1' Instruction

The single qubit depolarizing channel.

Applies a randomly chosen Pauli with a given probability.

Parens Arguments:

    A single float specifying the depolarization strength.

Targets:

    Qubits to apply single-qubit depolarizing noise to.

Pauli Mixture:

    1-p: I
    p/3: X
    p/3: Y
    p/3: Z

Example:

    DEPOLARIZE1(0.001) 5
    DEPOLARIZE1(0.001) 42
    DEPOLARIZE1(0.001) 5 42
    

<a name="DEPOLARIZE2"></a>
### The 'DEPOLARIZE2' Instruction

The two qubit depolarizing channel.

Applies a randomly chosen two-qubit Pauli product with a given probability.

Parens Arguments:

    A single float specifying the depolarization strength.

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

Example:

    DEPOLARIZE2(0.001) 5 6
    DEPOLARIZE2(0.001) 42 43
    DEPOLARIZE2(0.001) 5 6 42 43
    

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

- Example:

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

<a name="PAULI_CHANNEL_1"></a>
### The 'PAULI_CHANNEL_1' Instruction

A single qubit Pauli error channel with explicitly specified probabilities for each case.

Parens Arguments:

    Three floats specifying disjoint Pauli case probabilities.
    px: Probability of applying an X operation.
    py: Probability of applying a Y operation.
    pz: Probability of applying a Z operation.

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
If this gate is parameterized by a probability argument, the recorded result will be flipped with that probability. If not, the recorded result is noiseless. Note that the noise only affects the recorded result, not the target qubit's state.

Prefixing a target with ! inverts its recorded measurement result.

Example:

    M 5
    M !42
    M(0.001) 5 !42
    
Stabilizer Generators:

    Z -> m xor chance(p)
    Z -> +Z
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `M 0`
    M 0
    
    # (The decomposition is trivial because this gate is in the target gate set.)
    

<a name="MPP"></a>
### The 'MPP' Instruction

Measure Pauli products.

Parens Arguments:

    Optional.
    A single float specifying the probability of flipping each reported measurement result.

Targets:

    A series of Pauli products to measure.

    Each Pauli product is a series of Pauli targets (`[XYZ]#`) separated by combiners (`*`).
    Products can be negated by prefixing a Pauli target in the product with an inverter (`!`)

Examples:

    # Measure the two-body +X1*Y2 observable.
    MPP X1*Y2

    # Measure the one-body -Z5 observable.
    MPP !Z5

    # Measure the two-body +X1*Y2 observable and also the three-body -Z3*Z4*Z5 observable.
    MPP X1*Y2 !Z3*Z4*Z5

    # Noisily measure +Z1+Z2 and +X1*X2 (independently flip each reported result 0.1% of the time).
    MPP(0.001) Z1*Z2 X1*X2

If this gate is parameterized by a probability argument, the recorded result will be flipped with that probability. If not, the recorded result is noiseless. Note that the noise only affects the recorded result, not the target qubit's state.

Prefixing a target with ! inverts its recorded measurement result.

Example:

    MPP 5
    MPP !42
    MPP(0.001) 5 !42
    
Stabilizer Generators:

    P -> m xor chance(p)
    P -> P
    

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
If this gate is parameterized by a probability argument, the recorded result will be flipped with that probability. If not, the recorded result is noiseless. Note that the noise only affects the recorded result, not the target qubit's state.

Prefixing a target with ! inverts its recorded measurement result.

Example:

    MR 5
    MR !42
    MR(0.001) 5 !42
    
Stabilizer Generators:

    Z -> m xor chance(p)
    1 -> +Z
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MR 0`
    M 0
    R 0
    

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
If this gate is parameterized by a probability argument, the recorded result will be flipped with that probability. If not, the recorded result is noiseless. Note that the noise only affects the recorded result, not the target qubit's state.

Prefixing a target with ! inverts its recorded measurement result.

Example:

    MRX 5
    MRX !42
    MRX(0.001) 5 !42
    
Stabilizer Generators:

    X -> m xor chance(p)
    1 -> +X
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MRX 0`
    H 0
    M 0
    R 0
    H 0
    

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
If this gate is parameterized by a probability argument, the recorded result will be flipped with that probability. If not, the recorded result is noiseless. Note that the noise only affects the recorded result, not the target qubit's state.

Prefixing a target with ! inverts its recorded measurement result.

Example:

    MRY 5
    MRY !42
    MRY(0.001) 5 !42
    
Stabilizer Generators:

    Y -> m xor chance(p)
    1 -> +Y
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MRY 0`
    S 0
    S 0
    S 0
    H 0
    R 0
    M 0
    H 0
    S 0
    

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
If this gate is parameterized by a probability argument, the recorded result will be flipped with that probability. If not, the recorded result is noiseless. Note that the noise only affects the recorded result, not the target qubit's state.

Prefixing a target with ! inverts its recorded measurement result.

Example:

    MX 5
    MX !42
    MX(0.001) 5 !42
    
Stabilizer Generators:

    X -> +m xor chance(p)
    X -> +X
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MX 0`
    H 0
    M 0
    H 0
    

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
If this gate is parameterized by a probability argument, the recorded result will be flipped with that probability. If not, the recorded result is noiseless. Note that the noise only affects the recorded result, not the target qubit's state.

Prefixing a target with ! inverts its recorded measurement result.

Example:

    MY 5
    MY !42
    MY(0.001) 5 !42
    
Stabilizer Generators:

    Y -> m xor chance(p)
    Y -> +Y
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `MY 0`
    S 0
    S 0
    S 0
    H 0
    M 0
    H 0
    S 0
    

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

Example:

    R 5
    R 42
    R 5 42
    
Stabilizer Generators:

    1 -> +Z
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `R 0`
    R 0
    
    # (The decomposition is trivial because this gate is in the target gate set.)
    

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

    # Result multiple qubits into the |+> state.
    RX 2 3 5

Example:

    RX 5
    RX 42
    RX 5 42
    
Stabilizer Generators:

    1 -> +X
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `RX 0`
    H 0
    R 0
    H 0
    

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

    # Result multiple qubits into the |i> state.
    RY 2 3 5

Example:

    RY 5
    RY 42
    RY 5 42
    
Stabilizer Generators:

    1 -> +Y
    
Decomposition (into H, S, CX, M, R):

    # The following circuit is equivalent (up to global phase) to `RY 0`
    S 0
    S 0
    S 0
    H 0
    R 0
    H 0
    S 0
    

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

