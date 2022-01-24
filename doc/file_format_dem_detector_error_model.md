# The Detector Error Model File Format (.dem)

A detector error model file (.dem) is a human-readable specification of error mechanisms.
The intent of the file format is to act as a reasonably flexible configuration language that can be easily consumed by
*decoders*, which attempt to predict the logical observable frame from symptoms within the context of an error model.

## Index

- [Encoding](#Encoding)
- [Syntax](#Syntax)
- [Semantics](#Semantics)
    - [Instruction Types](#Instruction-Types)
        - [`detector` instruction](#detector-instruction)
        - [`logical_observable` instruction](#logical_observable-instruction)
        - [`shift_detectors` instruction](#shift_detectors-instruction)
        - [`error` instruction](#error-instruction)
        - [`repeat` block](#repeat-block)
    - [Target Types](#State-Space)
        - [`D#`: relative detector target](#relative-detector-target)
        - [`L#`: logical observable target](#logical-observable-target)
        - [`#`: numeric target](#numeric-target)
        - [`^`: separator target](#separator-target)
    - [State Space](#State-Space)
- [Examples](#Examples)
    - [Circular Error Model](#circular-error-model)
    - [Repetition Code Error Model](#repetition-code-error-model)
    - [Surface Code Error Model](#surface-code-error-model)


## Encoding

Detector error model files are always encoded using UTF-8.
Furthermore, the only place in the file where non-ASCII characters are permitted is inside of comments.

## Syntax

A detector error model file is made up of a series of lines.
Each line is either blank, an instruction, a block initiator, or a block terminator.
Also, each line may be indented with spacing characters and may end with a comment indicated by a hash (`#`).

```
<DETECTOR_ERROR_MODEL> ::= <LINE>*
<LINE> ::= <INDENT> (<INSTRUCTION> | <BLOCK_START> | <BLOCK_END>)? <COMMENT>? '\n'
<INDENT> ::= /[ \t]*/
<COMMENT> ::= '#' /[^\n]*/
```

An *instruction* is composed of a name,
then an optional comma-separated list of arguments inside of parentheses,
then a list of space-separated targets.
For example, the line `error(0.1) D5 D6 L0` is an instruction with a name (`error`),
one argument (`0.1`), and three targets (`D5`, `D6`, and `L0`).

```
<INSTRUCTION> ::= <NAME> <PARENS_ARGUMENTS>? <TARGETS>
<PARENS_ARGUMENTS> ::= '(' <ARGUMENTS> ')' 
<ARGUMENTS> ::= /[ \t]*/ <ARG> /[ \t]*/ (',' <ARGUMENTS>)?
<TARGETS> ::= /[ \t]+/ <TARG> <TARGETS>?
```

An instruction *name* starts with a letter and then contains a series of letters, digits, and underscores.
Names are case-insensitive.

An *argument* is a double precision floating point number.

A *target* can either be a relative detector target (a non-negative integer prefixed by `D`),
a logical observable target (a non-negative integer prefixed by `L`),
a component separator (`^`),
or an unsigned integer target (a non-negative integer).

```
<NAME> ::= /[a-zA-Z][a-zA-Z0-9_]*/ 
<ARG> ::= <double> 
<TARG> ::= <RELATIVE_DETECTOR_TARGET> | <LOGICAL_OBSERVABLE_TARGET> | <NUMBER_TARGET> | <SEPARATOR>  
<RELATIVE_DETECTOR_TARGET> ::= 'D' <uint>
<LOGICAL_OBSERVABLE_TARGET> ::= 'L' <uint>
<NUMBER_TARGET> = <uint>
<SEPARATOR> = '^'
```

A *block initiator* is an instruction suffixed with `{`.
Every block initiator must be followed, eventually, by a matching *block terminator* which is just a `}`.
The `{` always goes in the same line as its instruction, and the `}` always goes on a line by itself. 

```
<BLOCK_START> ::= <INSTRUCTION> /[ \t]*/ '{'
<BLOCK_END> ::= '}' 
```

## Semantics

A *detector error model* is a list of independent error mechanisms.
Each error mechanism has a *probability*,
*symptoms* (the detectors that the error flips),
and *frame changes* (the logical observables that the error flips).
Error mechanisms may also suggest a *decomposition* into simpler error mechanisms.

A detector error model can be sampled by independently keeping or discarding each error mechanism,
with a keep probability equal to each error mechanism's probability.
The resulting sample contains the symptoms and frame changes that appeared an odd number of times total in the kept
error mechanisms.

A detector error model file (.dem) specifies a detector error model by a series of instructions which are interpreted
one by one, from start to finish. The instructions iteratively build up a detector error model by introducing error
mechanisms.

### Instruction Types

A detector error model file can contain several different types of instructions and blocks.

#### detector instruction

A `detector` instruction declares a particular symptom that is in the error model.
It is not necessary to explicitly declare detectors using detector instructions.
Detectors can also be implicitly declared simply by being mentioned in an error mechanism,
or by a detector with a larger absolute index being declared.
However, an explicit declaration is the only way to annotate the detector with coordinates,
suggesting a spacetime location for the detector.

The detector instruction should have a detector target (the detector being declared, relative to the
current detector index offset) and can include  any number of arguments specifying the detector's
coordinates (relative to the current coordinate offset).
Note that when the current coordinate offset has more coordinates than the detector,
the additional coordinates are skipped.
For example, if the offset is `(1,2,3)` and the detector relative position is `(10,10)` then the
detector's absolute position would be `11,12`; not `11,12,3`. 

Example: `detector D4` declares a detector with index 4 (relative to the current detector index offset).

Example: `detector D5 D6` declares a detector with index 5 (relative to the current detector index offset)
and also a detector with index 6 (relative to the current detector index offset).

Example: `detector(2.5,3.5,6) D7` declares a detector with index 7
(relative to the current detector index offset)
and coordinates `2.5`, `3.5`, `6` (relative to the current coordinate offset).

#### logical_observable instruction

A `logical_observable` instruction ensures a particular frame change's existence is noted by the error model,
even if no error mechanism affects it.
Frame changes can also be implicitly declared simply by being mentioned in an error mechanism or other
instruction, or by a frame change with a larger absolute index being declared.

Example: `logical_observable L1` declares a logical observable with index 1.

Example: `logical_observable L1 L2` declares a logical observable with index 1 and also a logical observable with
index 2.

#### shift_detectors instruction

A `shift_detectors` instruction adds an offset into the current detector offset and the current coordinate offset.
Takes 1 numeric target indicating the detector offset.
Takes a variable number of arguments indicating the coordinate offset.
Shifting is useful when writing loops, because otherwise the detectors declared by each iteration
of the loop would all lie on top of each other.
The detector offset can only be increased, not decreased.

Example: `shift_detectors(0, 0.5) 2` leaves the first coordinate's offset alone,
increases the second coordinate's offset by `0.5`,
leaves all other coordinate offsets alone,
and increases the detector offset by 2.

Example: declaring a diagonal line of detectors.
```
detector(0, 0) D0
repeat 1000 {
    detector(0.5, 0.5) D1
    error(0.01) D0 D1
    shift_detectors(0.5, 0.5) 1
}
```

#### error instruction

An `error` instruction adds an error mechanism to the error model.
The error instruction takes 1 argument (the probability of the error) and multiple targets.
The targets can include detectors, observables, and separators.
Separators are used to suggest a way to decompose complicated error mechanisms into simpler ones.

For example: `error(0.1) D2 D3 L0` adds an error mechanism with
probability equal to 10%,
two symptoms (`D2`, `D3`),
one frame change (`L0`),
and no suggested decomposition.

Another example: `error(0.02) D2 L0 ^ D5 D6` adds an error mechanism with
probability equal to 2%,
three symptoms (`D2`, `D5`, `D6`),
one frame change (`L0`),
and a suggested decomposition into `D2 L0` and `D5 D6`.

Yet another example: `error(0.03) D2 L0 ^ D3 L0` adds an error mechanism with
probability equal to 3%,
two symptoms (`D2`, `D3`),
no frame changes (because the two `L0` cancel out),
and a suggested decomposition into `D2 L0` and `D3 L0`.

An example of a situation where the decomposition is relevant is a surface code with X and Z basis stabilizers.
In such a surface code, Y errors can be factored into correlated X and Z errors.
So, error mechanisms in the detector error model corresponding to Y errors in the circuit can suggest decomposing
into the X and Z parts.

It is valid for multiple error mechanisms to have the exact same targets.
Typically they would be fused as part of building the error model (via the equation
`p_{combined} = p_1 (1 - p_2) + p_2 (1 - p_1)`).
It is also valid for error mechanisms to have the same symptoms but different frame changes
(though this guarantees the error correcting code has distance at most 2).
Similarly, an error mechanism may have frame changes with no symptoms (guaranteeing a code distance equal to 1).

#### repeat block

A detector error model file can also contain `REPEAT K { ... }` blocks,
which indicate that the block's instructions should be iterated over `K` times instead of just once.

Example: declaring a diagonal line of detectors.

```
detector(0, 0) D0
repeat 1000 {
    detector(0.5, 0.5) D1
    error(0.01) D0 D1
    shift_detectors(0.5, 0.5) 1
}
```

### Target Types

There are four types of targets that can be given to instructions:
relative detector targets,
logical observable targets,
numeric targets,
and separator targets.

#### relative detector target

A relative detector target is a non-negative integer prefixed by `D`, such as `D5`.
It refers to a symptom in the error model.
To get the actual detector target specified by the relative detector target, the integer after the `D`
has to be added into the current relative detector offset.

#### logical observable target

A logical observable target is a non-negative integer prefixed by `L`, such as `L5`.
It refers to a possible frame change that an error can cause.

#### numeric target

A numeric target is a non-negative integer.
For example, the `REPEAT` block instruction takes a single numeric target indicating the number of repetitions
and the `shift_detectors` instruction takes a single numeric target indicating the detector index shift.


#### separator target

A separator target (`^`) is not an actual thing to target, but rather a marker used to split up the targets of an error
mechanism into a suggested decomposition.

### State Space

Interpreting a detector error model file, to produce a detector error model,
involves tracking several pieces of state.

1. **The Offsets**
    As the error model file is interpreted, the *relative detector index* and *relative coordinate offset* are shifted
    by `shift_detectors` instructions.
    Interpreting relative detector targets and coordinate annotations requires tracking these
    two values, since they shift the targets and coordinates.
2. **The Nodes (possible symptoms and frame changes)**.
    The error model must include every explicitly and implicitly declared detector (symptom) and
    logical observable (frame change).
    In practice this means computing the absolute index of the largest detector, and including
    a number of detectors equal to that index plus one.
    The same is done for frame changes: find the largest mentioned frame change index, and include
    a number of frame changes equal to that index plus one.
    Getting the number of nodes correct is important when parsing densely packed data that does not
    include explicit detector indices or frame change indices.
3. **The Edges (error mechanisms)**.
    The error model must include the mentioned error mechanisms.

## Examples

### Circular Error Model

This error model defines 10 symptoms, and 10 error mechanisms with two symptoms.
One of the error mechanisms, the "bad error", also has a frame change (`L0`).
If the symptoms are nodes, and error mechanisms connect two nodes, then the model forms
the 10 node circular graph.

```
error(0.1) D9 D0 L0
error(0.1) D0 D1
error(0.1) D1 D2
error(0.1) D2 D3
error(0.1) D3 D4
error(0.1) D4 D5
error(0.1) D5 D6
error(0.1) D6 D7
error(0.1) D7 D8
error(0.1) D8 D9
```

This model can be defined more succinctly by using a `repeat` block:

```
error(0.1) D9 D0 L0
repeat 9 {
    error(0.1) D0 D1
    shift_detectors 1
}
```

### Repetition Code Error Model

This is the output from
`stim --gen repetition_code --task memory --rounds 1000 --distance 4 --after_clifford_depolarization 0.001 | stim --analyze_errors --fold_loops`.
It includes coordinate annotations for the spacetime layout of the detectors.

```
error(0.0002667378157289137966) D0
error(0.0002667378157289137966) D0 D1
error(0.0005333333333331479603) D0 D3
error(0.0005333333333331479603) D0 D4
error(0.0002667378157289137966) D1 D2
error(0.0005333333333331479603) D1 D4
error(0.0005333333333331479603) D1 D5
error(0.0005333333333331479603) D2 D5
error(0.0002667378157289137966) D2 L0
error(0.0002667378157289137966) D3
error(0.0002667378157289137966) D3 D4
error(0.0002667378157289137966) D4 D5
error(0.0002667378157289137966) D5 L0
detector(1, 0) D0
detector(3, 0) D1
detector(5, 0) D2
repeat 998 {
    error(0.0002667378157289137966) D3
    error(0.0002667378157289137966) D3 D4
    error(0.0005333333333331479603) D3 D6
    error(0.0005333333333331479603) D3 D7
    error(0.0002667378157289137966) D4 D5
    error(0.0005333333333331479603) D4 D7
    error(0.0005333333333331479603) D4 D8
    error(0.0005333333333331479603) D5 D8
    error(0.0002667378157289137966) D5 L0
    error(0.0002667378157289137966) D6
    error(0.0002667378157289137966) D6 D7
    error(0.0002667378157289137966) D7 D8
    error(0.0002667378157289137966) D8 L0
    shift_detectors(0, 1) 0
    detector(1, 0) D3
    detector(3, 0) D4
    detector(5, 0) D5
    shift_detectors 3
}
error(0.0002667378157289137966) D3
error(0.0002667378157289137966) D3 D4
error(0.0005333333333331479603) D3 D6
error(0.0005333333333331479603) D3 D7
error(0.0002667378157289137966) D4 D5
error(0.0005333333333331479603) D4 D7
error(0.0005333333333331479603) D4 D8
error(0.0005333333333331479603) D5 D8
error(0.0002667378157289137966) D5 L0
error(0.0002667378157289137966) D6
error(0.0002667378157289137966) D6 D7
error(0.0002667378157289137966) D7 D8
error(0.0002667378157289137966) D8 L0
shift_detectors(0, 1) 0
detector(1, 0) D3
detector(3, 0) D4
detector(5, 0) D5
detector(1, 1) D6
detector(3, 1) D7
detector(5, 1) D8
```

### Surface Code Error Model

This is the output from
`stim --gen surface_code --task rotated_memory_x --rounds 1000 --distance 2 --after_clifford_depolarization 0.001 | stim --analyze_errors --fold_loops --decompose_errors`.
It includes coordinate annotations for the spacetime layout of the detectors.

```
error(0.0002667378157289137966) D0
error(0.001332444444444449679) D0 D2
error(0.0002667378157289137966) D0 L0
error(0.0005333333333331479603) D1
error(0.001332444444444449679) D1 D4
error(0.0002667378157289137966) D1 L0
error(0.0001333866998761607556) D1 L0 ^ D2 L0
error(0.0002667378157289137966) D1 L0 ^ D3
error(0.0001333866998761607556) D1 L0 ^ D3 ^ D2 L0
error(0.0002667378157289137966) D1 ^ D3
error(0.0005333333333331479603) D2
error(0.001332444444444449679) D2 D5
error(0.0007997866287252842132) D2 L0
error(0.0002667378157289137966) D2 L0 ^ D0 L0
error(0.0004000533570511300221) D2 L0 ^ D3
error(0.0002667378157289137966) D2 ^ D0
error(0.0001333866998761607556) D2 ^ D1
error(0.0001333866998761607556) D2 ^ D3
error(0.0001333866998761607556) D2 ^ D3 ^ D1
error(0.001731254257715537058) D3
error(0.0002667378157289137966) D3 ^ D1
error(0.0001333866998761607556) D3 ^ D2
error(0.0001333866998761607556) D3 ^ D2 L0
error(0.0001333866998761607556) D3 ^ D4
error(0.0001333866998761607556) D3 ^ D4 ^ D1
error(0.0001333866998761607556) D3 ^ D5
error(0.0001333866998761607556) D3 ^ D5 ^ D2
error(0.0004666977902291165391) D4
error(0.001332444444444449679) D4 D7
error(0.0003334000326131335203) D4 L0
error(0.0001333866998761607556) D4 L0 ^ D1 L0
error(0.0001333866998761607556) D4 L0 ^ D1 L0 ^ D3
error(0.0002000667052120994559) D4 L0 ^ D3
error(6.669779853440971351e-05) D4 L0 ^ D5 L0
error(6.669779853440971351e-05) D4 L0 ^ D5 L0 ^ D3
error(0.0002000667052120994559) D4 L0 ^ D6
error(6.669779853440971351e-05) D4 L0 ^ D6 ^ D3
error(6.669779853440971351e-05) D4 L0 ^ D6 ^ D5 L0
error(6.669779853440971351e-05) D4 L0 ^ D6 ^ D5 L0 ^ D3
error(0.0001333866998761607556) D4 ^ D1
error(0.0002000667052120994559) D4 ^ D3
error(0.0001333866998761607556) D4 ^ D6
error(0.0001333866998761607556) D4 ^ D6 ^ D3
error(0.0002000667052120994559) D5
error(0.0003334000326131335203) D5 L0
error(0.0001333866998761607556) D5 L0 ^ D2 L0
error(0.0001333866998761607556) D5 L0 ^ D2 L0 ^ D3
error(0.0003334000326131335203) D5 L0 ^ D3
error(0.0001333866998761607556) D5 L0 ^ D6
error(0.0001333866998761607556) D5 L0 ^ D6 ^ D3
error(0.0001333866998761607556) D5 ^ D2
error(6.669779853440971351e-05) D5 ^ D3
error(6.669779853440971351e-05) D5 ^ D4
error(6.669779853440971351e-05) D5 ^ D4 ^ D3
error(6.669779853440971351e-05) D5 ^ D6
error(6.669779853440971351e-05) D5 ^ D6 ^ D3
error(6.669779853440971351e-05) D5 ^ D6 ^ D4
error(6.669779853440971351e-05) D5 ^ D6 ^ D4 ^ D3
error(0.0006665777540627741476) D6
error(0.0004000533570511300221) D6 ^ D3
error(0.0002000667052120994559) D6 ^ D4
error(6.669779853440971351e-05) D6 ^ D4 ^ D3
error(6.669779853440971351e-05) D6 ^ D5 L0
error(6.669779853440971351e-05) D6 ^ D5 L0 ^ D3
error(0.0001333866998761607556) D6 ^ D7
error(0.0001333866998761607556) D6 ^ D7 ^ D4
error(0.0001333866998761607556) D7
error(0.0001333866998761607556) D7 L0
error(0.0001333866998761607556) D7 L0 ^ D4 L0
error(0.0001333866998761607556) D7 L0 ^ D4 L0 ^ D6
error(0.0001333866998761607556) D7 L0 ^ D6
error(0.0001333866998761607556) D7 ^ D4
detector(2, 0, 0) D0
detector(2, 4, 0) D1
shift_detectors(0, 0, 1) 0
detector(2, 0, 0) D2
detector(2, 2, 0) D3
detector(2, 4, 0) D4
repeat 498 {
    error(0.0001333866998761607556) D5
    error(0.001332444444444449679) D5 D8
    error(0.0001333866998761607556) D5 L0
    error(0.0001333866998761607556) D5 L0 ^ D6
    error(0.0006665777540627741476) D6
    error(0.0001333866998761607556) D6 ^ D5
    error(0.0001333866998761607556) D6 ^ D8
    error(0.0001333866998761607556) D6 ^ D8 ^ D5
    error(0.0003334000326131335203) D7
    error(0.001332444444444449679) D7 D10
    error(0.0002000667052120994559) D7 L0
    error(6.669779853440971351e-05) D7 L0 ^ D6
    error(6.669779853440971351e-05) D7 L0 ^ D8 L0
    error(6.669779853440971351e-05) D7 L0 ^ D8 L0 ^ D6
    error(0.0002000667052120994559) D7 L0 ^ D9
    error(6.669779853440971351e-05) D7 L0 ^ D9 ^ D6
    error(6.669779853440971351e-05) D7 L0 ^ D9 ^ D8 L0
    error(6.669779853440971351e-05) D7 L0 ^ D9 ^ D8 L0 ^ D6
    error(0.0002000667052120994559) D7 ^ D6
    error(0.0001333866998761607556) D7 ^ D9
    error(0.0001333866998761607556) D7 ^ D9 ^ D6
    error(0.0003334000326131335203) D8
    error(0.001332444444444449679) D8 D11
    error(0.0004666977902291165391) D8 L0
    error(0.0001333866998761607556) D8 L0 ^ D5 L0
    error(0.0001333866998761607556) D8 L0 ^ D5 L0 ^ D6
    error(0.0003334000326131335203) D8 L0 ^ D6
    error(0.0002667378157289137966) D8 L0 ^ D9
    error(0.0001333866998761607556) D8 L0 ^ D9 ^ D6
    error(0.0001333866998761607556) D8 ^ D5
    error(6.669779853440971351e-05) D8 ^ D6
    error(6.669779853440971351e-05) D8 ^ D7
    error(6.669779853440971351e-05) D8 ^ D7 ^ D6
    error(6.669779853440971351e-05) D8 ^ D9
    error(6.669779853440971351e-05) D8 ^ D9 ^ D6
    error(6.669779853440971351e-05) D8 ^ D9 ^ D7
    error(6.669779853440971351e-05) D8 ^ D9 ^ D7 ^ D6
    error(0.001332266856321125473) D9
    error(0.0004000533570511300221) D9 ^ D6
    error(0.0002000667052120994559) D9 ^ D7
    error(6.669779853440971351e-05) D9 ^ D7 ^ D6
    error(0.0001333866998761607556) D9 ^ D8
    error(6.669779853440971351e-05) D9 ^ D8 L0
    error(6.669779853440971351e-05) D9 ^ D8 L0 ^ D6
    error(0.0001333866998761607556) D9 ^ D10
    error(0.0001333866998761607556) D9 ^ D10 ^ D7
    error(0.0001333866998761607556) D9 ^ D11
    error(0.0001333866998761607556) D9 ^ D11 ^ D8
    error(0.0004666977902291165391) D10
    error(0.001332444444444449679) D10 D13
    error(0.0003334000326131335203) D10 L0
    error(0.0001333866998761607556) D10 L0 ^ D7 L0
    error(0.0001333866998761607556) D10 L0 ^ D7 L0 ^ D9
    error(0.0002000667052120994559) D10 L0 ^ D9
    error(6.669779853440971351e-05) D10 L0 ^ D11 L0
    error(6.669779853440971351e-05) D10 L0 ^ D11 L0 ^ D9
    error(0.0002000667052120994559) D10 L0 ^ D12
    error(6.669779853440971351e-05) D10 L0 ^ D12 ^ D9
    error(6.669779853440971351e-05) D10 L0 ^ D12 ^ D11 L0
    error(6.669779853440971351e-05) D10 L0 ^ D12 ^ D11 L0 ^ D9
    error(0.0001333866998761607556) D10 ^ D7
    error(0.0002000667052120994559) D10 ^ D9
    error(0.0001333866998761607556) D10 ^ D12
    error(0.0001333866998761607556) D10 ^ D12 ^ D9
    error(0.0002000667052120994559) D11
    error(0.0003334000326131335203) D11 L0
    error(0.0001333866998761607556) D11 L0 ^ D8 L0
    error(0.0001333866998761607556) D11 L0 ^ D8 L0 ^ D9
    error(0.0003334000326131335203) D11 L0 ^ D9
    error(0.0001333866998761607556) D11 L0 ^ D12
    error(0.0001333866998761607556) D11 L0 ^ D12 ^ D9
    error(0.0001333866998761607556) D11 ^ D8
    error(6.669779853440971351e-05) D11 ^ D9
    error(6.669779853440971351e-05) D11 ^ D10
    error(6.669779853440971351e-05) D11 ^ D10 ^ D9
    error(6.669779853440971351e-05) D11 ^ D12
    error(6.669779853440971351e-05) D11 ^ D12 ^ D9
    error(6.669779853440971351e-05) D11 ^ D12 ^ D10
    error(6.669779853440971351e-05) D11 ^ D12 ^ D10 ^ D9
    error(0.0006665777540627741476) D12
    error(0.0004000533570511300221) D12 ^ D9
    error(0.0002000667052120994559) D12 ^ D10
    error(6.669779853440971351e-05) D12 ^ D10 ^ D9
    error(6.669779853440971351e-05) D12 ^ D11 L0
    error(6.669779853440971351e-05) D12 ^ D11 L0 ^ D9
    error(0.0001333866998761607556) D12 ^ D13
    error(0.0001333866998761607556) D12 ^ D13 ^ D10
    error(0.0001333866998761607556) D13
    error(0.0001333866998761607556) D13 L0
    error(0.0001333866998761607556) D13 L0 ^ D10 L0
    error(0.0001333866998761607556) D13 L0 ^ D10 L0 ^ D12
    error(0.0001333866998761607556) D13 L0 ^ D12
    error(0.0001333866998761607556) D13 ^ D10
    shift_detectors(0, 0, 1) 0
    detector(2, 0, 0) D5
    detector(2, 2, 0) D6
    detector(2, 4, 0) D7
    shift_detectors(0, 0, 1) 0
    detector(2, 0, 0) D8
    detector(2, 2, 0) D9
    detector(2, 4, 0) D10
    shift_detectors 6
}
error(0.0001333866998761607556) D5
error(0.001332444444444449679) D5 D8
error(0.0001333866998761607556) D5 L0
error(0.0001333866998761607556) D5 L0 ^ D6
error(0.0006665777540627741476) D6
error(0.0001333866998761607556) D6 ^ D5
error(0.0001333866998761607556) D6 ^ D8
error(0.0001333866998761607556) D6 ^ D8 ^ D5
error(0.0003334000326131335203) D7
error(0.001332444444444449679) D7 D10
error(0.0002000667052120994559) D7 L0
error(6.669779853440971351e-05) D7 L0 ^ D6
error(6.669779853440971351e-05) D7 L0 ^ D8 L0
error(6.669779853440971351e-05) D7 L0 ^ D8 L0 ^ D6
error(0.0002000667052120994559) D7 L0 ^ D9
error(6.669779853440971351e-05) D7 L0 ^ D9 ^ D6
error(6.669779853440971351e-05) D7 L0 ^ D9 ^ D8 L0
error(6.669779853440971351e-05) D7 L0 ^ D9 ^ D8 L0 ^ D6
error(0.0002000667052120994559) D7 ^ D6
error(0.0001333866998761607556) D7 ^ D9
error(0.0001333866998761607556) D7 ^ D9 ^ D6
error(0.0003334000326131335203) D8
error(0.001332444444444449679) D8 D11
error(0.0004666977902291165391) D8 L0
error(0.0001333866998761607556) D8 L0 ^ D5 L0
error(0.0001333866998761607556) D8 L0 ^ D5 L0 ^ D6
error(0.0003334000326131335203) D8 L0 ^ D6
error(0.0002667378157289137966) D8 L0 ^ D9
error(0.0001333866998761607556) D8 L0 ^ D9 ^ D6
error(0.0001333866998761607556) D8 ^ D5
error(6.669779853440971351e-05) D8 ^ D6
error(6.669779853440971351e-05) D8 ^ D7
error(6.669779853440971351e-05) D8 ^ D7 ^ D6
error(6.669779853440971351e-05) D8 ^ D9
error(6.669779853440971351e-05) D8 ^ D9 ^ D6
error(6.669779853440971351e-05) D8 ^ D9 ^ D7
error(6.669779853440971351e-05) D8 ^ D9 ^ D7 ^ D6
error(0.001731254257715537058) D9
error(0.0004000533570511300221) D9 ^ D6
error(0.0002000667052120994559) D9 ^ D7
error(6.669779853440971351e-05) D9 ^ D7 ^ D6
error(0.0001333866998761607556) D9 ^ D8
error(6.669779853440971351e-05) D9 ^ D8 L0
error(6.669779853440971351e-05) D9 ^ D8 L0 ^ D6
error(0.0001333866998761607556) D9 ^ D10
error(0.0001333866998761607556) D9 ^ D10 ^ D7
error(0.0001333866998761607556) D9 ^ D11
error(0.0001333866998761607556) D9 ^ D11 ^ D8
error(0.0007997866287252842132) D10
error(0.001332444444444449679) D10 D12
error(0.0005333333333331479603) D10 L0
error(0.0001333866998761607556) D10 L0 ^ D7 L0
error(0.0001333866998761607556) D10 L0 ^ D7 L0 ^ D9
error(0.0002667378157289137966) D10 L0 ^ D9
error(0.0001333866998761607556) D10 L0 ^ D11 L0
error(0.0001333866998761607556) D10 L0 ^ D11 L0 ^ D9
error(0.0001333866998761607556) D10 ^ D7
error(0.0004000533570511300221) D10 ^ D9
error(0.0002667378157289137966) D11
error(0.0005333333333331479603) D11 L0
error(0.0001333866998761607556) D11 L0 ^ D8 L0
error(0.0001333866998761607556) D11 L0 ^ D8 L0 ^ D9
error(0.0005333333333331479603) D11 L0 ^ D9
error(0.0001333866998761607556) D11 ^ D8
error(0.0001333866998761607556) D11 ^ D9
error(0.0001333866998761607556) D11 ^ D10
error(0.0001333866998761607556) D11 ^ D10 ^ D9
error(0.0002667378157289137966) D12
error(0.0002667378157289137966) D12 L0
error(0.0002667378157289137966) D12 L0 ^ D10 L0
error(0.0002667378157289137966) D12 ^ D10
shift_detectors(0, 0, 1) 0
detector(2, 0, 0) D5
detector(2, 2, 0) D6
detector(2, 4, 0) D7
shift_detectors(0, 0, 1) 0
detector(2, 0, 0) D8
detector(2, 2, 0) D9
detector(2, 4, 0) D10
detector(2, 0, 1) D11
detector(2, 4, 1) D12
```
