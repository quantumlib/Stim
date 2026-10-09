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

`FlipsideSimulator` does the same for `0`/`1` inputs of `LEAKAGE_TRANSITION_1` and `LEAKAGE_TRANSITION_2`, and for `0`/`1` keys of `MPAD[LEAKAGE_MEASUREMENT: ...]` (see the `MPAD` notes below): each shot's $Z$ value is the noiseless reference value XOR its X frame bit, and a target that is superposed in the noiseless reference is collapsed there (to `0`), with a 50% $Z$ kick on the shot to model the measurement back-action.

### Transitions into `0` and `1`
In all three simulators, a `LEAKAGE_TRANSITION_1`/`LEAKAGE_TRANSITION_2` output of `0` or `1` resets the qubit into that $Z$ eigenstate (like `R`, or `R` then `X`), unleaking it without depolarizing it, whatever its input state. A leaked qubit transitioning to `X`, `Y` or `Z` in `LEAKAGE_TRANSITION_2` is first reset to `|0>` and then has that Pauli applied. `FlipsideSimulator` can only represent such a reset when the target's $Z$ value is definite in the noiseless circuit (or collapsed by a `0`/`1` input key of the same instruction); otherwise it raises a `ValueError`.

### Instructions with Repeated Qubit Targets
Instructions that reference the same qubit more than once in a single instruction (for example `CX 0 1 1 0`, `H 0 0`, or tagged leakage operations with repeated targets) are **processed sequentially in target order** (left to right), matching Stim's execution semantics.

### `LEAKAGE_PROJECTION_Z` and `MPAD` Support
- `LEAKAGE_PROJECTION_Z` is supported across single-qubit projective and reset measurements in the $Z$, $X$, and $Y$ bases: `M`, `MZ`, `MR`, `MRZ`, `MX`, `MY`, `MRX`, and `MRY`.
  - For $Z$-basis measurements (`M`, `MZ`, `MR`, `MRZ`), unleaked qubits (`state < 2`) condition on their collapsed computational basis state (`0` or `1`) before applying the confusion probability `p(0)` or `p(1)`.
  - For $X$- and $Y$-basis measurements (`MX`, `MY`, `MRX`, `MRY`), unleaked qubits measure in their native $X$ or $Y$ basis and condition on the resulting binary measurement outcome (`0` for the $+1$ eigenstate, `1` for the $-1$ eigenstate) when applying `p(0)` or `p(1)`, while leaked qubits (`state >= 2`) report `1` with probability `p(state)`. For reset variants (`MR`, `MRZ`, `MRX`, `MRY`), the computational proxy state of unleaked targets is reset to the $+1$ eigenstate of the measurement basis (`|0>`, `|+>`, or `|+i>`). Leaked targets keep their leaked classical state (`state >= 2`) unless explicitly reset by a leakage transition tag, and their computational proxy state is fully depolarized rather than reset, as after any `LEAKAGE_PROJECTION_Z` measurement.
- `MPAD` is supported in `TablesideSimulator`, `CosetsideSimulator`, and `FlipsideSimulator`, including bare `MPAD 0 1`, noisy `MPAD(p) 0 1`, and tagged `MPAD[LEAKAGE_MEASUREMENT: ...] 0 1`.
  - If a `MPAD[LEAKAGE_MEASUREMENT: ...]` tag lists `0` and/or `1`, `TablesideSimulator` (any `batch_size`, with or without `sync_tableside_rng`), `CosetsideSimulator`, and `FlipsideSimulator` collapse each superposed unleaked target in the $Z$ basis (Born rule), report `1` with probability `p(0)` or `p(1)` of the collapsed value, and leave the qubit in that collapsed state (no reset). Without `0`/`1` keys the targets are not collapsed.
  - A qubit whose state isn't listed in the `MPAD[LEAKAGE_MEASUREMENT: ...]` tag reports `0`, even an unleaked qubit in `1` (unlike `LEAKAGE_PROJECTION_Z`, where an unlisted `0` or `1` is read out noiselessly), and an `MPAD` value of `1` (e.g. `MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) : 0] 1`) inverts the reported bit.
  - Since stim fuses identical consecutive instructions (same gate, args and tag), an `MPAD[LEAKAGE_MEASUREMENT: ... : q1..qn]` (or `CONDITIONED_ON_OTHER: ... : c1..cn`) instruction with `k*n` targets is applied as `k` copies in sequence; other target counts still raise.

