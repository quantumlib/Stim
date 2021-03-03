# Stim

Stim is a fast simulator for non-adaptive quantum stabilizer circuits.

Stim can be installed into a python 3 environment using pip:

```bash
pip install stim
```

Once stim is installed, you can `import stim` and use it.
There are two supported use cases: interactive usage and high speed sampling.

You can use the Tableau simulator in an interactive fashion:

```python
import stim

s = stim.TableauSimulator()

# Create a GHZ state.
s.h(0)
s.cnot(0, 1)
s.cnot(0, 2)

# Measure the GHZ state.
print(s.measure_many(0, 1, 2))  # [False, False, False] or [True, True, True]
```

Alternatively, you can compile a circuit and then begin generating samples from it:

```python
import stim

# Create a circuit that measures a large GHZ state.
c = stim.Circuit()
c.append_operation("H", [0])
for k in range(1, 30):
    c.append_operation("CNOT", [0, k])
c.append_operation("M", range(30))

# Compile the circuit into a high performance sampler.
sampler = c.compile_sampler()

# Collect a batch of samples.
# Note: the ideal batch size, in terms of speed per sample, is roughly 1024.
# Smaller batches are slower because they are not sufficiently vectorized.
# Bigger batches are slower because they use more memory.
batch = sampler.sample(1024)
print(type(batch))  # numpy.ndarray
print(batch.dtype)  # numpy.uint8
print(batch.shape)  # (1024, 30)
print(batch)
# Prints something like:
# [[1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  ...
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]
#  [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
#  [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1]]
```

The circuit can also include noise:

```python
import stim
import numpy as np

c = stim.Circuit()
c.append_from_stim_program_text("""
    X_ERROR(0.1) 0
    Y_ERROR(0.2) 1
    Z_ERROR(0.3) 2
    DEPOLARIZE1(0.4) 3
    DEPOLARIZE2(0.5) 4 5
    M 0 1 2 3 4 5
""")
batch = c.compile_sampler().sample(2**20)
print(np.mean(batch, axis=0).round(3))
# Prints something like:
# [0.1   0.2   0.    0.267 0.267 0.266]
```

You can also sample annotated detection events using `stim.Circuit.compile_detector_sampler`.


## Supported Gates

### General facts about all gates.

- **Qubit Targets**:
    Qubits are referred to by non-negative integers.
    There is a qubit `0`, a qubit `1`, and so forth (up to an implemented-defined maximum of `16777215`).
    For example, the line `X 2` says to apply an `X` gate to qubit `2`.
    Beware that touching qubit `999999` implicitly tells simulators to resize their internal state to accommodate a million qubits.

- **Measurement Record Targets**:
    Measurement results are referred to by `rec[-#]` arguments, where the index within
    the square brackets uses python-style negative indices to refer to the end of the
    growing measurement record.
    For example, `CNOT rec[-1] 3` says "toggle qubit `3` if the most recent measurement returned `True`
    and `CZ 1 rec[-2]` means "phase flip qubit `1` if the second most recent measurement returned `True`.
    There is implementation-defined maximum lookback of `-16777215` when accessing the measurement record.
    Non-negative indices are not permitted.

- **Broadcasting**:
    Most gates support broadcasting over multiple targets.
    For example, `H 0 1 2` will broadcast a Hadamard gate over qubits `0`, `1`, and `2`.
    Two qubit gates can also broadcast, and do so over aligned pair of targets.
    For example, `CNOT 0 1 2 3` will apply `CNOT 0 1` and then `CNOT 2 3`.
    Broadcasting is always evaluated in left-to-right order.

### Single qubit gates

