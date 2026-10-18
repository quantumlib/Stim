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


def unrolled_to_flattened_indices(
    circuit: stim.Circuit, *, validate: bool = True
) -> NDArray[np.int64]:
    """Map each unrolled op index to its index in ``list(circuit.flattened())``.

    ``SHIFT_COORDS`` ops (dropped by ``flattened()``) map to -1. ``flattened()``
    fuses identical consecutive ops (e.g. the last op of a ``REPEAT`` iteration
    with the first op of the next one), so several consecutive unrolled ops can
    map to one flattened op, whose targets are theirs in order. With
    ``validate=True`` the map is checked against ``circuit.flattened()``: the
    mapped ops must agree in name, tag and targets, and in gate args except for
    ``DETECTOR``/``QUBIT_COORDS`` (whose coordinates ``flattened()`` shifts).
    """
    unrolled: list[stim.CircuitInstruction] = []
    _unroll_ops(circuit, unrolled)
    flat_ops = list(circuit.flattened())
    flat_idx = np.full(len(unrolled), -1, dtype=np.int64)
    f = -1
    flat_targets: list[stim.GateTarget] = []
    used = 0  # targets of flat_ops[f] covered by the unrolled ops mapped so far
    for u_idx, a in enumerate(unrolled):
        if a.name == "SHIFT_COORDS":
            continue
        a_targets = a.targets_copy()
        if used == len(flat_targets):
            f += 1
            if f >= len(flat_ops):
                raise ValueError(
                    f"circuit.flattened() has {len(flat_ops)} ops, too few for the unrolled circuit."
                )
            flat_targets = flat_ops[f].targets_copy()
            used = 0
        if validate:
            b = flat_ops[f]
            if (
                a.name != b.name
                or a.tag != b.tag
                or a_targets != flat_targets[used : used + len(a_targets)]
                or (
                    a.name not in ("DETECTOR", "QUBIT_COORDS")
                    and a.gate_args_copy() != b.gate_args_copy()
                )
            ):
                raise ValueError(f"Unrolled op {u_idx} ({a}) does not match flattened op {f} ({b}).")
        flat_idx[u_idx] = f
        used += len(a_targets)
    if f != len(flat_ops) - 1 or used != len(flat_targets):
        raise ValueError(
            f"circuit.flattened() has {len(flat_ops)} ops, more than the unrolled circuit maps to."
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
