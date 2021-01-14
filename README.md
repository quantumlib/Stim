# Stim

Stim is a fast quantum stabilizer circuit simulator.

Stim uses a stabilizer tableau representation, very similar to [Scott Aaronson's CHP simulator](https://arxiv.org/abs/quant-ph/0406196),
but with two major improvements.

First, the tableau is inverted.
The tableau indexes how each qubit's X and Z observables at the current time map to compound observables at the start of time
(instead of mapping from the start of time to the current time).
This is done so that the sign of the current-time observables is always known.
As a result, deterministic measurements can be resolved in linear time instead of quadratic time.

Second, the tableau is operated on using 256-bit-wide SIMD instructions.
This makes basic building block operations extremely fast.
For example, applying a CNOT operation to a ten thousand qubit tableau takes less than a microsecond.
The tableau is transposed as needed to keep operations fast: column major order for unitary operations and row major
order for non-deterministic measurements.
(The overhead of transposing means that non-deterministic measurements are more efficient when grouped together.)

# Usage

Stim reads a quantum circuit from `stdin` and writes measurement results to `stdout`.
The input format is a series of lines.
Each line may start with a command, such as `CNOT 0 1`, and may end with a comment prefixed by `#`.
The output format is a series of lines, with each line containing either `0` or `1` indicating a measurement result.
The order of the results is the same as the order of the measurement commands in the input.
Measurements are not printed immediately; they are only flushed when the input ends or when the command `TICK` is
encountered.

Here is an example of using stim from the command line:

```bash
$ echo -e "H 0\nCNOT 0 1\nM 0\n M 1\n" | stim
1
1
```

Here is a list of the supported no-qubit commands:

```
TICK
```

Here is a list of the supported single qubit gates:

```
# Identity gate
I
# Pauli gates
X
Y
Z
# Axis exchange gates
H  # alternate name H_XZ
H_XY
H_YZ
# Quarter turns.
SQRT_X
SQRT_X_DAG
SQRT_Y
SQRT_Y_DAG
SQRT_Z      # alternate name S
SQRT_Z_DAG  # alternate name S_DAG
```

Here is a list of the supported two qubit gates:

```
# Swap gates.
SWAP
ISWAP
ISWAP_DAG
# Controlled gates.
CX  # alternate name CNOT
CY
CZ
# Controlled gates with X or Y basis controls.
XCX
XCY
XCZ
YCX
YCY
YCZ
```

# Building

```bash
cmake .
make stim
# output file: out/stim
```

# Testing

```bash
cmake .
make stim_tests
./out/stim_tests
```

# Manual Build

```bash
ls | grep "\\.cc" | grep -v "\\.test\\.cc" | xargs g++ -pthread -std=c++20 -march=native -O3
# output file: ./a.out
```