- `Z`: Pauli Z gate. Phase flip.
- `Y`: Pauli Y gate.
- `X`: Pauli X gate. Bit flip.
- `H` (alternate name `H_XZ`): Hadamard gate. Swaps the X and Z axes. Unitary equals (X + Z) / sqrt(2).
- `H_XY`: Variant of the Hadamard gate that swaps the X and Y axes (instead of X and Z). Unitary equals (X + Y) / sqrt(2).
- `H_YZ`: Variant of the Hadamard gate that swaps the Y and Z axes (instead of X and Z). Unitary equals (Y + Z) / sqrt(2).
- `S` (alternate name `SQRT_Z`): Principle square root of Z gate. Equal to `diag(1, i)`.
- `S_DAG` (alternate name `SQRT_Z_DAG`): Adjoint square root of Z gate. Equal to `diag(1, -i)`.
- `SQRT_Y`: Principle square root of Y gate. Equal to `H_YZ*S*H_YZ`.
- `SQRT_Y_DAG`: Adjoint square root of Y gate. Equal to `H_YZ*S_DAG*H_YZ`.
- `SQRT_X`: Principle square root of X gate. Equal to `H*S*H`.
- `SQRT_X_DAG`: Adjoint square root of X gate. Equal to `H*S_DAG*H`.
- `I`: Identity gate. Does nothing. Why is this even here? Probably out of a misguided desire for closure.

### Two qubit gates

- `SWAP`: Swaps two qubits.
- `ISWAP`: Swaps two qubits while phasing the ZZ observable by i. Equal to `SWAP * CZ * (S tensor S)`.
- `ISWAP_DAG`: Swaps two qubits while phasing the ZZ observable by -i. Equal to `SWAP * CZ * (S_DAG tensor S_DAG)`.
- `CNOT` (alternate names `CX`, `ZCX`):
    Controlled NOT operation.
    Qubit pairs are in name order (first qubit is the control, second is the target).
    This gate can be controlled by on the measurement record.
    Examples: unitary `CNOT 1 2`, feedback `CNOT rec[-1] 4`.
- `CY` (alternate name `ZCY`):
    Controlled Y operation.
    Qubit pairs are in name order (first qubit is the control, second is the target).
    This gate can be controlled by on the measurement record.
    Examples: unitary `CY 1 2`, feedback `CY rec[-1] 4`.
- `CZ` (alternate name `ZCZ`):
    Controlled Z operation.
    This gate can be controlled by on the measurement record.
    Examples: unitary `CZ 1 2`, feedback `CZ rec[-1] 4` or `CZ 4 rec[-1]`.
- `YCZ`:
    Y-basis-controlled Z operation (i.e. the reversed-argument-order controlled-Y).
    Qubit pairs are in name order.
    This gate can be controlled by on the measurement record.
    Examples: unitary `YCZ 1 2`, feedback `YCZ 4 rec[-1]`.
- `YCY`: Y-basis-controlled Y operation.
- `YCX`: Y-basis-controlled X operation. Qubit pairs are in name order.
- `XCZ`:
    X-basis-controlled Z operation (i.e. the reversed-argument-order controlled-not).
    Qubit pairs are in name order.
    This gate can be controlled by on the measurement record.
    Examples: unitary `XCZ 1 2`, feedback `XCZ 4 rec[-1]`.
- `XCY`: X-basis-controlled Y operation. Qubit pairs are in name order.
- `XCX`: X-basis-controlled X operation.

### Collapsing gates

- `M`:
    Z-basis measurement.
    Examples: `M 0`, `M 2 1`, `M 0 !3 1 2`.
    Collapses the target qubits and reports their values (optionally flipped).
    Prefixing a target with a `!` indicates that the measurement result should be inverted when reported.
    In the tableau simulator, this operation may require a transpose and so is more efficient when grouped
    (e.g. prefer `M 0 1 \n H 0` over `M 0 \n H 0 \n M 1`).
- `R`:
    Reset to |0>.
    Examples: `R 0`, `R 2 1`, `R 0 3 1 2`.
    Silently measures the target qubits and bit flips them if they're in the |1> state.
    Equivalently, discards the target qubits for zero'd qubits.
    In the tableau simulator, this operation may require a transpose and so is more efficient when grouped
    (e.g. prefer `R 0 1 \n X 0` over `R 0 \n X 0 \ nR 1`).