### Untagged Instructions on Leaked Qubits
What an untagged instruction does to a leaked qubit (`state >= 2`) depends on the simulator:
- **`FlipsideSimulator`: scrambled, then applied.** The qubit is fully depolarized when it leaks, untagged gates and noise act on it as usual, and it is depolarized again right after any `M`, `MX`, `MY`, `MR`, `MRX`, `MRY`, `R`, `RX` or `RY` that touches it. `MPP`, `MXX`, `MYY` and `MZZ` don't depolarize it again, which matches measuring the same product through an ancilla qubit.
- **`TablesideSimulator` and `CosetsideSimulator` (default, `unconditional_condition_on_U=True`): skipped.** The qubit's computational state is frozen when it leaks, and untagged unitary gates and noise channels skip it: single-qubit instructions per target, two-qubit instructions per pair (a pair with a leaked qubit is skipped whole), `CORRELATED_ERROR`/`ELSE_CORRELATED_ERROR` as a whole (a skipped one still fires with its probability, without effect, so the rest of its `ELSE` chain is unchanged), and `SPP`/`SPP_DAG` per Pauli product term. Instructions that produce measurement results (including `MPP`, `MXX`, `MYY`, `MZZ`, `HERALDED_ERASE` and `HERALDED_PAULI_CHANNEL_1`) and resets are not filtered: they act on, or read, the frozen state.
- **`TablesideSimulator` and `CosetsideSimulator` with `unconditional_condition_on_U=False`: frozen, then applied.** The computational state is frozen when the qubit leaks, and untagged instructions act on it as if it were unleaked.

`CosetsideSimulator` does not support `SPP`/`SPP_DAG` and raises `NotImplementedError` for them. With `unconditional_condition_on_U=True`, so does `MarginalLeakageDemGenerator` (and the decoders built on it, `MarginalDecoder` and `BranchAndBoundDecoder`), whichever simulator the circuit is sampled with: it does not model a Pauli product term that touches a leaked qubit.

An untagged `SWAP` never moves leakage states, in any simulator: each qubit keeps its own leakage state, and only the computational states (the Pauli frames in `FlipsideSimulator`) are swapped as described above. To move leakage with the qubits, use `SWAP[LEAKAGE_SWAP]`.

### `SWAP[LEAKAGE_SWAP]`
`SWAP[LEAKAGE_SWAP] 0 1 2 3` is a `SWAP` that also swaps the leakage states of each target pair, in all three simulators. The tag takes no arguments and is only allowed on `SWAP`; anything else (e.g. `CX[LEAKAGE_SWAP]` or `SWAP[LEAKAGE_SWAP: (0.5)]`) raises `ValueError`.
- **`TablesideSimulator` and `CosetsideSimulator`:** every pair is swapped, leaked qubits included, whatever `unconditional_condition_on_U` is. A leaked qubit's frozen computational state moves with its leakage state, so the whole qubit moves: after `SWAP[LEAKAGE_SWAP] 0 1` with qubit 0 leaked and qubit 1 in `|1>`, qubit 1 is leaked and qubit 0 is in `|1>`.
- **`FlipsideSimulator`:** the Pauli frames are swapped (as by an untagged `SWAP`), then the leakage states. There is no extra depolarization.
- Pairs are processed in order, left to right: `SWAP[LEAKAGE_SWAP] 0 1 1 2` moves a leak on qubit 0 to qubit 2.
- Leakage events (`record_leakage_events=True` in `TablesideSimulator` and `CosetsideSimulator`) compare each target's leakage state before and after the instruction: a leak moving from qubit 0 to qubit 1 gives an unleak event on qubit 0 and a leak event on qubit 1, and a qubit whose state leaves and comes back within the instruction gives none.
- **Unleaked-to-leaked records count moves.** The qubit that receives a leak counts as an unleaked-to-leaked transition at the `SWAP[LEAKAGE_SWAP]` (like a `LEAKAGE_TRANSITION_2` hop), so a leak is counted when it happens and again each time it is moved onto an unleaked qubit. In circuits that move leakage, `get_unleaked_to_leaked_records()` therefore over-counts leaks, and leak counts and leakage rates computed from it are inflated. Tracking moved leaks separately is a pending to-do.
- `MarginalLeakageDemGenerator`, and so `MarginalDecoder`, `BranchAndBoundDecoder` and the loss oracle (`MarginalLeakageDemGenerator(loss_oracle=True)`), raise `NotImplementedError` for circuits that contain `SWAP[LEAKAGE_SWAP]`, whatever `unconditional_condition_on_U` is: they don't model leakage moving between qubits. A sampler with a `MarginalDecoder` raises at its first decode, after simulating its first batch.

