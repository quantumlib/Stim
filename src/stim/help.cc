// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/help.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stim/circuit/circuit.h>

#include "stim/arg_parse.h"
#include "stim/circuit/gate_data.h"
#include "stim/io/stim_data_formats.h"
#include "stim/stabilizers/tableau.h"

using namespace stim;

struct CommandLineSingleModeData {
    std::string mode_summary;
    std::string mode_description;
    std::set<std::string> flags;
};

struct CommandLineFlagData {
    std::map<std::string, std::string> non_mode_help;
    std::map<std::string, CommandLineSingleModeData> mode_help;
};

CommandLineFlagData make_mode_help() {
    std::map<std::string, CommandLineSingleModeData> modes;
    std::map<std::string, std::string> flags;

    modes["help"] = CommandLineSingleModeData{
        "Prints helpful information about stim.",
        R"PARAGRAPH(
Use `stim help [topic]` for information about specific topics. Available topics include:

    stim help gates    # List all circuit instructions supported by stim.
    stim help formats  # List all result formats supported by stim.
    stim help modes    # List all tasks performed by stim.
    stim help flags    # List all command line flags supported by stim.
    stim help [mode]   # Print information about a mode, such as `sample` or `analyze_errors`.
    stim help [flag]   # Print information about a command line flag, such as `--out` or `--in_format`.
    stim help [gate]   # Print information about a circuit instruction, such as the `CNOT` gate.
    stim help [format] # Print information about a supported result format, such as the `01` format.
)PARAGRAPH",
        {}};

    modes["repl"] = CommandLineSingleModeData{
        "Read-eval-print-loop mode.",
        R"PARAGRAPH(
Reads operations from stdin while immediately writing measurement results to stdout.

stdin: A circuit to execute.

stdout: Measurement results.

stderr: Ignored errors encountered while parsing/simulating the circuit arriving via stdin.

- Example:

    ```
    >>> stim repl
    ... M 0
    0
    ... X 0
    ... M 0
    1
    ... X 2 3 9
    ... M 0 1 2 3 4 5 6 7 8 9
    1 0 1 1 0 0 0 0 0 1
    ... REPEAT 5 {
    ...     R 0 1
    ...     H 0
    ...     CNOT 0 1
    ...     M 0 1
    ... }
    00
    11
    11
    00
    11
    ```
)PARAGRAPH",
        {},
    };

    modes["sample"] = CommandLineSingleModeData{
        "Samples measurements from a circuit.",
        R"PARAGRAPH(
stdin: The circuit to sample from, specified using the [stim circuit file format](https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md).

stdout: The sample data.

- Examples:

    ```
    >>> stim sample --shots 5
    ... H 0
    ... CNOT 0 1
    ... M 0 1
    00
    11
    11
    00
    11
    ```

    ```
    >>> stim sample --out_format dets
    ... X 2 3 5
    ... M 0 1 2 3 4 5 6 7 8 9
    shot M2 M3 M5
    ```
)PARAGRAPH",
        {"--out_format", "--seed", "--in", "--out", "--skip_reference_sample", "--shots"},
    };

    modes["m2d"] = CommandLineSingleModeData{
        "Convert measurement data to detection event data.",
        R"PARAGRAPH(
Note that this conversion requires taking a reference sample from the circuit, in order to determine whether the
measurement sets defining the detectors and observables have an expected parity of 0 or an expected parity of 1.

stdin: The measurement data, in the format specified by --in_format.

stdout: The detection event data, in the format specified by --out_format (defaults to '01').

- Examples:

    ```
    >>> echo -e "X 0\nM 0 1\nDETECTOR rec[-2]\nDETECTOR rec[-1]\nOBSERVABLE_INCLUDE(2) rec[-1]" > tmp.stim
    >>> stim m2d --in_format 01 --out_format dets --circuit tmp.stim --append_observables
    ... 00
    ... 01
    ... 10
    ... 11
    shot D0
    shot D0 D1 L2
    shot
    shot D1 L2
    ```
)PARAGRAPH",
        {"--out_format", "--in", "--out", "--in_format", "--circuit", "--skip_reference_sample"},
    };

    flags["--circuit"] = R"PARAGRAPH(Specifies the circuit to use when converting measurement data to detector data.

The argument must be a filepath leading to a [stim circuit format file](https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md).
)PARAGRAPH";

    flags["--out_format"] = R"PARAGRAPH(Specifies a data format to use when writing shot data, e.g. `01` or `r8`.

Defaults to `01` when not specified.

See `stim help formats` for a list of supported formats.
)PARAGRAPH";

    flags["--in_format"] = R"PARAGRAPH(Specifies a data format to use when reading shot data, e.g. `01` or `r8`.

See `stim help formats` for a list of supported formats.
)PARAGRAPH";

    flags["--shots"] = R"PARAGRAPH(Specifies the number of times to run a circuit, producing data each time.

Defaults to 1.
Must be an integer between 0 and a quintillion (10^18).
)PARAGRAPH";
    flags["--skip_reference_sample"] = R"PARAGRAPH(Instead of computing a reference sample for the given circuit, use
a vacuous reference sample where where all measurement results are 0.

