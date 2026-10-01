# Stimside Simulator

## tl;dr

Stimside aims to enable classical state transition, tracking and conditioning that can be used to emulate leakage and loss events on top of either the flip simulator or the tableau simulator in Stim. The flip simulator wrapper is called flipside, and the tableau simulator wrapper is called tableside.
It's based on and heavily intertwined with Stim and Sinter.

## Setup & Installation

From the `Stim/glue/stimside` directory, run a single command to compile the C++ kernel shared libraries (`libcoset_kernels.so`, `libtableside_kernels.so`, and `libmarginal_dem_kernels.so`) and install the `stimside` Python package:

```bash
# Editable development install (compiles C++ kernels in-place and links Python package):
pip install -e .

# Or standard wheel install:
pip install .
```

## Running Tests

After installation, run the test suite with `pytest`:

```bash
pytest
```

## Behavioral Semantics

### Conditioning on Computational States (`0` and `1`)
When a leakage tag conditions on the computational basis states `0` or `1` (for example in `CONDITIONED_ON`, `CONDITIONED_ON_SELF`, `CONDITIONED_ON_OTHERS`, `LEAKAGE_TRANSITION_1`, `LEAKAGE_TRANSITION_2`, `LEAKAGE_PROJECTION_Z`, or `LEAKAGE_MEASUREMENT`) in `TablesideSimulator` or `CosetsideSimulator`, inspecting whether an unleaked qubit is in `|0>` or `|1>` performs a projective check in the computational ($Z$) basis. If the qubit is in a $Z$-basis superposition (e.g. `|+>` or part of a Bell pair), conditioning on `0` or `1` **collapses the quantum state** in the $Z$ basis according to the Born rule (preserving stabilizer entanglement with any partner qubits) before evaluating the condition or transition. To condition only on whether a qubit is in the computational subspace without collapsing superpositions, condition on `U` (unleaked) instead of `0` or `1`.

### Instructions with Repeated Qubit Targets
Instructions that reference the same qubit more than once in a single instruction (for example `CX 0 1 1 0`, `H 0 0`, or tagged leakage operations with repeated targets) are **processed sequentially in target order** (left to right), matching Stim's execution semantics.

### `LEAKAGE_PROJECTION_Z` and `MPAD` Support
- `LEAKAGE_PROJECTION_Z` (and its alias `LEAKAGE_MEASUREMENT`) is supported across single-qubit projective and reset measurements in the $Z$, $X$, and $Y$ bases: `M`, `MZ`, `MR`, `MRZ`, `MX`, `MY`, `MRX`, and `MRY`.
  - For $Z$-basis measurements (`M`, `MZ`, `MR`, `MRZ`), unleaked qubits (`state < 2`) condition on their collapsed computational basis state (`0` or `1`) before applying the confusion probability `p(0)` or `p(1)`.
  - For $X$- and $Y$-basis measurements (`MX`, `MY`, `MRX`, `MRY`), unleaked qubits measure in their native $X$ or $Y$ basis and condition on the resulting binary measurement outcome (`0` for the $+1$ eigenstate, `1` for the $-1$ eigenstate) when applying `p(0)` or `p(1)`, while leaked qubits (`state >= 2`) report `1` with probability `p(state)`. For reset variants (`MR`, `MRZ`, `MRX`, `MRY`), the computational proxy state is reset to the $+1$ eigenstate of the measurement basis (`|0>`, `|+>`, or `|+i>`), while leaked classical state (`state >= 2`) is preserved unless explicitly reset by a leakage transition tag.
- `MPAD` is supported in `TablesideSimulator`, `CosetsideSimulator`, and `FlipsideSimulator`, including bare `MPAD 0 1`, noisy `MPAD(p) 0 1`, and tagged `MPAD[LEAKAGE_MEASUREMENT: ...] 0 1`.

### Heralded Channels and Pauli Product / Pair Measurements (`HERALDED_ERASE`, `HERALDED_PAULI_CHANNEL_1`, `MPP`, `MXX`, `MYY`, `MZZ`)
- `HERALDED_ERASE` and `HERALDED_PAULI_CHANNEL_1` are supported across `TablesideSimulator`, `CosetsideSimulator`, and `FlipsideSimulator`, appending one herald bit per target to the measurement record (`True` when an erasure/Pauli event is heralded, `False` otherwise).
- `MPP`, `MXX`, `MYY`, and `MZZ` are supported across all three simulators.
- **Behavior when a qubit in `MPP`, `MXX`, `MYY`, or `MZZ` is leaked (`state >= 2`):** Unlike single-qubit `M`/`MR` without tags (where `TablesideSimulator`'s untagged gate filter previously skipped leaked qubits), product and pair measurements (`MPP`, `MXX`, `MYY`, `MZZ`) **never drop measurement records** when one or more participating qubits are leaked. Instead, all three simulators (`TablesideSimulator`, `CosetsideSimulator`, and `FlipsideSimulator`) preserve the measurement record alignment and evaluate the joint Pauli product measurement on the underlying computational stabilizer state (where any leaked qubit has been maximally depolarized upon leaking or participating in entangling gates).

### `TablesideSimulator` and `TablesideSampler` (`batch_size > 1`)
`TablesideSimulator` and `TablesideSampler` support `batch_size > 1`. In standard mode (`sync_tableside_rng=False`), each batch runs a **single** tableau trajectory with a **single** classical leakage sample to build a conditioned `stim.Circuit` (`_new_circuit`), and then generates `batch_size` shots from `_new_circuit` via Stim's fast Pauli-frame randomization (`compile_sampler` / `compile_detector_sampler`).

> [!CAUTION]
> **Shared Leakage Trajectory Within a Batch (`TablesideSimulator(..., batch_size > 1)`):** Because standard `TablesideSimulator` (`sync_tableside_rng=False`) samples classical leakage transitions once per `run()` call and uses Pauli-frame randomization on `_new_circuit` to produce the `batch_size` shots, **all shots within the same batch share the exact same leakage events and leakage state history**. Only Pauli errors and measurement outcomes conditioned on that fixed leakage realization vary across the `batch_size` shots. To sample independent leakage trajectories for every shot in `TablesideSimulator`, either use `batch_size=1` (where `TablesideSampler` runs independent batches) or set `sync_tableside_rng=True` (which executes an independent tableau and leakage trajectory per shot `b` seeded with `seed + b`).


### Synchronized Shot-for-Shot Equivalence (`sync_tableside_rng=True`)
Passing `sync_tableside_rng=True` to `TablesideSimulator` and `CosetsideSimulator` (with the same integer `seed` and `batch_size`) synchronizes all stabilizer measurement collapses, noise channels, herald draws, and leakage transition alias-table draws per shot (`seed + b`), producing identical shot-for-shot measurement records, detector flips, observable flips, and `unleaked_to_leaked` event indices across both simulators. This guarantee assumes each instruction lists distinct targets: under sync, duplicate targets in `LEAKAGE_PROJECTION_Z` measurements raise `NotImplementedError` when readout is noisy or any target is leaked. Duplicate targets in conditioned noise, and out-of-order targets in untagged X/Y-basis measurements and resets, are not guaranteed to match shot-for-shot.