### Heralded Channels and Pauli Product / Pair Measurements (`HERALDED_ERASE`, `HERALDED_PAULI_CHANNEL_1`, `MPP`, `MXX`, `MYY`, `MZZ`)
- `HERALDED_ERASE` and `HERALDED_PAULI_CHANNEL_1` are supported across `TablesideSimulator`, `CosetsideSimulator`, and `FlipsideSimulator`, appending one herald bit per target to the measurement record (`True` when an erasure/Pauli event is heralded, `False` otherwise).
- `MPP`, `MXX`, `MYY`, and `MZZ` are supported across all three simulators.
- **Behavior when a qubit in `MPP`, `MXX`, `MYY`, or `MZZ` is leaked (`state >= 2`):** like untagged `M`, product and pair measurements are never filtered and **never drop measurement records** when one or more participating qubits are leaked. All three simulators evaluate the joint Pauli product measurement on the leaked qubit's computational state: its frozen state in `TablesideSimulator` and `CosetsideSimulator`, and a scrambled one in `FlipsideSimulator`, which (unlike after `M`) doesn't re-scramble it after the measurement. See [Untagged Instructions on Leaked Qubits](#untagged-instructions-on-leaked-qubits).

### `TablesideSimulator` and `TablesideSampler` (`batch_size > 1`)
`TablesideSimulator` and `TablesideSampler` support `batch_size > 1`. In standard mode (`sync_tableside_rng=False`), each batch runs a **single** tableau trajectory with a **single** classical leakage sample to build a conditioned `stim.Circuit` (`_new_circuit`), and then generates `batch_size` shots from `_new_circuit` via Stim's fast Pauli-frame randomization (`compile_sampler` / `compile_detector_sampler`).

> [!CAUTION]
> **Shared Leakage Trajectory Within a Batch (`TablesideSimulator(..., batch_size > 1)`):** Because standard `TablesideSimulator` (`sync_tableside_rng=False`) samples classical leakage transitions once per `run()` call and uses Pauli-frame randomization on `_new_circuit` to produce the `batch_size` shots, **all shots within the same batch share the exact same leakage events and leakage state history**. Only Pauli errors and measurement outcomes conditioned on that fixed leakage realization vary across the `batch_size` shots. To sample independent leakage trajectories for every shot in `TablesideSimulator`, either use `batch_size=1` (where `TablesideSampler` runs independent batches) or set `sync_tableside_rng=True` (which executes an independent tableau and leakage trajectory per shot, seeded with a hash of `seed` and the shot's index in the run).


### Synchronized Shot-for-Shot Equivalence (`sync_tableside_rng=True`)
Passing `sync_tableside_rng=True` to `TablesideSimulator` and `CosetsideSimulator` (with the same integer `seed` and `batch_size`) synchronizes all stabilizer measurement collapses, noise channels, herald draws, and leakage transition alias-table draws per shot (shot `n` of the run, counting across batches, is seeded with a splitmix64 hash of `seed` and `n`; without `sync_tableside_rng`, batch `n` is seeded the same way, so runs with different seeds don't share random streams), producing identical shot-for-shot measurement records, detector flips, observable flips, and `unleaked_to_leaked` event indices across both simulators. This guarantee assumes each instruction lists distinct targets: under sync, duplicate targets in `LEAKAGE_PROJECTION_Z` measurements raise `NotImplementedError` when readout is noisy or any target is leaked. Duplicate targets in conditioned noise, and out-of-order targets in untagged X/Y-basis measurements and resets, are not guaranteed to match shot-for-shot.

