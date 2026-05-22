from __future__ import annotations

import base64
import collections
import dataclasses
import math
import random
import sys
from collections.abc import Callable, Iterable
from typing import TYPE_CHECKING

import stim

from stimflow._viz._viz_patch_svg import svg_path_directions_for_tile

if TYPE_CHECKING:
    import stimflow
    from stimflow._core import str_html


PITCH = 48 * 2
DIAM = 32
RAD = DIAM / 2
NOISY_GATES = {
    "X_ERROR",
    "Y_ERROR",
    "Z_ERROR",
    "E",
    "ELSE_CORRELATED_ERROR",
    "DEPOLARIZE1",
    "DEPOLARIZE2",
    "HERALDED_ERASE",
    "HERALDED_PAULI_CHANNEL_1",
    "PAULI_CHANNEL_1",
    "PAULI_CHANNEL_2",
    "I_ERROR",
    "II_ERROR",
}


def rand_color() -> str:
    color = "#"
    for _ in range(6):
        color += "0123456789abcdef"[random.randint(0, 15)]
    return color


MEASUREMENT_NAMES = {"M", "MX", "MY", "MR", "MRX", "MRY"}


@dataclasses.dataclass
class GateStyle:
    label: str
    fill_color: str
    text_color: str


def _init_gate_box_labels() -> dict[str, GateStyle]:
    result = {"I": GateStyle(label="I", fill_color="white", text_color="gray")}
    for name in ["X", "Y", "Z", "II", "I"]:
        result[name] = GateStyle(label=name, fill_color="white", text_color="black")
    for name in ["R", "M", "RX", "RY", "MX", "MY", "MR", "MRX", "MRY"]:
        result[name] = GateStyle(label=name, fill_color="black", text_color="white")
    for key in [
        "H",
        "H_YZ",
        "H_XY",
        "S",
        "SQRT_X",
        "SQRT_Y",
        "S_DAG",
        "SQRT_X_DAG",
        "SQRT_Y_DAG",
        "H_NXY",
        "H_NXZ",
        "H_NYZ",
    ]:
        name = key.replace("SQRT_", "√")
        name = name.replace("_DAG", "⁻¹")
        a, b = name.split("_") if "_" in name else (name, "")
        result[key] = GateStyle(label=a + b.lower(), fill_color="yellow", text_color="black")
    for name in ["C_XYZ", "C_NXYZ", "C_XNYZ", "C_XYNZ", "C_ZYX", "C_NZYX", "C_ZNYX", "C_ZYNX"]:
        result[name] = GateStyle(
            label=name[0] + name[2:].lower(), fill_color="teal", text_color="black"
        )
    return result


GATE_BOX_LABELS = _init_gate_box_labels()
TWO_QUBIT_GATE_STYLES = {
    "CX": ("Z", "X"),
    "CY": ("Z", "Y"),
    "CZ": ("Z", "Z"),
    "XCX": ("X", "X"),
    "XCY": ("X", "Y"),
    "XCZ": ("X", "Z"),
    "YCX": ("Y", "X"),
    "YCY": ("Y", "Y"),
    "YCZ": ("Y", "Z"),
    "SQRT_XX": ("SQRT_XX", "SQRT_XX"),
    "SQRT_XX_DAG": ("SQRT_XX", "SQRT_XX"),
    "SQRT_YY": ("SQRT_YY", "SQRT_YY"),
    "SQRT_YY_DAG": ("SQRT_YY", "SQRT_YY"),
    "SQRT_ZZ": ("SQRT_ZZ", "SQRT_ZZ"),
    "SQRT_ZZ_DAG": ("SQRT_ZZ", "SQRT_ZZ"),
    "ISWAP": ("ISWAP", "ISWAP"),
    "ISWAP_DAG": ("ISWAP", "ISWAP"),
    "SWAP": ("SWAP", "SWAP"),
    "CXSWAP": ("ZSWAP", "XSWAP"),
    "SWAPCX": ("XSWAP", "ZSWAP"),
    "CZSWAP": ("ZSWAP", "ZSWAP"),
    "MXX": ("MXX", "MXX"),
    "MYY": ("MYY", "MYY"),
    "MZZ": ("MZZ", "MZZ"),
}


def tag_str(tag, *, content: bool | str = False, **kwargs) -> str:
    parts = [f"<{tag}"]
    for k, v in kwargs.items():
        parts.append(f"{k.replace('_', '-')}={str(v)!r}")
    instr = " ".join(parts)
    if not content:
        instr += " />"
    elif isinstance(content, str):
        instr += f">{content}</{tag}>"
    elif content is True:
        instr += ">"
    else:
        raise NotImplementedError(repr(content))

    return instr


