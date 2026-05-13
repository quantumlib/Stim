from __future__ import annotations

import math
from collections.abc import Callable, Iterable
from typing import Literal, TYPE_CHECKING

import stim

from stimflow._viz._viz_patch_svg import _draw_patch

if TYPE_CHECKING:
    import stimflow


def svg(
    objects: Iterable[stimflow.Patch | stimflow.StabilizerCode | stimflow.ChunkInterface | stim.Circuit],
    *,
    background: stimflow.Patch | stimflow.StabilizerCode | stimflow.ChunkInterface | stim.Circuit | None = None,
    title: str | list[str] | None = None,
    canvas_height: int | None = None,
    show_order: bool = False,
    show_obs: bool = True,
    opacity: float = 1,
    show_measure_qubits: bool = True,
    show_data_qubits: bool = False,
    system_qubits: Iterable[complex] = (),
    show_all_qubits: bool = False,
    extra_used_coords: Iterable[complex] = (),
    show_coords: bool = True,
    find_logical_err_max_weight: int | None = None,
    rows: int | None = None,
    cols: int | None = None,
    tile_color_func: (
        Callable[
            [stimflow.Tile], str | tuple[float, float, float] | tuple[float, float, float, float] | None
        ]
        | None
    ) = None,
    stabilizer_style: Literal["polygon", "circles"] | None = "polygon",
    observable_style: Literal["label", "polygon", "circles"] = "label",
    show_frames: bool = True,
    pad: float | None = None,
) -> stimflow.str_svg:
    """Returns an SVG image of the given objects."""
    system_qubits = frozenset(system_qubits)
    if canvas_height is None:
        canvas_height = 500

    extra_used_coords = frozenset(extra_used_coords)
    from stimflow._layers import LayerCircuit

    patches = tuple(
        patch.to_stim_circuit() if isinstance(patch, LayerCircuit) else patch for patch in objects
    )
    all_points: set[complex] = set()
    all_points.update(system_qubits)
    all_points.update(extra_used_coords)
    for patch in patches:
        if isinstance(patch, stim.Circuit):
            all_points.update(
                v[0] + v[1] * 1j for v in patch.get_final_qubit_coordinates().values()
            )
        else:
            all_points.update(patch.used_set)
    if show_all_qubits:
        system_qubits = frozenset(all_points)
    from stimflow._core import min_max_complex

    min_c, max_c = min_max_complex(all_points, default=0)
    min_c -= 0.5 + 0.5j
    max_c += 0.5 + 0.5j
    offset: complex = 0
    if title is not None:
        min_c -= 1j
        offset += 1j
    if show_coords:
        min_c -= 1 + 1j
    box_width = max_c.real - min_c.real
    box_height = max_c.imag - min_c.imag
    if pad is None:
        pad = max(box_width, box_height) * 0.01 + 0.25
    box_x_pitch = box_width + pad
    box_y_pitch = box_height + pad
    if cols is None and rows is None:
        cols = min(len(patches), math.ceil(math.sqrt(len(patches) * 2)))
        rows = math.ceil(len(patches) / max(1, cols))
    elif cols is None:
        cols = math.ceil(len(patches) / max(1, rows))
    elif rows is None:
        rows = math.ceil(len(patches) / max(1, cols))
    else:
        assert cols * rows >= len(patches)
    total_height = max(1.0, box_y_pitch * rows - pad + offset.imag)
    total_width = max(1.0, box_x_pitch * cols - pad + offset.real)
    scale_factor = canvas_height / max(total_height, 1)
    canvas_width = int(math.ceil(canvas_height * (total_width / total_height)))

    def patch_q2p(patch_index: int, q: complex) -> complex:
        q -= min_c
        q += offset
        q += box_x_pitch * (patch_index % cols)
        q += box_y_pitch * (patch_index // cols) * 1j
        q *= scale_factor
        return q

    lines = [
        f"""<svg viewBox="0 0 {canvas_width} {canvas_height}" xmlns="http://www.w3.org/2000/svg">"""
    ]

    if isinstance(title, str):
        lines.append(
            f"<text"
            f' x="{canvas_width / 2}"'
            f' y="{0.2 * scale_factor}"'
            f' fill="black"'
            f' font-size="{0.9 * scale_factor}"'
            f' text-anchor="middle"'
            f' dominant-baseline="hanging"'
            f">{title}</text>"
        )
    elif title is not None:
        for plan_i, part in enumerate(title):
            lines.append(
                f"<text"
                f' x="{(box_x_pitch * (plan_i % cols) + (box_x_pitch - pad) / 2) * scale_factor}"'
                f' y="{(box_y_pitch * (plan_i // cols) + 0.2) * scale_factor}"'
                f' fill="black"'
                f' font-size="{0.9 * scale_factor}"'
                f' text-anchor="middle"'
                f' dominant-baseline="hanging"'
                f">{part}</text>"
            )

    clip_path_id_ptr = [0]
    for plan_i, plan in enumerate(patches):
        layers = [plan]
        if background is not None:
            if isinstance(background, (tuple, list)):
                layers.insert(0, background[plan_i % len(background)])
            else:
                layers.insert(0, background)
        for layer in layers:
            _draw_patch(
                obj=layer,
                q2p=lambda q: patch_q2p(plan_i, q),
                show_coords=show_coords,
                opacity=opacity,
                show_data_qubits=show_data_qubits,
                show_measure_qubits=show_measure_qubits,
                system_qubits=system_qubits,
                clip_path_id_ptr=clip_path_id_ptr,
                out_lines=lines,
                show_order=show_order,
                show_obs=show_obs,
                find_logical_err_max_weight=find_logical_err_max_weight,
                tile_color_func=tile_color_func,
                stabilizer_style=stabilizer_style,
                observable_style=observable_style,
            )

    # Draw frame outlines
    if show_frames:
        for outline_index in range(len(patches)):
            a = patch_q2p(outline_index, min_c)
            a += offset
            b = patch_q2p(outline_index, max_c)
            lines.append(
                f'<rect fill="none" stroke="#999" stroke-width="{scale_factor * 0.01}" '
                f'x="{a.real}" y="{a.imag}" width="{(b - a).real}" height="{(b - a).imag}" />'
            )

    lines.append("</svg>")
    from stimflow._core import str_svg

    return str_svg("\n".join(lines))
