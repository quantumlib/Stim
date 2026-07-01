from __future__ import annotations

import collections
import functools
from collections.abc import Callable, Iterable

from stimflow._chunk._patch import Patch
from stimflow._chunk._stabilizer_code import StabilizerCode
from stimflow._core import PauliMap, str_svg, Tile


class ChunkInterface:
    """Specifies a set of stabilizers and observables that a chunk can consume or prepare."""

    def __init__(self, ports: Iterable[PauliMap], *, discards: Iterable[PauliMap] = ()):
        self.ports = frozenset(ports)
        self.discards = frozenset(discards)

    def partitioned_detector_flows(self) -> list[list[PauliMap]]:
        """Returns the stabilizers of the interface, split into non-overlapping groups."""
        qubit_used: set[tuple[complex, int]] = set()
        layers: collections.defaultdict[int, list[PauliMap]] = collections.defaultdict(list)

        for port in sorted(self.ports):
            if port.obs_name is None:
                layer_index = 0
                while any((q, layer_index) in qubit_used for q in port.keys()):
                    layer_index += 1
                qubit_used.update((q, layer_index) for q in port.keys())
                layers[layer_index].append(port)
        return [v for k, v in sorted(layers.items())]

    def with_transformed_coords(self, transform: Callable[[complex], complex]) -> ChunkInterface:
        """Returns the same interface, but with coordinates transformed by the given function."""
        return ChunkInterface(
            ports=[port.with_transformed_coords(transform) for port in self.ports],
            discards=[discard.with_transformed_coords(transform) for discard in self.discards],
        )

    @functools.cached_property
    def used_set(self) -> frozenset[complex]:
        """Returns the set of qubits used in any flow mentioned by the chunk interface."""
        return frozenset(q for port in self.ports | self.discards for q in port.keys())

    def to_svg(
        self,
        *,
        show_order: bool = False,
        show_measure_qubits: bool = False,
        show_data_qubits: bool = True,
        system_qubits: Iterable[complex] = (),
        opacity: float = 1,
        show_coords: bool = True,
        show_obs: bool = True,
        other: StabilizerCode | Patch | Iterable[StabilizerCode | Patch] | None = None,
        tile_color_func: Callable[[Tile], str] | None = None,
        rows: int | None = None,
        cols: int | None = None,
        find_logical_err_max_weight: int | None = None,
    ) -> str_svg:
        flat: list[StabilizerCode | Patch | ChunkInterface] = [self]
        if isinstance(other, (StabilizerCode, Patch, ChunkInterface)):
            flat.append(other)
        elif other is not None:
            flat.extend(other)

        from stimflow._viz import svg_viewer

        return svg_viewer(
            objects=flat,
            show_obs=show_obs,
            show_measure_qubits=show_measure_qubits,
            show_data_qubits=show_data_qubits,
            show_order=show_order,
            find_logical_err_max_weight=find_logical_err_max_weight,
            system_qubits=system_qubits,
            opacity=opacity,
            show_coords=show_coords,
            tile_color_func=tile_color_func,
            cols=cols,
            rows=rows,
        )

    def without_discards(self) -> ChunkInterface:
        """Returns the same chunk interface, but with discarded flows not included."""
        return self.with_edits(discards=())

    def without_keyed(self) -> ChunkInterface:
        """Returns the same chunk interface, but without logical flows (named flows)."""
        return ChunkInterface(
            ports=[port for port in self.ports if port.obs_name is None],
            discards=[discard for discard in self.discards if discard.obs_name is None],
        )

    def with_discards_as_ports(self) -> ChunkInterface:
        """Returns the same chunk interface, but with discarded flows turned into normal flows."""
        return self.with_edits(discards=(), ports=self.ports | self.discards)

    def __repr__(self) -> str:
        lines = ["stimflow.ChunkInterface("]

        lines.append("    ports=[")
        for port in sorted(self.ports):
            lines.append(f"        {port!r},")
        lines.append("    ],")

        if self.discards:
            lines.append("    discards=[")
            for discard in sorted(self.discards):
                lines.append(f"        {discard!r},")
            lines.append("    ],")

        lines.append(")")
        return "\n".join(lines)

    def __str__(self) -> str:
        lines = []
        for port in sorted(self.ports):
            lines.append(str(port))
        for discard in sorted(self.discards):
            lines.append(f"discard {discard}")
        return "\n".join(lines)

    def with_edits(
        self, *, ports: Iterable[PauliMap] | None = None, discards: Iterable[PauliMap] | None = None
    ) -> ChunkInterface:
        """Returns an equivalent chunk interface but with the given values replaced."""
        return ChunkInterface(
            ports=self.ports if ports is None else ports,
            discards=self.discards if discards is None else discards,
        )

    def __eq__(self, other):
        if not isinstance(other, ChunkInterface):
            return NotImplemented
        return self.ports == other.ports and self.discards == other.discards

    @functools.cached_property
    def data_set(self) -> frozenset[complex]:
        """Returns the set of qubits used by the interface's stabilizers and observables."""
        return frozenset(
            q
            for pauli_string_list in [self.ports, self.discards]
            for ps in pauli_string_list
            for q in ps
        )

    def to_patch(self) -> Patch:
        """Returns a stimflow.Patch with tiles equal to the chunk interface's stabilizers."""
        return Patch(
            tiles=[
                Tile(bases="".join(port.values()), data_qubits=port.keys(), measure_qubit=None)
                for pauli_string_list in [self.ports, self.discards]
                for port in pauli_string_list
                if port.obs_name is None
            ]
        )

    def to_code(self) -> StabilizerCode:
        """Returns a stimflow.StabilizerCode with an equivalent interface."""
        return StabilizerCode(
            stabilizers=self.to_patch(),
            logicals=[
                port
                for pauli_string_list in [self.ports, self.discards]
                for port in pauli_string_list
                if port.obs_name is not None
            ],
        )