class _SvgLayer:
    def __init__(
        self,
        patch: stimflow.Patch,
        tile_color_func: Callable[
            [stimflow.Tile], tuple[float, float, float, float] | tuple[float, float, float] | str
        ],
    ):
        self.svg_instructions: list[str] = []
        self.q2i_dict: dict[int, tuple[float, float]] = {}
        self.used_indices: set[int] = set()
        self.used_positions: set[tuple[float, float]] = set()
        self.measurement_positions: dict[int, tuple[float, float]] = {}
        if patch is not None:
            self.add_patch(patch, tile_color_func=tile_color_func)

    def add(self, tag, *, content: bool | str = False, **kwargs) -> None:
        self.svg_instructions.append("    " + tag_str(tag, content=content, **kwargs))

    def bounds(self) -> tuple[float, float, float, float]:
        min_y = min([e for _, e in self.used_positions], default=0)
        max_y = max([e for _, e in self.used_positions], default=0)
        min_x = min([e for e, _ in self.used_positions], default=0)
        max_x = max([e for e, _ in self.used_positions], default=0)
        min_x -= PITCH
        min_y -= PITCH
        max_x += PITCH
        max_y += PITCH
        return min_x, min_y, max_x, max_y

    def add_idles(self, all_used_positions: set[tuple[float, float]]):
        for x, y in all_used_positions - self.used_positions:
            self.add("circle", cx=x, cy=y, r=5, fill="gray", stroke="black")
        self.used_positions |= all_used_positions
        min_x, _min_y, _max_x, max_y = self.bounds()
        xs = {e for e, _ in self.used_positions}
        ys = {e for _, e in self.used_positions}
        for x in xs:
            x2 = x
            x2 /= PITCH
            if x2 == int(x2):
                x2 = int(x2)
            self.add(
                "text",
                x=x,
                y=max_y - 5,
                fill="black",
                content=str(x2),
                text_anchor="middle",
                dominant_baseline="auto",
                font_size=24,
            )
        for y in ys:
            y2 = y
            y2 /= PITCH
            if y2 == int(y2):
                y2 = int(y2)
            self.add(
                "text",
                x=min_x + 5,
                y=y,
                fill="black",
                content=str(y2),
                text_anchor="left",
                alignment_baseline="middle",
                font_size=24,
            )

    def add_patch(
        self,
        patch: stimflow.Patch,
        tile_color_func: Callable[
            [stimflow.Tile], tuple[float, float, float, float] | tuple[float, float, float] | str
        ],
    ):
        for tile in patch.tiles:
            color = tile_color_func(tile)
            if isinstance(color, tuple):
                if len(color) == 3:
                    r, g, b = color
                    a = 0.3
                elif len(color) == 4:
                    r, g, b, a = color
                else:
                    raise NotImplementedError(f"{color=}")
                assert 0 <= r <= 1
                assert 0 <= g <= 1
                assert 0 <= b <= 1
                assert 0 <= a <= 1
                color = (
                    "#"
                    + f"{round(r * 255.49):x}".rjust(2, "0")
                    + f"{round(g * 255.49):x}".rjust(2, "0")
                    + f"{round(b * 255.49):x}".rjust(2, "0")
                    + f"{round(a * 255.49):x}".rjust(2, "0")
                )

            path_directions = svg_path_directions_for_tile(
                tile=tile, draw_coord=lambda pt: pt * PITCH
            )
            if path_directions is not None:
                self.svg_instructions.append(
                    f"""<path d="{path_directions}" fill="{color}" stroke="none" />"""
                )
        for tile in patch.tiles:
            path_directions = svg_path_directions_for_tile(
                tile=tile, draw_coord=lambda pt: pt * PITCH
            )
            if path_directions is not None:
                self.svg_instructions.append(
                    f'<path d="{path_directions}" fill="none" stroke="#808080" stroke-width="0.1"/>'
                )

    def svg(
        self,
        *,
        html_id: str | None = None,
        as_img_with_data_uri: bool = False,
        width: int,
        height: int,
    ) -> str:
        min_x, min_y, max_x, max_y = self.bounds()
        kwargs = {} if html_id is None or as_img_with_data_uri else {"id": html_id}
        svg = "\n".join(
            [
                tag_str(
                    "svg",
                    xmlns="http://www.w3.org/2000/svg",
                    viewBox=f"{min_x} {min_y} {max_x - min_x} {max_y - min_y}",
                    content=True,
                    **kwargs,
                ),
                *self.svg_instructions,
                "</svg>",
            ]
        )
        if as_img_with_data_uri:
            kwargs = {} if html_id is None else {"id": html_id}
            svg = tag_str(
                "img",
                width=width,
                height=height,
                **kwargs,
                src="data:image/svg+xml;base64,"
                + base64.standard_b64encode(svg.encode("utf-8")).decode("utf-8"),
            )
            svg = svg.replace("/>", ">")
        return svg


