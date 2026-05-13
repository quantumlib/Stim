from __future__ import annotations

from collections.abc import Callable, Iterable, Mapping
from typing import Any, cast

import stim

from stimflow._core._complex_util import xor_sorted
from stimflow._core._pauli_map import PauliMap
from stimflow._core._tile import Tile


class _UNSPECIFIED_:
    def __repr__(self):
        return "<keep_original>"
_UNSPECIFIED: Any = _UNSPECIFIED_()


class Flow:
    """A rule for how a stabilizer travels into, through, and/or out of a circuit."""

    def __init__(
        self,
        *,
        start: PauliMap | Tile | None = None,
        end: PauliMap | Tile | None = None,
        measurement_indices: Iterable[int] = (),
        center: complex | None = None,
        flags: Iterable[Any] = frozenset(),
        sign: bool | None = None,
    ):
        """Initializes a Flow.

        Args:
            start: Defaults to None (empty). The Pauli product operator at the beginning of the
                circuit (before *all* operations, including resets).
            end: Defaults to None (empty). The Pauli product operator at the end of the
                circuit (after *all* operations, including measurements).
            measurement_indices: Defaults to empty. Indices of measurements that mediate the flow (that multiply
                into it as it traverses the circuit).
            center: Defaults to None (unspecified). Specifies a 2d coordinate to use in metadata
                when the flow is completed into a detector. Incompatible with obs_key.
            flags: Defaults to empty. Custom information about the flow, that can be used by code
                operating on chunks for a variety of purposes. For example, this could identify the
                "color" of the flow in a color code.
            sign: Defaults to None (unsigned). The expected sign of the flow.
        """
        if start is not None and not isinstance(start, (PauliMap, Tile)):
            raise TypeError(
                f"{start=} is not None and not isinstance(start, (stimflow.PauliMap, stimflow.Tile))"
            )
        if end is not None and not isinstance(end, (PauliMap, Tile)):
            raise TypeError(
                f"{end=} is not None and not isinstance(end, (stimflow.PauliMap, stimflow.Tile))"
            )
        if isinstance(flags, str):
            raise TypeError(f"{flags=} is a str instead of a set")
        if isinstance(start, PauliMap) and isinstance(end, PauliMap) and start.name != end.name:
            raise ValueError(f'{start.name=} != {end.name=}')

        if center is None and isinstance(start, Tile):
            center = start.measure_qubit
        if center is None and isinstance(end, Tile):
            center = end.measure_qubit

        if isinstance(start, PauliMap):
            obs_key = start.name
        elif isinstance(end, PauliMap):
            obs_key = end.name
        else:
            obs_key = None
        if isinstance(start, Tile):
            start = start.to_pauli_map().with_name(obs_key)
        elif start is None:
            start = PauliMap(name=obs_key)
        if isinstance(end, Tile):
            end = end.to_pauli_map().with_name(obs_key)
        elif end is None:
            end = PauliMap(name=obs_key)

        if center is None:
            qubits: list[complex] = []
            qubits.extend(start.keys())
            qubits.extend(end.keys())
            if qubits:
                center = sum(qubits) / len(qubits)

        self.start: PauliMap = start
        self.end: PauliMap = end
        self.measurement_indices: tuple[int, ...] = tuple(xor_sorted(measurement_indices))
        self.flags: frozenset[Any] = frozenset(flags)
        self.center: complex | None = center
        self.sign: bool | None = sign

    def to_stim_flow(
        self, *, q2i: dict[complex, int], o2i: Mapping[Any, int | None] | None = None
    ) -> stim.Flow:
        out = self.end.to_stim_pauli_string(q2i)
        if self.sign:
            out.sign = -1
        included_observables: list[int] | None
        if self.obs_key is None:
            included_observables = None
        elif o2i is None:
            raise ValueError(f"{self.obs_key=} is not None but {o2i=}")
        else:
            v = o2i[self.obs_key]
            if v is None:
                included_observables = None
            else:
                included_observables = [v]
        return stim.Flow(
            input=self.start.to_stim_pauli_string(q2i),
            output=out,
            measurements=self.measurement_indices,
            included_observables=included_observables,
        )

    @property
    def obs_key(self) -> Any:
        return self.start.name

    def with_edits(
        self,
        *,
        start: PauliMap = _UNSPECIFIED,
        end: PauliMap = _UNSPECIFIED,
        measurement_indices: Iterable[int] = _UNSPECIFIED,
        center: complex | None = _UNSPECIFIED,
        flags: Iterable[str] = _UNSPECIFIED,
        sign: Any = _UNSPECIFIED,
    ) -> Flow:
        return Flow(
            start=self.start if start is _UNSPECIFIED else start,
            end=self.end if end is _UNSPECIFIED else end,
            measurement_indices=(
                self.measurement_indices
                if measurement_indices is _UNSPECIFIED
                else cast(Any, measurement_indices)
            ),
            center=self.center if center is _UNSPECIFIED else center,
            flags=self.flags if flags is _UNSPECIFIED else flags,
            sign=self.sign if sign is _UNSPECIFIED else sign,
        )

    def __eq__(self, other: Any) -> bool:
        if not isinstance(other, Flow):
            return NotImplemented
        return (
            self.start == other.start
            and self.end == other.end
            and self.measurement_indices == other.measurement_indices
            and self.obs_key == other.obs_key
            and self.flags == other.flags
            and self.center == other.center
            and self.sign == other.sign
        )

    def __hash__(self) -> int:
        return hash(
            (
                self.start,
                self.end,
                self.measurement_indices,
                self.obs_key,
                self.flags,
                self.center,
                self.sign,
            )
        )

    def __str__(self) -> str:
        q: Any

        start_terms = []
        for q, p in self.start.items():
            q = complex(q)
            if q.real == 0:
                q = "0+" + str(q)
            q = str(q).replace("(", "").replace(")", "")
            start_terms.append(f"{p}[{q}]")

        end_terms = []
        for q, p in self.end.items():
            q = complex(q)
            if q.real == 0:
                q = "0+" + str(q)
            q = str(q).replace("(", "").replace(")", "")
            end_terms.append(f"{p}[{q}]")

        for m in self.measurement_indices:
            end_terms.append(f"rec[{m}]")

        if not start_terms:
            start_terms.append("1")
        if not end_terms:
            end_terms.append("1")

        key = "" if self.obs_key is None else f" (obs={self.obs_key})"
        result = f'{"*".join(start_terms)} -> {"*".join(end_terms)}{key}'
        if self.sign is None:
            pass
        elif self.sign:
            result = "-" + result
        else:
            result = "+" + result
        if self.flags:
            result += f" (flags={sorted(self.flags)})"
        return result

    def __repr__(self):
        return (
            f"stimflow.Flow(start={self.start!r}, "
            f"end={self.end!r}, "
            f"measurement_indices={self.measurement_indices!r}, "
            f"flags={self.flags!r}, "
            f"center={self.center!r}, "
            f"sign={self.sign!r}"
        )

    def with_xz_flipped(self) -> Flow:
        return self.with_edits(start=self.start.with_xz_flipped(), end=self.end.with_xz_flipped())

    def with_transformed_coords(self, transform: Callable[[complex], complex]) -> Flow:
        return self.with_edits(
            start=self.start.with_transformed_coords(transform),
            end=self.end.with_transformed_coords(transform),
            center=None if self.center is None else transform(self.center),
        )

    def fused_with_next_flow(self, next_flow: Flow, *, next_flow_measure_offset: int) -> Flow:
        if next_flow.start != self.end:
            raise ValueError("other.start != self.end")
        if next_flow.obs_key != self.obs_key:
            raise ValueError("other.obs_key != self.obs_key")
        if self.center is None:
            new_center = next_flow.center
        elif next_flow.center is None:
            new_center = self.center
        else:
            new_center = (self.center + next_flow.center) / 2
        assert isinstance(self.measurement_indices, tuple)
        assert isinstance(next_flow.measurement_indices, tuple)
        return Flow(
            start=self.start,
            end=next_flow.end,
            center=new_center,
            measurement_indices=(
                *(m + next_flow_measure_offset * (m < 0) for m in self.measurement_indices),
                *(m + next_flow_measure_offset for m in next_flow.measurement_indices),
            ),
            flags=self.flags | next_flow.flags,
            sign=(
                None if self.sign is None or next_flow.sign is None else self.sign ^ next_flow.sign
            ),
        )

    def __mul__(self, other: Flow) -> Flow:
        """Computes the product of two flows.

        The product of A -> B and C -> D is (A*C) -> (B*D).
        """
        if self.obs_key != other.obs_key:
            raise ValueError(f"{self.obs_key=} != {other.obs_key=}")
        if (self.sign is None) != (other.sign is None):
            raise ValueError(f"({self.sign=} is None) != ({other.sign=} is None)")

        new_start: PauliMap = self.start * other.start
        new_end: PauliMap = self.end * other.end
        new_center: complex | None
        if self.center is not None and other.center is not None:
            new_center = (self.center + other.center) / 2
        elif self.center is not None:
            new_center = self.center
        elif other.center is not None:
            new_center = other.center
        else:
            new_center = None

        return Flow(
            start=new_start,
            end=new_end,
            measurement_indices=xor_sorted(self.measurement_indices + other.measurement_indices),
            obs_key=self.obs_key,
            flags=self.flags | other.flags,
            center=new_center,
            sign=(None if self.sign is None else self.sign ^ other.sign),
        )
