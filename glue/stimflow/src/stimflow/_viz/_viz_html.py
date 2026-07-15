from __future__ import annotations

from collections.abc import Callable, Iterable
from typing import TYPE_CHECKING, Any

import stim

from stimflow._viz._viz_circuit_html import _stim_circuit_html_viewer

if TYPE_CHECKING:
    import stimflow
    from stimflow._core import str_html


def html_viewer(
    obj: stim.Circuit | Any,
    *,
    background: (
        stimflow.Patch
        | stimflow.StabilizerCode
        | stimflow.ChunkInterface
        | dict[int, stimflow.Patch | stimflow.StabilizerCode | stimflow.ChunkInterface]
        | None
    ) = None,
    tile_color_func: (
        Callable[[stimflow.Tile], tuple[float, float, float, float] | tuple[float, float, float] | str]
        | None
    ) = None,
    width: int = 500,
    height: int = 500,
    known_error: Iterable[stim.ExplainedError] | None = None,
) -> str_html:
    """Creates an HTML page for viewing the given object.

    Args:
        obj: The object to be visualized.
        background: Something to draw in the background of the viewer (e.g. the
            stimflow.StabilizerCode implemented by the circuit being viewed).
        tile_color_func: Customizes how stabilizers and other operators are drawn.
        width: The width of the viewer.
        height: The height of the viewer.
        known_error: An error (e.g. returned from stim.Circuit.shortest_graphlike_error)
            to show as part of the object.

    Returns:
        The HTML string (as a stimflow.str_html, which inherits from python's `str`).

        (The result is of type stimflow.str_html so that its viewer is shown automatically
        in Jupyter notebooks, and also for convenience methods like `write_to`.)
    """

    from stimflow._chunk import Chunk
    if isinstance(obj, stim.Circuit):
        return _stim_circuit_html_viewer(
            obj,
            background=background,
            tile_color_func=tile_color_func,
            width=width,
            height=height,
            known_error=known_error,
        )

    elif isinstance(obj, Chunk):
        circuit = obj.to_closed_circuit(skip_verification=True)
        if background is None:
            start = obj.start_patch()
            end = obj.end_patch()
            if len(start.tiles) == 0:
                background = end
            elif len(end.tiles) == 0:
                background = start
            else:
                background = {0: start, circuit.num_ticks: end}
        return _stim_circuit_html_viewer(
            circuit,
            background=background,
            tile_color_func=tile_color_func,
            known_error=known_error,
            width=width,
            height=height,
        )

    else:
        raise NotImplementedError(f"Don't know how to make an html viewer for {type(obj)=}.")
