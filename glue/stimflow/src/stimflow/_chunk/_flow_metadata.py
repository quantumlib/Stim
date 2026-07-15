from __future__ import annotations

from collections.abc import Iterable


class FlowMetadata:
    """Metadata, based on a flow, to use during circuit generation."""

    def __init__(self, *, extra_coords: Iterable[float] = (), tag: str | None = ""):
        """Initializes a FlowMetadata instance.

        Args:
            extra_coords: Extra numbers to add to DETECTOR coordinate arguments. By default stimflow
                gives each detector an X, Y, and T coordinate. These numbers go afterward.
            tag: A tag to attach to DETECTOR or OBSERVABLE_INCLUDE instructions.

        Examples:
            >>> import stim
            >>> import stimflow as sf

            >>> def metadata_func(flow: sf.Flow) -> sf.FlowMetadata:
            ...     if 'postselect' in flow.flags:
            ...         return sf.FlowMetadata(extra_coords=[-1])
            ...     elif 'color=r' in flow.flags:
            ...         return sf.FlowMetadata(tag="red")
            ...     elif 'color=g' in flow.flags:
            ...         return sf.FlowMetadata(tag="green", extra_coords=[5, 6, 7])
            ...     elif 'color=b' in flow.flags:
            ...         return sf.FlowMetadata(tag="blue")
            ...     else:
            ...         raise NotImplementedError(f"Couldn't figure out {flow}")

            >>> compiler = sf.ChunkCompiler(metadata_func=metadata_func)
            >>> compiler.append(sf.Chunk(
            ...     circuit=stim.Circuit('''
            ...         QUBIT_COORDS(0) 0
            ...         R 0
            ...     '''),
            ...     flows=[sf.Flow(end=sf.PauliMap.from_zs([0]), flags={"color=g"})],
            ... ))
            >>> compiler.append_magic_end_chunk()
            >>> compiler.finish_circuit()
            stim.Circuit('''
                QUBIT_COORDS(0, 0) 0
                R 0
                TICK
                MPP Z0
                DETECTOR[green](0, 0, 0, 5, 6, 7) rec[-1]
            ''')
        """
        self.extra_coords: tuple[float, ...] = tuple(extra_coords)
        self.tag: str = tag or ""

    def __eq__(self, other) -> bool:
        if isinstance(other, FlowMetadata):
            return self.extra_coords == other.extra_coords and self.tag == other.tag
        return NotImplemented

    def __hash__(self) -> int:
        return hash((FlowMetadata, self.extra_coords, self.tag))

    def __repr__(self):
        return f"stimflow.FlowMetadata(extra_coords={self.extra_coords!r}, tag={self.tag!r})"
