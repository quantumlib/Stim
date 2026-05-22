from __future__ import annotations

import collections
from collections.abc import Callable

import stim

from stimflow._core import str_svg


def append_circuit_layer_to_svg(
    *, circuit: stim.Circuit, lines: list[str], q2p: Callable[[complex], complex]
):
    i2q = {i: v[0] + v[1] * 1j for i, v in circuit.get_final_qubit_coordinates().items()}
    scale = abs(q2p(1) - q2p(0))
    uses = collections.defaultdict(list)

    def t2p(t: stim.GateTarget) -> complex:
        i = t.qubit_value
        q = i2q[i]
        return q2p(q)

    def slot1(t: stim.GateTarget) -> complex:
        p = t2p(t)
        uses[t].append(time)
        offset = len(uses[t]) - 1.0
        offset *= r * 1.7
        return p + offset

    def slot2(t1: stim.GateTarget, t2: stim.GateTarget) -> tuple[complex, complex]:
        p1 = t2p(t1)
        p2 = t2p(t2)
        key = frozenset([t1, t2])
        uses[key].append(time)
        offset = len(uses[key]) - 1.0
        offset *= r * 0.8
        return p1 + offset, p2 + offset

    time = 0
    r = scale * 0.08
    sw = scale * 0.01
    fs = scale * 0.1
    for q in i2q.values():
        p = q2p(q)
        lines.append(
            f"""<circle cx="{p.real}" cy="{p.imag}" r="{r*0.7}" stroke="none" fill="black"/>"""
        )
    for instruction in circuit.flattened():
        if instruction.name == "TICK":
            time += 1
        elif instruction.name == "RX" or instruction.name == "MX":
            for t in instruction.targets_copy():
                p = slot1(t)
                lines.append(
                    f"""<rect """
                    f"""x="{p.real - r}" """
                    f"""y="{p.imag - r}" """
                    f"""width="{2*r}" """
                    f"""height="{2*r}" """
                    f"""stroke-width="{sw}" """
                    f"""stroke="black" fill="white"/>"""
                )
                lines.append(
                    f"""<text """
                    f"""x="{p.real}" """
                    f"""y="{p.imag}" """
                    f"""font-size="{fs}" """
                    f"""text-anchor="middle" """
                    f"""dominant-baseline="central" """
                    f"""fill="black">{instruction.name}</text>"""
                )
        elif instruction.name == "R" or instruction.name == "M":
            for t in instruction.targets_copy():
                p = slot1(t)
                lines.append(
                    f"""<rect """
                    f"""x="{p.real - r}" """
                    f"""y="{p.imag - r}" """
                    f"""width="{2*r}" """
                    f"""height="{2*r}" """
                    f"""stroke-width="{sw}" """
                    f"""stroke="black" """
                    f"""fill="black"/>"""
                )
                lines.append(
                    f"""<text """
                    f"""x="{p.real}" """
                    f"""y="{p.imag}" """
                    f"""font-size="{fs}" """
                    f"""text-anchor="middle" """
                    f"""dominant-baseline="central" """
                    f"""fill="white">{instruction.name}Z</text>"""
                )
        elif instruction.name == "CX" or instruction.name == "CZ":
            for a, b in instruction.target_groups():
                pa, pb = slot2(a, b)
                pc = (pa + pb) / 2
                pa = pa * 0.4 + pc * 0.6
                pb = pb * 0.4 + pc * 0.6
                lines.append(
                    f"""<path """
                    f"""d="M{pa.real},{pa.imag} """
                    f"""L{pb.real},{pb.imag}" """
                    f"""stroke-width="{sw}" """
                    f"""stroke="black" """
                    f"""fill="none"/>"""
                )
                lines.append(
                    f"""<circle """
                    f"""cx="{pa.real}" """
                    f"""cy="{pa.imag}" """
                    f"""r="{r}" """
                    f"""stroke="black" """
                    f"""stroke-width="{sw}" """
                    f"""fill="black"/>"""
                )
                if instruction.name == "CX":
                    lines.append(
                        f"""<circle """
                        f"""cx="{pb.real}" """
                        f"""cy="{pb.imag}" """
                        f"""r="{r}" """
                        f"""stroke-width="{sw}" """
                        f"""stroke="black" """
                        f"""fill="white"/>"""
                    )
                    lines.append(
                        f"""<path """
                        f"""d="M{pb.real-r},{pb.imag} """
                        f"""l{2*r},0 """
                        f"""M{pb.real},{pb.imag-r} """
                        f"""l0,{2*r}" """
                        f"""stroke-width="{sw}" """
                        f"""stroke="black" """
                        f"""fill="none"/>"""
                    )
                elif instruction.name == "CZ":
                    lines.append(
                        f"""<circle """
                        f"""cx="{pb.real}" """
                        f"""cy="{pb.imag}" """
                        f"""r="{r}" """
                        f"""stroke-width="{sw}" """
                        f"""stroke="black" """
                        f"""fill="black"/>"""
                    )
                else:
                    raise NotImplementedError(f"{instruction=}")
        elif instruction.name == "H":
            for t in instruction.targets_copy():
                p = slot1(t)
                lines.append(
                    f"""<rect """
                    f"""x="{p.real - r}" """
                    f"""y="{p.imag - r}" """
                    f"""width="{2 * r}" """
                    f"""height="{2 * r}" """
                    f"""stroke-width="{sw}" """
                    f"""stroke="black" """
                    f"""fill="yellow"/>"""
                )
                lines.append(
                    f"""<text """
                    f"""x="{p.real}" """
                    f"""y="{p.imag}" """
                    f"""font-size="8" """
                    f"""text-anchor="middle" """
                    f"""dominant-baseline="central" """
                    f"""fill="black">H</text>"""
                )
        elif instruction.name == "QUBIT_COORDS":
            pass
        else:
            raise NotImplementedError(f"{instruction=}")
    for k, v in list(uses.items()):
        label = ",".join(str(e) for e in v)
        if isinstance(k, stim.GateTarget):
            p = slot1(k)
            p -= r * 0.5
        else:
            a, b = k
            pa, pb = slot2(a, b)
            if pa.real > pb.real:
                pa, pb = pb, pa
            pc = (pa + pb) / 2
            p = pa
            p += r * 0.7j * (1 if pb.imag > pa.imag else -1)
            p = p * 0.5 + pc * 0.5
            p += 0.7 * r
        lines.append(
            f"""<text """
            f"""x="{p.real}" """
            f"""y="{p.imag}" """
            f"""font-size="{fs*8/5}" """
            f"""text-anchor="left" """
            f"""dominant-baseline="central" """
            f"""fill="black">{label}</text>"""
        )
    return str_svg("\n".join(lines))