class _SvgState:
    def __init__(
        self,
        patch: stimflow.Patch | None,
        tile_color_func: Callable[
            [stimflow.Tile], tuple[float, float, float, float] | tuple[float, float, float] | str
        ],
    ):
        self.patch: stimflow.Patch | None = patch
        self.layers: list[_SvgLayer] = [_SvgLayer(self.patch, tile_color_func=tile_color_func)]
        self.coord_shift: list[int] = [0, 0]
        self.measurement_layer_indices: list[int] = []
        self.detector_index: int = 0
        self.detector_coords: dict[int, list[float]] = {}
        self.measurement_marks: collections.Counter[int] = collections.Counter()
        self.highlighted_detectors: set[int] = set()
        self.highlighted_errors: list[tuple[int, int, str]] = []
        self.flipped_measurements: set[int] = set()
        self.noted_errors: list[tuple[int, int, str]] = []
        self.control_count: int = 0
        self.tile_color_func = tile_color_func

    def tick(self) -> None:
        self.layers.append(_SvgLayer(self.patch, self.tile_color_func))
        self.layers[-1].q2i_dict = dict(self.layers[-2].q2i_dict)

    def i2xy(self, i: int) -> tuple[float, float]:
        x, y = self.layers[-1].q2i_dict.setdefault(i, (i, 0))
        pt = x * PITCH, y * PITCH
        self.layers[-1].used_indices.add(i)
        self.layers[-1].used_positions.add(pt)
        return pt

    def are_adjacent(self, q1: stim.GateTarget, q2: stim.GateTarget) -> bool:
        if q1.is_qubit_target and q2.is_qubit_target:
            x1, y1 = self.layers[-1].q2i_dict.setdefault(q1.value, (q1.value, 0))
            x2, y2 = self.layers[-1].q2i_dict.setdefault(q2.value, (q2.value, 0))
            if abs(x2 - x1) + abs(y2 - y1) < 1.5:
                return True
        return False

    def add(self, tag, *, content="", **kwargs) -> None:
        self.layers[-1].add(tag, content=content, **kwargs)

    def add_box(self, x: float, y: float, text: str, *, fill="white", text_color="black"):
        self.add("rect", x=x - RAD, y=y - RAD, width=DIAM, height=DIAM, fill=fill, stroke="black")
        self.add(
            "text",
            x=x,
            y=y,
            fill=text_color,
            content=text,
            font_size=32 if len(text) == 1 else 24 if len(text) == 2 else 18,
            text_anchor="middle",
            alignment_baseline="central",
        )

    def add_measurement(self, *targets: stim.GateTarget) -> None:
        for target in targets:
            assert (
                target.is_qubit_target
                or target.is_x_target
                or target.is_y_target
                or target.is_z_target
            )
        m_index = len(self.measurement_layer_indices)
        self.measurement_layer_indices.append(len(self.layers) - 1)
        x: float = 0
        y: float = 0
        for target in targets:
            dx, dy = self.i2xy(target.value)
            x += dx
            y += dy
        x /= len(targets)
        y /= len(targets)
        self.layers[-1].measurement_positions[m_index] = (x, y)

    def mark_measurements(
        self, targets: list[stim.GateTarget], prefix: str, index: int | None
    ) -> None:
        if index is None:
            assert prefix == "D"
            index = self.detector_index
            self.detector_index += 1
        if prefix == "D":
            color = "black"
            if index in self.highlighted_detectors:
                color = "red"
        elif prefix == "L":
            color = "blue"
        elif prefix == "C":
            color = "green"
        else:
            color = "black"
        name = f"{prefix}{index}"
        for t in targets:
            m_index = len(self.measurement_layer_indices) + t.value
            if m_index < 0:
                print(
                    "Attempted to mark a measurement before the beginning of time.\n"
                    "Skipping this mark.",
                    file=sys.stderr,
                )
                continue
            assert m_index >= 0, m_index
            if t.is_measurement_record_target:
                layer = self.layers[self.measurement_layer_indices[m_index]]
                x, y = layer.measurement_positions[m_index]
                x += RAD + 1
                y -= RAD
                y += self.measurement_marks[m_index] * 15
                self.measurement_marks[m_index] += 1
                layer.add(
                    "text",
                    x=x,
                    y=y,
                    fill=color,
                    content=name,
                    text_anchor="left",
                    alignment_baseline="hanging",
                    font_size=16,
                )


