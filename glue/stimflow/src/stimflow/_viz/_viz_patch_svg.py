from __future__ import annotations

import collections
import math
import sys
from collections.abc import Callable, Sequence
from typing import Any, Literal, TYPE_CHECKING

import stim

if TYPE_CHECKING:
    import stimflow


def is_colinear(a: complex, b: complex, c: complex, *, atol: float = 1e-4) -> bool:
    d1 = b - a
    d2 = c - a
    return abs(d1.real * d2.imag - d2.real * d1.imag) <= atol


def _path_commands_for_points_with_one_point(
    *, a: complex, draw_coord: Callable[[complex], complex], draw_radius: float | None = None
):
    draw_a = draw_coord(a)
    if draw_radius is None:
        draw_radius = abs(draw_coord(0.2) - draw_coord(0))
    r = draw_radius
    left = draw_a - draw_radius
    return [
        f"""M {left.real},{left.imag}""",
        f"""a {r},{r} 0 0,0 {2*r},{0}""",
        f"""a {r},{r} 0 0,0 {-2*r},{0}""",
    ]


def _path_commands_for_points_with_two_points(
    *, a: complex, b: complex, hint_point: complex, draw_coord: Callable[[complex], complex]
) -> list[str]:
    def transform_dif(d: complex) -> complex:
        return draw_coord(d) - draw_coord(0)

    da = a - hint_point
    db = b - hint_point
    angle = math.atan2(da.imag, da.real) - math.atan2(db.imag, db.real)
    angle %= math.pi * 2
    if angle < math.pi:
        a, b = b, a

    if abs(abs(da) - abs(db)) < 1e-4 < abs(da + db):
        # Semi-circle oriented towards measure qubit.
        draw_a = draw_coord(a)
        draw_ba = transform_dif(b - a)
        aspect_x = 1.0
        aspect_z = 1.0

        # Try to squash the oval slightly, so when two face each other there's a small gap.
        if b.imag == a.imag:
            aspect_z = 0.8
        elif b.real == a.real:
            aspect_x = 0.8

        return [
            f"""M {draw_a.real},{draw_a.imag}""",
            f"""a {aspect_x},{aspect_z} 0 0,0 {draw_ba.real},{draw_ba.imag}""",
            f"""L {draw_a.real},{draw_a.imag}""",
        ]
    else:
        # A wedge between the two data qubits.
        dif = b - a
        average = (a + b) * 0.5
        perp = dif * 1j
        if abs(perp) > 1:
            perp /= abs(perp)
        ac1 = average + perp * 0.2 - dif * 0.2
        ac2 = average + perp * 0.2 + dif * 0.2
        bc1 = average + perp * -0.2 + dif * 0.2
        bc2 = average + perp * -0.2 - dif * 0.2

        tac1 = draw_coord(ac1)
        tac2 = draw_coord(ac2)
        tbc1 = draw_coord(bc1)
        tbc2 = draw_coord(bc2)
        draw_a = draw_coord(a)
        draw_b = draw_coord(b)
        return [
            f"M {draw_a.real},{draw_a.imag}",
            f"C {tac1.real} {tac1.imag}, {tac2.real} {tac2.imag}, {draw_b.real} {draw_b.imag}",
            f"C {tbc1.real} {tbc1.imag}, {tbc2.real} {tbc2.imag}, {draw_a.real} {draw_a.imag}",
        ]


def _path_commands_for_points_with_many_points(
    *, pts: Sequence[complex], draw_coord: Callable[[complex], complex]
) -> list[str]:
    assert len(pts) >= 3
    ori = draw_coord(pts[-1])
    path_commands = [f"""M{ori.real},{ori.imag}"""]
    for k in range(len(pts)):
        prev_prev_q = pts[k - 2]
        prev_q = pts[k - 1]
        q = pts[k]
        next_q = pts[(k + 1) % len(pts)]
        if is_colinear(prev_q, q, next_q) or is_colinear(prev_prev_q, prev_q, q):
            prev_pt = draw_coord(prev_q)
            cur_pt = draw_coord(q)
            d = cur_pt - prev_pt
            p1 = prev_pt + d * (-0.25 + 0.05j)
            p2 = cur_pt + d * (0.25 + 0.05j)
            path_commands.append(
                f"""C {p1.real} {p1.imag}, {p2.real} {p2.imag}, {cur_pt.real} {cur_pt.imag}"""
            )
        else:
            q2 = draw_coord(q)
            path_commands.append(f"""L {q2.real},{q2.imag}""")
    return path_commands


