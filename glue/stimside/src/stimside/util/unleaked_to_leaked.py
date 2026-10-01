"""Unleaked-to-leaked (u2l) event-record API shared by the Flipside/Tableside/Cosetside simulators."""

from __future__ import annotations

import numpy as np


class UnleakedToLeakedRecordsMixin:
    record_unleaked_to_leaked: bool
    _unleaked_to_leaked_events: list[list[int]]
    _circuit_time: int

    def _record_unleaked_to_leaked_counts(
        self, counts: np.ndarray, op_idx: int | None = None
    ) -> None:
        """Record unleaked-to-leaked transition counts per shot at unrolled instruction index `op_idx`."""
        if not self.record_unleaked_to_leaked:
            return
        if op_idx is None:
            op_idx = self._circuit_time
        idx_val = int(op_idx)
        events = self._unleaked_to_leaked_events
        for b in np.flatnonzero(counts):
            c = int(counts[b])
            if c == 1:
                events[b].append(idx_val)
            else:
                events[b].extend([idx_val] * c)

    def get_unleaked_to_leaked_records(self) -> list[np.ndarray]:
        """Return per-shot 1D int64 arrays of unrolled circuit instruction indices (`op_idx`)
        where a qubit transitioned from an unleaked state (state < 2) to a leaked state (state >= 2).

        Note: `op_idx` is the 0-based index into the unrolled circuit (`_unroll_circuit(circuit)`),
        where `REPEAT` blocks are expanded in-place and metadata instructions (`QUBIT_COORDS`,
        `SHIFT_COORDS`, `TICK`, `DETECTOR`, `OBSERVABLE_INCLUDE`) are included at their unrolled
        positions. Only transitions from unleaked to leaked states are recorded.
        """
        if not self.record_unleaked_to_leaked:
            raise ValueError(
                "Cannot access unleaked_to_leaked_records when initialized with record_unleaked_to_leaked=False."
            )
        return [
            np.asarray(ev, dtype=np.int64)
            for ev in self._unleaked_to_leaked_events
        ]

    get_unleaked_to_leaked_op_indices = get_unleaked_to_leaked_records  # alias
    unleaked_to_leaked_records = property(get_unleaked_to_leaked_records)
    unleaked_to_leaked_op_indices = property(get_unleaked_to_leaked_records)
