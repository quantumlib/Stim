"""Workarounds for stim behaviour that stimside depends on."""

from typing import Literal

import numpy as np
import stim  # type: ignore[import-untyped]

from stimside.util.numpy_types import Bool2DArray


def split_fused_instruction(
    op: stim.CircuitInstruction, copy_len: int
) -> list[stim.CircuitInstruction]:
    """Undo stim's fusion of identical consecutive instructions.

    stim merges consecutive instructions with the same name, gate args and tag
    into one (both when parsing text and in ``Circuit.append``). For a tag that
    lists its own ``copy_len`` qubits per instruction (``MPAD[LEAKAGE_MEASUREMENT:
    ... : q1..qn]``, ``CONDITIONED_ON_OTHER: ... : c1..cn``), an instruction with
    ``k * copy_len`` targets (k >= 2) is k such instructions applied in sequence;
    this returns them, copy i having ``targets[i*copy_len:(i+1)*copy_len]``.
    Otherwise (including target counts that are not a multiple) returns ``[op]``.
    """
    targets = op.targets_copy()
    if copy_len <= 0 or len(targets) <= copy_len or len(targets) % copy_len:
        return [op]
    args = op.gate_args_copy()
    return [
        stim.CircuitInstruction(op.name, targets[i : i + copy_len], args, tag=op.tag)
        for i in range(0, len(targets), copy_len)
    ]


def broadcast_pauli_errors(
    flip_simulator: stim.FlipSimulator,
    *,
    pauli: Literal["X", "Y", "Z"],
    mask: Bool2DArray,
    p: float,
    np_rng: np.random.Generator,
) -> None:
    """``flip_simulator.broadcast_pauli_errors(pauli=pauli, mask=mask, p=p)``, correct for any batch size.

    For ``0 < p < 1`` stim refills its random bits only for the full 64-shot
    words of the batch (``mask_batch_size / 64`` in
    ``frame_simulator.pybind.cc``), so the shots in a trailing partial word get
    stale bits instead of probability-``p`` errors (none at all for batch sizes
    below 64). When ``batch_size % 64 != 0`` this draws the errors with
    ``np_rng`` (one uniform per True entry of ``mask``) and applies them with
    ``p=1``, which draws no randomness. Otherwise (and for ``p`` 0 or 1) it
    calls stim unchanged; note that stim applies the whole mask for ``p=0``
    too, so callers must skip ``p <= 0`` themselves.
    """
    if 0 < p < 1 and flip_simulator.batch_size % 64:
        hits = np.zeros(mask.shape, dtype=np.bool_)
        hits[mask] = np_rng.random(np.count_nonzero(mask)) < p
        flip_simulator.broadcast_pauli_errors(pauli=pauli, mask=hits, p=1.0)
    else:
        flip_simulator.broadcast_pauli_errors(pauli=pauli, mask=mask, p=p)