def svg_path_directions_for_tile(
    *,
    tile: stimflow.Tile,
    draw_coord: Callable[[complex], complex],
    contract_towards: complex | None = None,
) -> str | None:
    hint_point = tile.measure_qubit
    if hint_point is None or any(abs(q - hint_point) < 1e-4 for q in tile.data_set):
        hint_point = sum(tile.data_set) / (len(tile.data_set) or 1)

    points = sorted(
        tile.data_set,
        key=lambda p2: math.atan2(p2.imag - hint_point.imag, p2.real - hint_point.real),
    )

    if len(points) == 0:
        return None

    if len(points) == 1:
        return " ".join(
            _path_commands_for_points_with_one_point(a=points[0], draw_coord=draw_coord)
        )

    if len(points) == 2:
        return " ".join(
            _path_commands_for_points_with_two_points(
                a=points[0], b=points[1], hint_point=hint_point, draw_coord=draw_coord
            )
        )

    if contract_towards is not None:
        c = 0.8
        points = [p * c + (1 - c) * contract_towards for p in points]

    return " ".join(_path_commands_for_points_with_many_points(pts=points, draw_coord=draw_coord))


def _draw_obs(
    *,
    obj: stimflow.StabilizerCode,
    observable_style: str,
    labels_out: list[tuple[complex, str, dict[str, Any]]],
    scale_factor: float,
    out_lines: list[str],
    q2p: Callable[[complex], complex],
):
    from stimflow._core import PauliMap

    if observable_style == "label":
        combined = obj.logicals + obj.scattered_logicals
        for k in range(len(combined)):
            if len(combined) <= 1:
                suffix = ""
            elif k < 10:
                suffix = "₀₁₂₃₄₅₆₇₈₉"[k]
            else:
                suffix = str(k)
            entry = combined[k]
            cases: list[PauliMap]
            if isinstance(entry, PauliMap):
                prefixes = ["L"]
                cases = [entry]
            else:
                obs_a, obs_b = entry
                cases = [obs_a, obs_b]
                prefixes = [
                    (next(iter(set(obs_a.values()))) if len(set(obs_a.values())) == 1 else "A"),
                    (next(iter(set(obs_b.values()))) if len(set(obs_b.values())) == 1 else "B"),
                ]
            for prefix, obs in zip(prefixes, cases):
                for q, basis2 in obs.items():
                    label = prefix + suffix
                    if prefix != "L" and basis2 != prefix:
                        label += "[" + basis2 + "]"
                    labels_out.append(
                        (
                            q,
                            label,
                            {
                                "text-anchor": "end",
                                "dominant-baseline": "hanging",
                                "font-size": scale_factor * 0.6,
                                "fill": BASE_COLORS_DARK[basis2],
                            },
                        )
                    )
    elif observable_style == "circles":
        for obs in obj.flat_logicals:
            for q, p in sorted(obs.items(), key=lambda e: (e[0].real, e[0].imag)):
                c = q2p(q)
                out_lines.append(
                    f"""<circle cx="{c.real}" cy="{c.imag}" r="{scale_factor * 0.45}" """
                    f"""fill="{BASE_COLORS[p]}" />"""
                )
    elif observable_style == "polygon":
        for obs in obj.flat_logicals:
            path_directions = svg_path_directions_for_tile(tile=obs.to_tile(), draw_coord=q2p)
            fill_color = BASE_COLORS[obs.to_tile().basis]
            out_lines.append(
                f'''<path d="{path_directions}"'''
                f''' fill="{fill_color}" stroke="black"'''
                """ />"""
            )
    else:
        raise NotImplementedError(f"{observable_style=}")


