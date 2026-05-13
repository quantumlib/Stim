from __future__ import annotations

import collections
import functools
from collections.abc import Callable, Iterable, Iterator
from typing import Literal, overload, TYPE_CHECKING

from stimflow._core import PauliMap, str_svg, Tile

if TYPE_CHECKING:
    from stimflow._chunk._stabilizer_code import StabilizerCode


class Patch:
    """A collection of annotated stabilizers."""

    def __init__(self, tiles: Iterable[Tile | PauliMap], *, do_not_sort: bool = False):
        kept_tiles = []
        for tile in tiles:
            if isinstance(tile, Tile):
                kept_tiles.append(tile)
            elif isinstance(tile, PauliMap):
                kept_tiles.append(tile.to_tile())
            else:
                raise ValueError(f"Don't know how to interpret this as a stimflow.Tile: {tile=}")
        if not do_not_sort:
            kept_tiles = sorted(kept_tiles)

        self.tiles: tuple[Tile, ...] = tuple(kept_tiles)

    def __len__(self) -> int:
        return len(self.tiles)

    @overload
    def __getitem__(self, item: int) -> Tile:
        pass

    @overload
    def __getitem__(self, item: slice) -> Patch:
        pass

    def __getitem__(self, item: int | slice) -> Patch | Tile:
        if isinstance(item, slice):
            return Patch(self.tiles[item])
        if isinstance(item, int):
            return self.tiles[item]
        raise NotImplementedError(f"{item=}")

    def __iter__(self) -> Iterator[Tile]:
        return self.tiles.__iter__()

    @functools.cached_property
    def partitioned_tiles(self) -> tuple[tuple[Tile, ...], ...]:
        """Returns the tiles of the patch, but split into non-overlapping groups."""
        qubit_used: set[tuple[complex, int]] = set()
        layers: collections.defaultdict[int, list[Tile]] = collections.defaultdict(list)

        for tile in self.tiles:
            layer_index = 0
            while any((q, layer_index) in qubit_used for q in tile.data_set):
                layer_index += 1
            qubit_used.update((q, layer_index) for q in tile.data_set)
            layers[layer_index].append(tile)
        return tuple(tuple(v) for _, v in sorted(layers.items()))

    def with_remaining_degrees_of_freedom_as_logicals(self) -> StabilizerCode:
        """Solves for the logical observables, given only the stabilizers."""
        from stimflow._chunk import StabilizerCode

        return StabilizerCode(stabilizers=self).with_remaining_degrees_of_freedom_as_logicals()

    def with_edits(self, *, tiles: Iterable[Tile] | None = None) -> Patch:
        return Patch(tiles=self.tiles if tiles is None else tiles)

    def with_transformed_coords(self, coord_transform: Callable[[complex], complex]) -> Patch:
        return Patch([e.with_transformed_coords(coord_transform) for e in self.tiles])

    def with_transformed_bases(
        self, basis_transform: Callable[[Literal["X", "Y", "Z"]], Literal["X", "Y", "Z"]]
    ) -> Patch:
        return Patch([e.with_transformed_bases(basis_transform) for e in self.tiles])

    def with_only_x_tiles(self) -> Patch:
        return Patch([tile for tile in self.tiles if tile.basis == "X"])

    def with_only_y_tiles(self) -> Patch:
        return Patch([tile for tile in self.tiles if tile.basis == "Y"])

    def with_only_z_tiles(self) -> Patch:
        return Patch([tile for tile in self.tiles if tile.basis == "Z"])

    @functools.cached_property
    def m2tile(self) -> dict[complex, Tile]:
        return {e.measure_qubit: e for e in self.tiles}

    def _repr_svg_(self) -> str:
        return self.to_svg()

    def to_svg(
        self,
        *,
        title: str | list[str] | None = None,
        other: Patch | StabilizerCode | Iterable[Patch | StabilizerCode] = (),
        show_order: bool = False,
        show_measure_qubits: bool = False,
        show_data_qubits: bool = True,
        system_qubits: Iterable[complex] = (),
        show_coords: bool = True,
        opacity: float = 1,
        show_obs: bool = False,
        rows: int | None = None,
        cols: int | None = None,
        tile_color_func: Callable[[Tile], str] | None = None,
    ) -> str_svg:
        from stimflow._chunk._stabilizer_code import StabilizerCode
        from stimflow._viz import svg

        patches = [self] + ([other] if isinstance(other, (Patch, StabilizerCode)) else list(other))

        return svg(
            objects=patches,
            show_measure_qubits=show_measure_qubits,
            show_data_qubits=show_data_qubits,
            show_order=show_order,
            system_qubits=system_qubits,
            opacity=opacity,
            show_coords=show_coords,
            show_obs=show_obs,
            rows=rows,
            cols=cols,
            tile_color_func=tile_color_func,
            title=title,
        )

    def with_xz_flipped(self) -> Patch:
        trans: dict[Literal["X", "Y", "Z"], Literal["X", "Y", "Z"]] = {"X": "Z", "Y": "Y", "Z": "X"}
        return self.with_transformed_bases(trans.__getitem__)

    @functools.cached_property
    def used_set(self) -> frozenset[complex]:
        """Returns the set of all data and measure qubits used by tiles in the patch."""
        result: set[complex] = set()
        for e in self.tiles:
            result |= e.used_set
        return frozenset(result)

    @functools.cached_property
    def data_set(self) -> frozenset[complex]:
        """Returns the set of all data qubits used by tiles in the patch."""
        result = set()
        for e in self.tiles:
            for q in e.data_qubits:
                if q is not None:
                    result.add(q)
        return frozenset(result)

    def __eq__(self, other):
        if not isinstance(other, Patch):
            return NotImplemented
        return self.tiles == other.tiles

    def __ne__(self, other):
        return not (self == other)

    @functools.cached_property
    def measure_set(self) -> frozenset[complex]:
        """Returns the set of all measure qubits used by tiles in the patch."""
        return frozenset(e.measure_qubit for e in self.tiles if e.measure_qubit is not None)

    def __add__(self, other: Patch) -> Patch:
        if not isinstance(other, Patch):
            return NotImplemented
        return Patch([*self, *other])

    def __repr__(self):
        return "\n".join(
            [
                "stimflow.Patch(tiles=[",
                *[f"    {e!r},".replace("\n", "\n    ") for e in self.tiles],
                "])",
            ]
        )
