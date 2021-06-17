# Gates supported by Stim

- [CNOT](#CNOT)
- [CORRELATED_ERROR](#CORRELATED_ERROR)
- [CX](#CX)
- [CY](#CY)
- [CZ](#CZ)
- [C_XYZ](#C_XYZ)
- [C_ZYX](#C_ZYX)
- [DEPOLARIZE1](#DEPOLARIZE1)
- [DEPOLARIZE2](#DEPOLARIZE2)
- [DETECTOR](#DETECTOR)
- [E](#E)
- [ELSE_CORRELATED_ERROR](#ELSE_CORRELATED_ERROR)
- [H](#H)
- [H_XY](#H_XY)
- [H_XZ](#H_XZ)
- [H_YZ](#H_YZ)
- [I](#I)
- [ISWAP](#ISWAP)
- [ISWAP_DAG](#ISWAP_DAG)
- [M](#M)
- [MR](#MR)
- [MRX](#MRX)
- [MRY](#MRY)
- [MRZ](#MRZ)
- [MX](#MX)
- [MY](#MY)
- [MZ](#MZ)
- [OBSERVABLE_INCLUDE](#OBSERVABLE_INCLUDE)
- [PAULI_CHANNEL_1](#PAULI_CHANNEL_1)
- [PAULI_CHANNEL_2](#PAULI_CHANNEL_2)
- [QUBIT_COORDS](#QUBIT_COORDS)
- [R](#R)
- [REPEAT](#REPEAT)
- [RX](#RX)
- [RY](#RY)
- [RZ](#RZ)
- [S](#S)
- [SHIFT_COORDS](#SHIFT_COORDS)
- [SQRT_X](#SQRT_X)
- [SQRT_XX](#SQRT_XX)
- [SQRT_XX_DAG](#SQRT_XX_DAG)
- [SQRT_X_DAG](#SQRT_X_DAG)
- [SQRT_Y](#SQRT_Y)
- [SQRT_YY](#SQRT_YY)
- [SQRT_YY_DAG](#SQRT_YY_DAG)
- [SQRT_Y_DAG](#SQRT_Y_DAG)
- [SQRT_Z](#SQRT_Z)
- [SQRT_ZZ](#SQRT_ZZ)
- [SQRT_ZZ_DAG](#SQRT_ZZ_DAG)
- [SQRT_Z_DAG](#SQRT_Z_DAG)
- [SWAP](#SWAP)
- [S_DAG](#S_DAG)
- [TICK](#TICK)
- [X](#X)
- [XCX](#XCX)
- [XCY](#XCY)
- [XCZ](#XCZ)
- [X_ERROR](#X_ERROR)
- [Y](#Y)
- [YCX](#YCX)
- [YCY](#YCY)
- [YCZ](#YCZ)
- [Y_ERROR](#Y_ERROR)
- [Z](#Z)
- [ZCX](#ZCX)
- [ZCY](#ZCY)
- [ZCZ](#ZCZ)
- [Z_ERROR](#Z_ERROR)

## Pauli Gates

- <a name="I"></a>**`I`**
    
    Identity gate.
    Does nothing to the target qubits.
    
    - Example:
    
        ```
        I 5
        I 42
        I 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +X
        Z -> +Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: 
        Angle: 0 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ]
        [    , +1  ]
        ```
        
    
- <a name="X"></a>**`X`**
    
    Pauli X gate.
    The bit flip gate.
    
    - Example:
    
        ```
        X 5
        X 42
        X 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +X
        Z -> -Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [    , +1  ]
        [+1  ,     ]
        ```
        
    
- <a name="Y"></a>**`Y`**
    
    Pauli Y gate.
    
    - Example:
    
        ```
        Y 5
        Y 42
        Y 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> -X
        Z -> -Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Y
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [    ,   -i]
        [  +i,     ]
        ```
        
    
- <a name="Z"></a>**`Z`**
    
    Pauli Z gate.
    The phase flip gate.
    
    - Example:
    
        ```
        Z 5
        Z 42
        Z 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> -X
        Z -> +Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Z
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ]
        [    , -1  ]
        ```
        
    
## Single Qubit Clifford Gates

- <a name="C_XYZ"></a>**`C_XYZ`**
    
    Right handed period 3 axis cycling gate, sending X -> Y -> Z -> X.
    
    - Example:
    
        ```
        C_XYZ 5
        C_XYZ 42
        C_XYZ 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Y
        Z -> +X
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X+Y+Z
        Angle: 120 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1-i, -1-i]
        [+1-i, +1+i] / 2
        ```
        
    
- <a name="C_ZYX"></a>**`C_ZYX`**
    
    Left handed period 3 axis cycling gate, sending Z -> Y -> X -> Z.
    
    - Example:
    
        ```
        C_ZYX 5
        C_ZYX 42
        C_ZYX 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Z
        Z -> +Y
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X+Y+Z
        Angle: -120 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1+i, +1+i]
        [-1+i, +1-i] / 2
        ```
        
    
- <a name="H"></a>**`H`**
    
    Alternate name: <a name="H_XZ"></a>`H_XZ`
    
    The Hadamard gate.
    Swaps the X and Z axes.
    
    - Example:
    
        ```
        H 5
        H 42
        H 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Z
        Z -> +X
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X+Z
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  , +1  ]
        [+1  , -1  ] / sqrt(2)
        ```
        
    
- <a name="H_XY"></a>**`H_XY`**
    
    A variant of the Hadamard gate that swaps the X and Y axes (instead of X and Z).
    
    - Example:
    
        ```
        H_XY 5
        H_XY 42
        H_XY 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Y
        Z -> -Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X+Y
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [    , +1-i]
        [+1+i,     ] / sqrt(2)
        ```
        
    
- <a name="H_YZ"></a>**`H_YZ`**
    
    A variant of the Hadamard gate that swaps the Y and Z axes (instead of X and Z).
    
    - Example:
    
        ```
        H_YZ 5
        H_YZ 42
        H_YZ 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> -X
        Z -> +Y
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Y+Z
        Angle: 180 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,   -i]
        [  +i, -1  ] / sqrt(2)
        ```
        
    
- <a name="S"></a>**`S`**
    
    Alternate name: <a name="SQRT_Z"></a>`SQRT_Z`
    
    Principle square root of Z gate.
    Phases the amplitude of |1> by i.
    
    - Example:
    
        ```
        S 5
        S 42
        S 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Y
        Z -> +Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Z
        Angle: 90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ]
        [    ,   +i]
        ```
        
    
- <a name="SQRT_X"></a>**`SQRT_X`**
    
    Principle square root of X gate.
    Phases the amplitude of |-> by i.
    Equivalent to `H` then `S` then `H`.
    
    - Example:
    
        ```
        SQRT_X 5
        SQRT_X 42
        SQRT_X 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +X
        Z -> -Y
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X
        Angle: 90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1+i, +1-i]
        [+1-i, +1+i] / 2
        ```
        
    
- <a name="SQRT_X_DAG"></a>**`SQRT_X_DAG`**
    
    Adjoint square root of X gate.
    Phases the amplitude of |-> by -i.
    Equivalent to `H` then `S_DAG` then `H`.
    
    - Example:
    
        ```
        SQRT_X_DAG 5
        SQRT_X_DAG 42
        SQRT_X_DAG 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +X
        Z -> +Y
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +X
        Angle: -90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1-i, +1+i]
        [+1+i, +1-i] / 2
        ```
        
    
- <a name="SQRT_Y"></a>**`SQRT_Y`**
    
    Principle square root of Y gate.
    Phases the amplitude of |-i> by i.
    Equivalent to `S` then `H` then `S` then `H` then `S_DAG`.
    
    - Example:
    
        ```
        SQRT_Y 5
        SQRT_Y 42
        SQRT_Y 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> -Z
        Z -> +X
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Y
        Angle: 90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1+i, -1-i]
        [+1+i, +1+i] / 2
        ```
        
    
- <a name="SQRT_Y_DAG"></a>**`SQRT_Y_DAG`**
    
    Principle square root of Y gate.
    Phases the amplitude of |-i> by -i.
    Equivalent to `S` then `H` then `S_DAG` then `H` then `S_DAG`.
    
    - Example:
    
        ```
        SQRT_Y_DAG 5
        SQRT_Y_DAG 42
        SQRT_Y_DAG 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> +Z
        Z -> -X
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Y
        Angle: -90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1-i, +1-i]
        [-1+i, +1-i] / 2
        ```
        
    
- <a name="S_DAG"></a>**`S_DAG`**
    
    Alternate name: <a name="SQRT_Z_DAG"></a>`SQRT_Z_DAG`
    
    Principle square root of Z gate.
    Phases the amplitude of |1> by -i.
    
    - Example:
    
        ```
        S_DAG 5
        S_DAG 42
        S_DAG 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> -Y
        Z -> +Z
        ```
        
    - Bloch Rotation:
    
        ```
        Axis: +Z
        Angle: -90 degrees
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ]
        [    ,   -i]
        ```
        
    
## Two Qubit Clifford Gates

- <a name="CX"></a>**`CX`**
    
    Alternate name: <a name="ZCX"></a>`ZCX`
    
    Alternate name: <a name="CNOT"></a>`CNOT`
    
    The Z-controlled X gate.
    First qubit is the control, second qubit is the target.
    The first qubit can be replaced by a measurement record.
    
    Applies an X gate to the target if the control is in the |1> state.
    
    Negates the amplitude of the |1>|-> state.
    
    - Example:
    
        ```
        CX 5 6
        CX 42 43
        CX 5 6 42 43
        CX rec[-1] 111
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XX
        Z_ -> +Z_
        _X -> +_X
        _Z -> +ZZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,     ,     , +1  ]
        [    ,     , +1  ,     ]
        [    , +1  ,     ,     ]
        ```
        
    
- <a name="CY"></a>**`CY`**
    
    Alternate name: <a name="ZCY"></a>`ZCY`
    
    The Z-controlled Y gate.
    First qubit is the control, second qubit is the target.
    The first qubit can be replaced by a measurement record.
    
    Applies a Y gate to the target if the control is in the |1> state.
    
    Negates the amplitude of the |1>|-i> state.
    
    - Example:
    
        ```
        CY 5 6
        CY 42 43
        CY 5 6 42 43
        CY rec[-1] 111
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XY
        Z_ -> +Z_
        _X -> +ZX
        _Z -> +ZZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,     ,     ,   -i]
        [    ,     , +1  ,     ]
        [    ,   +i,     ,     ]
        ```
        
    
- <a name="CZ"></a>**`CZ`**
    
    Alternate name: <a name="ZCZ"></a>`ZCZ`
    
    The Z-controlled Z gate.
    First qubit is the control, second qubit is the target.
    Either qubit can be replaced by a measurement record.
    
    Applies a Z gate to the target if the control is in the |1> state.
    
    Negates the amplitude of the |1>|1> state.
    
    - Example:
    
        ```
        CZ 5 6
        CZ 42 43
        CZ 5 6 42 43
        CZ rec[-1] 111
        CZ 111 rec[-1]
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XZ
        Z_ -> +Z_
        _X -> +ZX
        _Z -> +_Z
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    , +1  ,     ,     ]
        [    ,     , +1  ,     ]
        [    ,     ,     , -1  ]
        ```
        
    
- <a name="ISWAP"></a>**`ISWAP`**
    
    Swaps two qubits and phases the -1 eigenspace of the ZZ observable by i.
    Equivalent to `SWAP` then `CZ` then `S` on both targets.
    
    - Example:
    
        ```
        ISWAP 5 6
        ISWAP 42 43
        ISWAP 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +ZY
        Z_ -> +_Z
        _X -> +YZ
        _Z -> +Z_
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,     ,   +i,     ]
        [    ,   +i,     ,     ]
        [    ,     ,     , +1  ]
        ```
        
    
- <a name="ISWAP_DAG"></a>**`ISWAP_DAG`**
    
    Swaps two qubits and phases the -1 eigenspace of the ZZ observable by -i.
    Equivalent to `SWAP` then `CZ` then `S_DAG` on both targets.
    
    - Example:
    
        ```
        ISWAP_DAG 5 6
        ISWAP_DAG 42 43
        ISWAP_DAG 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> -ZY
        Z_ -> +_Z
        _X -> -YZ
        _Z -> +Z_
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,     ,   -i,     ]
        [    ,   -i,     ,     ]
        [    ,     ,     , +1  ]
        ```
        
    
- <a name="SQRT_XX"></a>**`SQRT_XX`**
    
    Phases the -1 eigenspace of the XX observable by i.
    
    - Example:
    
        ```
        SQRT_XX 5 6
        SQRT_XX 42 43
        SQRT_XX 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +X_
        Z_ -> -YX
        _X -> +_X
        _Z -> -XY
        ```
        
    - Unitary Matrix:
    
        ```
        [+1+i,     ,     , +1-i]
        [    , +1+i, +1-i,     ]
        [    , +1-i, +1+i,     ]
        [+1-i,     ,     , +1+i] / 2
        ```
        
    
- <a name="SQRT_XX_DAG"></a>**`SQRT_XX_DAG`**
    
    Phases the -1 eigenspace of the XX observable by -i.
    
    - Example:
    
        ```
        SQRT_XX_DAG 5 6
        SQRT_XX_DAG 42 43
        SQRT_XX_DAG 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +X_
        Z_ -> +YX
        _X -> +_X
        _Z -> +XY
        ```
        
    - Unitary Matrix:
    
        ```
        [+1-i,     ,     , +1+i]
        [    , +1-i, +1+i,     ]
        [    , +1+i, +1-i,     ]
        [+1+i,     ,     , +1-i] / 2
        ```
        
    
- <a name="SQRT_YY"></a>**`SQRT_YY`**
    
    Phases the -1 eigenspace of the YY observable by i.
    
    - Example:
    
        ```
        SQRT_YY 5 6
        SQRT_YY 42 43
        SQRT_YY 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> -ZY
        Z_ -> +XY
        _X -> -YZ
        _Z -> +YX
        ```
        
    - Unitary Matrix:
    
        ```
        [+1+i,     ,     , -1+i]
        [    , +1+i, +1-i,     ]
        [    , +1-i, +1+i,     ]
        [-1+i,     ,     , +1+i] / 2
        ```
        
    
- <a name="SQRT_YY_DAG"></a>**`SQRT_YY_DAG`**
    
    Phases the -1 eigenspace of the YY observable by -i.
    
    - Example:
    
        ```
        SQRT_YY_DAG 5 6
        SQRT_YY_DAG 42 43
        SQRT_YY_DAG 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +ZY
        Z_ -> -XY
        _X -> +YZ
        _Z -> -YX
        ```
        
    - Unitary Matrix:
    
        ```
        [+1-i,     ,     , -1-i]
        [    , +1-i, +1+i,     ]
        [    , +1+i, +1-i,     ]
        [-1-i,     ,     , +1-i] / 2
        ```
        
    
- <a name="SQRT_ZZ"></a>**`SQRT_ZZ`**
    
    Phases the -1 eigenspace of the ZZ observable by i.
    
    - Example:
    
        ```
        SQRT_ZZ 5 6
        SQRT_ZZ 42 43
        SQRT_ZZ 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +YZ
        Z_ -> +Z_
        _X -> +ZY
        _Z -> +_Z
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,   +i,     ,     ]
        [    ,     ,   +i,     ]
        [    ,     ,     , +1  ]
        ```
        
    
- <a name="SQRT_ZZ_DAG"></a>**`SQRT_ZZ_DAG`**
    
    Phases the -1 eigenspace of the ZZ observable by -i.
    
    - Example:
    
        ```
        SQRT_ZZ_DAG 5 6
        SQRT_ZZ_DAG 42 43
        SQRT_ZZ_DAG 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> -YZ
        Z_ -> +Z_
        _X -> -ZY
        _Z -> +_Z
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,   -i,     ,     ]
        [    ,     ,   -i,     ]
        [    ,     ,     , +1  ]
        ```
        
    
- <a name="SWAP"></a>**`SWAP`**
    
    Swaps two qubits.
    
    - Example:
    
        ```
        SWAP 5 6
        SWAP 42 43
        SWAP 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +_X
        Z_ -> +_Z
        _X -> +X_
        _Z -> +Z_
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    ,     , +1  ,     ]
        [    , +1  ,     ,     ]
        [    ,     ,     , +1  ]
        ```
        
    
- <a name="XCX"></a>**`XCX`**
    
    The X-controlled X gate.
    First qubit is the control, second qubit is the target.
    
    Applies an X gate to the target if the control is in the |-> state.
    
    Negates the amplitude of the |->|-> state.
    
    - Example:
    
        ```
        XCX 5 6
        XCX 42 43
        XCX 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +X_
        Z_ -> +ZX
        _X -> +_X
        _Z -> +XZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  , +1  , +1  , -1  ]
        [+1  , +1  , -1  , +1  ]
        [+1  , -1  , +1  , +1  ]
        [-1  , +1  , +1  , +1  ] / 2
        ```
        
    
- <a name="XCY"></a>**`XCY`**
    
    The X-controlled Y gate.
    First qubit is the control, second qubit is the target.
    
    Applies a Y gate to the target if the control is in the |-> state.
    
    Negates the amplitude of the |->|-i> state.
    
    - Example:
    
        ```
        XCY 5 6
        XCY 42 43
        XCY 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +X_
        Z_ -> +ZY
        _X -> +XX
        _Z -> +XZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  , +1  ,   -i,   +i]
        [+1  , +1  ,   +i,   -i]
        [  +i,   -i, +1  , +1  ]
        [  -i,   +i, +1  , +1  ] / 2
        ```
        
    
- <a name="XCZ"></a>**`XCZ`**
    
    The X-controlled Z gate.
    First qubit is the control, second qubit is the target.
    The second qubit can be replaced by a measurement record.
    
    Applies a Z gate to the target if the control is in the |-> state.
    
    Negates the amplitude of the |->|1> state.
    
    - Example:
    
        ```
        XCZ 5 6
        XCZ 42 43
        XCZ 5 6 42 43
        XCZ 111 rec[-1]
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +X_
        Z_ -> +ZZ
        _X -> +XX
        _Z -> +_Z
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    , +1  ,     ,     ]
        [    ,     ,     , +1  ]
        [    ,     , +1  ,     ]
        ```
        
    
- <a name="YCX"></a>**`YCX`**
    
    The Y-controlled X gate.
    First qubit is the control, second qubit is the target.
    
    Applies an X gate to the target if the control is in the |-i> state.
    
    Negates the amplitude of the |-i>|-> state.
    
    - Example:
    
        ```
        YCX 5 6
        YCX 42 43
        YCX 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XX
        Z_ -> +ZX
        _X -> +_X
        _Z -> +YZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,   -i, +1  ,   +i]
        [  +i, +1  ,   -i, +1  ]
        [+1  ,   +i, +1  ,   -i]
        [  -i, +1  ,   +i, +1  ] / 2
        ```
        
    
- <a name="YCY"></a>**`YCY`**
    
    The Y-controlled Y gate.
    First qubit is the control, second qubit is the target.
    
    Applies a Y gate to the target if the control is in the |-i> state.
    
    Negates the amplitude of the |-i>|-i> state.
    
    - Example:
    
        ```
        YCY 5 6
        YCY 42 43
        YCY 5 6 42 43
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XY
        Z_ -> +ZY
        _X -> +YX
        _Z -> +YZ
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,   -i,   -i, +1  ]
        [  +i, +1  , -1  ,   -i]
        [  +i, -1  , +1  ,   -i]
        [+1  ,   +i,   +i, +1  ] / 2
        ```
        
    
- <a name="YCZ"></a>**`YCZ`**
    
    The Y-controlled Z gate.
    First qubit is the control, second qubit is the target.
    The second qubit can be replaced by a measurement record.
    
    Applies a Z gate to the target if the control is in the |-i> state.
    
    Negates the amplitude of the |-i>|1> state.
    
    - Example:
    
        ```
        YCZ 5 6
        YCZ 42 43
        YCZ 5 6 42 43
        YCZ 111 rec[-1]
        ```
        
    - Stabilizer Generators:
    
        ```
        X_ -> +XZ
        Z_ -> +ZZ
        _X -> +YX
        _Z -> +_Z
        ```
        
    - Unitary Matrix:
    
        ```
        [+1  ,     ,     ,     ]
        [    , +1  ,     ,     ]
        [    ,     ,     ,   -i]
        [    ,     ,   +i,     ]
        ```
        
    
## Noise Channels

- <a name="DEPOLARIZE1"></a>**`DEPOLARIZE1`**
    
    The single qubit depolarizing channel.
    
    Applies a randomly chosen Pauli with a given probability.
    
    - Pauli Mixture:
    
        ```
        1-p: I
        p/3: X
        p/3: Y
        p/3: Z
        ```
    
    - Example:
    
        ```
        DEPOLARIZE1(0.001) 5
        DEPOLARIZE1(0.001) 42
        DEPOLARIZE1(0.001) 5 42
        ```
        
    
- <a name="DEPOLARIZE2"></a>**`DEPOLARIZE2`**
    
    The two qubit depolarizing channel.
    
    Applies a randomly chosen two-qubit Pauli product with a given probability.
    
    - Pauli Mixture:
    
        ```
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
        ```
    
    - Example:
    
        ```
        DEPOLARIZE2(0.001) 5 6
        DEPOLARIZE2(0.001) 42 43
        DEPOLARIZE2(0.001) 5 6 42 43
        ```
        
    
- <a name="E"></a>**`E`**
    
    Alternate name: <a name="CORRELATED_ERROR"></a>`CORRELATED_ERROR`
    
    Probabilistically applies a Pauli product error with a given probability.
    Sets the "correlated error occurred flag" to true if the error occurred.
    Otherwise sets the flag to false.
    
    See also: `ELSE_CORRELATED_ERROR`.
    
    - Example:
    
        ```
        # With 40% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
        CORRELATED_ERROR(0.2) X1 Y2
        ELSE_CORRELATED_ERROR(0.25) Z2 Z3
        ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3
        ```
    
- <a name="ELSE_CORRELATED_ERROR"></a>**`ELSE_CORRELATED_ERROR`**
    
    Probabilistically applies a Pauli product error with a given probability, unless the "correlated error occurred flag" is set.
    If the error occurs, sets the "correlated error occurred flag" to true.
    Otherwise leaves the flag alone.
    
    See also: `CORRELATED_ERROR`.
    
    - Example:
    
        ```
        # With 40% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
        CORRELATED_ERROR(0.2) X1 Y2
        ELSE_CORRELATED_ERROR(0.25) Z2 Z3
        ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3
        ```
    
- <a name="PAULI_CHANNEL_1"></a>**`PAULI_CHANNEL_1`**
    
    A single qubit Pauli error channel with explicitly specified probabilities for each case.
    
    The gate is parameterized by 3 disjoint probabilities, one for each Pauli other than I, ordered as follows:
    
        X, Y, Z
    
    - Example:
    
        ```
        # Sample errors from the distribution 10% X, 15% Y, 20% Z, 55% I.
        # Apply independently to qubits 1, 2, 4.
        PAULI_CHANNEL_1(0.1, 0.15, 0.2) 1 2 4
        ```
    
    - Pauli Mixture:
    
        ```
        1-px-py-pz: I
        px: X
        py: Y
        pz: Z
        ```
    
- <a name="PAULI_CHANNEL_2"></a>**`PAULI_CHANNEL_2`**
    
    A two qubit Pauli error channel with explicitly specified probabilities for each case.
    
    The gate is parameterized by 15 disjoint probabilities, one for each Pauli pair other than II, ordered as follows:
    
        IX, IY, IZ, XI, XX, XY, XZ, YI, YX, YY, XZ, ZI, ZX, ZY, ZZ
    
    - Example:
    
        ```
        # Sample errors from the distribution 10% XX, 20% YZ, 70% II.
        # Apply independently to qubit pairs (1,2) (5,6) (8,3)
        PAULI_CHANNEL_2(0,0,0, 0.1,0,0,0, 0,0,0,0.2, 0,0,0,0) 1 2 5 6 8 3
        ```
    
    - Pauli Mixture:
    
        ```
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
        pyz: XZ
        pzi: ZI
        pzx: ZX
        pzy: ZY
        pzz: ZZ
        ```
    
- <a name="X_ERROR"></a>**`X_ERROR`**
    
    Applies a Pauli X with a given probability.
    
    - Pauli Mixture:
    
        ```
        1-p: I
         p : X
        ```
    
    - Example:
    
        ```
        X_ERROR(0.001) 5
        X_ERROR(0.001) 42
        X_ERROR(0.001) 5 42
        ```
        
    
- <a name="Y_ERROR"></a>**`Y_ERROR`**
    
    Applies a Pauli Y with a given probability.
    
    - Pauli Mixture:
    
        ```
        1-p: I
         p : Y
        ```
    
    - Example:
    
        ```
        Y_ERROR(0.001) 5
        Y_ERROR(0.001) 42
        Y_ERROR(0.001) 5 42
        ```
        
    
- <a name="Z_ERROR"></a>**`Z_ERROR`**
    
    Applies a Pauli Z with a given probability.
    
    - Pauli Mixture:
    
        ```
        1-p: I
         p : Z
        ```
    
    - Example:
    
        ```
        Z_ERROR(0.001) 5
        Z_ERROR(0.001) 42
        Z_ERROR(0.001) 5 42
        ```
        
    
## Collapsing Gates

- <a name="M"></a>**`M`**
    
    Alternate name: <a name="MZ"></a>`MZ`
    
    Z-basis measurement.
    Projects each target qubit into `|0>` or `|1>` and reports its value (false=`|0>`, true=`|1>`).
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        M 5
        M !42
        M 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        Z -> m
        Z -> +Z
        ```
        
    
- <a name="MR"></a>**`MR`**
    
    Alternate name: <a name="MRZ"></a>`MRZ`
    
    Z-basis demolition measurement.
    Projects each target qubit into `|0>` or `|1>`, reports its value (false=`|0>`, true=`|1>`), then resets to `|0>`.
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        MR 5
        MR !42
        MR 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        Z -> m
        1 -> +Z
        ```
        
    
- <a name="MRX"></a>**`MRX`**
    
    X-basis demolition measurement.
    Projects each target qubit into `|+>` or `|->`, reports its value (false=`|+>`, true=`|->`), then resets to `|+>`.
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        MRX 5
        MRX !42
        MRX 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> m
        1 -> +X
        ```
        
    
- <a name="MRY"></a>**`MRY`**
    
    Y-basis demolition measurement.
    Projects each target qubit into `|i>` or `|-i>`, reports its value (false=`|i>`, true=`|-i>`), then resets to `|i>`.
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        MRY 5
        MRY !42
        MRY 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        Y -> m
        1 -> +Y
        ```
        
    
- <a name="MX"></a>**`MX`**
    
    X-basis measurement.
    Projects each target qubit into `|+>` or `|->` and reports its value (false=`|+>`, true=`|->`).
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        MX 5
        MX !42
        MX 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        X -> m
        X -> +X
        ```
        
    
- <a name="MY"></a>**`MY`**
    
    Y-basis measurement.
    Projects each target qubit into `|i>` or `|-i>` and reports its value (false=`|i>`, true=`|-i>`).
    Prefixing a target with ! inverts its recorded measurement result.
    
    - Example:
    
        ```
        MY 5
        MY !42
        MY 5 !42
        ```
        
    - Stabilizer Generators:
    
        ```
        Y -> m
        Y -> +Y
        ```
        
    
- <a name="R"></a>**`R`**
    
    Alternate name: <a name="RZ"></a>`RZ`
    
    Z-basis reset.
    Forces each target qubit into the `|0>` state by silently measuring it in the Z basis and applying an `X` gate if it ended up in the `|1>` state.
    
    - Example:
    
        ```
        R 5
        R 42
        R 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        1 -> +Z
        ```
        
    
- <a name="RX"></a>**`RX`**
    
    X-basis reset.
    Forces each target qubit into the `|+>` state by silently measuring it in the X basis and applying a `Z` gate if it ended up in the `|->` state.
    
    - Example:
    
        ```
        RX 5
        RX 42
        RX 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        1 -> +X
        ```
        
    
- <a name="RY"></a>**`RY`**
    
    Y-basis reset.
    Forces each target qubit into the `|i>` state by silently measuring it in the Y basis and applying an `X` gate if it ended up in the `|-i>` state.
    
    - Example:
    
        ```
        RY 5
        RY 42
        RY 5 42
        ```
        
    - Stabilizer Generators:
    
        ```
        1 -> +Y
        ```
        
    
## Control Flow

- <a name="REPEAT"></a>**`REPEAT`**
    
    Repeats the instructions in its body N times.
    N can be any positive integer from 1 to a quintillion (10^18).
    
    Currently, repetition counts of 0 are not allowed because they create corner cases with ambiguous resolutions.
    For example, if a logical observable is only given measurements inside a repeat block with a repetition count of 0, it's
    ambiguous whether the output of sampling the logical observables includes a bit for that logical observable.
    
    - Example:
    
        ```
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
        ```
    
## Annotations

- <a name="DETECTOR"></a>**`DETECTOR`**
    
    Annotates that a set of measurements have a deterministic result, which can be used to detect errors.
    
    Detectors are ignored in measurement sampling mode.
    In detector sampling mode, detectors produce results (false=expected parity, true=incorrect parity detected).
    
    Detectors can optionally be parameterized by coordinates (e.g. `DETECTOR(1,2)` has coordinates 1,2).
    These coordinates aren't really used for anything in stim, but can act as drawing hints for other tools.
    Note that the coordinates are relative to accumulated coordinate shifts from SHIFT_COORDS instructions.
    Also note that putting two detectors at the same coordinate does not fuse them into one detector
    (beware that OBSERVABLE_INCLUDE does fuse observables at the same index, which looks very similar).
    See SHIFT_COORDS for more details on using coordinates.
    
    Note that detectors are always defined with respect to *noiseless behavior*. For example, placing an `X` gate before a
    measurement cannot create detection events on detectors that include that measurement, but placing an `X_ERROR(1)`
    does create detection events.
    
    - Example:
    
        ```
        H 0
        CNOT 0 1
        M 0 1
        DETECTOR rec[-1] rec[-2]
        ```
    
- <a name="OBSERVABLE_INCLUDE"></a>**`OBSERVABLE_INCLUDE`**
    
    Adds measurement results to a given logical observable index.
    
    A logical observable's measurement result is the parity of all physical measurement results added to it.
    
    A logical observable is similar to a Detector, except the measurements making up an observable can be built up
    incrementally over the entire circuit.
    
    Logical observables are ignored in measurement sampling mode.
    In detector sampling mode, observables produce results (false=expected parity, true=incorrect parity detected).
    These results are optionally appended to the detector results, depending on simulator arguments / command line flags.
    
    Note that observables are always defined with respect to *noiseless behavior*. For example, placing an `X` gate before a
    measurement cannot flip a logical observable that include that measurement, but placing an `X_ERROR(1)` does flip the
    observable. This is because observables are used for detecting errors, not for verifying noiseless functionality.
    
    Note that observable indices are NOT shifted by SHIFT_COORDS.
    
    - Example:
    
        ```
        H 0
        CNOT 0 1
        M 0 1
        OBSERVABLE_INCLUDE(5) rec[-1] rec[-2]
        ```
    
- <a name="QUBIT_COORDS"></a>**`QUBIT_COORDS`**
    
    An annotation used to indicate the intended location of a qubit.
    The coordinates are double precision floating point numbers, relative to accumulated offsets from SHIFT_COORDS.
    The coordinates are not in any particular order or number of dimensions.
    As far as stim is concerned the coordinates are a list of opaque and mysterious numbers.
    They have no effect on simulations of the circuit, but are potentially useful for tasks such as drawing the circuit.
    
    See also: SHIFT_COORDS.
    
    Note that a qubit's coordinates can be specified multiple times.
    The intended interpretation is that the qubit is at the location of the most recent assignment.
    For example, this could be used to indicate a simulated qubit is iteratively playing the role of many physical qubits.
    
    - Example:
    
        ```
        QUBIT_COORDS(100, 101) 0
        QUBIT_COORDS(100, 101) 1
        SQRT_XX 0 1
        MR 0 1
        QUBIT_COORDS(2.5, 3.5) 2  # Floating point coordinates are allowed.
        QUBIT_COORDS(2.5, 4.5) 1  # Hint that qubit 1 is now referring to a different physical location.
        SQRT_XX 1 2
        M 1 2
        ```
    
- <a name="SHIFT_COORDS"></a>**`SHIFT_COORDS`**
    
    Accumulates offsets to apply to qubit coordinates and detector coordinates.
    
    See also: QUBIT_COORDS, DETECTOR.
    
    Note: when qubit/detector coordinates use fewer dimensions than SHIFT_COORDS, the offsets from the additional dimensions
    are ignored (i.e. not specifying a dimension is different from specifying it to be 0).
    
    - Example:
    
        ```
        SHIFT_COORDS(500.5)
        QUBIT_COORDS(1510) 0  # Actually at 2010.5
        SHIFT_COORDS(1500)
        QUBIT_COORDS(11) 1    # Actually at 2011.5
        QUBIT_COORDS(10.5) 2  # Actually at 2011.0
        REPEAT 1000 {
            CNOT 0 2
            CNOT 1 2
            MR 2
            DETECTOR(10.5, 0) rec[-1] rec[-2]  # Actually at (2011.0, iteration_count).
            SHIFT_COORDS(0, 1)  # Advance 2nd coordinate to track loop iterations.
        }
        ```
    
- <a name="TICK"></a>**`TICK`**
    
    Indicates the end of a layer of gates, or that time is advancing.
    For example, used by `stimcirq` to preserve the moment structure of cirq circuits converted to/from stim circuits.
    
    - Example:
    
        ```
        TICK
        TICK
        # Oh, and of course:
        TICK
        ```
    
