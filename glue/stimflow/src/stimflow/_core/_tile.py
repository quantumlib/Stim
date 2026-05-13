from __future__ import annotations

import functools
from collections.abc import Callable, Iterable
from typing import Any, cast, Literal

from stimflow._core._pauli_map import PauliMap


class Tile:
    """A stabilizer with some associated metadata.

    The exact meaning of the tile's fields are often context dependent. For example,
    different circuits will use the measure qubit in different ways (or not at all)
    and the flags could be essentially anything at all. Tile is intended to be useful
    as an intermediate step in the production of a circuit.

    For example, it's much easier to create a color code circuit when you have a list
    of the hexagonal and trapezoidal shapes making up the color code. So it's natural to
    split the color code circuit generation problem into two steps: (1) making the shapes
    then (2) making the circuit given the shapes. In other words, deal with the spatial
    complexities first then deal with the temporal complexities second. The Tile class
    is a reasonable representation for the shapes, because:

    - The X/Z basis of the stabilizer can be stored in the `bases` field.
    - The red/green/blue coloring can be stored as flags.
    - The ancilla qubits for the shapes be stored as measure_qubit values.
    - You can get diagrams of the shapes by passing the tiles into a `stimflow.Patch`.
    - You can verify the tiles form a code by passing the patch into a `stimflow.StabilizerCode`.
    """

    def __init__(
        self,
        *,
        bases: str,
        data_qubits: Iterable[complex | None],
        measure_qubit: complex | None = None,
        flags: Iterable[str] = (),
    ):
        """

        Args:
            bases: Basis of the stabilizer. A string of XYZ characters the same
                length as the data_qubits argument. It is permitted to
                give a single-character string, which will automatically be
                expanded to the full length. For example, 'X' will become 'XXXX'
                if there are four data qubits.
            measure_qubit: The ancilla qubit used to measure the stabilizer.
            data_qubits: The data qubits in the stabilizer, in the order
                that they are interacted with. Some entries may be None,
                indicating that no data qubit is interacted with during the
                corresponding interaction layer.
        """
        assert isinstance(bases, str)
        self.data_qubits = tuple(data_qubits)
        self.measure_qubit: complex | None = measure_qubit
        if len(bases) == 1:
            bases *= len(self.data_qubits)
        self.bases: str = bases
        self.flags: frozenset[str] = frozenset(flags)
        if len(self.bases) != len(self.data_qubits):
            raise ValueError("len(self.bases_2) != len(self.data_qubits_order)")

    def center(self) -> complex:
        if self.measure_qubit is not None:
            return self.measure_qubit
        if self.data_set:
            return sum(self.data_set) / len(self.data_set)
        return 0

    def _cmp_key(self) -> Any:
        return (
            self.center().real,
            self.center().imag,
            self.to_pauli_map(),
            tuple(sorted(self.flags)),
        )

    def __eq__(self, other):
        if not isinstance(other, Tile):
            return False
        return (
            self.data_qubits == other.data_qubits
            and self.measure_qubit == other.measure_qubit
            and self.bases == other.bases
            and self.flags == other.flags
        )

    def __lt__(self, other: Any) -> bool:
        if isinstance(other, Tile):
            return self._cmp_key() < other._cmp_key()
        return NotImplemented

    def __ne__(self, other: Any) -> bool:
        return not (self == other)

    def __hash__(self) -> int:
        return hash((Tile, self.data_qubits, self.measure_qubit, self.bases, self.flags))

    def __repr__(self) -> str:
        b = self.basis or self.bases
        extra = "" if not self.flags else f"\n    flags={sorted(self.flags)!r},"
        return f"""stimflow.Tile(
    data_qubits={self.data_qubits!r},
    measure_qubit={self.measure_qubit!r},
    bases={b!r},{extra}
)"""

    def to_pauli_map(self) -> PauliMap:
        return PauliMap({q: b for q, b in zip(self.data_qubits, self.bases) if q is not None})

    def with_data_qubit_cleared(self, q: complex) -> Tile:
        return self.with_edits(data_qubits=[None if d == q else d for d in self.data_qubits])

    def with_edits(
        self,
        *,
        bases: str | None = None,
        measure_qubit: complex | None | Literal["unspecified"] = "unspecified",
        data_qubits: Iterable[complex | None] | None = None,
        flags: Iterable[str] | None = None,
    ) -> Tile:
        if data_qubits is not None:
            data_qubits = tuple(data_qubits)
            if len(data_qubits) != len(self.data_qubits) and bases is None:
                if self.basis is None:
                    raise ValueError("Changed data qubit count of non-uniform basis tile.")
                bases = self.basis

        return Tile(
            bases=self.bases if bases is None else bases,
            measure_qubit=self.measure_qubit if measure_qubit == "unspecified" else measure_qubit,
            data_qubits=self.data_qubits if data_qubits is None else data_qubits,
            flags=self.flags if flags is None else flags,
        )

    def with_bases(self, bases: str) -> Tile:
        return self.with_edits(bases=bases)

    with_basis = with_bases

    def with_xz_flipped(self) -> Tile:
        f = {"X": "Z", "Y": "Y", "Z": "X"}
        return self.with_bases("".join(f[e] for e in self.bases))

    def with_transformed_coords(self, coord_transform: Callable[[complex], complex]) -> Tile:
        return self.with_edits(
            data_qubits=[None if d is None else coord_transform(d) for d in self.data_qubits],
            measure_qubit=(
                None if self.measure_qubit is None else coord_transform(self.measure_qubit)
            ),
        )

    def with_transformed_bases(
        self, basis_transform: Callable[[Literal["X", "Y", "Z"]], Literal["X", "Y", "Z"]]
    ) -> Tile:
        return self.with_bases(
            "".join(basis_transform(cast(Literal["X", "Y", "Z"], e)) for e in self.bases)
        )

    def __len__(self) -> int:
        return len(self.data_set)

    @functools.cached_property
    def data_set(self) -> frozenset[complex]:
        return frozenset(e for e in self.data_qubits if e is not None)

    @functools.cached_property
    def used_set(self) -> frozenset[complex]:
        if self.measure_qubit is None:
            return self.data_set
        return self.data_set | frozenset([self.measure_qubit])

    @functools.cached_property
    def basis(self) -> Literal["X", "Y", "Z"] | None:
        bs: set[Literal["X", "Y", "Z"]]
        bs = cast(Any, {b for q, b in zip(self.data_qubits, self.bases) if q is not None})
        if len(bs) == 0:
            # Fallback to including ejected qubits.
            bs = cast(Any, set(self.bases))
        if len(bs) != 1:
            return None
        return next(iter(bs))