def _draw_endpoint(x: float, y: float, style: str, *, out: _SvgState) -> None:
    add = out.add
    if style == "X":
        add("circle", cx=x, cy=y, r=RAD, stroke="black", fill="white")
        add("line", x1=x - RAD, x2=x + RAD, y1=y, y2=y, stroke="black")
        add("line", x1=x, x2=x, y1=y - RAD, y2=y + RAD, stroke="black")
    elif style == "Y":
        s = 0.5**0.5
        add("circle", cx=x, cy=y, r=RAD, stroke="black", fill="white")
        add("line", x1=x, x2=x, y1=y, y2=y + RAD, stroke="black")
        add("line", x1=x, x2=x - RAD * s, y1=y, y2=y - RAD * s, stroke="black")
        add("line", x1=x, x2=x + RAD * s, y1=y, y2=y - RAD * s, stroke="black")
    elif style == "Z":
        add("circle", cx=x, cy=y, r=RAD, fill="black")
    elif style == "SWAP":
        r = RAD / 3
        add("line", x1=x - r, x2=x + r, y1=y - r, y2=y + r, stroke="black")
        add("line", x1=x - r, x2=x + r, y1=y + r, y2=y - r, stroke="black")
    elif style == "ISWAP":
        r = RAD
        add("circle", cx=x, cy=y, r=RAD / 2, fill="gray")
        add("line", x1=x - r, x2=x + r, y1=y - r, y2=y + r, stroke="black")
        add("line", x1=x - r, x2=x + r, y1=y + r, y2=y - r, stroke="black")
    elif style == "MXX":
        out.add_box(x=x, y=y, text="Mxx", fill="black", text_color="white")
    elif style == "MYY":
        out.add_box(x=x, y=y, text="Myy", fill="black", text_color="white")
    elif style == "MZZ":
        out.add_box(x=x, y=y, text="Mzz", fill="black", text_color="white")
    elif style == "SQRT_ZZ":
        out.add_box(x=x, y=y, text="√ZZ")
    elif style == "SQRT_YY":
        out.add_box(x=x, y=y, text="√YY")
    elif style == "SQRT_XX":
        out.add_box(x=x, y=y, text="√XX")
    elif style == "XSWAP":
        r = RAD * 0.4
        add("circle", cx=x, cy=y, r=RAD, fill="white", stroke="black")
        add("line", x1=x - r, x2=x + r, y1=y - r, y2=y + r, stroke="black", stroke_width=5)
        add("line", x1=x - r, x2=x + r, y1=y + r, y2=y - r, stroke="black", stroke_width=5)
    elif style == "ZSWAP":
        r = RAD * 0.4
        add("circle", cx=x, cy=y, r=RAD, fill="black", stroke="black")
        add("line", x1=x - r, x2=x + r, y1=y - r, y2=y + r, stroke="white", stroke_width=5)
        add("line", x1=x - r, x2=x + r, y1=y + r, y2=y - r, stroke="white", stroke_width=5)
    else:
        raise NotImplementedError(style)


def _draw_2q(instruction: stim.CircuitInstruction, *, out: _SvgState) -> None:
    style1, style2 = TWO_QUBIT_GATE_STYLES[instruction.name]
    targets = instruction.targets_copy()
    i2qq = out.i2xy
    is_measurement = stim.gate_data(instruction.name).produces_measurements

    assert len(targets) % 2 == 0
    for k in range(0, len(targets), 2):
        t1 = targets[k]
        t2 = targets[k + 1]
        if is_measurement:
            out.add_measurement(t1, t2)
        if t1.is_measurement_record_target or t2.is_measurement_record_target:
            if t1.is_qubit_target:
                t = t1.value
                m = t2
            elif t2.is_qubit_target:
                t = t2.value
                m = t1
            else:
                continue
            b = (
                "X"
                if instruction.name in ["XCZ", "CX"]
                else (
                    "Y"
                    if instruction.name in ["YCZ", "CY"]
                    else "Z" if instruction.name == "CZ" else "?"
                )
            )
            x, y = i2qq(t)
            out.add(
                "text",
                x=x - RAD + 1,
                y=y,
                fill="green",
                content=b,
                font_size=18,
                text_anchor="left",
                alignment_baseline="central",
            )
            out.add(
                "text",
                x=x - 1,
                y=y - RAD / 2,
                fill="green",
                content=f"C{out.control_count}",
                font_size=8,
                text_anchor="left",
                alignment_baseline="central",
            )
            out.mark_measurements([m], prefix="C", index=out.control_count)
            out.control_count += 1
            continue
        assert t1.is_qubit_target
        assert t2.is_qubit_target
        x1, y1 = i2qq(t1.value)
        x2, y2 = i2qq(t2.value)
        dx = x2 - x1
        dy = y2 - y1
        r = (dx * dx + dy * dy) ** 0.5
        px = dy
        py = -dx
        px *= 25 / r
        py *= 25 / r
        cx1 = dx / 10 + px
        cy1 = dy / 10 + py
        cx2 = dx - dx / 10 + px
        cy2 = dy - dy / 10 + py

        if out.are_adjacent(t1, t2):
            out.add("line", x1=x1, x2=x2, y1=y1, y2=y2, stroke="black")
        else:
            out.add(
                "path",
                d=f"M {x1},{y1} c {cx1},{cy1} {cx2},{cy2} {dx},{dy}",
                stroke="black",
                fill="none",
            )

        _draw_endpoint(x1, y1, style1, out=out)
        _draw_endpoint(x2, y2, style2, out=out)


