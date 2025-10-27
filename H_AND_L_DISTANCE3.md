# H and L Matrices for Distance-3 Surface Code

## Data Qubit Layout

```
  0  1  2
  3  4  5
  6  7  8
```

9 data qubits arranged in a 3×3 grid.

## Parity Check Matrix H

For depolarizing noise, we need two parity check matrices:

### H_x: Detects X errors (via Z-type stabilizers)

**Shape:** 8 × 9 (8 detectors × 9 data qubits)

```
H_x =
     q0 q1 q2 q3 q4 q5 q6 q7 q8
S0 [[ 1  1  0  1  1  0  0  0  0 ]   ← Z₀Z₁Z₃Z₄
S1  [ 0  1  1  0  1  1  0  0  0 ]   ← Z₁Z₂Z₄Z₅
S2  [ 0  0  0  1  1  0  1  1  0 ]   ← Z₃Z₄Z₆Z₇
S3  [ 0  0  0  0  1  1  0  1  1 ]   ← Z₄Z₅Z₇Z₈
S4  [ 0  0  0  0  0  0  0  0  0 ]   (X-type stabilizers don't detect X errors)
S5  [ 0  0  0  0  0  0  0  0  0 ]
S6  [ 0  0  0  0  0  0  0  0  0 ]
S7  [ 0  0  0  0  0  0  0  0  0 ]]
```

**Interpretation:**
- H_x[i,j] = 1 means Z-type stabilizer i has a Z operator on data qubit j
- When an X error occurs on qubit j, all stabilizers i with H_x[i,j]=1 will flip
- For X error vector e_x: syndrome s = H_x @ e_x (mod 2)

### H_z: Detects Z errors (via X-type stabilizers)

**Shape:** 8 × 9 (8 detectors × 9 data qubits)

```
H_z =
     q0 q1 q2 q3 q4 q5 q6 q7 q8
S0 [[ 0  0  0  0  0  0  0  0  0 ]   (Z-type stabilizers don't detect Z errors)
S1  [ 0  0  0  0  0  0  0  0  0 ]
S2  [ 0  0  0  0  0  0  0  0  0 ]
S3  [ 0  0  0  0  0  0  0  0  0 ]
S4  [ 1  1  0  1  1  0  0  0  0 ]   ← X₀X₁X₃X₄
S5  [ 0  1  1  0  1  1  0  0  0 ]   ← X₁X₂X₄X₅
S6  [ 0  0  0  1  1  0  1  1  0 ]   ← X₃X₄X₆X₇
S7  [ 0  0  0  0  1  1  0  1  1 ]]  ← X₄X₅X₇X₈
```

**Interpretation:**
- H_z[i,j] = 1 means X-type stabilizer i has an X operator on data qubit j
- When a Z error occurs on qubit j, all stabilizers i with H_z[i,j]=1 will flip
- For Z error vector e_z: syndrome s = H_z @ e_z (mod 2)

### Combined Syndrome Calculation

For depolarizing noise with errors e = (e_x, e_z):

```
s = (H_x @ e_x + H_z @ e_z) mod 2
```

where:
- e_x ∈ {0,1}⁹ indicates X errors on each qubit
- e_z ∈ {0,1}⁹ indicates Z errors on each qubit
- s ∈ {0,1}⁸ is the syndrome (detector outcomes)

## Logical Operators L

### L_x: Logical X Operator

**Shape:** 1 × 9

```
L_x = [1  1  1  0  0  0  0  0  0]
```

**Pauli String:** X₀X₁X₂ (horizontal string across top row)

**Interpretation:**
- Applies X operator to qubits 0, 1, 2
- This is a logical X operation on the encoded qubit
- Weight = 3 (minimum for distance-3 code)

### L_z: Logical Z Operator

**Shape:** 1 × 9

```
L_z = [1  0  0  1  0  0  1  0  0]
```

**Pauli String:** Z₀Z₃Z₆ (vertical string down left column)

