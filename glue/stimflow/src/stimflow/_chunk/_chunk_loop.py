from __future__ import annotations

from collections.abc import Callable, Iterable
from typing import TYPE_CHECKING

import stim

from stimflow._chunk._code_util import (
    verify_distance_is_at_least,
)
from stimflow._chunk._patch import Patch
from stimflow._chunk._stabilizer_code import StabilizerCode
from stimflow._core import NoiseModel, str_html, Tile

if TYPE_CHECKING:
    from stimflow._chunk._chunk import Chunk
    from stimflow._chunk._chunk_interface import ChunkInterface
    from stimflow._chunk._chunk_reflow import ChunkReflow


class ChunkLoop:
    """Specifies a series of chunks to repeat a fixed number of times.

    The loop invariant is that the last chunk's end interface should match the
    first chunk's start interface (unless the number of repetitions is less than
    2).

    For duck typing purposes, many methods supported by Chunk are supported by
    ChunkLoop.

    Examples:
        >>> import stim
        >>> import stimflow as sf
        >>> zz = sf.PauliMap({0: 'Z', 1 + 1j: 'Z'})
        >>> lz = sf.PauliMap({0: 'Z'}, obs_name='L_ZI')
        >>> lx = sf.PauliMap({0: 'X', 1 + 1j: 'X'}, obs_name='L_XX')
        >>> idle_chunk = sf.Chunk(
        ...     stim.Circuit('''
        ...         QUBIT_COORDS(0, 0) 0
        ...         QUBIT_COORDS(0, 1) 1
        ...         QUBIT_COORDS(1, 1) 2
        ...         R 1
        ...         CX 0 1 2 1
        ...         M 1
        ...     '''),
        ...     flows=[
        ...         sf.Flow(start=zz, measurement_indices=[0]),
        ...         sf.Flow(end=zz, measurement_indices=[0]),
        ...         sf.Flow(start=lz, end=lz),
        ...         sf.Flow(start=lx, end=lx),
        ...     ]
        ... )
        >>> idle_ten_times = sf.ChunkLoop([idle_chunk], repetitions=10)
        >>> idle_ten_times.verify()
    """

    def __init__(self, chunks: Iterable[Chunk | ChunkLoop], repetitions: int):
        self.chunks = tuple(chunks)
        self.repetitions = repetitions

    def start_interface(self) -> ChunkInterface:
        """Returns the start interface of the first chunk in the loop."""
        return self.chunks[0].start_interface()

    def end_interface(self) -> ChunkInterface:
        """Returns the end interface of the last chunk in the loop."""
        return self.chunks[-1].end_interface()

    def verify(
        self,
        *,
        expected_in: ChunkInterface | None = None,
        expected_out: ChunkInterface | None = None,
    ):
        """Verifies that the loop is consistent with its declared flows.

        A loop is consistent with its declared flows if:
        - It has repetitions=1 or else its start_interface matches its end_interface.
        - Sub-chunks have start/end interfaces consistent with their neighbors.
        """
        expected_ins: list[ChunkInterface | None] = [c.end_interface() for c in self.chunks]
        expected_ins = expected_ins[-1:] + expected_ins[:-1]

        expected_outs: list[ChunkInterface | None] = [c.start_interface() for c in self.chunks]
        expected_outs = expected_outs[1:] + expected_outs[:1]

        if self.repetitions == 1:
            expected_ins[0] = None
            expected_outs[-1] = None
        if expected_in is not None:
            expected_ins[0] = expected_in
        if expected_out is not None:
            expected_outs[-1] = expected_out
        for k, (chunk, inp, out) in enumerate(zip(self.chunks, expected_ins, expected_outs)):
            try:
                chunk.verify(expected_in=inp, expected_out=out)
            except (AssertionError, ValueError) as ex:
                raise ValueError(f"ChunkLoop failed to verify at sub-chunk index {k}") from ex

    def __mul__(self, other: int) -> ChunkLoop:
        return self.with_repetitions(other * self.repetitions)

    def time_reversed(self) -> ChunkLoop:
        """Returns the same loop, but time reversed.

        The time reversed loop has reversed flows, implemented by performs operations in the
        reverse order and exchange measurements for resets (and vice versa) as appropriate.
        It has exactly the same fault tolerant structure, just mirrored in time.
        """
        rev_chunks = [chunk.time_reversed() for chunk in self.chunks[::-1]]
        return ChunkLoop(rev_chunks, self.repetitions)

    def with_repetitions(self, new_repetitions: int) -> ChunkLoop:
        """Returns the same loop, but with a different number of repetitions."""
        return ChunkLoop(chunks=self.chunks, repetitions=new_repetitions)

    def start_patch(self) -> Patch:
        return self.chunks[0].start_patch()

    def end_patch(self) -> Patch:
        return self.chunks[-1].end_patch()

    def flattened(self) -> list[Chunk | ChunkReflow]:
        """Unrolls the loop, and any sub-loops, into a series of chunks.

        Examples:
            >>> import stim
            >>> import stimflow as sf
            >>> chunk = sf.Chunk(
            ...     circuit=stim.Circuit(),
            ...     flows=[],
            ... )
            >>> loop = sf.ChunkLoop([chunk, chunk], repetitions=4)
            >>> loop.flattened() == [chunk, chunk]
            True
            >>> loop2 = sf.ChunkLoop([loop, chunk], repetitions=5)
            >>> loop2.flattened() == [chunk, chunk, chunk]
            True
        """
        return [e for c in self.chunks for e in c.flattened()]

    def find_distance(
        self,
        *,
        max_search_weight: int,
        noise: float | NoiseModel = 1e-3,
        noiseless_qubits: Iterable[float | int | complex] = (),
    ) -> int:
        err = self.find_logical_error(
            max_search_weight=max_search_weight, noise=noise, noiseless_qubits=noiseless_qubits
        )
        return len(err)

    def to_closed_circuit(self) -> stim.Circuit:
        """Compiles the chunk into a circuit by conjugating with mpp init/end chunks."""
        from stimflow._chunk._chunk_compiler import ChunkCompiler

        compiler = ChunkCompiler()
        compiler.append_magic_init_chunk()
        compiler.append(self)
        compiler.append_magic_end_chunk()
        return compiler.finish_circuit()

    def verify_distance_is_at_least(self, minimum_distance: int, *, noise: float | NoiseModel = 1e-3):
        """Verifies undetected logical errors require at least the given number of physical errors.

        Verifies using a uniform depolarizing circuit noise model.
        """
        __tracebackhide__ = True
        circuit = self.to_closed_circuit()
        if isinstance(noise, float):
            noise = NoiseModel.uniform_depolarizing(1e-3, allow_multiple_uses_of_a_qubit_in_one_tick=True)
        circuit = noise.noisy_circuit_skipping_mpp_boundaries(circuit)
        verify_distance_is_at_least(circuit, minimum_distance)

    def find_logical_error(
        self,
        *,
        max_search_weight: int,
        noise: float | NoiseModel = 1e-3,
        noiseless_qubits: Iterable[float | int | complex] = (),
    ) -> list[stim.ExplainedError]:
        """Searches for a minium distance undetected logical error.

        By default, searches using a uniform depolarizing circuit noise model.
        """
        circuit = self.to_closed_circuit()
        if isinstance(noise, float):
            noise = NoiseModel.uniform_depolarizing(1e-3, allow_multiple_uses_of_a_qubit_in_one_tick=True)
        circuit = noise.noisy_circuit_skipping_mpp_boundaries(
            circuit, immune_qubit_coords=noiseless_qubits
        )
        if max_search_weight == 2:
            return circuit.shortest_graphlike_error(canonicalize_circuit_errors=True)
        return circuit.search_for_undetectable_logical_errors(
            dont_explore_edges_with_degree_above=max_search_weight,
            dont_explore_detection_event_sets_with_size_above=max_search_weight,
            dont_explore_edges_increasing_symptom_degree=False,
            canonicalize_circuit_errors=True,
        )

    def to_html_viewer(
        self,
        *,
        patch: Patch | StabilizerCode | ChunkInterface | None = None,
        tile_color_func: Callable[[Tile], tuple[float, float, float, float]] | None = None,
        known_error: Iterable[stim.ExplainedError] | None = None,
    ) -> str_html:
        """Returns an HTML document containing a viewer for the chunk loop's circuit."""
        from stimflow._viz import html_viewer

        if patch is None:
            patch = self.start_patch()
            if len(patch.tiles) == 0:
                patch = self.end_patch()
        return html_viewer(
            self.to_closed_circuit(),
            background=patch,
            tile_color_func=tile_color_func,
            known_error=known_error,
        )