Skipping the reference sample can significantly improve performance, because acquiring the reference sample requires
using the tableau simulator. If the vacuous reference sample is actually a result that can be produced by the circuit,
under noiseless execution, then specifying this flag has no observable outcome other than improving performance.

When the all-zero sample isn't a result that can be produced by the circuit under noiseless execution, the effects of
skipping the reference sample vary depending on the mode. For example, in measurement sampling mode, the reported
measurements are not true measurement results but rather reports of which measurement results would have been flipped
due to errors or Heisenberg uncertainty. They need to be XOR'd against a noiseless reference sample to become true
measurement results.
)PARAGRAPH";

    modes["detect"] = CommandLineSingleModeData{
        "Samples detection events from a circuit.",
        R"PARAGRAPH(
stdin: The circuit, specified using the stim circuit file format, to sample detection events from.

stdout: The sampled detection event data. Each output bit corresponds to a `DETECTOR` instruction or (if
    `--append_observables` is specified) accumulated results from `OBSERVABLE_INCLUDE` instructions.

See also: `stim help DETECTOR` and `stim help OBSERVABLE_INCLUDE`.

- Examples:

    ```
    >>> stim detect --shots 5
    ... H 0
    ... CNOT 0 1
    ... X_ERROR(0.1) 0 1
    ... M 0 1
    ... DETECTOR rec[-1] rec[-2]
    0
    1
    0
    0
    0
    ```

    ```
    >>> stim detect --shots 10 --append_observables
    ... # Single-shot X-basis rep code circuit.
    ... RX 0 1 2 3 4 5 6
    ... MPP X0*X1 X1*X2 X2*X3 X3*X4 X4*X5 X5*X6
    ... Z_ERROR(0.1) 0 1 2 3 4 5 6
    ... MPP X0 X1 X2 X3 X4 X5 X6
    ... DETECTOR rec[-1] rec[-2] rec[-8]   # X6 X5 now = X5*X6 before
    ... DETECTOR rec[-2] rec[-3] rec[-9]   # X5 X4 now = X4*X5 before
    ... DETECTOR rec[-3] rec[-4] rec[-10]  # X4 X3 now = X3*X4 before
    ... DETECTOR rec[-4] rec[-5] rec[-11]  # X3 X2 now = X2*X3 before
    ... DETECTOR rec[-5] rec[-6] rec[-12]  # X2 X1 now = X1*X2 before
    ... DETECTOR rec[-6] rec[-7] rec[-13]  # X1 X0 now = X0*X1 before
    ... OBSERVABLE_INCLUDE(0) rec[-1]
    0110000
    0000000
    1000001
    0110010
    1100000
    0000010
    1000001
    0110000
    0000000
    0011000
    ```

    ```
    >>> stim detect --shots 10 --append_observables --out_format=dets
    ... # Single-shot X-basis rep code circuit.
    ... RX 0 1 2 3 4 5 6
    ... MPP X0*X1 X1*X2 X2*X3 X3*X4 X4*X5 X5*X6
    ... Z_ERROR(0.1) 0 1 2 3 4 5 6
    ... MPP X0 X1 X2 X3 X4 X5 X6
    ... DETECTOR rec[-1] rec[-2] rec[-8]   # X6 X5 now = X5*X6 before
    ... DETECTOR rec[-2] rec[-3] rec[-9]   # X5 X4 now = X4*X5 before
    ... DETECTOR rec[-3] rec[-4] rec[-10]  # X4 X3 now = X3*X4 before
    ... DETECTOR rec[-4] rec[-5] rec[-11]  # X3 X2 now = X2*X3 before
    ... DETECTOR rec[-5] rec[-6] rec[-12]  # X2 X1 now = X1*X2 before
    ... DETECTOR rec[-6] rec[-7] rec[-13]  # X1 X0 now = X0*X1 before
    ... OBSERVABLE_INCLUDE(0) rec[-1]
    shot D1 D2
    shot
    shot D0 L0
    shot D1 D2 D5
    shot D0 D1
    shot D5
    shot D0 L0
    shot D1 D2
    shot
    shot D2 D3
    ```
)PARAGRAPH",
        {"--out_format", "--seed", "--append_observables", "--in", "--out", "--shots"}};

    flags["--append_observables"] = R"PARAGRAPH(Treat observables as extra detectors at the end of the circuit.

By default, when reporting detection events, observables are not reported. This flag causes the observables to instead
be reported as if they were detectors. For example, if there are 100 detectors and 10 observables in the circuit, then
the output will contain 110 detectors and the last 10 are the observables. A notable exception to the "observables are
just extra detectors" behavior of this flag is that, when using `out_format=dets`, the observables are distinguished
from detectors by being named e.g. `L0` through `L9` instead of `D100` through `D109`.
)PARAGRAPH";

    flags["--seed"] = R"PARAGRAPH(Make simulation results PARTIALLY deterministic.

When not set, the random number generator is seeded with external system entropy.

When set to an integer, using the exact same other flags on the exact same machine with the exact
same version of Stim will produce the exact same simulation results.
The integer must be a non-negative 64 bit signed integer.

CAUTION: simulation results *WILL NOT* be consistent between versions of Stim. This restriction is
present to make it possible to have future optimizations to the random sampling, and is enforced by
introducing intentional differences in the seeding strategy from version to version.

