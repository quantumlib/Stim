"""We do not have ColorZ from the SAT/SMT. Now we color the Z-pipes."""

from typing import Sequence, Mapping, Any, Union, Tuple


def if_uncolorK(
    n_i: int,
    n_j: int,
    n_k: int,
    ExistK: Sequence[Sequence[Sequence[int]]],
    ColorKP: Sequence[Sequence[Sequence[int]]],
    ColorKM: Sequence[Sequence[Sequence[int]]],
) -> bool:
    """return whether there are uncolored K-pipes"""
    for i in range(n_i):
        for j in range(n_j):
            for k in range(n_k):
                if ExistK[i][j][k] and (
                    ColorKP[i][j][k] == -1 or ColorKM[i][j][k] == -1
                ):
                    return True
    return False


def in_bound(n_i: int, n_j: int, n_k: int, i: int, j: int, k: int) -> bool:
    if i in range(n_i) and j in range(n_j) and k in range(n_k):
        return True
    return False


def propogate_IJcolor(
    n_i: int,
    n_j: int,
    n_k: int,
    ExistI: Sequence[Sequence[Sequence[int]]],
    ExistJ: Sequence[Sequence[Sequence[int]]],
    ExistK: Sequence[Sequence[Sequence[int]]],
    ColorI: Sequence[Sequence[Sequence[int]]],
    ColorJ: Sequence[Sequence[Sequence[int]]],
    ColorKP: Sequence[Sequence[Sequence[int]]],
    ColorKM: Sequence[Sequence[Sequence[int]]],
) -> None:
    """propagate the color of I- and J-pipes to their neighbor K-pipes."""

    for i in range(n_i):
        for j in range(n_j):
            for k in range(n_k):
                if ExistK[i][j][k]:
                    # 4 possible neighbor I/J pipe for the minus end of K-pipe
                    if in_bound(n_i, n_j, n_k, i - 1, j, k) and ExistI[i - 1][j][k]:
                        ColorKM[i][j][k] = 1 - ColorI[i - 1][j][k]
                    if ExistI[i][j][k]:
                        ColorKM[i][j][k] = 1 - ColorI[i][j][k]
                    if in_bound(n_i, n_j, n_k, i, j - 1, k) and ExistJ[i][j - 1][k]:
                        ColorKM[i][j][k] = 1 - ColorJ[i][j - 1][k]
                    if ExistJ[i][j][k]:
                        ColorKM[i][j][k] = 1 - ColorJ[i][j][k]

                    # 4 possible neighbor I/J pipe for the plus end of K-pipe
                    if (
                        in_bound(n_i, n_j, n_k, i - 1, j, k + 1)
                        and ExistI[i - 1][j][k + 1]
                    ):
                        ColorKP[i][j][k] = 1 - ColorI[i - 1][j][k + 1]
                    if in_bound(n_i, n_j, n_k, i, j, k + 1) and ExistI[i][j][k + 1]:
                        ColorKP[i][j][k] = 1 - ColorI[i][j][k + 1]
                    if (
                        in_bound(n_i, n_j, n_k, i, j - 1, k + 1)
                        and ExistJ[i][j - 1][k + 1]
                    ):
                        ColorKP[i][j][k] = 1 - ColorJ[i][j - 1][k + 1]
                    if in_bound(n_i, n_j, n_k, i, j, k + 1) and ExistJ[i][j][k + 1]:
                        ColorKP[i][j][k] = 1 - ColorJ[i][j][k + 1]


def propogate_Kcolor(
    n_i: int,
    n_j: int,
    n_k: int,
    ExistK: Sequence[Sequence[Sequence[int]]],
    ColorKP: Sequence[Sequence[Sequence[int]]],
    ColorKM: Sequence[Sequence[Sequence[int]]],
    NodeY: Sequence[Sequence[Sequence[int]]],
) -> bool:
    """propagate color from colored K-pipes to uncolored K-pipes.
    If no new color can be assigned, return False; otherwise, return True."""

    did_something = False
    for i in range(n_i):
        for j in range(n_j):
            for k in range(n_k):
                if ExistK[i][j][k]:
                    # consider propagate color from below
                    if (
                        in_bound(n_i, n_j, n_k, i, j, k - 1)
                        and ExistK[i][j][k - 1]
                        and NodeY[i][j][k - 1] == 0
                    ):
                        if ColorKP[i][j][k - 1] > -1 and ColorKM[i][j][k] == -1:
                            ColorKM[i][j][k] = ColorKP[i][j][k - 1]
                            did_something = True
                    # consider propagate color from above
                    if (
                        in_bound(n_i, n_j, n_k, i, j, k + 1)
                        and ExistK[i][j][k + 1]
                        and NodeY[i][j][k + 1] == 0
                    ):
                        if ColorKM[i][j][k + 1] > -1 and ColorKP[i][j][k] == -1:
                            ColorKP[i][j][k] = ColorKM[i][j][k + 1]
                            did_something = True

                    # if K-pipe connects a Y Cube, two ends can be colored same
                    if (
                        NodeY[i][j][k]
                        and ColorKM[i][j][k] == -1
                        and ColorKP[i][j][k] > -1
                    ):
                        ColorKM[i][j][k] = ColorKP[i][j][k]
                        did_something = True
                    if (
                        in_bound(n_i, n_j, n_k, i, j, k + 1)
                        and NodeY[i][j][k + 1]
                        and ColorKM[i][j][k] > -1
                        and ColorKP[i][j][k] == -1
                    ):
                        ColorKP[i][j][k] = ColorKM[i][j][k]
                        did_something = True
    return did_something


