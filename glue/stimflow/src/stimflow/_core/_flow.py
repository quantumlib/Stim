from __future__ import annotations

from collections.abc import Callable, Iterable, Mapping
from typing import Any, cast, Literal

import stim

from stimflow._core._complex_util import xor_sorted
from stimflow._core._pauli_map import PauliMap
from stimflow._core._tile import Tile


class _UNSPECIFIED_:
    def __repr__(self):
        return "_UNSPECIFIED"
_UNSPECIFIED: Any = _UNSPECIFIED_()


class Flow:
    """A rule for how a stabilizer travels into, through, and/or out of a circuit."""

    def __init__(
        self,
        *,
        start: PauliMap | Tile | None = None,
        end: PauliMap | Tile | None = None,
        measurement_indices: Iterable[int] = (),
        center: complex | None | Literal['infer'] = 'infer',
        flags: Iterable[Any] = frozenset(),
        sign: bool | None = None,
    ):
        """Initializes a Flow.

        Args:
            start: Defaults to None (empty). The Pauli product operator at the beginning of
                the circuit (before *all* operations, including resets).
            end: Defaults to None (empty). The Pauli product operator at the end of the
                circuit (after *all* operations, including measurements).
            measurement_indices: Defaults to empty. Indices of measurements that mediate
                the flow (that multiply into it as it traverses the circuit).
            center: Defaults to 'infer' (attempt to infer). Specifies a 2d coordinate to
                use in metadata, when the flow is completed into a detector. Can be set to a
                complex number or to None.
            flags: Defaults to empty. Custom information about the flow, that can be used by
                code operating on chunks for a variety of purposes. For example, this could
                identify the "color" of the flow in a color code.
            sign: Defaults to None (unsigned).
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
        if isinstance(start, PauliMap) and isinstance(end, PauliMap) and start.obs_name != end.obs_name:
            raise ValueError(f'{start.obs_name=} != {end.obs_name=}')
        if sign == -1:
            raise ValueError(f"sign is a bool, not an int. Specify sign=True instead of {sign=}.")

        if center == 'infer' and isinstance(start, Tile):
            center = start.measure_qubit
        if center == 'infer' and isinstance(end, Tile):
            center = end.measure_qubit

        if isinstance(start, PauliMap):
            obs_name = start.obs_name
        elif isinstance(end, PauliMap):
            obs_name = end.obs_name
        else:
            obs_name = None
        if isinstance(start, Tile):
            start = start.to_pauli_map().with_obs_name(obs_name)
        elif start is None:
            start = PauliMap(obs_name=obs_name)
        if isinstance(end, Tile):
            end = end.to_pauli_map().with_obs_name(obs_name)
        elif end is None:
            end = PauliMap(obs_name=obs_name)

        if center == 'infer':
            qubits: list[complex] = []
            qubits.extend(start.keys())
            qubits.extend(end.keys())
            if qubits:
                center = sum(qubits) / len(qubits)
            else:
                center = None

        self.start: PauliMap = start
        self.end: PauliMap = end
        self.measurement_indices: tuple[int, ...] = tuple(xor_sorted(measurement_indices))
        self.flags: frozenset[Any] = frozenset(flags)
        self.center: complex | None = center
        self.sign: bool | None = sign

    def to_stim_flow(
        self, *, q2i: dict[complex, int], o2i: Mapping[Any, int | None] | None = None
    ) -> stim.Flow:
        """Converts this `stimflow.Flow` into a `stim.Flow`.

        Args:
            q2i: A mapping from stimflow qubit positions to stim qubit indices.
            o2i: A mapping from stimflow obs names to stim obs indices.
                This argument can be skipped if the flow has no obs_name.

        Returns:
            The stim flow.

        Raise:
            ValueError:
                The flow has an `obs_name` but `o2i` wasn't specified.

        Examples:
            >>> import stimflow as sf
            >>> flow = sf.Flow(
            ...     start=sf.PauliMap({'Z': 1j}, obs_name="test"),
            ...     end=sf.PauliMap({'X': 1 + 1j}, obs_name="test"),
            ...     measurement_indices=[1, 2],
            ...     sign=True,
            ... )
            >>> flow.to_stim_flow(q2i={1j: 2, 1 + 1j: 3}, o2i={"test": 0})
            stim.Flow("__Z -> -___X xor rec[1] xor rec[2] xor obs[0]")
        """
        out = self.end.to_stim_pauli_string(q2i)
        if self.sign:
            out.sign = -1
        included_observables: list[int] | None
        if self.obs_name is None:
            included_observables = None
        elif o2i is None:
            raise ValueError(f"{self.obs_name=} is not None but {o2i=}")
        else:
            v = o2i[self.obs_name]
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
    def obs_name(self) -> Any:
        return self.start.obs_name

    def with_edits(
        self,
        *,
        start: PauliMap = _UNSPECIFIED,
        end: PauliMap = _UNSPECIFIED,
        measurement_indices: Iterable[int] = _UNSPECIFIED,
        center: complex | None = _UNSPECIFIED,
        flags: Iterable[str] = _UNSPECIFIED,
        sign: Any = _UNSPECIFIED,
        obs_name: None | str = _UNSPECIFIED,
    ) -> Flow:
        """Returns the same flow but with specified edits.

        Args:
            start: If specified, the returned flow has the specified start instead of the
                start used by the original flow. Note: if `obs_name` is also specified,
                the obs_name of this argument must be consistent with the given `obs_name`.
            end: If specified, the returned flow has the specified end instead of the
                end used by the original flow. Note: if `obs_name` is also specified,
                the obs_name of this argument must be consistent with the given `obs_name`.
            measurement_indices: If specified, the returned flow has the specified
                measurement_indices instead of the measurement_indices used by the original
                flow.
            center: If specified, the returned flow has the specified center instead of the
                center used by the original flow.
            flags: If specified, the returned flow has the specified flags instead of the
                flags used by the original flow.
            sign: If specified, the returned flow has the specified sign instead of the
                sign used by the original flow.
            obs_name: If specified, the returned flow has the obs_name of both its start and
                end changed to the given value. If `start` or `end` are specified alongside
                this argument, they must use the same observable name.

        Returns:
            The edited flow.

        Raises:
            ValueError:
                Specified contradictory `obs_name=` and `start=` values.

                OR

                Specified contradictory `obs_name=` and `end=` values.

                OR

                The edits produced an invalid flow (stimflow.Flow.__init__ raised an error).
        """
        if start is not _UNSPECIFIED and obs_name is not _UNSPECIFIED and start.obs_name != obs_name:
            raise ValueError(f"Specified contradictory observable names in `start` and `obs_name`.\n    {start.obs_name=}\n    {obs_name=}")
        if end is not _UNSPECIFIED and obs_name is not _UNSPECIFIED and end.obs_name != obs_name:
            raise ValueError(f"Specified contradictory observable names in `end` and `obs_name`.\n    {end.obs_name=}\n    {obs_name=}")

        start = self.start if start is _UNSPECIFIED else start
        end = self.end if end is _UNSPECIFIED else end
        if obs_name is not _UNSPECIFIED:
            start = start.with_obs_name(obs_name)
            end = end.with_obs_name(obs_name)

        return Flow(
            start=start,
            end=end,
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
            and self.obs_name == other.obs_name
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
                self.obs_name,
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

        key = "" if self.obs_name is None else f" (obs={self.obs_name})"
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
        lines = ["stimflow.Flow("]
        if self.start:
            lines.append(f"    start={self.start!r},")
        if self.end:
            lines.append(f"    end={self.end!r},")
        if self.measurement_indices:
            lines.append(f"    measurement_indices={self.measurement_indices!r},")
        if self.flags:
            lines.append(f"    flags={self.flags!r},")
        if self.center is not None:
            lines.append(f"    center={self.center!r},")
        if self.sign is not None:
            lines.append(f"    sign={self.sign!r},")
        lines.append(")")
        return '\n'.join(lines)

    def with_xz_flipped(self) -> Flow:
        return self.with_edits(start=self.start.with_xz_flipped(), end=self.end.with_xz_flipped())

    def with_transformed_coords(self, transform: Callable[[complex], complex]) -> Flow:
        return self.with_edits(
            start=self.start.with_transformed_coords(transform),
            end=self.end.with_transformed_coords(transform),
            center=None if self.center is None else transform(self.center),
        )

    def fused_with_next_flow(self, next_flow: Flow, *, next_flow_measure_offset: int) -> Flow:
        """Combines flows tail-to-head.

        For example, fusing X1 -> Y2 with Y2 -> Z3 produces X1 -> Z3.

        Measurement sets are xored, adjusting for the offset. Centers are
        taken as is, preferring the center of the prior flow. Signs are xored.
        flags are union'd.

        Args:
            next_flow: The flow that occurs after this flow. Must have a start
                that matches the end of this flow.
            next_flow_measure_offset: What offset to add into measurement indices
                used by the other flow.

        Returns:
            The fused flow.

        Examples:
            >>> import stimflow as sf
            >>> a = sf.Flow(
            ...     start=sf.PauliMap({1: 'X'}),
            ...     end=sf.PauliMap({2: 'Y'}),
            ...     measurement_indices=[-1, 2],
            ... )
            >>> b = sf.Flow(
            ...     start=sf.PauliMap({2: 'Y'}),
            ...     end=sf.PauliMap({3: 'Z'}),
            ...     measurement_indices=[-10, 20],
            ... )
            >>> a.fused_with_next_flow(b, next_flow_measure_offset=100)
            stimflow.Flow(
                start=stimflow.PauliMap({(1+0j): 'X'}),
                end=stimflow.PauliMap({(3+0j): 'Z'}),
                measurement_indices=(2, 90, 99, 120),
                center=(2+0j),
            )
        """
        if next_flow.start != self.end:
            raise ValueError("other.start != self.end")
        if next_flow.obs_name != self.obs_name:
            raise ValueError("other.obs_name != self.obs_name")
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

        The product of two flows sends the product of their inputs to the product of their
        outputs. For example, (A -> B) * (C -> D) = (A*C) -> (B*D).

        Starts are multiplied. Ends are multiplied. Measurement sets are xored. Centers are
        averaged. Signs are xored. flags are union'd.

        Args:
            other: The other flow in the multiplication.

        Raises:
            ValueError:
                The flows have incompatible observable names.

                OR

                The flows disagree on whether they're unsigned.

        Examples:
            >>> import stimflow as sf
            >>> a = sf.Flow(
            ...     start=sf.PauliMap({1: 'X'}),
            ...     end=sf.PauliMap({2: 'Y'}),
            ...     measurement_indices=[-1, 2],
            ... )
            >>> b = sf.Flow(
            ...     start=sf.PauliMap({2: 'Y'}),
            ...     end=sf.PauliMap({3: 'Z'}),
            ...     measurement_indices=[-10, 20],
            ... )
            >>> a * b
            stimflow.Flow(
                start=stimflow.PauliMap({(1+0j): 'X', (2+0j): 'Y'}),
                end=stimflow.PauliMap({(2+0j): 'Y', (3+0j): 'Z'}),
                measurement_indices=(-10, -1, 2, 20),
                center=(2+0j),
            )
        """
        if self.obs_name != other.obs_name:
            raise ValueError(f"{self.obs_name=} != {other.obs_name=}")
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
            flags=self.flags | other.flags,
            center=new_center,
            sign=(None if self.sign is None else self.sign ^ other.sign),
        )
