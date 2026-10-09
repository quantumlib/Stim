"""Simulator-independent per-shot leakage events (plain data).

Neither the simulators nor the DEM generators own this module: the Tableside
and Cosetside simulators can record these events (opt-in), and DEM generators
that need them (e.g. ``MarginalLeakageDemGenerator(loss_oracle=True)``) consume
them.

Op-index convention: ``LeakageEvent.op_index`` is the 0-based index into
``stimside.util.known_states._unroll_circuit(circuit)``, i.e. the circuit with
``REPEAT`` blocks expanded in place and every other instruction (including
``SHIFT_COORDS``, ``QUBIT_COORDS``, ``TICK``, ``DETECTOR`` and
``OBSERVABLE_INCLUDE``) kept at its position. This is the same index as the
simulators' ``get_unleaked_to_leaked_records()``. Use
``unrolled_to_flattened_indices`` to convert it to an index into
``list(circuit.flattened())`` (which drops ``SHIFT_COORDS``).

A shot's events are ordered by ``(op_index, qubit)``, keeping the order in which
they happened for equal keys.
"""

from __future__ import annotations

from typing import NamedTuple, Sequence

import numpy as np
from numpy.typing import NDArray
import stim  # type: ignore[import-untyped]


class LeakageEvent(NamedTuple):
    """A change of one qubit's leakage state in one shot.

    Only changes where ``old_state != new_state`` and at least one side is a
    leaked state (``>= 2``) are events: leaks (``< 2 -> >= 2``), unleaks
    (``>= 2 -> < 2``) and changes between leaked states.
    """

    op_index: int
    qubit: int
    old_state: int
    new_state: int


ShotLeakageEvents = Sequence[LeakageEvent]


def is_leakage_event(old_state: int, new_state: int) -> bool:
    """Whether a state change ``old_state -> new_state`` is recorded as a LeakageEvent."""
    return old_state != new_state and (old_state >= 2 or new_state >= 2)


def sort_shot_events(events: Sequence[LeakageEvent]) -> list[LeakageEvent]:
    """A shot's events in the canonical ``(op_index, qubit)`` order (stable)."""
    return sorted(events, key=lambda ev: (ev.op_index, ev.qubit))


class LeakageEventsRecorderMixin:
    """Opt-in per-shot LeakageEvent recording (``record_leakage_events=True``).

    Recording only observes state changes; it draws no random numbers, so the
    simulated shots are the same with and without it.
    """

    record_leakage_events: bool
    _leakage_events: list[list[LeakageEvent]]

    def _record_leakage_event(
        self, op_idx: int, qubit: int, old_state: int, new_state: int, shot_idx: int = 0
    ) -> None:
        old_i = int(old_state)
        new_i = int(new_state)
        if is_leakage_event(old_i, new_i):
            self._leakage_events[shot_idx].append(
                LeakageEvent(int(op_idx), int(qubit), old_i, new_i)
            )

    def get_leakage_events(self) -> list[list[LeakageEvent]]:
        """Per-shot lists of the LeakageEvents of the last run (see ``stimside.util.leakage_events``)."""
        if not self.record_leakage_events:
            raise ValueError(
                "Cannot access leakage events when initialized with record_leakage_events=False."
            )
        return [sort_shot_events(ev) for ev in self._leakage_events]


def _unrolled_names(circuit: stim.Circuit, out: list[str]) -> None:
    for item in circuit:
        if isinstance(item, stim.CircuitRepeatBlock):
            body: list[str] = []
            _unrolled_names(item.body_copy(), body)
            out.extend(body * item.repeat_count)
        else:
            out.append(item.name)


def unrolled_to_flattened_indices(
    circuit: stim.Circuit, *, validate: bool = True
) -> NDArray[np.int64]:
    """Map each unrolled op index to its index in ``list(circuit.flattened())``.

    ``SHIFT_COORDS`` ops (dropped by ``flattened()``) map to -1. With
    ``validate=True`` the map is checked against ``circuit.flattened()``: the
    mapped ops must agree in name, tag and targets, and in gate args except for
    ``DETECTOR``/``QUBIT_COORDS`` (whose coordinates ``flattened()`` shifts).
    """
    names: list[str] = []
    _unrolled_names(circuit, names)
    is_shift = np.fromiter((n == "SHIFT_COORDS" for n in names), dtype=bool, count=len(names))
    flat_idx = np.cumsum(~is_shift, dtype=np.int64) - 1
    flat_idx[is_shift] = -1
    if validate:
        unrolled: list[stim.CircuitInstruction] = []
        _unroll_ops(circuit, unrolled)
        flat_ops = list(circuit.flattened())
        if len(flat_ops) != int(np.count_nonzero(~is_shift)):
            raise ValueError(
                f"circuit.flattened() has {len(flat_ops)} ops but the unrolled circuit "
                f"has {int(np.count_nonzero(~is_shift))} non-SHIFT_COORDS ops."
            )
        for u_idx, f_idx in enumerate(flat_idx):
            if f_idx < 0:
                continue
            a = unrolled[u_idx]
            b = flat_ops[int(f_idx)]
            if (
                a.name != b.name
                or a.tag != b.tag
                or a.targets_copy() != b.targets_copy()
                or (
                    a.name not in ("DETECTOR", "QUBIT_COORDS")
                    and a.gate_args_copy() != b.gate_args_copy()
                )
            ):
                raise ValueError(
                    f"Unrolled op {u_idx} ({a}) does not match flattened op {int(f_idx)} ({b})."
                )
    return flat_idx


def _unroll_ops(circuit: stim.Circuit, out: list[stim.CircuitInstruction]) -> None:
    for item in circuit:
        if isinstance(item, stim.CircuitRepeatBlock):
            body: list[stim.CircuitInstruction] = []
            _unroll_ops(item.body_copy(), body)
            out.extend(body * item.repeat_count)
        else:
            out.append(item)
