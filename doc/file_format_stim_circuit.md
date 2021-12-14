# The Stim Circuit File Format (.stim)

A stim circuit file (.stim) is a human-readable specification of an annotated stabilizer circuit.
The circuit file includes gates to apply to qubits,
noise processes to apply during simulations,
and annotations for tasks such as detection event sampling and drawing the circuit.

## Index

- [Encoding](#Encoding)
- [Syntax](#Syntax)
- [Semantics](#Semantics)
    - [Instruction Types](#Instruction-Types)
        - [Supported Gates](gates.md)
    - [Broadcasting](#Broadcasting)
    - [State Space](#State-Space)
    - [Vacuous repeat blocks are not allowed](#Vacuous-repeat-blocks-are-not-allowed)
- [Examples](#Examples)
    - [Teleportation Circuit](#Teleportation-Circuit)
    - [Repetition Code Circuit](#Repetition-Code-Circuit)
    - [Annotated Noisy Repetition Code Circuit](#Annotated-Noisy-Repetition-Code-Circuit)
    - [Annotated Noisy Surface Code Circuit](#Annotated-Noisy-Surface-Code-Circuit)


## Encoding

Stim circuit files are always encoded using UTF-8.
Furthermore, the only place in the file where non-ASCII characters are permitted is inside of comments.

## Syntax

A stim circuit file is made up of a series of lines.
Each line is either blank, an instruction, a block initiator, or a block terminator.
Also, each line may be indented with spacing characters and may end with a comment indicated by a hash (`#`).
Comments and indentation are purely decorative; they carry no semantic significance.

```
<CIRCUIT> ::= <LINE>*
<LINE> ::= <INDENT> (<INSTRUCTION> | <BLOCK_START> | <BLOCK_END>)? <COMMENT>? '\n'
<INDENT> ::= /[ \t]*/
<COMMENT> ::= '#' /[^\n]*/
```

An *instruction* is composed of a name,
then an optional comma-separated list of arguments inside of parentheses,
then a list of space-separated targets.
For example, the line `X_ERROR(0.1) 5 6` is an instruction with a name (`X_ERROR`),
one argument (`0.1`), and two targets (`5` and `6`).

```
<INSTRUCTION> ::= <NAME> <PARENS_ARGUMENTS>? <TARGETS>
<PARENS_ARGUMENTS> ::= '(' <ARGUMENTS> ')' 
<ARGUMENTS> ::= /[ \t]*/ <ARG> /[ \t]*/ (',' <ARGUMENTS>)?
<TARGETS> ::= /[ \t]+/ <TARG> <TARGETS>?
```

An instruction *name* starts with a letter and then contains a series of letters, digits, and underscores.
Names are case-insensitive.

An *argument* is a double precision floating point number.

A *target* can either be a qubit target (a non-negative integer),
a measurement record target (a negative integer prefixed by `rec[` and suffixed by `]`),
a sweep bit target (a non-negative integer prefixed by `sweep[` and suffixed by `]`),
a Pauli target (an integer prefixed by `X`, `Y`, or `Z`),
or a combiner (`*`).
Additionally, qubit targets and Pauli targets may be prefixed by a `!` to indicate that
measurement results should be negated.

```
<NAME> ::= /[a-zA-Z][a-zA-Z0-9_]*/ 
<ARG> ::= <double> 
<TARG> ::= <QUBIT_TARGET> | <MEASUREMENT_RECORD_TARGET> | <SWEEP_BIT_TARGET> | <PAULI_TARGET> | <COMBINER_TARGET> 
<QUBIT_TARGET> ::= '!'? <uint>
<MEASUREMENT_RECORD_TARGET> ::= "rec[-" <uint> "]"
<SWEEP_BIT_TARGET> ::= "sweep[" <uint> "]"
<PAULI_TARGET> ::= '!'? /[XYZ]/ <uint>
<COMBINER_TARGET> ::= '*'
```

A *block initiator* is an instruction suffixed with `{`.
Every block initiator must be followed, eventually, by a matching *block terminator* which is just a `}`.
The `{` always goes in the same line as its instruction, and the `}` always goes on a line by itself.

```
<BLOCK_START> ::= <INSTRUCTION> /[ \t]*/ '{'
<BLOCK_END> ::= '}' 
```

Blocks can be nested.
Block contents are indented by convention, but this is not necessary.

## Semantics

A stim circuit file is executed by executing each of its instructions and blocks, one by one, from start to finish.

### Instruction Types

For a complete list of instructions supported by stim and their individual meanings,
see the [gates reference](gates.md).

Generally speaking, the instructions that can appear in a stim circuit file can be divided up into three groups:

1. Operations
2. Annotations
3. Control Flow

An *operation* is a quantum channel to apply to the quantum state of the system,
possibly resulting in bits being appended to the measurement record.
There are Clifford operations (e.g. the Hadamard gate `H` or the controlled-not gate `CNOT`),
stabilizers operations (e.g. the measurement gate `M` or the reset gate `R`),
and noise operations (e.g. the phase damping channel `Z_ERROR` or the two qubit depolarizing channel `DEPOLARIZE2`).

An *annotation* is a piece of additional information that is not strictly necessary, but which enables other useful capabilities.
The most functionally useful annotations are `DETECTOR` and `OBSERVABLE_INCLUDE`,
which define the measurements that are compared when sampling a circuit's detection events.
Other annotations include `QUBIT_COORDS` and `TICK`, which can be used to hint at the intended spacetime layout of a circuit.

(Depending on your needs, you may also find yourself considering noisy operations to be annotations.
They define a noise model for the circuit.)

*Control flow* blocks make changes to the usual one-after-another execution order of instructions.
Currently, control flow is limited to *repetition*.
A circuit can contain `REPEAT K { ... }` blocks,
which indicate that the block's instructions should be iterated over `K` times instead of just once.

### Target Types

There are four types of targets that can be given to instructions:
qubit targets, measurement record targets, sweep bit targets, and Pauli targets.

A qubit target refers to a qubit by index.
There's a qubit `0`, a qubit `1`, a qubit `2`, and so forth.
A qubit target may be prefixed by `!`, like `!2`, to mark it as inverted.
Inverted qubit targets are only meaningful for operations that produce measurement results.
They indicate that the recorded measurement result, for the given qubit target, should be inverted.
For example `M 0 !1` measures qubit `0` and qubit `1`, but also inverts the result recorded for qubit `1`.

A measurement record target refers to a recorded measurement result, relative to the current end of the measurement record.
For example, `rec[-1]` is the most recent measurement result, `rec[-2]` is the second most recent, and so forth.
(The semantics of the negative indices into the measurement record match the semantics of negative indices into lists in Python.
The reason negative indices are used is to make it possible to write loops.)
It is an error to refer to a measurement result so far back that it would precede the start of the circuit.

A sweep bit target refers to a column in a data table where each row refers to a separate shots of the circuit, and each column refers to configuration bits that vary from shot to shot.
For example, when using randomized spin echo, the spin echo operations that actually occurred could be recording into a table.
For example, `CNOT sweep[5] 1` says an X operation was applied (or should be applied) to qubit 1 for shots where the sweep bit in the column with index 5 is set.
Sweep bits default to False when running in a context where no table is provided, and sweep bits past the end of the provided table also default to False.

A Pauli target is a qubit target prefixed by a Pauli operation `X`, `Y`, or `Z`.
They are used when specifying Pauli products.
For example, `CORRELATED_ERROR(0.1) X1 Y3 Z2` uses Pauli targets to specify the error that is applied.
Pauli targets may be grouped using combiners (`*`) and may be prefixed by `!` to mark them as inverted.
Inverted Pauli targets are only meaningful for operations that produce measurement results.
They indicate that the recorded measurement result, for the given group of Paulis, should be inverted.
For example `MPP !X1*Z2 Y3` measures the Pauli product `X1*Z2` and inverts the result, then also measures
the Pauli `Y3`.

### Broadcasting

When quantum operations are applied to too many targets, the operation is *broadcast* over the targets.

When a single qubit operation (e.g. `H` or `DEPOLARIZE1`) is given multiple targets,
it is applied to each target in order.
For example, `H 0 1 2` is equivalent to `H 0` then `H 1` then `H 2`.
Similarly, `X_ERROR(0.1) 3 2` is equivalent to `X_ERROR(0.1) 3` then `X_ERROR(0.1) 2`.

When a two qubit operation (e.g. `CNOT` or `DEPOLARIZE2`) is given multiple targets,
it is applied to aligned target pairs in order.
For example, `CNOT 0 1 1 2 2 0` is equivalent to `CNOT 0 1` then `CNOT 1 2` then `CNOT 2 0`.
It is an error to give a two qubit operation an odd number of targets.

### State Space

A simulator executing a stim circuit is expected to store three things:

1. **The Qubits**.
    By convention, all qubits start in the |0> state.
    The simulator then tracks the state of any qubits that have been operated on.
    
    Note that stim circuit files don't explicitly state the number of qubits needed.
    Instead, the number of qubits is implied by the qubit targets present in the file.
    For example, a simulator may look over the circuit and find the largest qubit target `n-1` that is
    operated on and then size itself for operating on `n` qubits.
2. **The Measurement Record**.
    When a measurement operation is performed, the measurement result is appended to a list of bits
    referred to as the measurement record.
    The measurement record is an immutable log of all measurement results so far.
    Controlled operations can use a measurement record target as a control, instead of a qubit target.
    For example, `CZ rec[-1] 5` says "if the most recent measurement result was TRUE then apply a Z
    gate to qubit `5`".
    The measurement record is also used when defining detectors and observables.
3. **The "Correlated Error Occurred" Flag**.
    The `ELSE_CORRELATED_ERROR` instruction applies an error mechanism conditioned on the preceding
    `CORRELATED_ERROR` instruction (and any intermediate `ELSE_CORRELATED_ERROR` instructions)
    having not occurred. This is tracked by a hidden boolean flag. 

(The interpreter of the circuit may also track coordinate offsets accumulated from `SHIFT_COORDS` annotations,
which affect the meaning of `QUBIT_COORDS` annotations and the coordinate arguments given to `DETECTOR`.
But these have no effect on simulations, and so are often not strictly necessary to track.)

### Vacuous repeat blocks are not allowed

It's an error for a circuit to contain a repeat block that is repeated 0 times.

The reason it's an error is because it's ambiguous whether observables and qubits mentioned in the block "exist".
For example, consider this malformed circuit:

```
REPEAT 0 {
    M 0
    OBSERVABLE_INCLUDE(0) rec[-1]
}
```

This circuit mentions a logical observable with index 0, suggesting the circuit has a logical observable.
So, a tool that samples logical observables should produce 1 bit of information when sampling this circuit.
But the logical observable is only mentioned in a block that is never run, effectively commenting it out, leaving behind
an empty circuit with 0 logical observables.
So, a tool that samples logical observables should produce 0 bits of information when sampling this circuit.
Is there an observable in the circuit or isn't there?
Should the tool produce 0 bits or 1 bit?
That's the ambiguity.

Note that a tool that unrolls loops in the circuit will implicitly delete the ambiguous logical observables.
Conversely, note that a tool that finds logical observables by iterating over the lines of the circuit file, looking
for `OBSERVABLE_INCLUDE` instructions, will implicitly keep the ambiguous logical observables.
Both of these methods seem "obviously correct" on their own, but they disagree about whether or not to keep the
ambiguous observables.
It's very easy to write code that accidentally disagrees with itself about the correct behavior, and introduce a bug.
Which is why vacuous repeat blocks are not allowed. 

## Examples

### Teleportation Circuit

[View equivalent circuit in Quirk](https://algassert.com/quirk#circuit=%7B%22cols%22%3A%5B%5B%22H%22%5D%2C%5B%22%E2%80%A2%22%2C1%2C1%2C1%2C1%2C1%2C1%2C%22X%22%5D%2C%5B1%2C%22H%22%5D%2C%5B1%2C%22Z%5E%C2%BD%22%5D%2C%5B%22%E2%80%A2%22%2C%22X%22%5D%2C%5B%22H%22%5D%2C%5B%22Measure%22%2C%22Measure%22%5D%2C%5B%22%E2%80%A2%22%2C1%2C1%2C1%2C1%2C1%2C1%2C%22Z%22%5D%2C%5B1%2C%22%E2%80%A2%22%2C1%2C1%2C1%2C1%2C1%2C%22X%22%5D%5D%7D)

```
# Distribute a Bell Pair.
H 0
CNOT 0 99

# Sender creates an arbitrary qubit state to send.
H 1
S 1

# Sender performs a Bell Basis measurement.
CNOT 0 1
H 0
M 0 1  # Measure both of the sender's qubits.

# Receiver performs frame corrections based on measurement results.
CZ rec[-2] 99
CNOT rec[-1] 99
```

### Repetition Code Circuit

[View equivalent circuit in Quirk](https://algassert.com/quirk#circuit=%7B%22cols%22%3A%5B%5B%22~ch91%22%2C1%2C%22~ch91%22%2C1%2C%22~ch91%22%5D%2C%5B1%2C%22~tbv6%22%2C1%2C%22~tbv6%22%2C1%2C%22~tbv6%22%5D%2C%5B1%2C%22ZDetectControlReset%22%2C1%2C%22ZDetectControlReset%22%2C1%2C%22ZDetectControlReset%22%5D%2C%5B%22~ch91%22%2C1%2C%22~ch91%22%2C1%2C%22~ch91%22%5D%2C%5B1%2C%22~tbv6%22%2C1%2C%22~tbv6%22%2C1%2C%22~tbv6%22%5D%2C%5B1%2C%22ZDetectControlReset%22%2C1%2C%22ZDetectControlReset%22%2C1%2C%22ZDetectControlReset%22%5D%2C%5B%22%E2%80%A6%22%2C%22%E2%80%A6%22%2C%22%E2%80%A6%22%2C%22%E2%80%A6%22%2C%22%E2%80%A6%22%2C%22%E2%80%A6%22%2C%22%E2%80%A6%22%5D%2C%5B%22~ch91%22%2C1%2C%22~ch91%22%2C1%2C%22~ch91%22%5D%2C%5B1%2C%22~tbv6%22%2C1%2C%22~tbv6%22%2C1%2C%22~tbv6%22%5D%2C%5B1%2C%22ZDetectControlReset%22%2C1%2C%22ZDetectControlReset%22%2C1%2C%22ZDetectControlReset%22%5D%2C%5B%22~ch91%22%2C1%2C%22~ch91%22%2C1%2C%22~ch91%22%5D%2C%5B1%2C%22~tbv6%22%2C1%2C%22~tbv6%22%2C1%2C%22~tbv6%22%5D%2C%5B%22ZDetectControlReset%22%2C%22ZDetectControlReset%22%2C%22ZDetectControlReset%22%2C%22ZDetectControlReset%22%2C%22ZDetectControlReset%22%2C%22ZDetectControlReset%22%2C%22ZDetectControlReset%22%5D%5D%2C%22gates%22%3A%5B%7B%22id%22%3A%22~ch91%22%2C%22circuit%22%3A%7B%22cols%22%3A%5B%5B%22%E2%80%A2%22%2C%22X%22%5D%5D%7D%7D%2C%7B%22id%22%3A%22~tbv6%22%2C%22circuit%22%3A%7B%22cols%22%3A%5B%5B%22X%22%2C%22%E2%80%A2%22%5D%5D%7D%7D%5D%7D)
(without detector annotations).

```
# Measure the parities of adjacent data qubits.
# Data qubits are 0, 2, 4, 6.
# Measurement qubits are 1, 3, 5.
CNOT 0 1 2 3 4 5
CNOT 2 1 4 3 6 5
MR 1 3 5

# Annotate that the measurements should be deterministic.
DETECTOR rec[-3]
DETECTOR rec[-2]
DETECTOR rec[-1]

# Perform 1000 more rounds of measurements.
REPEAT 1000 {
    # Measure the parities of adjacent data qubits.
    CNOT 0 1 2 3 4 5
    CNOT 2 1 4 3 6 5
    MR 1 3 5

    # Annotate that the measurements should agree with previous round.
    DETECTOR rec[-3] rec[-6]
    DETECTOR rec[-2] rec[-5]
    DETECTOR rec[-1] rec[-4]
}

# Measure data qubits.
M 0 2 4 6

# Annotate that the data measurements should agree with the parity measurements.
DETECTOR rec[-3] rec[-4] rec[-7]
DETECTOR rec[-2] rec[-3] rec[-6]
DETECTOR rec[-1] rec[-2] rec[-5]

# Declare one of the data qubit measurements to a logical measurement result.
OBSERVABLE_INCLUDE(0) rec[-1]
```

### Fully Annotated Noisy Repetition Code Circuit

This is the output from
`stim --gen repetition_code --task memory --rounds 1000 --distance 4 --after_clifford_depolarization 0.001`.
It includes noise operations and annotations for the spacetime layout of the circuit.

```
# Generated repetition_code circuit.
# task: memory
# rounds: 1000
# distance: 3
# before_round_data_depolarization: 0
# before_measure_flip_probability: 0
# after_reset_flip_probability: 0
# after_clifford_depolarization: 0.001
# layout:
# L0 Z1 d2 Z3 d4 Z5 d6
# Legend:
#     d# = data qubit
#     L# = data qubit with logical observable crossing
#     Z# = measurement qubit
R 0 1 2 3 4 5 6
TICK
CX 0 1 2 3 4 5
DEPOLARIZE2(0.001) 0 1 2 3 4 5
TICK
CX 2 1 4 3 6 5
DEPOLARIZE2(0.001) 2 1 4 3 6 5
TICK
MR 1 3 5
DETECTOR(1, 0) rec[-3]
DETECTOR(3, 0) rec[-2]
DETECTOR(5, 0) rec[-1]
REPEAT 999 {
    TICK
    CX 0 1 2 3 4 5
    DEPOLARIZE2(0.001) 0 1 2 3 4 5
    TICK
    CX 2 1 4 3 6 5
    DEPOLARIZE2(0.001) 2 1 4 3 6 5
    TICK
    MR 1 3 5
    SHIFT_COORDS(0, 1)
    DETECTOR(1, 0) rec[-3] rec[-6]
    DETECTOR(3, 0) rec[-2] rec[-5]
    DETECTOR(5, 0) rec[-1] rec[-4]
}
M 0 2 4 6
DETECTOR(1, 1) rec[-3] rec[-4] rec[-7]
DETECTOR(3, 1) rec[-2] rec[-3] rec[-6]
DETECTOR(5, 1) rec[-1] rec[-2] rec[-5]
OBSERVABLE_INCLUDE(0) rec[-1]
```

### Fully Annotated Noisy Surface Code Circuit

This is the output from
`stim --gen surface_code --task rotated_memory_x --rounds 1000 --distance 3 --after_clifford_depolarization 0.001`.
It includes noise operations and annotations for the spacetime layout of the circuit.

```
# Generated surface_code circuit.
# task: rotated_memory_x
# rounds: 1000
# distance: 3
# before_round_data_depolarization: 0
# before_measure_flip_probability: 0
# after_reset_flip_probability: 0
# after_clifford_depolarization: 0.001
# layout:
#                 X25
#     L15     d17     d19
# Z14     X16     Z18
#     L8      d10     d12
#         Z9      X11     Z13
#     L1      d3      d5 
#         X2 
# Legend:
#     d# = data qubit
#     L# = data qubit with logical observable crossing
#     X# = measurement qubit (X stabilizer)
#     Z# = measurement qubit (Z stabilizer)
QUBIT_COORDS(1, 1) 1
QUBIT_COORDS(2, 0) 2
QUBIT_COORDS(3, 1) 3
QUBIT_COORDS(5, 1) 5
QUBIT_COORDS(1, 3) 8
QUBIT_COORDS(2, 2) 9
QUBIT_COORDS(3, 3) 10
QUBIT_COORDS(4, 2) 11
QUBIT_COORDS(5, 3) 12
QUBIT_COORDS(6, 2) 13
QUBIT_COORDS(0, 4) 14
QUBIT_COORDS(1, 5) 15
QUBIT_COORDS(2, 4) 16
QUBIT_COORDS(3, 5) 17
QUBIT_COORDS(4, 4) 18
QUBIT_COORDS(5, 5) 19
QUBIT_COORDS(4, 6) 25
RX 1 3 5 8 10 12 15 17 19
R 2 9 11 13 14 16 18 25
TICK
H 2 11 16 25
DEPOLARIZE1(0.001) 2 11 16 25
TICK
CX 2 3 16 17 11 12 15 14 10 9 19 18
DEPOLARIZE2(0.001) 2 3 16 17 11 12 15 14 10 9 19 18
TICK
CX 2 1 16 15 11 10 8 14 3 9 12 18
DEPOLARIZE2(0.001) 2 1 16 15 11 10 8 14 3 9 12 18
TICK
CX 16 10 11 5 25 19 8 9 17 18 12 13
DEPOLARIZE2(0.001) 16 10 11 5 25 19 8 9 17 18 12 13
TICK
CX 16 8 11 3 25 17 1 9 10 18 5 13
DEPOLARIZE2(0.001) 16 8 11 3 25 17 1 9 10 18 5 13
TICK
H 2 11 16 25
DEPOLARIZE1(0.001) 2 11 16 25
TICK
MR 2 9 11 13 14 16 18 25
DETECTOR(2, 0, 0) rec[-8]
DETECTOR(2, 4, 0) rec[-3]
DETECTOR(4, 2, 0) rec[-6]
DETECTOR(4, 6, 0) rec[-1]
REPEAT 999 {
    TICK
    H 2 11 16 25
    DEPOLARIZE1(0.001) 2 11 16 25
    TICK
    CX 2 3 16 17 11 12 15 14 10 9 19 18
    DEPOLARIZE2(0.001) 2 3 16 17 11 12 15 14 10 9 19 18
    TICK
    CX 2 1 16 15 11 10 8 14 3 9 12 18
    DEPOLARIZE2(0.001) 2 1 16 15 11 10 8 14 3 9 12 18
    TICK
    CX 16 10 11 5 25 19 8 9 17 18 12 13
    DEPOLARIZE2(0.001) 16 10 11 5 25 19 8 9 17 18 12 13
    TICK
    CX 16 8 11 3 25 17 1 9 10 18 5 13
    DEPOLARIZE2(0.001) 16 8 11 3 25 17 1 9 10 18 5 13
    TICK
    H 2 11 16 25
    DEPOLARIZE1(0.001) 2 11 16 25
    TICK
    MR 2 9 11 13 14 16 18 25
    SHIFT_COORDS(0, 0, 1)
    DETECTOR(2, 0, 0) rec[-8] rec[-16]
    DETECTOR(2, 2, 0) rec[-7] rec[-15]
    DETECTOR(4, 2, 0) rec[-6] rec[-14]
    DETECTOR(6, 2, 0) rec[-5] rec[-13]
    DETECTOR(0, 4, 0) rec[-4] rec[-12]
    DETECTOR(2, 4, 0) rec[-3] rec[-11]
    DETECTOR(4, 4, 0) rec[-2] rec[-10]
    DETECTOR(4, 6, 0) rec[-1] rec[-9]
}
MX 1 3 5 8 10 12 15 17 19
DETECTOR(2, 0, 1) rec[-8] rec[-9] rec[-17]
DETECTOR(2, 4, 1) rec[-2] rec[-3] rec[-5] rec[-6] rec[-12]
DETECTOR(4, 2, 1) rec[-4] rec[-5] rec[-7] rec[-8] rec[-15]
DETECTOR(4, 6, 1) rec[-1] rec[-2] rec[-10]
OBSERVABLE_INCLUDE(0) rec[-3] rec[-6] rec[-9]
```