CAUTION: simulation results *MAY NOT* be consistent across machines. For example, using the same
seed on a machine that supports AVX instructions and one that only supports SSE instructions may
produce different simulation results.

CAUTION: simulation results *MAY NOT* be consistent if you vary other flags and modes. For example,
`--skip_reference_sample` may result in fewer calls the to the random number generator before reported
sampling begins. More generally, using the same seed for `stim sample` and `stim detect` will not
result in detection events corresponding to the measurement results.
)PARAGRAPH";

    modes["analyze_errors"] = CommandLineSingleModeData{
        "Converts a circuit into a detector error model.",
        R"PARAGRAPH(
Determines the detectors and logical observables that are flipped by each error channel in the given circuit, and
summarizes this information as an error model framed entirely in terms of independent error mechanisms that flip sets of
detectors and observables.

stdin: The circuit to convert into a detector error model.

stdout: The detector error model in [detector error model file format](https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md).

stderr:
    Circuit failed to parse.
    Failed to produce a graphlike detector error model but `--decompose_errors` was set.
    Circuit contained gauge detectors but `--allow_gauge_detectors` wasn't set.
    Circuit contained disjoint error channels but `--approximate_disjoint_errors` wasn't set.

Note: currently, the `ELSE_CORRELATED_ERROR` instruction is not supported by this mode.

- Example:

    ```
    >>> stim analyze_errors
    ... # Single-shot X-basis rep code circuit.
    ... RX 0 1 2 3 4 5 6
    ... MPP X0*X1 X1*X2 X2*X3 X3*X4 X4*X5 X5*X6
    ... Z_ERROR(0.125) 0 1 2 3 4 5 6
    ... MPP X0 X1 X2 X3 X4 X5 X6
    ... DETECTOR rec[-1] rec[-2] rec[-8]   # X6 X5 now = X5*X6 before
    ... DETECTOR rec[-2] rec[-3] rec[-9]   # X5 X4 now = X4*X5 before
    ... DETECTOR rec[-3] rec[-4] rec[-10]  # X4 X3 now = X3*X4 before
    ... DETECTOR rec[-4] rec[-5] rec[-11]  # X3 X2 now = X2*X3 before
    ... DETECTOR rec[-5] rec[-6] rec[-12]  # X2 X1 now = X1*X2 before
    ... DETECTOR rec[-6] rec[-7] rec[-13]  # X1 X0 now = X0*X1 before
    ... OBSERVABLE_INCLUDE(0) rec[-1]
    error(0.125) D0 D1
    error(0.125) D0 L0
    error(0.125) D1 D2
    error(0.125) D2 D3
    error(0.125) D3 D4
    error(0.125) D4 D5
    error(0.125) D5
    ```
)PARAGRAPH",
        {
            "--allow_gauge_detectors",
            "--approximate_disjoint_errors",
            "--decompose_errors",
            "--fold_loops",
            "--out",
            "--in",
        },
    };

    flags["--fold_loops"] = R"PARAGRAPH(
Allows the output error model to contain `repeat` blocks.

Analyzes `REPEAT` blocks in the input circuit using a procedure that solves the loop in O(period) iterations, instead of
O(total_repetition_count) iterations, by using ["tortoise and hare"](https://en.wikipedia.org/wiki/Cycle_detection#Floyd's_tortoise_and_hare)
period finding algorithm. The "period" of a loop is the number of iterations required for the logical observables to end
up back in the same place and for any errors introduced in the current iteration to not affected any detectors defined
at least that many iterations later (including detectors after the end of the loop).

This flag substantially improves performance on circuits with `REPEAT` blocks that have large repetition counts. The
analysis will take less time and the output will be more compact.

Note that, although logical observables can "cross" the loop without preventing loop folding, detectors CANNOT. If there
is any detector introduced after the loop, whose sensitivity region extends to before the loop, loop folding will fail.
This is disastrous for loops with repetition counts in the billions, because in that case loop folding is the difference
between the error analysis finishing in seconds instead of days.
)PARAGRAPH";

    flags["--decompose_errors"] = R"PARAGRAPH(
Decomposes errors into components that are guaranteed to be "graphlike" (have at most two detection events).

Stim uses two strategies for decomposing errors: within-channel and other-error.

The *within-channel* strategy is always applied first, and works by looking at the various detector/observable sets
producing by each case of a single noise channel. If some cases are products of other cases, that product is used as
the decomposition. For example, suppose that a single qubit depolarizing channel has a `Y5` case that produces four
detection events `D0 D1 D2 D3`, an `X5` case that produces two detection events `D0 D1`, and a `Z5` case that produces
two detection events `D2 D3`. Because `D0 D1 D2 D3` is the combination of `D0 D1` and `D2 D3`, the `Y5` case will be
decomposed into `D0 D1 ^ D2 D3`.

The *other-error* strategy is used (as late as possible) while an error still has a component with more than two
detection events. It is checked of one or two of those detection events appear as an individual error elsewhere in the
model. If they do, they are split out of the component (decomposing it). This applies iteratively. For example, if an
error `D0 ^ D1 D2 D3` appears in the model, then stim will check if there is an error anywhere in the model that has
exactly `D1 D2`, `D1 D3`, `D2 D3`, `D1`, `D2`, or `D3` as its detection events. Suppose there is an error with `D1 D2`.
Then the original error will be decomposed into `D0 ^ D1 D2 ^ D3`.