def _draw_mpp(instruction: stim.CircuitInstruction, *, out: _SvgState) -> None:
    for chunk in instruction.target_groups():
        out.add_measurement(*chunk)
        _draw_single_mpp(chunk, out=out, tag=instruction.tag, include_qubit_boxes=True)


def _draw_spp(instruction: stim.CircuitInstruction, *, out: _SvgState) -> None:
    for chunk in instruction.target_groups():
        out.add_measurement(*chunk)
        _draw_single_spp(chunk, out=out, tag=instruction.tag, include_qubit_boxes=True)


def _draw_single_spp(
    chunk: list[stim.GateTarget], *, out: _SvgState, tag: str, include_qubit_boxes: bool
) -> None:
    tx, ty = 0.0, 0.0
    for t in chunk:
        x, y = out.i2xy(t.value)
        tx += x
        ty += y
    tx /= len(chunk)
    ty /= len(chunk)
    if tag:
        out.add_box(tx, ty, tag)
    color = rand_color()
    no_text = False
    if all(t.is_x_target for t in chunk):
        color = "red"
        no_text = True
    if all(t.is_y_target for t in chunk):
        color = "green"
        no_text = True
    if all(t.is_z_target for t in chunk):
        color = "blue"
        no_text = True
    for t in chunk:
        x, y = out.i2xy(t.value)
        out.add("line", x1=x, x2=tx, y1=y, y2=ty, stroke=color, stroke_width=8)
    if include_qubit_boxes:
        for c in chunk:
            if c.is_x_target:
                text = "SX"
            elif c.is_y_target:
                text = "SY"
            elif c.is_z_target:
                text = "SZ"
            else:
                raise NotImplementedError(repr(c))
            x, y = out.i2xy(c.value)
            out.add_box(x, y, text * (1 - int(no_text)), fill=color)


def _draw_single_mpp(
    chunk: list[stim.GateTarget], *, out: _SvgState, tag: str, include_qubit_boxes: bool
) -> None:
    add = out.add
    add_box = out.add_box
    q2i = out.i2xy

    tx, ty = 0.0, 0.0
    for t in chunk:
        x, y = q2i(t.value)
        tx += x
        ty += y
    tx /= len(chunk)
    ty /= len(chunk)
    if tag:
        add_box(tx, ty, tag)
    color = rand_color()
    no_text = False
    if all(t.is_x_target for t in chunk):
        color = "red"
        no_text = True
    if all(t.is_y_target for t in chunk):
        color = "green"
        no_text = True
    if all(t.is_z_target for t in chunk):
        color = "blue"
        no_text = True
    for t in chunk:
        x, y = q2i(t.value)
        add("line", x1=x, x2=tx, y1=y, y2=ty, stroke=color, stroke_width=8)
    if include_qubit_boxes:
        for c in chunk:
            if c.is_x_target:
                text = "PX"
            elif c.is_y_target:
                text = "PY"
            elif c.is_z_target:
                text = "PZ"
            else:
                raise NotImplementedError(repr(c))
            x, y = q2i(c.value)
            add_box(x, y, text * (1 - int(no_text)), fill=color)


def _draw_1q(instruction: stim.CircuitInstruction, *, out: _SvgState):
    targets = instruction.targets_copy()
    if instruction.name in MEASUREMENT_NAMES:
        for t in targets:
            out.add_measurement(t)
    for t in targets:
        assert t.is_qubit_target
        x, y = out.i2xy(t.value)
        style = GATE_BOX_LABELS[instruction.name]
        out.add_box(x, y, style.label, fill=style.fill_color, text_color=style.text_color)