def _draw_patch(
    *,
    obj: stimflow.Patch | stimflow.StabilizerCode | stimflow.ChunkInterface | stim.Circuit,
    q2p: Callable[[complex], complex],
    show_coords: bool,
    show_obs: bool,
    opacity: float,
    show_data_qubits: bool,
    show_measure_qubits: bool,
    system_qubits: frozenset[complex],
    clip_path_id_ptr: list[int],
    out_lines: list[str],
    show_order: bool,
    find_logical_err_max_weight: int | None,
    tile_color_func: (
        Callable[
            [stimflow.Tile], str | tuple[float, float, float] | tuple[float, float, float, float] | None
        ]
        | None
    ),
    stabilizer_style: Literal["polygon", "circles"] | None,
    observable_style: Literal["label", "polygon", "circles"],
) -> None:
    from stimflow._chunk import Patch, StabilizerCode, ChunkInterface
    if hasattr(obj, "_inline_svg_"):
        obj._inline_svg_(
            out_lines=out_lines,
            q2p=q2p,
        )
        return
    if not isinstance(obj, (Patch, StabilizerCode, ChunkInterface, stim.Circuit)):
        raise NotImplementedError(f'{type(obj)=}')
    if isinstance(obj, stim.Circuit):
        from stimflow._viz._viz_circuit_layer_svg import append_circuit_layer_to_svg

        append_circuit_layer_to_svg(circuit=obj, lines=out_lines, q2p=q2p)
        return

    layer_1q2: list[str] = []
    layer_1q: list[str] = []
    fill_layer2q: list[str] = []
    fill_layer_mq: list[str] = []
    stroke_layer_mq: list[str] = []
    scale_factor = abs(q2p(1) - q2p(0))

    from stimflow._chunk import ChunkInterface, StabilizerCode
    from stimflow._core import Tile

    if isinstance(obj, ChunkInterface):
        obj = obj.to_code()
        show_order = False

    labels: list[tuple[complex, str, dict[str, Any]]] = []
    if isinstance(obj, StabilizerCode):
        if find_logical_err_max_weight is not None:
            try:
                err = obj.find_logical_error(max_search_weight=find_logical_err_max_weight)
            except ValueError as ex:
                print(
                    f"WARNING: No logical error will be drawn.\n    Reason: {ex}",
                    file=sys.stderr,
                )
                err = {}
            for q, p in err.items():
                labels.append(
                    (
                        q,
                        p + "!",
                        {
                            "text-anchor": "middle",
                            "dominant-baseline": "central",
                            "font-size": scale_factor * 1.1,
                            "fill": BASE_COLORS_DARK[p],
                        },
                    )
                )

    if isinstance(obj, StabilizerCode) and show_obs:
        _draw_obs(
            out_lines=stroke_layer_mq,
            obj=obj,
            observable_style=observable_style,
            labels_out=labels,
            q2p=q2p,
            scale_factor=scale_factor,
        )

    for q, s, ts in labels:
        loc2 = q2p(q)
        terms = {"x": loc2.real, "y": loc2.imag, **ts}
        layer_1q2.append(
            "<text" + "".join(f' {key}="{val}"' for key, val in terms.items()) + f">{s}</text>"
        )

    all_points = set(system_qubits)
    if show_data_qubits:
        all_points |= getattr(obj, 'data_set', set())
    if show_measure_qubits:
        all_points |= getattr(obj, 'measure_set', set())
    if show_coords and all_points:
        all_x = sorted({q.real for q in all_points})
        all_y = sorted({q.imag for q in all_points})
        left = min(all_x) - 1
        top = min(all_y) - 1

        for x in all_x:
            if x == int(x):
                x = int(x)
            loc2 = q2p(x + top * 1j)
            stroke_layer_mq.append(
                "<text"
                f' x="{loc2.real}"'
                f' y="{loc2.imag}"'
                ' fill="black"'
                f' font-size="{0.5*scale_factor}"'
                ' text-anchor="middle"'
                ' dominant-baseline="central"'
                f">{x}</text>"
            )
        for y in all_y:
            if y == int(y):
                y = int(y)
            loc2 = q2p(y * 1j + left)
            stroke_layer_mq.append(
                "<text"
                f' x="{loc2.real}"'
                f' y="{loc2.imag}"'
                ' fill="black"'
                f' font-size="{0.5*scale_factor}"'
                ' text-anchor="middle"'
                ' dominant-baseline="central"'
                f">{y}i</text>"
            )

    sorted_tiles = sorted(obj.tiles, key=tile_data_span, reverse=True)
    d2tiles: collections.defaultdict[complex, list[Tile]] = collections.defaultdict(list)

    def contraction_point(tile) -> complex | None:
        if len(tile.data_set) <= 2:
            return None

        # Inset tiles that overlap with other tiles.
        for data_qubit in tile.data_set:
            for other_tile in d2tiles[data_qubit]:
                if other_tile is not tile:
                    if tile.data_set < other_tile.data_set or (
                        tile.data_set == other_tile.data_set and tile.bases < other_tile.bases
                    ):
                        return sum(other_tile.data_set) / len(other_tile.data_set)

        return None

    for tile in sorted_tiles:
        for d in tile.data_set:
            d2tiles[d].append(tile)

    for tile in sorted_tiles:
        c = tile.measure_qubit
        if c is None or any(abs(q - c) < 1e-4 for q in tile.data_set):
            c = sum(tile.data_set) / max(len(tile.data_set), 1)
        dq = sorted(tile.data_set, key=lambda p2: math.atan2(p2.imag - c.imag, p2.real - c.real))
        if not dq:
            continue
        fill_color: str | tuple[float, float, float] | tuple[float, float, float, float] | None
        fill_color = BASE_COLORS[tile.basis]
        tile_opacity = opacity
        if tile_color_func is not None:
            fill_color = tile_color_func(tile)
            if fill_color is None:
                continue
            if isinstance(fill_color, tuple):
                r: float
                g: float
                b: float
                if len(fill_color) == 3:
                    r, g, b = fill_color
                else:
                    a: float
                    r, g, b, a = fill_color
                    tile_opacity *= a
                fill_color = (
                    "#"
                    + f"{round(r * 255.49):x}".rjust(2, "0")
                    + f"{round(g * 255.49):x}".rjust(2, "0")
                    + f"{round(b * 255.49):x}".rjust(2, "0")
                )
        if len(tile.data_set) == 1:
            fl = layer_1q
            sl = stroke_layer_mq
        elif len(tile.data_set) == 2:
            fl = fill_layer2q
            sl = stroke_layer_mq
        else:
            fl = fill_layer_mq
            sl = stroke_layer_mq
        cp = contraction_point(tile)
        if stabilizer_style == "polygon":
            path_directions = svg_path_directions_for_tile(
                tile=tile, draw_coord=q2p, contract_towards=cp
            )
        elif stabilizer_style == "circles":
            for q, p in sorted(tile.to_pauli_map().items(), key=lambda e: (e[0].real, e[0].imag)):
                c = q2p(q)
                fl.append(
                    f"""<circle cx="{c.real}" cy="{c.imag}" r="{scale_factor * 0.45}" """
                    f"""fill="{BASE_COLORS[p]}" />"""
                )
            path_directions = None
        elif stabilizer_style is None:
            path_directions = None
        else:
            raise NotImplementedError(f"{stabilizer_style=}")
        if path_directions is not None:
            fl.append(
                f'''<path d="{path_directions}"'''
                f''' fill="{fill_color}"'''
                + (f''' opacity="{tile_opacity}"''' * (tile_opacity != 1))
                + ''' stroke="none"'''
                """ />"""
            )
            if cp is None:
                sl.append(
                    f'''<path d="{path_directions}"'''
                    f''' fill="none"'''
                    f''' stroke="black"'''
                    f""" stroke-width="{scale_factor * (0.02 if cp is None else 0.005)}" """
                    + (f''' opacity="{tile_opacity}"''' * (tile_opacity != 1))
                    + """ />"""
                )

        # Add basis glows around data qubits in multi-basis stabilizers.
        if path_directions is not None and tile.basis is None and tile_color_func is None:
            clip_path_id_ptr[0] += 1
            fl.append(f'<clipPath id="clipPath{clip_path_id_ptr[0]}">')
            fl.append(f"""    <path d="{path_directions}" />""")
            fl.append("</clipPath>")
            for k, q in enumerate(tile.data_qubits):
                if q is None:
                    continue
                v = q2p(q)
                fl.append(
                    f"<circle "
                    f'clip-path="url(#clipPath{clip_path_id_ptr[0]})" '
                    f'cx="{v.real}" '
                    f'cy="{v.imag}" '
                    f'r="{scale_factor * 0.45}" '
                    f'fill="{BASE_COLORS[tile.bases[k]]}" '
                    f'stroke="none" />'
                )

    drawn_qubits: set[complex] = set()
    if show_data_qubits:
        drawn_qubits |= obj.data_set
        for q in obj.data_set:
            loc2 = q2p(q)
            layer_1q2.append(
                f"<circle "
                f'cx="{loc2.real}" '
                f'cy="{loc2.imag}" '
                f'r="{scale_factor * 0.1}" '
                f'fill="black" '
                f"""stroke="none" />"""
            )

    if show_measure_qubits:
        drawn_qubits |= obj.measure_set
        for q in obj.measure_set:
            loc2 = q2p(q)
            layer_1q2.append(
                f"<circle "
                f'cx="{loc2.real}" '
                f'cy="{loc2.imag}" '
                f'r="{scale_factor * 0.05}" '
                f'fill="black" '
                f'stroke-width="{scale_factor * 0.02}" '
                f"""stroke="black" />"""
            )

    for q in system_qubits:
        if q not in drawn_qubits:
            loc2 = q2p(q)
            layer_1q2.append(
                f"<circle "
                f'cx="{loc2.real}" '
                f'cy="{loc2.imag}" '
                f'r="{scale_factor * 0.05}" '
                f'fill="black" '
                f'stroke-width="{scale_factor * 0.02}" '
                f"""stroke="black" />"""
            )

    out_lines += fill_layer_mq
    out_lines += stroke_layer_mq
    out_lines += fill_layer2q
    out_lines += layer_1q
    out_lines += layer_1q2

    # Draw each element's measurement order as a zig zag arrow.
    if show_order:
        for tile in obj.tiles:
            _draw_tile_order_arrow(q2p=q2p, tile=tile, out_lines=out_lines)


