# Stim

Stim is a fast simulator for quantum stabilizer circuits.
Stim is based on the stabilizer tableau representation introduced in
[Scott Aaronson et al's CHP simulator](https://arxiv.org/abs/quant-ph/0406196).
Stim makes three key improvements over CHP.

First, the stabilizer tableau that is being tracked is inverted.
The tableau tracked by Stim indexes how each qubit's X and Z observables at the current time map to compound observables
at the start of time (instead of mapping from the start of time to the current time).
This is done so that the sign of the tracked observables directly store the measurement result to return
when a measurement is deterministic.
As a result, deterministic measurements can be completed in linear time instead of quadratic time.

Second, when producing multiple samples, the initial stabilizer simulation is executed without noise in order
to create a reference sample.
Once a reference sample from the circuit is available, more samples can be derived by propagating Pauli frames through the circuit,
using the original sample as a template whose results are flipped or not flipped by the passing Pauli frame.
As long as all errors are probabilistic Pauli operations, and as long as 50/50 probability Z errors are placed after
every reset and every measurement, the derived samples will come from the same distribution as a full stabilizer simulation.
This ensures every gate has a worst case complexity of O(1), instead of O(n) or O(n^2).

Third, data is laid out in a cache friendly way and operated on using vectorized 256-bit-wide SIMD instructions.
This makes key operations fast.
For example, Stim can multiply a Pauli string with a *hundred billion terms* into another in *under a second*.
Pauli string multiplication is a key bottleneck operation when updating a stabilizer tableau.
Tracking Pauli frames can also benefit from vectorization, by batching the frames into groups of hundreds that are
all operated on simultaneously by individual CPU instructions.

# Data Formats

Circuits can be input using the [stim circuit file format (.stim)](doc/file_format_stim_circuit.md).

Samples can be output using [a variety of text and binary formats](doc/usage_command_line.md#out_format).

Error models can be output using the [detector error model file format (.dem)](doc/file_format_dem_detector_error_model.md).

# Supported Gates

See the [gate documentation](doc/gates.md).

# Usage (python)

See the [python documentation](glue/python/README.md).

# Usage (command line)

See the [command line documentation](doc/usage_command_line.md).

# Building the code

See the [developer documentation](doc/developer_documentation.md).