def _stim_circuit_to_svg_helper(circuit: stim.Circuit, state: _SvgState) -> None:
    for instruction in circuit:
        if isinstance(instruction, stim.CircuitRepeatBlock):
            body = instruction.body_copy()
            for _ in range(instruction.repeat_count):
                _stim_circuit_to_svg_helper(body, state)
        elif isinstance(instruction, stim.CircuitInstruction):
            targets: list[stim.GateTarget] = instruction.targets_copy()
            if instruction.name == "QUBIT_COORDS":
                pos = instruction.gate_args_copy()
                for t in instruction.targets_copy():
                    assert t.is_qubit_target
                    if len(pos):
                        if len(pos) == 1:
                            pos = (pos[0], 0)
                        state.layers[-1].q2i_dict[t.value] = (
                            pos[0] + state.coord_shift[0],
                            pos[1] + state.coord_shift[1],
                        )
            elif instruction.name == "SHIFT_COORDS":
                pos = instruction.gate_args_copy()
                if len(pos) >= 1:
                    state.coord_shift[0] += pos[0]
                if len(pos) >= 2:
                    state.coord_shift[1] += pos[1]
            elif instruction.name in GATE_BOX_LABELS:
                _draw_1q(instruction, out=state)
            elif instruction.name in TWO_QUBIT_GATE_STYLES:
                _draw_2q(instruction, out=state)
            elif instruction.name == "TICK":
                state.tick()
            elif instruction.name == "MPP":
                _draw_mpp(instruction, out=state)
            elif instruction.name == "SPP" or instruction.name == "SPP_DAG":
                _draw_spp(instruction, out=state)
            elif instruction.name == "DETECTOR":
                state.mark_measurements(targets, prefix="D", index=None)
            elif instruction.name == "OBSERVABLE_INCLUDE":
                paulis = [t for t in targets if t.pauli_type != "I"]
                if paulis:
                    _draw_single_mpp(
                        paulis, out=state, tag=instruction.tag, include_qubit_boxes=True
                    )
                state.mark_measurements(
                    targets, prefix="L", index=int(instruction.gate_args_copy()[0])
                )
            elif instruction.name == "E":
                _draw_single_mpp(
                    instruction.targets_copy(),
                    out=state,
                    tag=instruction.tag,
                    include_qubit_boxes=False,
                )
            elif instruction.name in NOISY_GATES:
                for t in instruction.targets_copy():
                    state.noted_errors.append((t.value, len(state.layers) - 1, "E"))
            elif instruction.name == "MPAD":
                for t in instruction.targets_copy():
                    state.add_measurement(t)
            else:
                raise NotImplementedError(repr(instruction))
        else:
            raise NotImplementedError(repr(instruction))


def append_patch_polygons(
    *,
    out: list[str],
    patch: stimflow.Patch,
    q2i: dict[complex, int],
    tile_color_func: Callable[
        [stimflow.Tile], tuple[float, float, float, float] | tuple[float, float, float] | str
    ],
):
    for e in patch.tiles:
        rgba = tile_color_func(e)
        if isinstance(rgba, str):
            raise NotImplementedError(f"{rgba=}")
        elif len(rgba) == 3:
            r, g, b = rgba
            a = 0.25
            assert 0 <= r <= 1
            assert 0 <= g <= 1
            assert 0 <= b <= 1
        elif len(rgba) == 4:
            r, g, b, a = rgba
            assert 0 <= r <= 1
            assert 0 <= g <= 1
            assert 0 <= b <= 1
            assert 0 <= a <= 1
        else:
            raise NotImplementedError(f"{rgba=}")
        qs = [q for q in e.data_qubits if q is not None]
        c = e.measure_qubit
        if c is None or any(abs(q - c) < 1e-4 for q in e.data_set):
            c = sum(e.data_set) / len(e.data_set)
        qs = sorted(qs, key=lambda q: math.atan2(q.imag - c.imag, q.real - c.real))
        line = f"POLYGON({r},{g},{b},{a})"
        for q in qs:
            line += f"_{q2i.get(q, 0)}"
        out.append(line)