If these strategies fail to decompose error into graphlike pieces, Stim will throw an error saying it failed to find a
satisfying decomposition.
)PARAGRAPH";

    flags["--allow_gauge_detectors"] = R"PARAGRAPH(
Normally, when a detector anti-commutes with a stabilizer of the circuit (forcing the detector
to have random results instead of deterministic results), error analysis throws an exception.

Specifying `--allow_gauge_detectors` instead allows this behavior and reports it as an `error(0.5)` in the model.

For example, in the following circuit, the two detectors are gauge detectors:

```
R 0
H 0
CNOT 0 1
M 0 1
DETECTOR rec[-1]
DETECTOR rec[-2]
```

Without `--allow_gauge_detectors`, stim will raise an exception when analyzing this circuit. With
`--allow_gauge_detectors`, stim will replace this exception with an `error(0.5) D1 D2` error mechanism in the output.
)PARAGRAPH";

    flags["--approximate_disjoint_errors"] = R"PARAGRAPH(
Specifies a threshold for allowing error mechanisms with disjoint components
(such as `PAULI_CHANNEL_1(0.1, 0.2, 0.0)`) to be approximated as having independent components.

Defaults to 0 (false) when not specified.
Defaults to 1 (true) when specified with an empty argument.
Must be set to a probability between 0 and 1.

If any of the component error probabilities (that will be approximated as independent) is larger than the given
threshold, error analysis will fail instead of performing the approximation.

For example, if `--approximate_disjoint_errors` is specified then a `PAULI_CHANNEL_1(0.1, 0.2, 0.0)` is
approximated as an `X_ERROR(0.1)` followed by a `Z_ERROR(0.2)`.
)PARAGRAPH";

    modes["gen"] = CommandLineSingleModeData{
        "Generates example circuits.",
        R"PARAGRAPH(
The generated circuits include annotations for noise, detectors, logical observables, the spacetial locations of qubits,
the spacetime locations of detectors, and the inexorable passage of time steps.

stdout: A circuit in [stim's circuit file format](https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md).

The type of circuit to generate is specified using the `--code` and `--task` flags. Each code supports different tasks.
Other information that must be specified is the number of `--rounds`, the `--distance`, and any desired noise.

- Example:

    ```
    >>> stim gen --code repetition_code --task memory --distance 3 --rounds 100 --after_clifford_depolarization 0.001
    # Generated repetition_code circuit.
    # task: memory
    # rounds: 100
    # distance: 3
    # before_round_data_depolarization: 0
    # before_measure_flip_probability: 0
    # after_reset_flip_probability: 0
    # after_clifford_depolarization: 0.001
    # layout:
    # L0 Z1 d2 Z3 d4
    # Legend:
    #     d# = data qubit
    #     L# = data qubit with logical observable crossing
    #     Z# = measurement qubit
    R 0 1 2 3 4
    TICK
    CX 0 1 2 3
    DEPOLARIZE2(0.001) 0 1 2 3
    TICK
    CX 2 1 4 3
    DEPOLARIZE2(0.001) 2 1 4 3
    TICK
    MR 1 3
    DETECTOR(1, 0) rec[-2]
    DETECTOR(3, 0) rec[-1]
    REPEAT 99 {
        TICK
        CX 0 1 2 3
        DEPOLARIZE2(0.001) 0 1 2 3
        TICK
        CX 2 1 4 3
        DEPOLARIZE2(0.001) 2 1 4 3
        TICK
        MR 1 3
        SHIFT_COORDS(0, 1)
        DETECTOR(1, 0) rec[-2] rec[-4]
        DETECTOR(3, 0) rec[-1] rec[-3]
    }
    M 0 2 4
    DETECTOR(1, 1) rec[-2] rec[-3] rec[-5]
    DETECTOR(3, 1) rec[-1] rec[-2] rec[-4]
    OBSERVABLE_INCLUDE(0) rec[-1]
    ```
)PARAGRAPH",
        {"--after_clifford_depolarization",
         "--after_reset_flip_probability",
         "--task",
         "--before_measure_flip_probability",
         "--before_round_data_depolarization",
         "--distance",
         "--out",
         "--in",
         "--rounds"}};

    flags["--code"] = R"PARAGRAPH(The error correcting code to use.