- `MR`:
    Z-basis measurement and reset.
    Examples: `MR 0`, `MR 2 1`, `MR 0 !3 1 2`.
    Collapses the target qubits, reports their values (optionally flipped), then resets them to the |0> state.
    Prefixing a target with a `!` indicates that the measurement result should be inverted when reported.
    (The ! does not change that the qubit is reset to |0>.)
    In the tableau simulator, this operation may require a transpose and so is more efficient when grouped
    (e.g. prefer `MR 0 1 \n H 0` over `MR 0 \n H 0 \n MR 1`).

### Noise Gates

- `DEPOLARIZE1(p)`:
    Single qubit depolarizing error.
    Examples: `DEPOLARIZE1(0.001) 1`, `DEPOLARIZE1(0.0003) 0 2 4 6`.
    With probability `p`, applies independent single-qubit depolarizing kicks to the given qubits.
    A single-qubit depolarizing kick is `X`, `Y`, or `Z` chosen uniformly at random.
- `DEPOLARIZE2(p)`:
    Two qubit depolarizing error.
    Examples: `DEPOLARIZE2(0.001) 0 1`, `DEPOLARIZE2(0.0003) 0 2 4 6`.
    With probability `p`, applies independent two-qubit depolarizing kicks to the given qubit pairs.
    A two-qubit depolarizing kick is
    `IX`, `IY`, `IZ`, `XI`, `XX`, `XY`, `XZ`, `YI`, `YX`, `YY`, `YZ`, `ZI`, `ZX`, `ZY`, `ZZ`
    chosen uniformly at random.
- `X_ERROR(p)`:
    Single-qubit probabilistic X error.
    Examples: `X_ERROR(0.001) 0 1`.
    For each target qubit, independently applies an X gate With probability `p`.
- `Y_ERROR(p)`:
    Single-qubit probabilistic Y error.
    Examples: `Y_ERROR(0.001) 0 1`.
    For each target qubit, independently applies a Y gate With probability `p`.
- `Z_ERROR(p)`:
    Single-qubit probabilistic Z error.
    Examples: `Z_ERROR(0.001) 0 1`.
    For each target qubit, independently applies a Z gate With probability `p`.
- `CORRELATED_ERROR(p)` (alternate name `E`) and `ELSE_CORRELATED_ERROR(p)`:
    Pauli product error cases.
    Probabilistically applies a Pauli product error with probability `p`,
    unless the "correlated error occurred" flag is already set.
    `CORRELATED_ERROR` is equivalent to `ELSE_CORRELATED_ERROR` except that
    `CORRELATED_ERROR` starts by clearing the "correlated error occurred" flag.
    Both operations set the "correlated error occurred" flag if they apply their error.
    Example:

        # With 40% probability, uniformly pick X1*Y2 or Z2*Z3 or X1*Y2*Z3.
        CORRELATED_ERROR(0.2) X1 Y2
        ELSE_CORRELATED_ERROR(0.25) Z2 Z3
        ELSE_CORRELATED_ERROR(0.33333333333) X1 Y2 Z3

### Annotations

- `DETECTOR`:
    Asserts that a set of measurements have a deterministic result,
    and that this result changing can be used to detect errors.
    Ignored in measurement sampling mode.
    In detection sampling mode, a detector produces a sample indicating if it was inverted by  noise or not.
    Example: `DETECTOR rec[-1] rec[-2]`.
- `OBSERVABLE_INCLUDE(k)`:
    Adds physical measurement locations to a specified logical observable.
    The logical measurement result is the parity of all physical measurements added to it.
    Behaves similarly to a Detector, except observables can be built up globally over the entire circuit instead of being defined locally.
    Ignored in measurement sampling mode.
    In detection sampling mode, a logical observable can produce a sample indicating if it was inverted by  noise or not.
    These samples are dropped or put before or after detector samples, depending on command line flags.
    Examples: `OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]`, `OBSERVABLE_INCLUDE(3) rec[-7]`.
