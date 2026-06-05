from __future__ import annotations

from collections.abc import Iterable


class FlowMetadata:
    """Metadata, based on a flow, to use during circuit generation."""

    def __init__(self, *, extra_coords: Iterable[float] = (), tag: str | None = ""):
        """

        Args:
            extra_coords: Extra numbers to add to DETECTOR coordinate arguments. By default stimflow
                gives each detector an X, Y, and T coordinate. These numbers go afterward.
            tag: A tag to attach to DETECTOR or OBSERVABLE_INCLUDE instructions.
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