Supported codes are:

    `--code surface_code`
    `--code repetition_code`
    `--code color_code`
)PARAGRAPH";

    flags["--task"] = R"PARAGRAPH(What the generated circuit should do; the experiment it should run.

    Different error correcting codes support different tasks.

    `--task=memory` (repetition_code):
        Initialize a logical `|0>`,
        preserve it against noise for the given number of rounds,
        then measure.
    `--task=rotated_memory_x` (surface_code):
        Initialize a logical `|+>` in a rotated surface code,
        preserve it against noise for the given number of rounds,
        then measure in the X basis.
    `--task=rotated_memory_z` (surface_code):
        Initialize a logical `|0>` in a rotated surface code,
        preserve it against noise for the given number of rounds,
        then measure in the X basis.
    `--task=unrotated_memory_x` (surface_code):
        Initialize a logical `|+>` in an unrotated surface code,
        preserve it against noise for the given number of rounds,
        then measure in the Z basis.
    `--task=unrotated_memory_z` (surface_code):
        Initialize a logical `|0>` in an unrotated surface code,
        preserve it against noise for the given number of rounds,
        then measure in the Z basis.
    `--task=memory_xyz` (color_code):
        Initialize a logical `|0>`,
        preserve it against noise for the given number of rounds,
        then measure.
        Use a color code that alternates between measuring X, then Y, then Z stabilizers.
)PARAGRAPH";

    flags["--distance"] = R"PARAGRAPH(The minimum number of physical errors needed to cause a logical error.

The code distance determines how large the generated circuit has to be. Conventionally, the code distance specifically
refers to single-qubit errors between rounds instead of circuit errors during rounds.

The distance must always be a positive integer. Different codes/tasks may place additional constraints on the distance
(e.g. must be larger than 2 or must be odd or etc).
)PARAGRAPH";

    flags["--rounds"] = R"PARAGRAPH(The number of times the circuit's measurement qubits are measured.

The number of rounds must be an integer between 1 and a quintillion (10^18). Different codes/tasks may place additional
constraints on the number of rounds (e.g. enough rounds to have measured all the stabilizers at least once).
)PARAGRAPH";

    flags["--after_clifford_depolarization"] = R"PARAGRAPH(Adds depolarizing noise after Clifford operations.

Must be a probability between 0 and 1.
Defaults to 0.

Adds a `DEPOLARIZE1(p)` operation after every single-qubit Clifford operation and a `DEPOLARIZE2(p)` noise operation
after every two-qubit Clifford operation.
When the probability is set to 0, the noise operations are not inserted.
)PARAGRAPH";

    flags["--after_reset_flip_probability"] = R"PARAGRAPH(Specifies a reset noise level.

Defaults to 0 when not specified.
Must be a number between 0 and 1.

Adds an `X_ERROR(p)` after `R` (`RZ`) and `RY` operations, and a `Z_ERROR(p)` after `RX` operations.
When set to 0, the noise operations are not inserted.
)PARAGRAPH";

    flags["--before_measure_flip_probability"] = R"PARAGRAPH(Specifies a measurement noise level.

Defaults to 0 when not specified.
Must be a number between 0 and 1.

Adds an `X_ERROR(p)` before `M` (`MZ`) and `MY` operations, and a `Z_ERROR(p)` before `MX` operations.
When set to 0, the noise operations are not inserted.
)PARAGRAPH";

    flags["--before_round_data_depolarization"] = R"PARAGRAPH(Specifies a phenomenological noise level.

Defaults to 0 when not specified.
Must be a number between 0 and 1.

Adds a `DEPOLARIZE1(p)` operation to each data qubit at the start of each round of stabilizer measurements.
When set to 0, the noise operations are not inserted.
)PARAGRAPH";

    flags["--in"] = R"PARAGRAPH(Specifies an input file to read from, instead of stdin.

What the file is used for depends on the mode stim is executing in. For example, in `stim sample` mode the circuit to
sample from is read from stdin (or the file specified by `--in`) whereas in `--m2d` mode the measurement data to convert
is read from stdin (or the file specified by `--in`).
)PARAGRAPH";

    flags["--out"] = R"PARAGRAPH(Specifies an output file to read from, instead of stdout.

What the output is used for depends on the mode stim is executing in. For example, in `stim gen` mode the generated circuit
is written to stdout (or the file specified by `--out`) whereas in `stim sample` mode the sampled measurement data is
written to stdout (or the file specified by `--out`).
)PARAGRAPH";

    return {flags, modes};
}

std::string upper(const std::string &val) {
    std::string copy = val;
    for (char &c : copy) {
        c = toupper(c);
    }
    return copy;
}

struct Acc {
    std::string settled;
    std::stringstream working;
    int indent{};

    void flush() {
        auto s = working.str();
        for (char c : s) {
            settled.push_back(c);
            if (c == '\n') {
                for (int k = 0; k < indent; k++) {
                    settled.push_back(' ');
                }
            }
        }
        working.str("");
    }

    void change_indent(int t) {
        flush();
        if (indent + t < 0) {
            throw std::out_of_range("negative indent");
        }
        indent += t;
        working << '\n';
    }

    template <typename T>
    Acc &operator<<(const T &other) {
        working << other;
        return *this;
    }
};

void print_fixed_width_float(Acc &out, float f, char u) {
    if (f == 0) {
        out << "  ";
    } else if (fabs(f - 1) < 0.0001) {
        out << "+" << u;
    } else if (fabs(f + 1) < 0.0001) {
        out << "-" << u;
    } else {
        if (f > 0) {
            out << "+";
        }
        out << f;
    }
}

