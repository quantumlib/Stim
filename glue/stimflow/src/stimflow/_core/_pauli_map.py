from __future__ import annotations

from collections.abc import Callable, Iterable, Iterator, Set
from typing import Any, cast, Literal, TYPE_CHECKING

import stim

from stimflow._core._complex_util import sorted_complex

if TYPE_CHECKING:
    from stimflow._core._tile import Tile


_multiplication_table: dict[
    Literal["X", "Y", "Z"] | None,
    dict[Literal["X", "Y", "Z"] | None, Literal["X", "Y", "Z"] | None],
] = {
    None: {None: None, "X": "X", "Y": "Y", "Z": "Z"},
    "X": {None: "X", "X": None, "Y": "Z", "Z": "Y"},
    "Y": {None: "Y", "X": "Z", "Y": None, "Z": "X"},
    "Z": {None: "Z", "X": "Y", "Y": "X", "Z": None},
}


class PauliMap:
    """A qubit-to-pauli mapping."""

    def __init__(
        self,
        mapping: (
            dict[complex, Literal["X", "Y", "Z"] | str]
            | dict[Literal["X", "Y", "Z"] | str, complex | Iterable[complex]]
            | PauliMap
            | Tile
            | stim.PauliString
            | None
        ) = None,
        *,
        name: Any = None,
    ):
        """Initializes a PauliMap using maps of Paulis to/from qubits.

        Args:
            mapping: The association between qubits and paulis, specifiable in a variety of ways.
            name: Defaults to None (no name). Can be set to an arbitrary hashable equatable value,
                in order to identify the Pauli map. A common convention used in the library is that
                named Pauli maps correspond to logical operators.
        """

        self.qubits: dict[complex, Literal["X", "Y", "Z"]]
        self.name = name
        self._hash: int

        from stimflow._core._tile import Tile

        if isinstance(mapping, Tile):
            self.qubits = dict(mapping.to_pauli_map().qubits)
        elif isinstance(mapping, PauliMap):
            self.qubits = dict(mapping.qubits)
        elif isinstance(mapping, stim.PauliString):
            self.qubits = {q: cast(Any, "_XYZ"[mapping[q]]) for q in mapping.pauli_indices()}
        elif mapping is not None:
            self.qubits = {}
            for k, v in mapping.items():
                if (v == "X" or v == "Y" or v == "Z") and isinstance(k, (int, float, complex)):
                    self._mul_term(k, cast(Any, v))
                elif (k == "X" or k == "Y" or k == "Z") and isinstance(v, (int, float, complex)):
                    self._mul_term(v, cast(Any, k))
                elif (
                    (k == "X" or k == "Y" or k == "Z")
                    and isinstance(v, Iterable)
                    and (v_copy := list(v)) is not None
                    and all(isinstance(v2, (int, float, complex)) for v2 in v_copy)
                ):
                    for v2 in v_copy:
                        self._mul_term(cast(Any, v2), cast(Any, k))
                else:
                    raise ValueError(f"Don't know how to interpret {k=}: {v=} as a pauli mapping.")

            self.qubits = {complex(q): self.qubits[q] for q in sorted_complex(self.keys())}
        else:
            self.qubits = {}
        self._hash = hash((self.name, tuple(self.qubits.items())))

    @staticmethod
    def from_xs(xs: Iterable[complex], *, name: Any = None) -> PauliMap:
        """Returns a PauliMap mapping the given qubits to the X basis."""
        return PauliMap({"X": xs}, name=name)

    @staticmethod
    def from_ys(ys: Iterable[complex], *, name: Any = None) -> PauliMap:
        """Returns a PauliMap mapping the given qubits to the Y basis."""
        return PauliMap({"Y": ys}, name=name)

    @staticmethod
    def from_zs(zs: Iterable[complex], *, name: Any = None) -> PauliMap:
        """Returns a PauliMap mapping the given qubits to the Z basis."""
        return PauliMap({"Z": zs}, name=name)

    def __contains__(self, item: complex) -> bool:
        """Determines if the PauliMap contains maps the given qubit to a non-identity Pauli."""
        return self.qubits.__contains__(item)

    def items(self) -> Iterable[tuple[complex, Literal["X", "Y", "Z"]]]:
        """Returns the (qubit, basis) pairs of the PauliMap."""
        return self.qubits.items()

    def values(self) -> Iterable[Literal["X", "Y", "Z"]]:
        """Returns the bases used by the PauliMap."""
        return self.qubits.values()

    def keys(self) -> Set[complex]:
        """Returns the qubits of the PauliMap."""
        return self.qubits.keys()

    def get(self, key: complex, default: Any = None) -> Any:
        return self.qubits.get(key, default)

    def __getitem__(self, item: complex) -> Literal["I", "X", "Y", "Z"]:
        return cast(Any, self.qubits.get(item, "I"))

    def __len__(self) -> int:
        return len(self.qubits)

    def __iter__(self) -> Iterator[complex]:
        return self.qubits.__iter__()

    def with_name(self, name: Any) -> PauliMap:
        """Returns the same PauliMap, but with the given name.

        Names are used to identify logical operators.
        """
        return PauliMap(self, name=name)

    def _mul_term(self, q: complex, b: Literal["X", "Y", "Z"]):
        new_b = _multiplication_table[self.qubits.pop(q, None)][b]
        if new_b is not None:
            self.qubits[q] = new_b

    def with_basis(self, basis: Literal["X", "Y", "Z"]) -> PauliMap:
        """Returns the same PauliMap, but with all its qubits mapped to the given basis."""
        return PauliMap({q: basis for q in self.keys()}, name=self.name)

    def __bool__(self) -> bool:
        return bool(self.qubits)

    def __mul__(self, other: PauliMap | Tile) -> PauliMap:
        from stimflow._core._tile import Tile

        if isinstance(other, Tile):
            other = other.to_pauli_map()

        result: dict[complex, Literal["X", "Y", "Z"]] = {}
        for q in self.keys() | other.keys():
            a = self.qubits.get(q, "I")
            b = other.qubits.get(q, "I")
            ax = a in "XY"
            az = a in "YZ"
            bx = b in "XY"
            bz = b in "YZ"
            cx = ax ^ bx
            cz = az ^ bz
            c = "IXZY"[cx + cz * 2]
            if c != "I":
                result[q] = cast(Literal["X", "Y", "Z"], c)
        return PauliMap(result)

    def __repr__(self) -> str:
        if self.name is None:
            s2 = ""
        else:
            s2 = f", name={self.name!r}"
        qs = sorted_complex(self.qubits)
        if len(self) > 1:
            p = set(self.values())
            if p == {'X'}:
                return f"stimflow.PauliMap.from_xs({qs!r}{s2})"
            if p == {'Z'}:
                return f"stimflow.PauliMap.from_zs({qs!r}{s2})"
        s = {q: self.qubits[q] for q in qs}
        return f"stimflow.PauliMap({s!r}{s2})"

    def __str__(self) -> str:
        def simplify(c: complex) -> str:
            if c == int(c.real):
                return str(int(c.real))
            if c == c.real:
                return str(c.real)
            return str(c)

        result = "*".join(f"{self.qubits[q]}{simplify(q)}" for q in sorted_complex(self.keys()))
        if self.name is not None:
            result = f"(name={self.name!r}) " + result
        return result

    def with_xz_flipped(self) -> PauliMap:
        """Returns the same PauliMap, but with all qubits conjugated by H."""
        remap = {"X": "Z", "Y": "Y", "Z": "X"}
        return PauliMap({q: remap[p] for q, p in self.qubits.items()}, name=self.name)

    def with_xy_flipped(self) -> PauliMap:
        """Returns the same PauliMap, but with all qubits conjugated by H_XY."""
        remap = {"X": "Y", "Y": "X", "Z": "Z"}
        return PauliMap({q: remap[p] for q, p in self.qubits.items()}, name=self.name)

    def commutes(self, other: PauliMap) -> bool:
        """Determines if the pauli map commutes with another pauli map."""
        return not self.anticommutes(other)

    def anticommutes(self, other: PauliMap) -> bool:
        """Determines if the pauli map anticommutes with another pauli map."""
        t = 0
        for q in self.keys() & other.keys():
            t += self.qubits[q] != other.qubits[q]
        return t % 2 == 1

    def with_transformed_coords(self, transform: Callable[[complex], complex]) -> PauliMap:
        """Returns the same PauliMap but with coordinates transformed by the given function."""
        return PauliMap({transform(q): p for q, p in self.qubits.items()}, name=self.name)

    def to_stim_pauli_string(
        self, q2i: dict[complex, int], *, num_qubits: int | None = None
    ) -> stim.PauliString:
        """Converts into a stim.PauliString."""
        if num_qubits is None:
            num_qubits = max([q2i[q] + 1 for q in self.keys()], default=0)
        result = stim.PauliString(num_qubits)
        for q, p in self.items():
            result[q2i[q]] = p
        return result

    def to_stim_targets(self, q2i: dict[complex, int]) -> list[stim.GateTarget]:
        """Converts into a stim combined pauli target like 'X1*Y2*Z3'."""
        assert len(self) > 0

        targets = []
        for q, p in self.items():
            targets.append(stim.target_pauli(q2i[q], p))
            targets.append(stim.target_combiner())
        targets.pop()
        return targets

    def to_tile(self) -> Tile:
        """Converts the PauliMap into a stimflow.Tile."""
        from stimflow._core._tile import Tile

        qs = list(self.keys())
        return Tile(bases="".join(self.values()), data_qubits=qs)

    def __hash__(self) -> int:
        return self._hash

    def __eq__(self, other) -> bool:
        if not isinstance(other, PauliMap):
            return NotImplemented
        return self.qubits == other.qubits

    def _sort_key(self) -> Any:
        return tuple((q.real, q.imag, p) for q, p in self.qubits.items())

    def __lt__(self, other) -> bool:
        if not isinstance(other, PauliMap):
            return NotImplemented
        return self._sort_key() < other._sort_key()
