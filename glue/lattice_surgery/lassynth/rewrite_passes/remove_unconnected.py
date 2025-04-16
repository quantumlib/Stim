"""In the generated LaS, there can be some 'floating donuts' not connecting to
any ports. These objects won't affect the function of the LaS. We remove them.
"""

from typing import Mapping, Any, Sequence, Union, Tuple


def check_cubes(
    n_i: int,
    n_j: int,
    n_k: int,
    ExistI: Sequence[Sequence[Sequence[int]]],
    ExistJ: Sequence[Sequence[Sequence[int]]],
    ExistK: Sequence[Sequence[Sequence[int]]],
    ports: Sequence[Mapping[str, Union[str, int]]],
    NodeY: Sequence[Sequence[Sequence[int]]],
) -> Sequence[Sequence[Sequence[int]]]:
    # we linearize the cubes, cube at (i,j,k) -> index i*n_j*n_k + j*n_k + k
    # construct adjancency list of the cubes from the pipes
    adj = [[] for _ in range(n_i * n_j * n_k)]
    for i in range(n_i):
        for j in range(n_j):
            for k in range(n_k):
                if ExistI[i][j][k] and i + 1 < n_i:
                    adj[i * n_j * n_k + j * n_k + k].append(
                        (i + 1) * n_j * n_k + j * n_k + k
                    )
                    adj[(i + 1) * n_j * n_k + j * n_k + k].append(
                        i * n_j * n_k + j * n_k + k
                    )
                if ExistJ[i][j][k] and j + 1 < n_j:
                    adj[i * n_j * n_k + j * n_k + k].append(
                        i * n_j * n_k + (j + 1) * n_k + k
                    )
                    adj[i * n_j * n_k + (j + 1) * n_k + k].append(
                        i * n_j * n_k + j * n_k + k
                    )
                if ExistK[i][j][k] and k + 1 < n_k:
                    adj[i * n_j * n_k + j * n_k + k].append(
                        i * n_j * n_k + j * n_k + k + 1
                    )
                    adj[i * n_j * n_k + j * n_k + k + 1].append(
                        i * n_j * n_k + j * n_k + k
                    )

    # if a cube can reach any of the vips, i.e., open cube for a port
    vips = [p["i"] * n_j * n_k + p["j"] * n_k + p["k"] for p in ports]

    # first assume all cubes are nonconnected
    connected_cubes = [
        [[0 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)
    ]

    # a Y cube is only effective if it is connected to a cube (i,j,k) that is
    # connected to ports. In this case, (i,j,k) will be in `connected_cubes`
    # and all pipes from (i,j,k) will be selected in `check_pipes`, so we can
    # assume all the Y cubes to be nonconnected for now.
    y_cubes = [
        i * n_j * n_k + j * n_k + k
        for i in range(n_i)
        for j in range(n_j)
        for k in range(n_k)
        if NodeY[i][j][k]
    ]

    for i in range(n_i):
        for j in range(n_j):
            for k in range(n_k):
                # breadth first search for each cube
                queue = [
                    i * n_j * n_k + j * n_k + k,
                ]
                if i * n_j * n_k + j * n_k + k in y_cubes:
                    continue
                visited = [0 for _ in range(n_i * n_j * n_k)]
                while len(queue) > 0:
                    if queue[0] in vips:
                        connected_cubes[i][j][k] = 1
                        break
                    visited[queue[0]] = 1
                    for v in adj[queue[0]]:
                        if not visited[v] and v not in y_cubes:
                            queue.append(v)
                    queue.pop(0)

    return connected_cubes


def check_pipes(
    n_i: int,
    n_j: int,
    n_k: int,
    ExistI: Sequence[Sequence[Sequence[int]]],
    ExistJ: Sequence[Sequence[Sequence[int]]],
    ExistK: Sequence[Sequence[Sequence[int]]],
    connected_cubes: Sequence[Sequence[Sequence[int]]],
) -> Tuple[
    Sequence[Sequence[Sequence[int]]],
    Sequence[Sequence[Sequence[int]]],
    Sequence[Sequence[Sequence[int]]],
]:
    EffectI = [[[0 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]
    EffectJ = [[[0 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]
    EffectK = [[[0 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]
    for i in range(n_i):
        for j in range(n_j):
            for k in range(n_k):
                if ExistI[i][j][k] and (
                    connected_cubes[i][j][k]
                    or (i + 1 < n_i and connected_cubes[i + 1][j][k])
                ):
                    EffectI[i][j][k] = 1
                if ExistJ[i][j][k] and (
                    connected_cubes[i][j][k]
                    or (j + 1 < n_j and connected_cubes[i][j + 1][k])
                ):
                    EffectJ[i][j][k] = 1
                if ExistK[i][j][k] and (
                    connected_cubes[i][j][k]
                    or (k + 1 < n_k and connected_cubes[i][j][k + 1])
                ):
                    EffectK[i][j][k] = 1
    return EffectI, EffectJ, EffectK


def array3DAnd(
    arr0: Sequence[Sequence[Sequence[int]]], arr1: Sequence[Sequence[Sequence[int]]]
) -> Sequence[Sequence[Sequence[int]]]:
    """taking the AND of two arrays of bits"""
    a = len(arr0)
    b = len(arr0[0])
    c = len(arr0[0][0])
    arrAnd = [[[0 for _ in range(c)] for _ in range(b)] for _ in range(a)]
    for i in range(a):
        for j in range(b):
            for k in range(c):
                if arr0[i][j][k] == 1 and arr1[i][j][k] == 1:
                    arrAnd[i][j][k] = 1
    return arrAnd


def remove_unconnected(lasre: Mapping[str, Any]) -> Mapping[str, Any]:
    n_i, n_j, n_k = (
        lasre["n_i"],
        lasre["n_j"],
        lasre["n_k"],
    )
    ExistI, ExistJ, ExistK, NodeY = (
        lasre["ExistI"],
        lasre["ExistJ"],
        lasre["ExistK"],
        lasre["NodeY"],
    )
    ports = lasre["ports"]

    connected_cubes = check_cubes(n_i, n_j, n_k, ExistI, ExistJ, ExistK, ports, NodeY)
    connectedI, connectedJ, connectedK = check_pipes(
        n_i, n_j, n_k, ExistI, ExistJ, ExistK, connected_cubes
    )
    maskedI, maskedJ, maskedK = (
        array3DAnd(ExistI, connectedI),
        array3DAnd(ExistJ, connectedJ),
        array3DAnd(ExistK, connectedK),
    )
    lasre["ExistI"], lasre["ExistJ"], lasre["ExistK"] = maskedI, maskedJ, maskedK

    return lasre