void print_example(Acc &out, const char *name, const Gate &gate) {
    out << "\n- Example:\n";
    out.change_indent(+4);
    out << "```\n";
    for (size_t k = 0; k < 3; k++) {
        out << name;
        if ((gate.flags & GATE_IS_NOISE) || (k == 2 && (gate.flags & GATE_PRODUCES_NOISY_RESULTS))) {
            out << "(" << 0.001 << ")";
        }
        if (k != 1) {
            out << " " << 5;
            if (gate.flags & GATE_TARGETS_PAIRS) {
                out << " " << 6;
            }
        }
        if (k != 0) {
            out << " ";
            if (gate.flags & GATE_PRODUCES_NOISY_RESULTS) {
                out << "!";
            }
            out << 42;
            if (gate.flags & GATE_TARGETS_PAIRS) {
                out << " " << 43;
            }
        }
        out << "\n";
    }
    if (gate.flags & GATE_CAN_TARGET_BITS) {
        if (gate.name[0] == 'C' || gate.name[0] == 'Z') {
            out << gate.name << " rec[-1] 111\n";
        }
        if (gate.name[gate.name_len - 1] == 'Z') {
            out << gate.name << " 111 rec[-1]\n";
        }
    }
    out << "```\n";
    out.change_indent(-4);
}

void print_decomposition(Acc &out, const Gate &gate) {
    const char *decomposition = gate.extra_data_func().h_s_cx_m_r_decomposition;
    if (decomposition != nullptr) {
        std::stringstream undecomposed;
        undecomposed << gate.name << " 0";
        if (gate.flags & GATE_TARGETS_PAIRS) {
            undecomposed << " 1";
        }

        out << "- Decomposition (into H, S, CX, M, R):\n";
        out.change_indent(+4);
        out << "```\n";
        out << "# The following circuit is equivalent (up to global phase) to `";
        out << undecomposed.str() << "`";
        out << decomposition;
        if (Circuit(decomposition) == Circuit(undecomposed.str().data())) {
            out << "\n# (The decomposition is trivial because this gate is in the target gate set.)\n";
        }
        out << "```\n";
        out.change_indent(-4);
    }
}

void print_stabilizer_generators(Acc &out, const Gate &gate) {
    if (gate.flags & GATE_IS_UNITARY) {
        out << "- Stabilizer Generators:\n";
        out.change_indent(+4);
        out << "```\n";
        auto tableau = gate.tableau();
        if (gate.flags & GATE_TARGETS_PAIRS) {
            out << "X_ -> " << tableau.xs[0] << "\n";
            out << "Z_ -> " << tableau.zs[0] << "\n";
            out << "_X -> " << tableau.xs[1] << "\n";
            out << "_Z -> " << tableau.zs[1] << "\n";
        } else {
            out << "X -> " << tableau.xs[0] << "\n";
            out << "Z -> " << tableau.zs[0] << "\n";
        }
        out << "```\n";
        out.change_indent(-4);
    } else {
        auto data = gate.extra_data_func();
        if (data.tableau_data.size()) {
            out << "- Stabilizer Generators:\n";
            out.change_indent(+4);
            out << "```\n";
            for (const auto &e : data.tableau_data) {
                out << e << "\n";
            }
            out << "```\n";
            out.change_indent(-4);
        }
    }
}

void print_bloch_vector(Acc &out, const Gate &gate) {
    if (!(gate.flags & GATE_IS_UNITARY) || (gate.flags & GATE_TARGETS_PAIRS)) {
        return;
    }

    out << "- Bloch Rotation:\n";
    out.change_indent(+4);
    out << "```\n";
    auto matrix = gate.unitary();
    auto a = matrix[0][0];
    auto b = matrix[0][1];
    auto c = matrix[1][0];
    auto d = matrix[1][1];
    auto i = std::complex<float>{0, 1};
    auto x = b + c;
    auto y = b * i + c * -i;
    auto z = a - d;
    auto s = a + d;
    s *= -i;
    std::complex<double> p = 1;
    if (s.imag() != 0) {
        p = s;
    }
    if (x.imag() != 0) {
        p = x;
    }
    if (y.imag() != 0) {
        p = y;
    }
    if (z.imag() != 0) {
        p = z;
    }
    p /= sqrt(p.imag() * p.imag() + p.real() * p.real());
    p *= 2;
    x /= p;
    y /= p;
    z /= p;
    s /= p;
    assert(x.imag() == 0);
    assert(y.imag() == 0);
    assert(z.imag() == 0);
    assert(s.imag() == 0);
    auto rx = x.real();
    auto ry = y.real();
    auto rz = z.real();
    auto rs = s.real();
    auto angle = (int)round(acosf(rs) * 360 / 3.14159265359);
    if (angle > 180) {
        angle -= 360;
    }
    out << "Axis: ";
    if (rx != 0) {
        out << "+-"[rx < 0] << 'X';
    }
    if (ry != 0) {
        out << "+-"[rx < 0] << 'Y';
    }
    if (rz != 0) {
        out << "+-"[rx < 0] << 'Z';
    }
    out << "\n";
    out << "Angle: " << angle << " degrees\n";
    out << "```\n";
    out.change_indent(-4);
}