def stim_circuit_html_viewer(
    circuit: stim.Circuit,
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
    q2i = {}
    for k, v in circuit.get_final_qubit_coordinates().items():
        if len(v) == 1:
            q2i[v[0]] = k
        elif len(v) >= 2:
            q2i[v[0] + 1j * v[1]] = k
    min_imag = min([q.imag for q in q2i.keys()], default=0)
    seen_qubit_indices = set(q2i.values())
    for q in range(circuit.num_qubits):
        if q not in seen_qubit_indices:
            q2i[min_imag * 1j - 1j + q] = q

    if tile_color_func is None:

        def default_tile_color_func(tile: stimflow.Tile) -> tuple[float, float, float, float]:
            if tile.basis == "X":
                return 1, 0, 0, 0.25
            elif tile.basis == "Y":
                return 0, 1, 0, 0.25
            elif tile.basis == "Z":
                return 0, 0, 1, 0.25
            else:
                return 0.5, 0.5, 0.5, 0.5

        tile_color_func = default_tile_color_func

    from stimflow._chunk import ChunkInterface, find_d2_error, Patch, StabilizerCode

    if isinstance(background, StabilizerCode):
        background = background.stabilizers
    if isinstance(background, ChunkInterface):
        background = background.to_patch()
    background: stimflow.Patch | None
    if isinstance(background, Patch):
        background = background
    elif isinstance(background, dict) and background:
        val = background[min(background.keys(), key=lambda e: (e < 0, e))]
        if isinstance(val, Patch):
            background = val
        elif isinstance(val, StabilizerCode):
            background = val.patch
        else:
            raise NotImplementedError(f"{val=}")
    else:
        background = None
    state = _SvgState(background, tile_color_func=tile_color_func)
    state.detector_coords = circuit.get_detector_coordinates()
    if known_error is None:
        # noinspection PyBroadException
        try:
            known_error = find_d2_error(circuit)
            if known_error is None:
                known_error = circuit.shortest_graphlike_error(
                    ignore_ungraphlike_errors=True, canonicalize_circuit_errors=True
                )
        except Exception:
            pass
    tick_highlights = {}
    if known_error is not None:
        for product in known_error:
            loc = next(iter(product.circuit_error_locations))
            for flipped in loc.flipped_pauli_product:
                if flipped.gate_target.is_x_target:
                    b = "X"
                elif flipped.gate_target.is_y_target:
                    b = "Y"
                elif flipped.gate_target.is_z_target:
                    b = "Z"
                else:
                    raise NotImplementedError(repr(loc))
                tick_highlights[loc.tick_offset] = "red"
                state.highlighted_errors.append((flipped.gate_target.value, loc.tick_offset, b))
            if loc.flipped_measurement is not None:
                state.flipped_measurements.add(loc.flipped_measurement.record_index)
            for term in product.dem_error_terms:
                target = term.dem_target
                if target.is_relative_detector_id():
                    state.highlighted_detectors.add(target.val)

    _stim_circuit_to_svg_helper(circuit, state)
    for t, layer in enumerate(state.layers):
        if layer.measurement_positions:
            if t not in tick_highlights:
                tick_highlights[t] = "gray"

    all_pos = {pt for layer in state.layers for pt in layer.used_positions}
    for layer in state.layers:
        layer.add_idles(all_pos)

    for m in state.flipped_measurements:
        layer = state.layers[state.measurement_layer_indices[m]]
        x, y = layer.measurement_positions[m]
        layer.add(
            "rect",
            x=x - RAD * 2,
            y=y - RAD * 2,
            width=DIAM * 2,
            height=DIAM * 2,
            fill="#FF000080",
            stroke="#FF0000",
        )
    for qubit, time, basis in state.highlighted_errors:
        layer = state.layers[time]
        x, y = state.i2xy(qubit)
        layer.add(
            "text",
            x=x,
            y=y,
            fill="red",
            content="," + basis,
            text_anchor="middle",
            dominant_baseline="middle",
            font_size=64,
        )
    for qubit, time, basis in set(state.noted_errors):
        if time >= len(state.layers):
            print(f"Error time is past end of circuit: {time}", file=sys.stderr)
            continue
        layer = state.layers[time]
        x, y = state.i2xy(qubit)
        layer.add(
            "text",
            x=x - RAD,
            y=y,
            fill="red",
            content=basis,
            text_anchor="end",
            dominant_baseline="middle",
            font_size=12,
        )

    # Draw the scrubber.
    for t, layer in enumerate(state.layers):
        min_x, min_y, max_x, _ = layer.bounds()
        dx = (max_x - min_x) / len(state.layers)
        layer.add(
            "rect", x=min_x, y=min_y, width=max_x - min_x, height=10, fill="white", stroke="none"
        )
        for t2, color in tick_highlights.items():
            layer.add(
                "rect", x=min_x + t2 * dx, y=min_y, width=dx, height=10, fill=color, stroke="none"
            )
        layer.add(
            "rect",
            x=min_x + (t + 0.25) * dx,
            y=min_y,
            width=dx * 0.5,
            height=10,
            fill="green",
            stroke="none",
        )
        layer.add(
            "rect", x=min_x, y=min_y, width=max_x - min_x, height=10, fill="none", stroke="black"
        )

    svg_image_tags = []
    for k, layer in enumerate(state.layers):
        svg = layer.svg(html_id=f"layer{k}", width=width, height=height)
        data = base64.standard_b64encode(svg.encode("utf-8")).decode("utf-8")
        svg_image_tags.append(
            f'<img style="max-width: 95%; max-height: 95%; display: none" '
            f"id=layer{k} "
            f'src="data:image/svg+xml;base64,{data}" />'
        )
    all_svg_image_tags = "\n".join(svg_image_tags)

    flattened = circuit.flattened()
    circuit_coords = [str(inst) for inst in flattened if inst.name == "QUBIT_COORDS"]
    from stimflow._chunk import Patch

    i2patch: dict[int, Patch]
    if isinstance(background, Patch):
        i2patch = {0: background}
    elif background is None:
        i2patch = {}
    elif isinstance(background, dict):
        num_ticks = circuit.num_ticks + len(background)
        i2patch = {}
        for k, v in background.items():
            if k < 0:
                k += num_ticks + 1
            if isinstance(v, StabilizerCode):
                i2patch[k] = v.patch
            elif isinstance(v, ChunkInterface):
                i2patch[k] = v.to_patch()
            elif isinstance(v, Patch):
                i2patch[k] = v
            else:
                raise NotImplementedError(f"{v=}")
    else:
        raise NotImplementedError(f"{background=}")
    tick = 0
    circuit_rest: list[str] = []
    for inst in flattened:
        if tick in i2patch:
            append_patch_polygons(
                out=circuit_rest, patch=i2patch[tick], q2i=q2i, tile_color_func=tile_color_func
            )
            circuit_rest.append("TICK")
            tick += 1
        if inst.name == "TICK":
            tick += 1
        if inst.name != "QUBIT_COORDS":
            circuit_rest.append(str(inst))
    max_patch_tick = max(i2patch.keys(), default=0)
    while tick <= max_patch_tick:
        if tick in i2patch:
            circuit_rest.append("TICK")
            append_patch_polygons(
                out=circuit_rest, patch=i2patch[tick], q2i=q2i, tile_color_func=tile_color_func
            )
        tick += 1

    escaped = ";".join(circuit_coords + circuit_rest)
    escaped = escaped.replace(", ", ",").replace(" ", "_")
    escaped = escaped.replace("QUBIT_COORDS", "Q")
    escaped = escaped.replace("DETECTOR", "DT")
    escaped = escaped.replace("(", "%28").replace(")", "%29")
    escaped = escaped.replace("[", "%5B").replace("]", "%5D")
    local_server_crumble_url = f"""https://algassert.com/crumble#circuit={escaped}"""

    from stimflow._core import str_html

    return str_html(
        f"""
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" />
</head>
<body>
    <div id="circuit-viewer-container" tabindex=0>
        <div id="step">Loading...</div>
        <button id="btnPrev">Previous Layer (hotkey: q)</button>
        <button id="btnNext">Next Layer (hotkey: e)</button>
        <a id="crumble-link" href="{local_server_crumble_url}">Open in Crumble</a>
        <div id="viewer"
         style="border: 1px solid black; margin-bottom: 50px; width: 100%; height: 90vh;
         resize: both; overflow: auto" tabindex="1" autofocus
         >"""
        + all_svg_image_tags
        + """
        </div>
        <script>
            (function() {
                let layer_index = 0;
                let url = document.URL;
                let parts = url.split('#');
                if (parts.length == 2) {
                    let frag = parts[1];
                    if (frag.startsWith('layer=')) {
                        let end = frag.split('=')[1];
                        let parsed = Number.parseInt(end);
                        if (!Number.isNaN(parsed)) {
                            layer_index = parsed - 1;
                        }
                    }
                }
                let layers = [];
                let container = document.getElementById("circuit-viewer-container");
                container.id = 'circuit-viewer-container-do-not-find-again';
                while (true) {
                    let svg = container.querySelector('#layer' + layers.length);
                    if (svg === null) {
                        break;
                    }
                    layers.push(svg);
                }

                function handleLayerIndexChange() {
                    if (layer_index < 0) {
                        layer_index = 0;
                    }
                    if (layer_index >= layers.length) {
                        layer_index = layers.length - 1;
                    }

                    let layerName = layer_index + 1;
                    let step = container.querySelector('#step');
                    step.innerHTML = "Layer: " + layerName + "/" + layers.length;
                    for (let k = 0; k < layers.length; k++) {
                        let svg = layers[k];
                        if (layer_index === k) {
                            svg.style.display = "";
                        } else {
                            svg.style.display = "none";
                        }
                    }
                    window.location.hash = `layer=${layer_index+1}`;
                }
                container.querySelector("#btnPrev").addEventListener("click", ev => {
                    layer_index -= 1;
                    handleLayerIndexChange();
                });
                container.querySelector("#btnNext").addEventListener("click", ev => {
                    layer_index += 1;
                    handleLayerIndexChange();
                });
                container.addEventListener('keydown', ev => {
                    if (ev.code == "KeyQ" && !ev.getModifierState("Control")) {
                        layer_index -= 1;
                        ev.preventDefault();
                        handleLayerIndexChange();
                    } else if (ev.code == "KeyE") {
                        layer_index += 1;
                        ev.preventDefault();
                        handleLayerIndexChange();
                    } else if (ev.code == "Home") {
                        layer_index = 0;
                        ev.preventDefault();
                        handleLayerIndexChange();
                    } else if (ev.code == "End") {
                        layer_index = layers.length;
                        ev.preventDefault();
                        handleLayerIndexChange();
                    }
                });

                handleLayerIndexChange();
            })();
        </script>
    </div>
</body>"""
    )