BASE_COLORS = {"X": "#FF8080", "Z": "#8080FF", "Y": "#80FF80", None: "#CCC"}
BASE_COLORS_DARK = {"X": "#B01010", "Z": "#1010B0", "Y": "#10B010", None: "black"}


def tile_data_span(tile: stimflow.Tile) -> Any:
    from stimflow._core import min_max_complex

    min_c, max_c = min_max_complex(tile.data_set, default=0)
    return max_c.real - min_c.real + max_c.imag - min_c.imag, tile.bases


def _draw_tile_order_arrow(
    *, tile: stimflow.Tile, q2p: Callable[[complex], complex], out_lines: list[str]
):
    scale_factor = abs(q2p(1) - q2p(0))

    c = tile.measure_qubit
    if c is None:
        c = sum(tile.data_set) / (len(tile.data_set) or 1)
    if len(tile.data_set) == 3 or c in tile.data_set:
        c = 0
        for q in tile.data_set:
            c += q
        c /= len(tile.data_set)
    pts: list[complex] = []

    path_cmd_start = '<path d="M'
    arrow_color = "black"
    delay = 0
    prev = None
    for q in tile.data_qubits:
        if q is not None:
            f = 0.6
            v = q * f + c * (1 - f)
            pp = q2p(v)
            path_cmd_start += f"{pp.real},{pp.imag} "
            v = q2p(v)
            pts.append(v)
            for d in range(delay):
                if prev is None:
                    prev = v
                v2 = (prev + v) / 2
                out_lines.append(
                    f'<circle cx="{v2.real}" cy="{v2.imag}" r="{scale_factor * (d * 0.06 + 0.04)}" '
                    f'stroke-width="{scale_factor * 0.02}" '
                    f'stroke="yellow" '
                    f'fill="none" />'
                )
            delay = 0
            prev = v
        else:
            delay += 1
    path_cmd_start = path_cmd_start.strip()
    path_cmd_start += (
        f'" fill="none" stroke-width="{scale_factor * 0.02}" stroke="{arrow_color}" />'
    )
    out_lines.append(path_cmd_start)

    # Draw arrow at end of arrow.
    if len(pts) > 1:
        p = pts[-1]
        d2 = p - pts[-2]
        if d2:
            d2 /= abs(d2)
            d2 *= 4 * scale_factor * 0.02
        a = p + d2
        b = p + d2 * 1j
        c = p + d2 * -1j
        out_lines.append(
            f"<path"
            f' d="M{a.real},{a.imag} {b.real},{b.imag} {c.real},{c.imag} {a.real},{a.imag}"'
            f' stroke="none"'
            f' fill="{arrow_color}" />'
        )