void print_unitary_matrix(Acc &out, const Gate &gate) {
    if (!(gate.flags & GATE_IS_UNITARY)) {
        return;
    }
    auto matrix = gate.unitary();
    out << "- Unitary Matrix:\n";
    out.change_indent(+4);
    bool all_halves = true;
    bool all_sqrt_halves = true;
    double s = sqrt(0.5);
    for (const auto &row : matrix) {
        for (const auto &cell : row) {
            all_halves &= cell.real() == 0.5 || cell.real() == 0 || cell.real() == -0.5;
            all_halves &= cell.imag() == 0.5 || cell.imag() == 0 || cell.imag() == -0.5;
            all_sqrt_halves &= fabs(fabs(cell.real()) - s) < 0.001 || cell.real() == 0;
            all_sqrt_halves &= fabs(fabs(cell.imag()) - s) < 0.001 || cell.imag() == 0;
        }
    }
    out << "```\n";
    double factor = all_halves ? 2 : all_sqrt_halves ? 1 / s : 1;
    bool first_row = true;
    for (const auto &row : matrix) {
        if (first_row) {
            first_row = false;
        } else {
            out << "\n";
        }
        out << "[";
        bool first = true;
        for (const auto &cell : row) {
            if (first) {
                first = false;
            } else {
                out << ", ";
            }
            print_fixed_width_float(out, cell.real() * factor, '1');
            print_fixed_width_float(out, cell.imag() * factor, 'i');
        }
        out << "]";
    }
    if (all_halves) {
        out << " / 2";
    }
    if (all_sqrt_halves) {
        out << " / sqrt(2)";
    }
    out << "\n```\n";
    out.change_indent(-4);
}

std::string generate_per_gate_help_markdown(const Gate &alt_gate, int indent, bool anchor) {
    Acc out;
    out.indent = indent;
    const Gate &gate = GATE_DATA.at(alt_gate.name);
    if (anchor) {
        out << "<a name=\"" << alt_gate.name << "\"></a>";
    }
    out << "**`" << alt_gate.name << "`**\n";
    for (const auto &other : GATE_DATA.gates()) {
        if (other.id == alt_gate.id && other.name != alt_gate.name) {
            out << "\nAlternate name: ";
            if (anchor) {
                out << "<a name=\"" << other.name << "\"></a>";
            }
            out << "`" << other.name << "`\n";
        }
    }
    auto data = gate.extra_data_func();
    out << data.help;
    if (gate.flags & GATE_PRODUCES_NOISY_RESULTS) {
        out << "If this gate is parameterized by a probability argument, the "
               "recorded result will be flipped with that probability. "
               "If not, the recorded result is noiseless. "
               "Note that the noise only affects the recorded result, not the "
               "target qubit's state.\n\n";
        out << "Prefixing a target with ! inverts its recorded measurement result.\n";
    }

    if (std::string(data.help).find("xample:\n") == std::string::npos) {
        print_example(out, alt_gate.name, gate);
    }
    print_stabilizer_generators(out, gate);
    print_bloch_vector(out, gate);
    print_unitary_matrix(out, gate);
    print_decomposition(out, gate);
    out.flush();
    return out.settled;
}

std::string generate_per_mode_markdown(
    const std::string &mode_name, const CommandLineSingleModeData &data, int indent, bool anchor) {
    Acc out;
    out.indent = indent;
    if (anchor) {
        out << "<a name=\"" << mode_name << "\"></a>\n";
    }
    out << "### stim " << mode_name << "\n\n";
    out << "*" << data.mode_summary << "*\n";
    out << data.mode_description;
    if (!data.flags.empty()) {
        out << "\nFlags used with this mode:\n";
        for (const auto &e : data.flags) {
            if (anchor) {
                out << "- [" << e << "](#" << e << ")\n";
            } else {
                out << "    " << e << "\n";
            }
        }
    }

    out.flush();
    return out.settled;
}

std::string generate_per_flag_markdown(const std::string &flag_name, const std::string &desc, int indent, bool anchor) {
    Acc out;
    out.indent = indent;
    if (anchor) {
        out << "<a name=\"" << flag_name << "\"></a>";
    }
    out << "**`" << flag_name << "`**\n";
    out << desc;
    out << "\n";

    out.flush();
    return out.settled;
}

std::string generate_per_format_markdown(const FileFormatData &format_data, int indent, bool anchor) {
    Acc out;
    out.indent = indent;
    if (anchor) {
        out << "<a name=\"" << format_data.name << "\"></a>";
    }
    out << "**`" << format_data.name << "`**\n";
    out << format_data.help;
    out << "\n";

    out << "- Sample parsing code (python):\n";
    out.change_indent(+4);
    out << "```python";
    out << format_data.help_python_parse;
    out << "```\n";
    out.change_indent(-4);

    out << "- Sample saving code (python):\n";
    out.change_indent(+4);
    out << "```python";
    out << format_data.help_python_save;
    out << "```\n";
    out.change_indent(-4);

    out.flush();
    return out.settled;
}