**Interpretation:**
- Applies Z operator to qubits 0, 3, 6
- This is a logical Z operation on the encoded qubit
- Weight = 3 (minimum for distance-3 code)

## Verification of Key Relationships

### 1. Logical-Stabilizer Orthogonality

**H_x * L_z^T = 0 (mod 2)** ✓

```
H_x @ L_z.T = [0 0 0 0 0 0 0 0]^T
```

Logical Z commutes with all Z-type stabilizers.

**H_z * L_x^T = 0 (mod 2)** ✓

```
H_z @ L_x.T = [0 0 0 0 0 0 0 0]^T
```

Logical X commutes with all X-type stabilizers.

### 2. Logical Anticommutativity

**L_x * L_z^T = 1 (mod 2)** ✓

```
L_x @ L_z.T = [1]
```

Logical X and Z anticommute, as required for a qubit.

## Usage with Your Code

In your script:

```python
circuit = stim.Circuit.generated(
    "surface_code:rotated_memory_z",
    distance=3,
    rounds=1,
    before_round_data_depolarization=p_data,
    before_measure_flip_probability=0.0,  # K1=0
    after_clifford_depolarization=0.0,     # K2=0
    after_reset_flip_probability=0.0,      # K3=0
)

sampler = circuit.compile_detector_sampler()
syn, log = sampler.sample(count, separate_observables=True)
```

### Interpretation:

- **syn[i]**: 8-bit syndrome vector = H * e (mod 2)
  - syn[i] tells you which of the 8 stabilizers detected errors
  - For depolarizing: syn = (H_x @ e_x + H_z @ e_z) mod 2

- **log[i]**: 1-bit logical observable = L * e (mod 2)
  - log[i] = 0: logical state preserved
  - log[i] = 1: logical state flipped
  - For depolarizing: log = (L_x @ e_x + L_z @ e_z) mod 2

### Relationship:

```
syn = H * e (mod 2)  ← Which stabilizers detected errors
log = L * e (mod 2)  ← Whether logical state flipped
```

With the orthogonality relationship:

```
H * L^T = 0 (mod 2)  ← Logical ops invisible to stabilizers
```

This means:
- Stabilizer measurements tell you WHICH errors occurred (up to equivalence)
- Logical measurement tells you IF the logical qubit was corrupted
- Logical operators are "invisible" to stabilizer measurements

## Stabilizer Generators (Full Pauli Strings)

### Z-type (Detect X errors):

- **S₀:** ZZIZZIIII (qubits 0,1,3,4)
- **S₁:** IZZIZZIII (qubits 1,2,4,5)
- **S₂:** IIIZZIZZI (qubits 3,4,6,7)
- **S₃:** IIIIZZIZZ (qubits 4,5,7,8)

### X-type (Detect Z errors):

- **S₄:** XXIXXIIII (qubits 0,1,3,4)
- **S₅:** IXXIXXIII (qubits 1,2,4,5)
- **S₆:** IIIXXIXXI (qubits 3,4,6,7)
- **S₇:** IIIIXXIXX (qubits 4,5,7,8)

### Logical Operators:

- **L_X:** XXXIIIIII (qubits 0,1,2) - horizontal string
- **L_Z:** ZIIZIIZII (qubits 0,3,6) - vertical string

## Code Parameters

**[[n, k, d]] = [[9, 1, 3]]**

- n = 9 physical qubits
- k = 1 logical qubit
- d = 3 code distance (can correct ⌊(d-1)/2⌋ = 1 error)

## Summary

The parity check matrices H_x and H_z define how errors are detected:
- **H_x**: Maps X-type errors to syndromes
- **H_z**: Maps Z-type errors to syndromes

The logical operators L_x and L_z define the logical qubit:
- **L_x**: Logical X operation (flips logical state in X basis)
- **L_z**: Logical Z operation (flips logical state in Z basis)

The fundamental relationship **H * L^T = 0** ensures that logical operations don't create syndromes, making them "invisible" to error detection while still acting on the encoded information.
