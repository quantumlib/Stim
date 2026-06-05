from __future__ import annotations

from collections.abc import Callable, Iterable, Iterator, Set
from typing import Any, cast, Literal, TYPE_CHECKING

import stim

from stimflow._core._complex_util import sorted_complex, min_max_complex

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
    """An immutable qubit-to-pauli mapping.

    Similar to a stim.PauliString, but sparse instead of dense and also PauliMap
    doesn't track signs (i.e. X*Y produces Z instead of i*Z).

    The mapping can also be given a name. In some contexts, stimflow requires that Pauli mappings
    have a name (e.g. when specifying the Pauli mapping of a logical operator for a stabilizer code).

    Examples:
        >>> import stimflow as sf
        >>> p1 = sf.PauliMap({0: "X", 1: "Y", 2: "Z"})
        >>> p2 = sf.PauliMap.from_xs([1, 2, 3])
        >>> p3 = sf.PauliMap({"Z": [3, 4j]})
        >>> print(p1 * p2 * p3)
        X0*Z4j*Z1*Y2*Y3
    """

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
        obs_name: Any = None,
    ):
        """Initializes a PauliMap using maps of Paulis to/from qubits.

        Args:
            mapping: The association between qubits and paulis, specifiable in a variety of ways.
            obs_name: Defaults to None (no name). Can be set to an arbitrary hashable equatable value,
                in order to identify the Pauli map. A common convention used in the library is that
                named Pauli maps correspond to logical operators.

        Examples:
            >>> import stimflow as sf
            >>> import stim

            >>> print(sf.PauliMap())
            I

            >>> print(sf.PauliMap({0: "X", 1: "Y", 2: "Z"}))
            X0*Y1*Z2

            >>> print(sf.PauliMap({"X": [1, 2], "Y": 1+1j}))
            X1*Y(1+1j)*X2

            >>> print(sf.PauliMap(stim.PauliString("XYZ_X")))
            X0*Y1*Z2*X4

            >>> print(sf.PauliMap(sf.Tile(data_qubits=[1, 2, 3], bases="X")))
            X1*X2*X3

            >>> print(sf.PauliMap({0: "X", "Y": [0, 1]}))
            Z0*Y1

            >>> print(sf.PauliMap({0: "X", 1: "Y", 2: "Z"}, obs_name="test"))
            (obs_name='test') X0*Y1*Z2
        """

        self._dict: dict[complex, Literal["X", "Y", "Z"]]
        self.obs_name: Any = obs_name
        self._hash: int

        from stimflow._core._tile import Tile

        if isinstance(mapping, Tile):
            self._dict = dict(mapping.to_pauli_map().items())
        elif isinstance(mapping, PauliMap):
            self._dict = dict(mapping.items())
        elif isinstance(mapping, stim.PauliString):
            self._dict = {q: cast(Any, "_XYZ"[mapping[q]]) for q in mapping.pauli_indices()}
        elif mapping is not None:
            self._dict = {}
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

            self._dict = {complex(q): self._dict[q] for q in sorted_complex(self.keys())}
        else:
            self._dict = {}
        self._hash = hash((self.obs_name, tuple(self._dict.items())))

    def _min_max_complex_(self) -> tuple[complex, complex]:
        return min_max_complex(self.keys(), default=0)

    def _inline_svg_(self, *, q2p: Callable[[complex], complex], out_lines: list[str]):
        scale = abs(q2p(1) - q2p(0))
        for q, p in self.items():
            pt = q2p(q)
            out_lines.append(f'''<text x="{pt.real}" y="{pt.imag}" dominant-baseline="central" text-anchor="middle" font-size="{scale}">{p}</text>''')

    @staticmethod
    def from_xs(xs: Iterable[complex], *, name: Any = None) -> PauliMap:
        """Returns a PauliMap mapping the given qubits to the X basis."""
        return PauliMap({"X": xs}, obs_name=name)

    @staticmethod
    def from_ys(ys: Iterable[complex], *, name: Any = None) -> PauliMap:
        """Returns a PauliMap mapping the given qubits to the Y basis."""
        return PauliMap({"Y": ys}, obs_name=name)

    @staticmethod
    def from_zs(zs: Iterable[complex], *, name: Any = None) -> PauliMap:
        """Returns a PauliMap mapping the given qubits to the Z basis."""
        return PauliMap({"Z": zs}, obs_name=name)

    def __contains__(self, item: complex) -> bool:
        """Determines if the PauliMap maps the given qubit to a non-identity Pauli."""
        return self._dict.__contains__(item)

    def items(self) -> Iterable[tuple[complex, Literal["X", "Y", "Z"]]]:
        """Returns the (qubit, basis) pairs of the PauliMap."""
        return self._dict.items()

    def values(self) -> Iterable[Literal["X", "Y", "Z"]]:
        """Returns the bases used by the PauliMap."""
        return self._dict.values()

    def keys(self) -> Set[complex]:
        """Returns the qubits of the PauliMap."""
        return self._dict.keys()

    def get(self, key: complex, default: Any = None) -> Any:
        return self._dict.get(key, default)

    def __getitem__(self, item: complex) -> Literal["I", "X", "Y", "Z"]:
        return cast(Any, self._dict.get(item, "I"))

    def __len__(self) -> int:
        return len(self._dict)

    def __iter__(self) -> Iterator[complex]:
        return self._dict.__iter__()

    def with_obs_name(self, name: Any) -> PauliMap:
        """Returns the same PauliMap, but with the given name.

        Names are used to identify logical operators.
        """
        return PauliMap(self, obs_name=name)

    def _mul_term(self, q: complex, b: Literal["X", "Y", "Z"]):
        new_b = _multiplication_table[self._dict.pop(q, None)][b]
        if new_b is not None:
            self._dict[q] = new_b

    def with_basis(self, basis: Literal["X", "Y", "Z"]) -> PauliMap:
        """Returns the same PauliMap, but with all its qubits mapped to the given basis."""
        return PauliMap({q: basis for q in self.keys()}, obs_name=self.obs_name)

    def __bool__(self) -> bool:
        return bool(self._dict)

    def __mul__(self, other: PauliMap | Tile) -> PauliMap:
        from stimflow._core._tile import Tile

        if isinstance(other, Tile):
            other = other.to_pauli_map()

        result: dict[complex, Literal["X", "Y", "Z"]] = {}
        for q in self.keys() | other.keys():
            a = self._dict.get(q, "I")
            b = other._dict.get(q, "I")
            ax = a in "XY"
            az = a in "YZ"
            bx = b in "XY"
            bz = b in "YZ"
            cx = ax ^ bx
            cz = az ^ bz
            c = "IXZY"[cx + cz * 2]
            if c != "I":
                result[q] = cast(Literal["X", "Y", "Z"], c)
        return PauliMap(result, obs_name=self.obs_name if self.obs_name == other.obs_name else None)

    def __repr__(self) -> str:
        if self.obs_name is None:
            s2 = ""
        else:
            s2 = f", obs_name={self.obs_name!r}"
        qs = sorted_complex(self._dict)
        if len(self) > 1:
            p = set(self.values())
            if p == {'X'}:
                return f"stimflow.PauliMap.from_xs({qs!r}{s2})"
            if p == {'Z'}:
                return f"stimflow.PauliMap.from_zs({qs!r}{s2})"
        s = {q: self._dict[q] for q in qs}
        return f"stimflow.PauliMap({s!r}{s2})"

    def __str__(self) -> str:
        def simplify(c: complex) -> str:
            if c == int(c.real):
                return str(int(c.real))
            if c == c.real:
                return str(c.real)
            return str(c)

        result = "*".join(f"{self._dict[q]}{simplify(q)}" for q in sorted_complex(self.keys()))
        if not result:
            result = 'I'
        if self.obs_name is not None:
            result = f"(obs_name={self.obs_name!r}) " + result
        return result

    def with_xz_flipped(self) -> PauliMap:
        """Returns the same PauliMap, but with all qubits conjugated by H."""
        remap = {"X": "Z", "Y": "Y", "Z": "X"}
        return PauliMap({q: remap[p] for q, p in self._dict.items()}, obs_name=self.obs_name)

    def with_xy_flipped(self) -> PauliMap:
        """Returns the same PauliMap, but with all qubits conjugated by H_XY."""
        remap = {"X": "Y", "Y": "X", "Z": "Z"}
        return PauliMap({q: remap[p] for q, p in self._dict.items()}, obs_name=self.obs_name)

    def commutes(self, other: PauliMap) -> bool:
        """Determines if the pauli map commutes with another pauli map."""
        return not self.anticommutes(other)

    def anticommutes(self, other: PauliMap) -> bool:
        """Determines if the pauli map anticommutes with another pauli map."""
        t = 0
        for q in self.keys() & other.keys():
            t += self._dict[q] != other._dict[q]
        return t % 2 == 1

    def with_transformed_coords(self, transform: Callable[[complex], complex]) -> PauliMap:
        """Returns the same PauliMap but with coordinates transformed by the given function."""
        return PauliMap({transform(q): p for q, p in self._dict.items()}, obs_name=self.obs_name)

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
        return self._dict == other._dict

    def _sort_key(self) -> Any:
        return tuple((q.real, q.imag, p) for q, p in self._dict.items())

    def __lt__(self, other) -> bool:
        if not isinstance(other, PauliMap):
            return NotImplemented
        return self._sort_key() < other._sort_key()