std::map<std::string, std::string> generate_format_help_markdown() {
    std::map<std::string, std::string> result;

    std::stringstream all;
    all << "Result data formats supported by Stim\n";
    all << "=====================================\n";
    for (const auto &kv : format_name_to_enum_map) {
        all << kv.first << "\n";
    }
    result[std::string("FORMATS")] = all.str();

    for (const auto &kv : format_name_to_enum_map) {
        result[upper(kv.first)] = generate_per_format_markdown(kv.second, 0, false);
    }

    all.str("");
    all << R"MARKDOWN(# Result formats supported by Stim

)MARKDOWN";
    for (const auto &kv : format_name_to_enum_map) {
        all << "- [" << kv.first << "](#" << kv.first << ")\n";
    }
    all << "\n";
    for (const auto &kv : format_name_to_enum_map) {
        all << "- " << generate_per_format_markdown(kv.second, 4, true) << "\n";
    }
    result[std::string("FORMATS_MARKDOWN")] = all.str();

    return result;
}

std::map<std::string, std::string> generate_flag_help_markdown() {
    std::map<std::string, std::string> result;

    CommandLineFlagData data = make_mode_help();

    std::stringstream markdown;

    markdown << "# Stim command line reference\n\n";
    markdown << "## Index\n\n";
    for (const auto &kv : data.mode_help) {
        markdown << "- **(mode)** [stim " << kv.first << "](#" << kv.first << ")\n";
        for (const auto &e : kv.second.flags) {
            markdown << "    - [" << e << "](#" << e << ")\n";
        }
    }

    markdown << "## Modes\n\n";
    for (const auto &kv : data.mode_help) {
        std::string key = upper(kv.first);
        while (true) {
            result[key] = generate_per_mode_markdown(kv.first, kv.second, 0, false);
            if (key[0] == '-') {
                key.erase(key.begin());
            } else {
                break;
            }
        }
        markdown << generate_per_mode_markdown(kv.first, kv.second, 0, true) << "\n";
    }
    markdown << "## Flags\n\n";
    for (const auto &kv : data.non_mode_help) {
        std::string key = upper(kv.first);
        while (true) {
            result[key] = generate_per_flag_markdown(kv.first, kv.second, 0, false);
            if (key[0] == '-') {
                key.erase(key.begin());
            } else {
                break;
            }
        }
        markdown << "- " << generate_per_flag_markdown(kv.first, kv.second, 4, true) << "\n";
    }
    result["FLAGS_MARKDOWN"] = markdown.str();

    std::stringstream flags;
    flags << "Available stim commands:\n\n";
    for (const auto &kv : data.mode_help) {
        flags << "    stim " << kv.first << std::string(20 - kv.first.size(), ' ') << "# " << kv.second.mode_summary
              << "\n";
    }
    result["MODES"] = flags.str();
    flags << "\nOther flags:\n";
    for (const auto &kv : data.non_mode_help) {
        flags << "    " << kv.first << "\n";
    }
    result["FLAGS"] = flags.str();

    result[""] = result["MODES"] + "\n" + data.mode_help["help"].mode_description;

    return result;
}

std::map<std::string, std::string> generate_gate_help_markdown() {
    std::map<std::string, std::string> result;
    for (const auto &g : GATE_DATA.gates()) {
        result[g.name] = generate_per_gate_help_markdown(g, 0, false);
    }

    std::map<std::string, std::set<std::string>> categories;
    for (const auto &g : GATE_DATA.gates()) {
        const auto &rep = GATE_DATA.at(g.name);
        categories[std::string(rep.extra_data_func().category)].insert(g.name);
    }

    std::stringstream all;
    all << "Gates supported by Stim\n";
    all << "=======================\n";
    for (auto &category : categories) {
        all << category.first.substr(2) << ":\n";
        for (const auto &name : category.second) {
            all << "    " << name << "\n";
        }
    }

    result["GATES"] = all.str();

    all.str("");
    all << "# Gates supported by Stim\n\n";
    for (auto &category : categories) {
        all << "- " << category.first.substr(2) << "\n";
        for (const auto &name : category.second) {
            all << "    - [" << name << "](#" << name << ")\n";
        }
    }
    all << "\n";
    for (auto &category : categories) {
        all << "## " << category.first.substr(2) << "\n\n";
        for (const auto &name : category.second) {
            if (name == GATE_DATA.at(name).name) {
                all << "- " << generate_per_gate_help_markdown(GATE_DATA.at(name), 4, true) << "\n";
            }
        }
    }
    result[std::string("GATES_MARKDOWN")] = all.str();

    return result;
}

std::string stim::help_for(std::string help_key) {
    auto m1 = generate_gate_help_markdown();
    auto m2 = generate_format_help_markdown();
    auto m3 = generate_flag_help_markdown();

    auto key = upper(help_key);
    auto p = m1.find(key);
    if (p == m1.end()) {
        p = m2.find(key);
        if (p == m2.end()) {
            p = m3.find(key);
            if (p == m3.end()) {
                return "";
            }
        }
    }
    return p->second;
}

int stim::main_help(int argc, const char **argv) {
    const char *help = find_argument("--help", argc, argv);
    if (help == nullptr) {
        help = "\0";
    }
    if (help[0] == '\0' && argc == 3) {
        help = argv[2];
        // Handle usage like "stim sample --help".
        if (strcmp(help, "help") == 0 || strcmp(help, "--help") == 0) {
            help = argv[1];
        }
    }

    auto msg = help_for(help);
    if (msg == "") {
        std::cerr << "Unrecognized help topic '" << help << "'.\n";
        return EXIT_FAILURE;
    }
    std::cout << msg;
    return EXIT_SUCCESS;
}
