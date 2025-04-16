"""assuming all T injections requiring fixup are on the top floor.
The output is also on the top floor"""

from typing import Mapping, Any


def attach_fixups(lasre: Mapping[str, Any]) -> Mapping[str, Any]:
    n_s = lasre["n_s"]
    n_i = lasre["n_i"]
    n_j = lasre["n_j"]
    n_k = lasre["n_k"]

    fixup_locs = []
    for p in lasre["optional"]["top_fixups"]:
        fixup_locs.append((lasre["ports"][p]["i"], lasre["ports"][p]["j"]))

    lasre["n_k"] += 2
    # we add two layers on the top. The lower layer will contain fixups dressed
    # as Y cubes that is connecting downwards. The upper layer will contain no
    # new cubes. This corresponds to waiting the machine to apply the fixups
    # because there is a finite interaction time from the machine knows whether
    # the injection is T or T^dagger to apply the fixups.
    for i in range(n_i):
        for j in range(n_j):
            if (i, j) in fixup_locs:
                lasre["NodeY"][i][j].append(1)  # fixup dressed as Y cubes
                lasre["ExistK"][i][j][n_k - 1] = 1  # connect fixup downwards
            else:
                lasre["NodeY"][i][j].append(0)  # no fixup
                lasre["ExistK"][i][j][n_k - 1] = 0
            lasre["NodeY"][i][j].append(0)  # the upper layer is empty

            # do not add any new pipes in the added layer
            for arr in ["ExistI", "ExistJ", "ExistK"]:
                lasre[arr][i][j].append(0)
                lasre[arr][i][j].append(0)
            for arr in ["ColorI", "ColorJ", "ColorKM", "ColorKP"]:
                lasre[arr][i][j].append(-1)
                lasre[arr][i][j].append(-1)
            for s in range(n_s):
                for arr in ["CorrIJ", "CorrIK", "CorrJK", "CorrJI", "CorrKI", "CorrKJ"]:
                    lasre[arr][s][i][j].append(0)
                    lasre[arr][s][i][j].append(0)

    # the output ports need to be extended in the two added layers
    for port in lasre["ports"]:
        if "f" in port and port["f"] == "output":
            ii, jj = port["i"], port["j"]
            lasre["ExistK"][ii][jj][n_k - 1] = 1
            lasre["ExistK"][ii][jj][n_k] = 1
            lasre["ExistK"][ii][jj][n_k + 1] = 1
            lasre["ColorKM"][ii][jj][n_k] = lasre["ColorKP"][ii][jj][n_k - 1]
            lasre["ColorKM"][ii][jj][n_k + 1] = lasre["ColorKM"][ii][jj][n_k]
            lasre["ColorKP"][ii][jj][n_k] = lasre["ColorKM"][ii][jj][n_k]
            lasre["ColorKP"][ii][jj][n_k + 1] = lasre["ColorKP"][ii][jj][n_k]
            for s in range(n_s):
                lasre["CorrKI"][s][ii][jj][n_k] = lasre["CorrKI"][s][ii][jj][n_k - 1]
                lasre["CorrKI"][s][ii][jj][n_k + 1] = lasre["CorrKI"][s][ii][jj][n_k]
                lasre["CorrKJ"][s][ii][jj][n_k] = lasre["CorrKJ"][s][ii][jj][n_k - 1]
                lasre["CorrKJ"][s][ii][jj][n_k + 1] = lasre["CorrKJ"][s][ii][jj][n_k]
            port["k"] += 2
            new_cubes = []
            for c in lasre["port_cubes"]:
                if c[0] == port["i"] and c[1] == port["j"]:
                    new_cubes.append((c[0], c[1], port["k"] + 1))
                else:
                    new_cubes.append(c)
            lasre["port_cubes"] = new_cubes

    t_injections = []
    for port in lasre["ports"]:
        if port["f"] == "T":
            if port["e"] == "+":
                t_injections.append([port["i"], port["j"], port["k"] + 1])
            else:
                t_injections.append([port["i"], port["j"], port["k"]])
    lasre["optional"]["t_injections"] = t_injections

    return lasre