def assign_Kcolor(
    n_i: int,
    n_j: int,
    n_k: int,
    ExistK: Sequence[Sequence[Sequence[int]]],
    ColorKP: Sequence[Sequence[Sequence[int]]],
    ColorKM: Sequence[Sequence[Sequence[int]]],
    NodeY: Sequence[Sequence[Sequence[int]]],
) -> None:
    """when no color can be deducted by propagating from other K-pipes, we
    assign some color variables at will. Then, we can continue to propagate."""

    # assign a color by letting the two ends of a K-pipe to be the same
    for i in range(n_i):
        for j in range(n_j):
            for k in range(n_k):
                if ExistK[i][j][k]:
                    if ColorKM[i][j][k] > -1 and ColorKP[i][j][k] == -1:
                        ColorKP[i][j][k] = ColorKM[i][j][k]
                        break
    # For K-pipes that have no color at both ends and connects a Y-cube
    for i in range(n_i):
        for j in range(n_j):
            for k in range(n_k):
                if ExistK[i][j][k]:
                    if NodeY[i][j][k] and ColorKM[i][j][k] == -1:
                        ColorKM[i][j][k] = 0
                        break
                    if (
                        in_bound(n_i, n_j, n_k, i, j, k + 1)
                        and NodeY[i][j][k + 1]
                        and ColorKP[i][j][k] == -1
                    ):
                        ColorKP[i][j][k] = 0
                        break


def color_ports(
    ports: Sequence[Mapping[str, Union[str, int]]],
    ColorKP: Sequence[Sequence[Sequence[int]]],
    ColorKM: Sequence[Sequence[Sequence[int]]],
) -> None:
    for port in ports:
        if port["d"] == "K":
            if port["e"] == "+":
                ColorKP[port["i"]][port["j"]][port["k"]] = port["c"]
            else:
                ColorKM[port["i"]][port["j"]][port["k"]] = port["c"]


def color_kp_km(
    n_i: int,
    n_j: int,
    n_k: int,
    ExistI: Sequence[Sequence[Sequence[int]]],
    ExistJ: Sequence[Sequence[Sequence[int]]],
    ExistK: Sequence[Sequence[Sequence[int]]],
    ColorI: Sequence[Sequence[Sequence[int]]],
    ColorJ: Sequence[Sequence[Sequence[int]]],
    ports: Sequence[Mapping[str, Union[str, int]]],
    NodeY: Sequence[Sequence[Sequence[int]]],
) -> Tuple[Sequence[Sequence[Sequence[int]]], Sequence[Sequence[Sequence[int]]]]:
    ColorKP = [[[-1 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]
    ColorKM = [[[-1 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]

    # at ports, the color follows from the port configuration
    color_ports(ports, ColorKP, ColorKM)

    # propogate the color of I-pipes and J-pipes to their neighboring K-pipes
    propogate_IJcolor(
        n_i, n_j, n_k, ExistI, ExistJ, ExistK, ColorI, ColorJ, ColorKP, ColorKM
    )

    # the rest of the K-pipes are only neighboring other K-pipes. Until all of
    # them are colored, we propagate colors of the existing K-pipes. If at one
    # point, nothing can be implied via propagation, we assign a color at will
    # and continue. Because of the domain wall operation, we can do this.
    while if_uncolorK(n_i, n_j, n_k, ExistK, ColorKP, ColorKM):
        if not propogate_Kcolor(n_i, n_j, n_k, ExistK, ColorKP, ColorKM, NodeY):
            assign_Kcolor(n_i, n_j, n_k, ExistK, ColorKP, ColorKM, NodeY)
    return ColorKP, ColorKM


def color_z(lasre: Mapping[str, Any]) -> Mapping[str, Any]:
    n_i, n_j, n_k = (
        lasre["n_i"],
        lasre["n_j"],
        lasre["n_k"],
    )
    ExistI, ColorI, ExistJ, ColorJ, ExistK = (
        lasre["ExistI"],
        lasre["ColorI"],
        lasre["ExistJ"],
        lasre["ColorJ"],
        lasre["ExistK"],
    )
    NodeY = lasre["NodeY"]
    ports = lasre["ports"]

    # for a K-pipe (i,j,k)-(i,j,k+1), ColorKP (plus) is its color at (i,j,k+1)
    # and ColorKM (minus) is its color at (i,j,k)
    lasre["ColorKP"], lasre["ColorKM"] = color_kp_km(
        n_i, n_j, n_k, ExistI, ExistJ, ExistK, ColorI, ColorJ, ports, NodeY
    )
    return lasre
