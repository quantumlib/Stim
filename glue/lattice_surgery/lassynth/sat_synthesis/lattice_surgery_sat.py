"""LatticeSurgerySAT to encode the synthesis problem to SAT/SMT"""

import os
import subprocess
import sys
import tempfile
import time
from typing import Any, Mapping, Sequence, Union, Tuple, Optional
import z3


def var_given(
    data: Mapping[str, Any],
    arr: str,
    i: int,
    j: int,
    k: int,
    l: Optional[int] = None,
) -> bool:
    """Check whether data[arr][i][j][k]([l]) is given.

    If the given indices are not found, return False; otherwise return True.

    Args:
        data (Mapping[str, Any]): contain arrays
        arr (str): ExistI, etc.
        i (int): first index
        j (int): second index
        k (int): third index
        l (int, optional): optional fourth index. Defaults to None.

    Returns:
        bool: whether the variable value is given

    Raises:
        ValueError: found value, but not 0 nor 1 nor -1.
    """

    if arr not in data:
        return False
    if l is None:
        if data[arr][i][j][k] == -1:
            return False
        elif data[arr][i][j][k] != 0 and data[arr][i][j][k] != 1:
            raise ValueError(f"{arr}[{i}, {j}, {k}] is not 0 nor 1 nor -1.")
        return True
    # l is not None, then
    if data[arr][i][j][k][l] == -1:
        return False
    elif data[arr][i][j][k][l] != 0 and data[arr][i][j][k][l] != 1:
        raise ValueError(f"{arr}[{i}, {j}, {k}, {l}] is not 0 nor 1 nor -1.")
    return True


def port_incident_pipes(
        port: Mapping[str, Union[str, int]], n_i: int, n_j: int,
        n_k: int) -> Tuple[Sequence[str], Sequence[Tuple[int, int, int]]]:
    """Compute the pipes incident to a port.

    A port is an pipe with a open end. The incident pipes of a port are the
    five other pipes connecting to that end. However, some of these pipes
    can be out of bound, we just want to compute those that are in bound.

    Args:
        port (Mapping[str, Union[str,  int]]): the port to consider
        n_i (int): spatial bound on I direction
        n_j (int): spatial bound on J direction
        n_k (int): spatial bound on K direction

    Returns:
        Tuple[Sequence[str], Sequence[Tuple[int, int, int]]]:
            Two lists of the same length [0,6): (dirs, coords)
            dirs: the direction of the incident pipes, can be "I", "J", or "K"
            coords: the coordinates of the incident pipes, each one is (i,j,k)
    """
    coords = []
    dirs = []

    # first, just consider adjancency without caring about out-of-bound
    if port["d"] == "I":
        adj_dirs = ["I", "J", "J", "K", "K"]
        if port["e"] == "-":  # empty cube is (i,j,k)
            adj_coords = [
                (port["i"] - 1, port["j"], port["k"]),  # (i-1,j,k)---(i,j,k)
                (port["i"], port["j"] - 1, port["k"]),  # (i,j-1,k)---(i,j,k)
                (port["i"], port["j"], port["k"]),  # (i,j,k)---(i,j+1,k)
                (port["i"], port["j"], port["k"] - 1),  # (i,j,k-1)---(i,j,k)
                (port["i"], port["j"], port["k"]),  # (i,j,k)---(i,j,k+1)
            ]
        elif port["e"] == "+":  # empty cube is (i+1,j,k)
            adj_coords = [
                (port["i"] + 1, port["j"], port["k"]),  # (i+1,j,k)---(i+2,j,k)
                (port["i"] + 1, port["j"] - 1,
                 port["k"]),  # (i+1,j-1,k)---(i+1,j,k)
                (port["i"] + 1, port["j"],
                 port["k"]),  # (i+1,j,k)---(i+1,j+1,k)
                (port["i"] + 1, port["j"],
                 port["k"] - 1),  # (i+1,j,k-1)---(i+1,j,k)
                (port["i"] + 1, port["j"],
                 port["k"]),  # (i+1,j,k)---(i+1,j,k+1)
            ]

    if port["d"] == "J":
        adj_dirs = ["J", "K", "K", "I", "I"]
        if port["e"] == "-":
            adj_coords = [
                (port["i"], port["j"] - 1, port["k"]),
                (port["i"], port["j"], port["k"] - 1),
                (port["i"], port["j"], port["k"]),
                (port["i"] - 1, port["j"], port["k"]),
                (port["i"], port["j"], port["k"]),
            ]
        elif port["e"] == "+":
            adj_coords = [
                (port["i"], port["j"] + 1, port["k"]),
                (port["i"], port["j"] + 1, port["k"] - 1),
                (port["i"], port["j"] + 1, port["k"]),
                (port["i"] - 1, port["j"] + 1, port["k"]),
                (port["i"], port["j"] + 1, port["k"]),
            ]

    if port["d"] == "K":
        adj_dirs = ["K", "I", "I", "J", "J"]
        if port["e"] == "-":
            adj_coords = [
                (port["i"], port["j"], port["k"] - 1),
                (port["i"] - 1, port["j"], port["k"]),
                (port["i"], port["j"], port["k"]),
                (port["i"], port["j"] - 1, port["k"]),
                (port["i"], port["j"], port["k"]),
            ]
        elif port["e"] == "+":
            adj_coords = [
                (port["i"], port["j"], port["k"] + 1),
                (port["i"] - 1, port["j"], port["k"] + 1),
                (port["i"], port["j"], port["k"] + 1),
                (port["i"], port["j"] - 1, port["k"] + 1),
                (port["i"], port["j"], port["k"] + 1),
            ]

    # only keep the pipes in bound
    for i, coord in enumerate(adj_coords):
        if ((coord[0] in range(n_i)) and (coord[1] in range(n_j))
                and (coord[2] in range(n_k))):
            coords.append(adj_coords[i])
            dirs.append(adj_dirs[i])

    return dirs, coords


def cnf_even_parity_upto4(eles: Sequence[Any]) -> Any:
    """Compute the CNF format of parity of up to four Z3 binary variables.

    Args:
        eles (Sequence[Any]): the binary variables.

    Returns:
        (Any) the Z3 constraint meaning the parity of the inputs is even.

    Raises:
        ValueError: number of elements is not 1, 2, 3, or 4.
    """

    if len(eles) == 1:
        # 1 var even parity -> this var is false
        return z3.Not(eles[0])

    elif len(eles) == 2:
        # 2 vars even pairty -> both True or both False
        return z3.Or(z3.And(z3.Not(eles[0]), z3.Not(eles[1])),
                     z3.And(eles[0], eles[1]))

    elif len(eles) == 3:
        # 3 vars even parity -> all False, or 2 True and 1 False
        return z3.Or(
            z3.And(z3.Not(eles[0]), z3.Not(eles[1]), z3.Not(eles[2])),
            z3.And(eles[0], eles[1], z3.Not(eles[2])),
            z3.And(eles[0], z3.Not(eles[1]), eles[2]),
            z3.And(z3.Not(eles[0]), eles[1], eles[2]),
        )

    elif len(eles) == 4:
        # 4 vars even parity -> 0, 2, or 4 vars are True
        return z3.Or(
            z3.And(z3.Not(eles[0]), z3.Not(eles[1]), z3.Not(eles[2]),
                   z3.Not(eles[3])),
            z3.And(z3.Not(eles[0]), z3.Not(eles[1]), eles[2], eles[3]),
            z3.And(z3.Not(eles[0]), eles[1], z3.Not(eles[2]), eles[3]),
            z3.And(z3.Not(eles[0]), eles[1], eles[2], z3.Not(eles[3])),
            z3.And(eles[0], z3.Not(eles[1]), z3.Not(eles[2]), eles[3]),
            z3.And(eles[0], z3.Not(eles[1]), eles[2], z3.Not(eles[3])),
            z3.And(eles[0], eles[1], z3.Not(eles[2]), z3.Not(eles[3])),
            z3.And(eles[0], eles[1], eles[2], eles[3]),
        )

    else:
        raise ValueError("This function only supports 1, 2, 3, or 4 vars.")


class LatticeSurgerySAT:
    """class of synthesizing LaSRe using Z3 SMT solver and Kissat SAT solver.

    It encodes a lattice surgery synthesis problem to SAT/SMT and checks
    whether there is a solution. We are given certain spacetime volume, certain
    ports, and certain stabilizers. LatticeSurgerySAT encodes the constraints
    on LaSRe variables such that the resulting variable assignments consist of
    a valid lattice surgery subroutine with the correct functionality
    (satisfies all the given stabilizers). LatticeSurgerySAT finds the solution
    with a SAT/SMT solver.
    """

    def __init__(
        self,
        input_dict: Mapping[str, Any],
        color_ij: bool = True,
        given_arrs: Optional[Mapping[str, Any]] = None,
        given_vals: Optional[Sequence[Mapping[str, Any]]] = None,
    ) -> None:
        """initialization of LatticeSurgerySAT.

        Args:
            input_dict (Mapping[str, Any]): specification of LaS.
            color_ij (bool, optional): if the color matching constraints of
                I and J pipes are imposed. Defaults to True. So far, we always
                impose these constraints.
            given_arrs (Mapping[str, Any], optional):
                    Arrays of values to plug in. Defaults to None.
            given_vals (Sequence[Mapping[str, Any]], optional):
                Values to plug in. Defaults to None. These values will
                replace existing values if already set by given_arrs.
        """
        self.input_dict = input_dict
        self.color_ij = color_ij
        self.goal = z3.Goal()
        self.process_input(input_dict)
        self.build_smt_model(given_arrs=given_arrs, given_vals=given_vals)

    def process_input(self, input_dict: Mapping[str, Any]) -> None:
        """read input specification, mainly translating the info at the ports.

        Args:
            input_dict (Mapping[str, Any]): LaS specification.

        Raises:
            ValueError: missing key in input specification.
            ValueError: some spatial bound <= 0.
            ValueError: more stabilizers than ports.
            ValueError: stabilizer length is not the same as
                the number of ports.
            ValueError: stabilizer contains things other than I, X, Y, or Z.
            ValueError: missing key in port.
            ValueError: port location is not a 3-tuple.
            ValueError: port direction is not 2-string.
            ValueError: port sign (which end is dangling) is not - or +.
            ValueError: port axis is not I, J, or K.
            ValueError: port location+direction is out of bound.
            ValueError: port Z basis direction is not I, J, or K, and
                the same with the pipe.
            ValueError: forbidden cube location is not a 3-tuple.
            ValueError: forbiddent cube location is out of bounds.
        """
        data = input_dict

        for key in ["max_i", "max_j", "max_k", "ports", "stabilizers"]:
            if key not in data:
                raise ValueError(f"missing key {key} in input specification.")

        # load spatial bound, check > 0
        self.n_i = data["max_i"]
        self.n_j = data["max_j"]
        self.n_k = data["max_k"]
        if min([self.n_i, self.n_j, self.n_k]) <= 0:
            raise ValueError("max_i or _j or _k <= 0.")

        self.n_p = len(data["ports"])
        self.n_s = len(data["stabilizers"])
        # there should be at most as many stabilizers as ports
        if self.n_s > self.n_p:
            raise ValueError(
                f"{self.n_s} stabilizers, too many for {self.n_p} ports.")

        # stabilizers should be paulistrings of length #ports
        self.paulistrings = [s.replace(".", "I") for s in data["stabilizers"]]
        for s in self.paulistrings:
            if len(s) != self.n_p:
                raise ValueError(
                    f"len({s}) = {len(s)}, but there are {self.n_p} ports.")
            for i in range(len(s)):
                if s[i] not in ["I", "X", "Y", "Z"]:
                    raise ValueError(
                        f"{s} has invalid Pauli. I, X, Y, and Z are allowed.")

        # transform port data
        self.ports = []
        for port in data["ports"]:
            for key in ["location", "direction", "z_basis_direction"]:
                if key not in port:
                    raise ValueError(f"missing key {key} in port {port}")

            if len(port["location"]) != 3:
                raise ValueError(f"port location should be 3-tuple {port}.")

            if len(port["direction"]) != 2:
                raise ValueError(
                    f"port direction should have 2 characters {port}.")
            if port["direction"][0] not in ["+", "-"]:
                raise ValueError(f"port direction with invalid sign {port}.")
            if port["direction"][1] not in ["I", "J", "K"]:
                raise ValueError(f"port direction with invalid axis {port}.")

            if port["direction"][0] == "-" and port["direction"][
                    1] == "I" and (port["location"][0] not in range(
                        1, self.n_i + 1)):
                raise ValueError(
                    f"{port['location']} with direction {port['direction']}"
                    f" should be in range [1, f{self.n_i+1}).")
            if port["direction"][0] == "+" and port["direction"][
                    1] == "I" and (port["location"][0] not in range(
                        0, self.n_i)):
                raise ValueError(
                    f"{port['location']} with direction {port['direction']}"
                    f" should be in range [0, f{self.n_i}).")
            if port["direction"][0] == "-" and port["direction"][
                    1] == "J" and (port["location"][1] not in range(
                        1, self.n_j + 1)):
                raise ValueError(
                    f"{port['location']} with direction {port['direction']}"
                    f" should be in range [1, {self.n_j+1}).")
            if port["direction"][0] == "+" and port["direction"][
                    1] == "J" and (port["location"][1] not in range(
                        0, self.n_j)):
                raise ValueError(
                    f"{port['location']} with direction {port['direction']}"
                    f" should be in range [0, f{self.n_j}).")
            if port["direction"][0] == "-" and port["direction"][
                    1] == "K" and (port["location"][2] not in range(
                        1, self.n_k + 1)):
                raise ValueError(
                    f"{port['location']} with direction {port['direction']}"
                    f" should be in range [1, f{self.n_k+1}).")
            if port["direction"][0] == "+" and port["direction"][
                    1] == "K" and (port["location"][2] not in range(
                        0, self.n_k)):
                raise ValueError(
                    f"{port['location']} with direction {port['direction']}"
                    f" should be in range [0, f{self.n_k}).")

            # internally, a port is an pipe. This is different from what we
            # expose to the user: in LaS specification, a port is a cube and
            # associated with a direction, e.g., cube [i,j,k] and direction
            # "-K". This means the port should be the pipe connecting (i,j,k)
            # downwards to the volume of LaS. Thus, that pipe is (i,j,k-1) --
            # (i,j,k) which by convention is the K-pipe (i,j,k-1).
            # a port here has fields "i", "j", "k", "d", "e", "f", "c"
            my_port = {}

            # "i", "j", and "k" are the i,j,k of the pipe
            my_port["i"], my_port["j"], my_port["k"] = port["location"]
            if port["direction"][0] == "-":
                my_port[port["direction"][1].lower()] -= 1

            # "d" is one of I, J, and K, corresponding to the port being an
            #     I-pipe, J-pipe, or a K-pipe
            my_port["d"] = port["direction"][1]

            # "e" is the end of the pipe that is open. For example, if the port
            #     is a K-pipe (i,j,k), then "e"="+" means the cube (i,j,k+1) is
            #     open; otherwise, "e"="-" means cube (i,j,k) is open.
            my_port["e"] = "-" if port["direction"][0] == "+" else "+"

            # "c" is the color variable of the pipe corresponding to the port
            z_dir = port["z_basis_direction"]
            if z_dir not in ["I", "J", "K"] or z_dir == my_port["d"]:
                raise ValueError(
                    f"port with invalid Z basis direction {port}.")
            if my_port["d"] == "I":
                my_port["c"] = 0 if z_dir == "J" else 1
            if my_port["d"] == "J":
                my_port["c"] = 0 if z_dir == "K" else 1
            if my_port["d"] == "K":
                my_port["c"] = 0 if z_dir == "I" else 1

            # "f" is the function of the pipe, e.g., it can say this port is a
            #     T injection. This field is not used in the SAT synthesis, but
            #     we keep this info to use in later stages like gltf generation
            if "function" in port:
                my_port["f"] = port["function"]

            self.ports.append(my_port)

        # from paulistrings to correlation surfaces
        self.stabs = self.derive_corr_boundary(self.paulistrings)

        self.optional = {}
        self.forbidden_cubes = []
        if "optional" in data:
            self.optional = data["optional"]

            if "forbidden_cubes" in data["optional"]:
                for cube in data["optional"]["forbidden_cubes"]:
                    if len(cube) != 3:
                        raise ValueError(
                            f"forbid cube should be 3-tuple {cube}.")
                    if (cube[0] not in range(self.n_i)
                            or cube[1] not in range(self.n_j)
                            or cube[2] not in range(self.n_k)):
                        raise ValueError(
                            f"forbidden {cube} out of range "
                            f"(i,j,k) < ({self.n_i, self.n_j, self.n_k})")
                    self.forbidden_cubes.append(cube)

        self.get_port_cubes()

    def get_port_cubes(self) -> None:
        """calculate which cubes are the open cube for the ports.
        Note that these are *** 3-tuples ***, not lists with 3 elements."""
        self.port_cubes = []
        for p in self.ports:
            # if e=-, (i,j,k); otherwise, +1 in the proper direction
            if p["e"] == "-":
                self.port_cubes.append((p["i"], p["j"], p["k"]))
            elif p["d"] == "I":
                self.port_cubes.append((p["i"] + 1, p["j"], p["k"]))
            elif p["d"] == "J":
                self.port_cubes.append((p["i"], p["j"] + 1, p["k"]))
            elif p["d"] == "K":
                self.port_cubes.append((p["i"], p["j"], p["k"] + 1))

    def derive_corr_boundary(
            self, paulistrings: Sequence[str]
    ) -> Sequence[Sequence[Mapping[str, int]]]:
        """derive the boundary correlation surface variable values.

        From the color orientation of the ports and the stabilizers, we can
        derive which correlation surface variables evaluates to True and which
        to False at the ports for each stabilizer.

        Args:
            paulistrings (Sequence[str]): stabilizers as a list of Paulistrings

        Returns:
            Sequence[Sequence[Mapping[str, int]]]: Outer layer list is the
            list of stabilizers. Inner layer list is the situation at each port
            for one specifeic stabilizer. Each port is specified with a
            dictionary of 2 bits for the 2 correaltion surfaces.
        """
        stabs = []
        for paulistring in paulistrings:
            corr = []
            for p in range(self.n_p):
                if paulistring[p] == "I":
                    # I -> no corr surf should be present
                    if self.ports[p]["d"] == "I":
                        corr.append({"IJ": 0, "IK": 0})
                    if self.ports[p]["d"] == "J":
                        corr.append({"JI": 0, "JK": 0})
                    if self.ports[p]["d"] == "K":
                        corr.append({"KI": 0, "KJ": 0})

                if paulistring[p] == "Y":
                    # Y -> both corr surf should be present
                    if self.ports[p]["d"] == "I":
                        corr.append({"IJ": 1, "IK": 1})
                    if self.ports[p]["d"] == "J":
                        corr.append({"JI": 1, "JK": 1})
                    if self.ports[p]["d"] == "K":
                        corr.append({"KI": 1, "KJ": 1})

                if paulistring[p] == "X":
                    # X -> only corr surf touching red faces
                    if self.ports[p]["d"] == "I":
                        if self.ports[p]["c"]:
                            corr.append({"IJ": 1, "IK": 0})
                        else:
                            corr.append({"IJ": 0, "IK": 1})
                    if self.ports[p]["d"] == "J":
                        if self.ports[p]["c"]:
                            corr.append({"JI": 0, "JK": 1})
                        else:
                            corr.append({"JI": 1, "JK": 0})
                    if self.ports[p]["d"] == "K":
                        if self.ports[p]["c"]:
                            corr.append({"KI": 1, "KJ": 0})
                        else:
                            corr.append({"KI": 0, "KJ": 1})

                if paulistring[p] == "Z":
                    # Z -> only corr surf touching blue faces
                    if self.ports[p]["d"] == "I":
                        if not self.ports[p]["c"]:
                            corr.append({"IJ": 1, "IK": 0})
                        else:
                            corr.append({"IJ": 0, "IK": 1})
                    if self.ports[p]["d"] == "J":
                        if not self.ports[p]["c"]:
                            corr.append({"JI": 0, "JK": 1})
                        else:
                            corr.append({"JI": 1, "JK": 0})
                    if self.ports[p]["d"] == "K":
                        if not self.ports[p]["c"]:
                            corr.append({"KI": 1, "KJ": 0})
                        else:
                            corr.append({"KI": 0, "KJ": 1})
            stabs.append(corr)
        return stabs

    def build_smt_model(
        self,
        given_arrs: Optional[Mapping[str, Any]] = None,
        given_vals: Optional[Sequence[Mapping[str, Any]]] = None,
    ) -> None:
        """build the SMT model with variables and constraints.

            Args:
                given_arrs (Mapping[str, Any], optional):
                    Arrays of values to plug in. Defaults to None.
                given_vals (Sequence[Mapping[str, Any]], optional):
                    Values to plug in. Defaults to None. These values will
                    replace existing values if already set by given_arrs.
            """
        self.define_vars()
        if given_arrs is not None:
            self.plugin_arrs(given_arrs)
        if given_vals is not None:
            self.plugin_vals(given_vals)

        # baseline order of constraint sets, '...' menas name in the paper

        # validity constraints that directly set variables values
        self.constraint_forbid_cube()
        self.constraint_port()  # 'no fanouts'
        self.constraint_connect_outside()  # 'no unexpected ports'

        # more complex validity constraints involving boolean logic
        self.constraint_timelike_y()  # 'time-like Y cubes'
        self.constraint_no_deg1()  # 'no degree-1 non-Y cubes'
        if self.color_ij:
            # 'matching colors at passthroughs' and '... at turns'
            self.constraint_ij_color()
        self.constraint_3d_corner()  # 'no 3D corners'

        # simpler functionality constraints
        self.constraint_corr_ports()  # 'stabilizer as boundary conditions'
        self.constraint_corr_y()  # 'both or non at Y cubes'

        # more complex functionality constraints
        # 'all or no orthogonal surfaces at non-Y cubes:
        self.constraint_corr_perp()
        # 'even parity of parallel surfaces at non-Y cubes':
        self.constraint_corr_para()

    def define_vars(self) -> None:
        """define the variables in Z3 into self.vars."""
        self.vars = {
            "ExistI":
            [[[z3.Bool(f"ExistI({i},{j},{k})") for k in range(self.n_k)]
              for j in range(self.n_j)] for i in range(self.n_i)],
            "ExistJ":
            [[[z3.Bool(f"ExistJ({i},{j},{k})") for k in range(self.n_k)]
              for j in range(self.n_j)] for i in range(self.n_i)],
            "ExistK":
            [[[z3.Bool(f"ExistK({i},{j},{k})") for k in range(self.n_k)]
              for j in range(self.n_j)] for i in range(self.n_i)],
            "NodeY":
            [[[z3.Bool(f"NodeY({i},{j},{k})") for k in range(self.n_k)]
              for j in range(self.n_j)] for i in range(self.n_i)],
            "CorrIJ":
            [[[[z3.Bool(f"CorrIJ({s},{i},{j},{k})") for k in range(self.n_k)]
               for j in range(self.n_j)] for i in range(self.n_i)]
             for s in range(self.n_s)],
            "CorrIK":
            [[[[z3.Bool(f"CorrIK({s},{i},{j},{k})") for k in range(self.n_k)]
               for j in range(self.n_j)] for i in range(self.n_i)]
             for s in range(self.n_s)],
            "CorrJK":
            [[[[z3.Bool(f"CorrJK({s},{i},{j},{k})") for k in range(self.n_k)]
               for j in range(self.n_j)] for i in range(self.n_i)]
             for s in range(self.n_s)],
            "CorrJI":
            [[[[z3.Bool(f"CorrJI({s},{i},{j},{k})") for k in range(self.n_k)]
               for j in range(self.n_j)] for i in range(self.n_i)]
             for s in range(self.n_s)],
            "CorrKI":
            [[[[z3.Bool(f"CorrKI({s},{i},{j},{k})") for k in range(self.n_k)]
               for j in range(self.n_j)] for i in range(self.n_i)]
             for s in range(self.n_s)],
            "CorrKJ":
            [[[[z3.Bool(f"CorrKJ({s},{i},{j},{k})") for k in range(self.n_k)]
               for j in range(self.n_j)] for i in range(self.n_i)]
             for s in range(self.n_s)],
        }

        if self.color_ij:
            self.vars["ColorI"] = [[[
                z3.Bool(f"ColorI({i},{j},{k})") for k in range(self.n_k)
            ] for j in range(self.n_j)] for i in range(self.n_i)]
            self.vars["ColorJ"] = [[[
                z3.Bool(f"ColorJ({i},{j},{k})") for k in range(self.n_k)
            ] for j in range(self.n_j)] for i in range(self.n_i)]

    def plugin_arrs(self, data: Mapping[str, Any]) -> None:
        """plug in the given arrays of values.

        Args:
            data (Mapping[str, Any]): contains gieven values.

        Raises:
            ValueError: data contains an invalid array name.
            ValueError: array given has wrong dimensions.
        """

        for key in data:
            if key in [
                    "NodeY",
                    "ExistI",
                    "ExistJ",
                    "ExistK",
                    "ColorI",
                    "ColorJ",
            ]:
                if len(data[key]) != self.n_i:
                    raise ValueError(f"dimension of {key} is wrong.")
                for tmp in data[key]:
                    if len(tmp) != self.n_j:
                        raise ValueError(f"dimension of {key} is wrong.")
                    for tmptmp in tmp:
                        if len(tmptmp) != self.n_k:
                            raise ValueError(f"dimension of {key} is wrong.")
            elif key in [
                    "CorrIJ",
                    "CorrIK",
                    "CorrJI",
                    "CorrJK",
                    "CorrKI",
                    "CorrKJ",
            ]:
                if len(data[key]) != self.n_s:
                    raise ValueError(f"dimension of {key} is wrong.")
                for tmp in data[key]:
                    if len(tmp) != self.n_i:
                        raise ValueError(f"dimension of {key} is wrong.")
                    for tmptmp in tmp:
                        if len(tmptmp) != self.n_j:
                            raise ValueError(f"dimension of {key} is wrong.")
                        for tmptmptmp in tmptmp:
                            if len(tmptmptmp) != self.n_k:
                                raise ValueError(
                                    f"dimension of {key} is wrong.")
            else:
                raise ValueError(f"{key} is not a valid array name")

        arrs = [
            "NodeY",
            "ExistI",
            "ExistJ",
            "ExistK",
        ]
        if self.color_ij:
            arrs += ["ColorI", "ColorJ"]

        for s in range(self.n_s):
            for i in range(self.n_i):
                for j in range(self.n_j):
                    for k in range(self.n_k):
                        if s == 0:  # Exist, Node, and Color vars
                            for arr in arrs:
                                if var_given(data, arr, i, j, k):
                                    self.goal.add(
                                        self.vars[arr][i][j][k]
                                        if data[arr][i][j][k] ==
                                        1 else z3.Not(self.vars[arr][i][j][k]))
                        # Corr vars
                        for arr in [
                                "CorrIJ",
                                "CorrIK",
                                "CorrJI",
                                "CorrJK",
                                "CorrKI",
                                "CorrKJ",
                        ]:
                            if var_given(data, arr, s, i, j, k):
                                self.goal.add(
                                    self.vars[arr][s][i][j][k]
                                    if data[arr][s][i][j][k] ==
                                    1 else z3.Not(self.vars[arr][s][i][j][k]))

    def plugin_vals(self, data_set: Sequence[Mapping[str, Any]]):
        """plug in the given values

        Args:
            data (Sequence[Mapping[str, Any]]): given values as a sequence
            of dicts. Each one contains three fields: "array", the name of
            the array, e.g., "ExistI"; "indices", a sequence of the indices;
            and "value".

        Raises:
            ValueError: given_vals missing a field.
            ValueError: array name is not valid.
            ValueError: indices dimension for certain array is wrong.
            ValueError: index value out of bound.
            ValueError: given value is neither 0 nor 1.
        """
        for data in data_set:
            for key in ["array", "indices", "value"]:
                if key not in data:
                    raise ValueError(f"{key} is not in given val")
            if data["array"] not in [
                    "NodeY",
                    "ExistI",
                    "ExistJ",
                    "ExistK",
                    "ColorI",
                    "ColorJ",
                    "CorrIJ",
                    "CorrIK",
                    "CorrJI",
                    "CorrJK",
                    "CorrKI",
                    "CorrKJ",
            ]:
                raise ValueError(f"{data['array']} is not a valid array.")
            if data["array"] in [
                    "NodeY",
                    "ExistI",
                    "ExistJ",
                    "ExistK",
                    "ColorI",
                    "ColorJ",
            ]:
                if len(data["indices"] != 3):
                    raise ValueError(f"Need 3 indices for {data['array']}.")
                if data["indices"][0] not in range(self.n_i):
                    raise ValueError(f"i index out of range")
                if data["indices"][1] not in range(self.n_j):
                    raise ValueError(f"j index out of range")
                if data["indices"][2] not in range(self.n_k):
                    raise ValueError(f"k index out of range")

            if data["array"] in [
                    "CorrIJ",
                    "CorrIK",
                    "CorrJI",
                    "CorrJK",
                    "CorrKI",
                    "CorrKJ",
            ]:
                if len(data["indices"] != 4):
                    raise ValueError(f"Need 4 indices for {data['array']}.")
                if data["indices"][0] not in range(self.n_s):
                    raise ValueError(f"s index out of range")
                if data["indices"][1] not in range(self.n_i):
                    raise ValueError(f"i index out of range")
                if data["indices"][2] not in range(self.n_j):
                    raise ValueError(f"j index out of range")
                if data["indices"][3] not in range(self.n_k):
                    raise ValueError(f"k index out of range")

            if data["value"] not in [0, 1]:
                raise ValueError("Given value can only be 0 or 1.")

            (arr, idx) = data["array"], data["indices"]
            if arr.startswith("Corr"):
                s, i, j, k = idx
                if data["value"] == 1:
                    self.goal.add(self.vars[arr][s][i][j][k])
                else:
                    self.goal.add(z3.Not(self.vars[arr][s][i][j][k]))
            else:
                i, j, k = idx
                if data["value"] == 1:
                    self.goal.add(self.vars[arr][i][j][k])
                else:
                    self.goal.add(z3.Not(self.vars[arr][i][j][k]))

    def constraint_forbid_cube(self) -> None:
        """forbid a list of cubes."""
        for cube in self.forbidden_cubes:
            (i, j, k) = cube[0], cube[1], cube[2]
            self.goal.add(z3.Not(self.vars["NodeY"][i][j][k]))
            if i > 0:
                self.goal.add(z3.Not(self.vars["ExistI"][i - 1][j][k]))
            self.goal.add(z3.Not(self.vars["ExistI"][i][j][k]))
            if j > 0:
                self.goal.add(z3.Not(self.vars["ExistJ"][i][j - 1][k]))
            self.goal.add(z3.Not(self.vars["ExistJ"][i][j][k]))
            if k > 0:
                self.goal.add(z3.Not(self.vars["ExistK"][i][j][k - 1]))
            self.goal.add(z3.Not(self.vars["ExistK"][i][j][k]))

    def constraint_port(self) -> None:
        """some pipes must exist and some must not depending on the ports."""
        for port in self.ports:
            # the pipe specified by the port exists
            self.goal.add(self.vars[f"Exist{port['d']}"][port["i"]][port["j"]][
                port["k"]])
            # if I- or J-pipe exist, set the color value too to the given one
            if self.color_ij:
                if port["d"] != "K":
                    if port["c"] == 1:
                        self.goal.add(self.vars[f"Color{port['d']}"][port["i"]]
                                      [port["j"]][port["k"]])
                    else:
                        self.goal.add(
                            z3.Not(self.vars[f"Color{port['d']}"][port["i"]][
                                port["j"]][port["k"]]))

            # collect the pipes touching the port to forbid them
            dirs, coords = port_incident_pipes(port, self.n_i, self.n_j,
                                               self.n_k)
            for i, coord in enumerate(coords):
                self.goal.add(
                    z3.Not(self.vars[f"Exist{dirs[i]}"][coord[0]][coord[1]][
                        coord[2]]))

    def constraint_connect_outside(self) -> None:
        """no pipe should cross the spatial bound except for ports."""
        for i in range(self.n_i):
            for j in range(self.n_j):
                # consider K-pipes crossing K-bound and not a port
                if (i, j, self.n_k) not in self.port_cubes:
                    self.goal.add(
                        z3.Not(self.vars["ExistK"][i][j][self.n_k - 1]))
        for i in range(self.n_i):
            for k in range(self.n_k):
                if (i, self.n_j, k) not in self.port_cubes:
                    self.goal.add(
                        z3.Not(self.vars["ExistJ"][i][self.n_j - 1][k]))
        for j in range(self.n_j):
            for k in range(self.n_k):
                if (self.n_i, j, k) not in self.port_cubes:
                    self.goal.add(
                        z3.Not(self.vars["ExistI"][self.n_i - 1][j][k]))

    def constraint_timelike_y(self) -> None:
        """forbid all I- and J- pipes to Y cubes."""
        for i in range(self.n_i):
            for j in range(self.n_j):
                for k in range(self.n_k):
                    if (i, j, k) not in self.port_cubes:
                        self.goal.add(
                            z3.Implies(
                                self.vars["NodeY"][i][j][k],
                                z3.Not(self.vars["ExistI"][i][j][k]),
                            ))
                        self.goal.add(
                            z3.Implies(
                                self.vars["NodeY"][i][j][k],
                                z3.Not(self.vars["ExistJ"][i][j][k]),
                            ))
                        if i - 1 >= 0:
                            self.goal.add(
                                z3.Implies(
                                    self.vars["NodeY"][i][j][k],
                                    z3.Not(self.vars["ExistI"][i - 1][j][k]),
                                ))
                        if j - 1 >= 0:
                            self.goal.add(
                                z3.Implies(
                                    self.vars["NodeY"][i][j][k],
                                    z3.Not(self.vars["ExistJ"][i][j - 1][k]),
                                ))

    def constraint_ij_color(self) -> None:
        """color matching for I- and J-pipes."""
        for i in range(self.n_i):
            for j in range(self.n_j):
                for k in range(self.n_k):
                    if i >= 1 and j >= 1:
                        # (i-1,j,k)-(i,j,k) and (i,j-1,k)-(i,j,k)
                        self.goal.add(
                            z3.Implies(
                                z3.And(
                                    self.vars["ExistI"][i - 1][j][k],
                                    self.vars["ExistJ"][i][j - 1][k],
                                ),
                                z3.Or(
                                    z3.And(
                                        self.vars["ColorI"][i - 1][j][k],
                                        z3.Not(self.vars["ColorJ"][i][j -
                                                                      1][k]),
                                    ),
                                    z3.And(
                                        z3.Not(self.vars["ColorI"][i -
                                                                   1][j][k]),
                                        self.vars["ColorJ"][i][j - 1][k],
                                    ),
                                ),
                            ))

                    if i >= 1:
                        # (i-1,j,k)-(i,j,k) and (i,j,k)-(i,j+1,k)
                        self.goal.add(
                            z3.Implies(
                                z3.And(
                                    self.vars["ExistI"][i - 1][j][k],
                                    self.vars["ExistJ"][i][j][k],
                                ),
                                z3.Or(
                                    z3.And(
                                        self.vars["ColorI"][i - 1][j][k],
                                        z3.Not(self.vars["ColorJ"][i][j][k]),
                                    ),
                                    z3.And(
                                        z3.Not(self.vars["ColorI"][i -
                                                                   1][j][k]),
                                        self.vars["ColorJ"][i][j][k],
                                    ),
                                ),
                            ))
                        # (i-1,j,k)-(i,j,k) and (i,j,k)-(i+1,j,k)
                        self.goal.add(
                            z3.Implies(
                                z3.And(
                                    self.vars["ExistI"][i - 1][j][k],
                                    self.vars["ExistI"][i][j][k],
                                ),
                                z3.Or(
                                    z3.And(
                                        self.vars["ColorI"][i - 1][j][k],
                                        self.vars["ColorI"][i][j][k],
                                    ),
                                    z3.And(
                                        z3.Not(self.vars["ColorI"][i -
                                                                   1][j][k]),
                                        z3.Not(self.vars["ColorI"][i][j][k]),
                                    ),
                                ),
                            ))

                    if j >= 1:
                        # (i,j,k)-(i+1,j,k) and (i,j-1,k)-(i,j,k)
                        self.goal.add(
                            z3.Implies(
                                z3.And(
                                    self.vars["ExistI"][i][j][k],
                                    self.vars["ExistJ"][i][j - 1][k],
                                ),
                                z3.Or(
                                    z3.And(
                                        self.vars["ColorI"][i][j][k],
                                        z3.Not(self.vars["ColorJ"][i][j -
                                                                      1][k]),
                                    ),
                                    z3.And(
                                        z3.Not(self.vars["ColorI"][i][j][k]),
                                        self.vars["ColorJ"][i][j - 1][k],
                                    ),
                                ),
                            ))
                        # (i,j-1,k)-(i,j,k) and (i,j,k)-(i,j+1,k)
                        self.goal.add(
                            z3.Implies(
                                z3.And(
                                    self.vars["ExistJ"][i][j - 1][k],
                                    self.vars["ExistJ"][i][j][k],
                                ),
                                z3.Or(
                                    z3.And(
                                        self.vars["ColorJ"][i][j - 1][k],
                                        self.vars["ColorJ"][i][j][k],
                                    ),
                                    z3.And(
                                        z3.Not(self.vars["ColorJ"][i][j -
                                                                      1][k]),
                                        z3.Not(self.vars["ColorJ"][i][j][k]),
                                    ),
                                ),
                            ))

                    # (i,j,k)-(i+1,j,k) and (i,j,k)-(i,j+1,k)
                    self.goal.add(
                        z3.Implies(
                            z3.And(self.vars["ExistI"][i][j][k],
                                   self.vars["ExistJ"][i][j][k]),
                            z3.Or(
                                z3.And(
                                    self.vars["ColorI"][i][j][k],
                                    z3.Not(self.vars["ColorJ"][i][j][k]),
                                ),
                                z3.And(
                                    z3.Not(self.vars["ColorI"][i][j][k]),
                                    self.vars["ColorJ"][i][j][k],
                                ),
                            ),
                        ))

    def constraint_3d_corner(self) -> None:
        """at least in one direction, both pipes nonexist."""
        for i in range(self.n_i):
            for j in range(self.n_j):
                for k in range(self.n_k):
                    i_pipes = [
                        self.vars["ExistI"][i][j][k],
                    ]
                    if i - 1 >= 0:
                        i_pipes.append(self.vars["ExistI"][i - 1][j][k])
                    j_pipes = [
                        self.vars["ExistJ"][i][j][k],
                    ]
                    if j - 1 >= 0:
                        j_pipes.append(self.vars["ExistJ"][i][j - 1][k])
                    k_pipes = [
                        self.vars["ExistK"][i][j][k],
                    ]
                    if k - 1 >= 0:
                        k_pipes.append(self.vars["ExistK"][i][j][k - 1])

                    # at least one of the three terms is true. The first term
                    # is that both I-pipes connecting to (i,j,k) do not exist.
                    self.goal.add(
                        z3.Or(
                            z3.Not(z3.Or(i_pipes)),
                            z3.Not(z3.Or(j_pipes)),
                            z3.Not(z3.Or(k_pipes)),
                        ))

    def constraint_no_deg1(self) -> None:
        """forbid degree-1 X or Z cubes by considering incident pipes."""
        for i in range(self.n_i):
            for j in range(self.n_j):
                for k in range(self.n_k):
                    for d in ["I", "J", "K"]:
                        for e in ["-", "+"]:
                            cube = {"I": i, "J": j, "K": k}
                            cube[d] += 1 if e == "+" else 0

                            # construct fake ports to get incident pipes
                            p0 = {
                                "i": i,
                                "j": j,
                                "k": k,
                                "d": d,
                                "e": e,
                                "c": 0
                            }
                            found_p0 = False
                            for port in self.ports:
                                if (i == port["i"] and j == port["j"]
                                        and k == port["k"] and d == port["d"]):
                                    found_p0 = True

                            # only non-port pipes need to consider
                            if (not found_p0 and cube["I"] < self.n_i
                                    and cube["J"] < self.n_j
                                    and cube["K"] < self.n_k):
                                # only cubes inside bound need to consider
                                dirs, coords = port_incident_pipes(
                                    p0, self.n_i, self.n_j, self.n_k)
                                pipes = [
                                    self.vars[f"Exist{dirs[l]}"][coord[0]][
                                        coord[1]][coord[2]]
                                    for l, coord in enumerate(coords)
                                ]
                                # if the cube is not Y and the pipe exist, then
                                # at least one of its incident pipes exists.
                                self.goal.add(
                                    z3.Implies(
                                        z3.And(
                                            z3.Not(
                                                self.vars["NodeY"][cube["I"]][
                                                    cube["J"]][cube["K"]]),
                                            self.vars[f"Exist{d}"][i][j][k],
                                        ),
                                        z3.Or(pipes),
                                    ))

    def constraint_corr_ports(self) -> None:
        """plug in the correlation surface values at the ports."""
        for s, stab in enumerate(self.stabs):
            for p, corrs in enumerate(stab):
                for k, v in corrs.items():
                    if v == 1:
                        self.goal.add(
                            self.vars[f"Corr{k}"][s][self.ports[p]["i"]][
                                self.ports[p]["j"]][self.ports[p]["k"]])
                    else:
                        self.goal.add(
                            z3.Not(self.vars[f"Corr{k}"][s][self.ports[p]["i"]]
                                   [self.ports[p]["j"]][self.ports[p]["k"]]))

    def constraint_corr_y(self) -> None:
        """correlation surfaces at Y-cubes should both exist or nonexist."""
        for s in range(self.n_s):
            for i in range(self.n_i):
                for j in range(self.n_j):
                    for k in range(self.n_k):
                        self.goal.add(
                            z3.Or(
                                z3.Not(self.vars["NodeY"][i][j][k]),
                                z3.Or(
                                    z3.And(
                                        self.vars["CorrKI"][s][i][j][k],
                                        self.vars["CorrKJ"][s][i][j][k],
                                    ),
                                    z3.And(
                                        z3.Not(
                                            self.vars["CorrKI"][s][i][j][k]),
                                        z3.Not(
                                            self.vars["CorrKJ"][s][i][j][k]),
                                    ),
                                ),
                            ))
                        if k - 1 >= 0:
                            self.goal.add(
                                z3.Or(
                                    z3.Not(self.vars["NodeY"][i][j][k]),
                                    z3.Or(
                                        z3.And(
                                            self.vars["CorrKI"][s][i][j][k -
                                                                         1],
                                            self.vars["CorrKJ"][s][i][j][k -
                                                                         1],
                                        ),
                                        z3.And(
                                            z3.Not(self.vars["CorrKI"][s][i][j]
                                                   [k - 1]),
                                            z3.Not(self.vars["CorrKJ"][s][i][j]
                                                   [k - 1]),
                                        ),
                                    ),
                                ))

    def constraint_corr_perp(self) -> None:
        """for corr surf perpendicular to normal vector, all or none exists."""
        for s in range(self.n_s):
            for i in range(self.n_i):
                for j in range(self.n_j):
                    for k in range(self.n_k):
                        if (i, j, k) not in self.port_cubes:
                            # only consider X or Z spider
                            # if normal is K meaning meaning both
                            # (i,j,k)-(i,j,k+1) and (i,j,k)-(i,j,k-1) are
                            # out of range, or in range but nonexistent
                            normal = z3.And(
                                z3.Not(self.vars["NodeY"][i][j][k]),
                                z3.Not(self.vars["ExistK"][i][j][k]),
                            )
                            if k - 1 >= 0:
                                normal = z3.And(
                                    normal,
                                    z3.Not(self.vars["ExistK"][i][j][k - 1]))

                            # for other pipes, we need to build an intermediate
                            # expression for (i,j,k)-(i+1,j,k) and
                            # (i,j,k)-(i,j+1,k), built expression meaning
                            # the pipe is nonexistent or exist and has
                            # the correlation surface perpendicular to
                            # the normal vector in them.
                            no_pipe_or_with_corr = [
                                z3.Or(
                                    z3.Not(self.vars["ExistI"][i][j][k]),
                                    self.vars["CorrIJ"][s][i][j][k],
                                ),
                                z3.Or(
                                    z3.Not(self.vars["ExistJ"][i][j][k]),
                                    self.vars["CorrJI"][s][i][j][k],
                                ),
                            ]

                            # for (i,j,k)-(i+1,j,k) and (i,j,k)-(i,j+1,k),
                            # build expression meaning the pipe is nonexistent
                            # or exist and does not have the correlation
                            # surface perpendicular to the normal vector.
                            no_pipe_or_no_corr = [
                                z3.Or(
                                    z3.Not(self.vars["ExistI"][i][j][k]),
                                    z3.Not(self.vars["CorrIJ"][s][i][j][k]),
                                ),
                                z3.Or(
                                    z3.Not(self.vars["ExistJ"][i][j][k]),
                                    z3.Not(self.vars["CorrJI"][s][i][j][k]),
                                ),
                            ]

                            if i - 1 >= 0:
                                # add (i-1,j,k)-(i,j,k) to the expression
                                no_pipe_or_with_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistI"][i -
                                                                   1][j][k]),
                                        self.vars["CorrIJ"][s][i - 1][j][k],
                                    ))
                                no_pipe_or_no_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistI"][i -
                                                                   1][j][k]),
                                        z3.Not(
                                            self.vars["CorrIJ"][s][i -
                                                                   1][j][k]),
                                    ))

                            if j - 1 >= 0:
                                # add (i,j-1,k)-(i,j,k) to the expression
                                no_pipe_or_with_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistJ"][i][j -
                                                                      1][k]),
                                        self.vars["CorrJI"][s][i][j - 1][k],
                                    ))
                                no_pipe_or_no_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistJ"][i][j -
                                                                      1][k]),
                                        z3.Not(
                                            self.vars["CorrJI"][s][i][j -
                                                                      1][k]),
                                    ))

                            # if normal vector is K, then in all its
                            # incident pipes that exist all correlation surface
                            # in I-J plane exist or nonexist
                            self.goal.add(
                                z3.Implies(
                                    normal,
                                    z3.Or(
                                        z3.And(no_pipe_or_with_corr),
                                        z3.And(no_pipe_or_no_corr),
                                    ),
                                ))

                            # if normal is I
                            normal = z3.And(
                                z3.Not(self.vars["NodeY"][i][j][k]),
                                z3.Not(self.vars["ExistI"][i][j][k]),
                            )
                            if i - 1 >= 0:
                                normal = z3.And(
                                    normal,
                                    z3.Not(self.vars["ExistI"][i - 1][j][k]))
                            no_pipe_or_with_corr = [
                                z3.Or(
                                    z3.Not(self.vars["ExistJ"][i][j][k]),
                                    self.vars["CorrJK"][s][i][j][k],
                                ),
                                z3.Or(
                                    z3.Not(self.vars["ExistK"][i][j][k]),
                                    self.vars["CorrKJ"][s][i][j][k],
                                ),
                            ]
                            no_pipe_or_no_corr = [
                                z3.Or(
                                    z3.Not(self.vars["ExistJ"][i][j][k]),
                                    z3.Not(self.vars["CorrJK"][s][i][j][k]),
                                ),
                                z3.Or(
                                    z3.Not(self.vars["ExistK"][i][j][k]),
                                    z3.Not(self.vars["CorrKJ"][s][i][j][k]),
                                ),
                            ]
                            if j - 1 >= 0:
                                no_pipe_or_with_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistJ"][i][j -
                                                                      1][k]),
                                        self.vars["CorrJK"][s][i][j - 1][k],
                                    ))
                                no_pipe_or_no_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistJ"][i][j -
                                                                      1][k]),
                                        z3.Not(
                                            self.vars["CorrJK"][s][i][j -
                                                                      1][k]),
                                    ))
                            if k - 1 >= 0:
                                no_pipe_or_with_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistK"][i][j][k -
                                                                         1]),
                                        self.vars["CorrKJ"][s][i][j][k - 1],
                                    ))
                                no_pipe_or_no_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistK"][i][j][k -
                                                                         1]),
                                        z3.Not(
                                            self.vars["CorrKJ"][s][i][j][k -
                                                                         1]),
                                    ))
                            self.goal.add(
                                z3.Implies(
                                    normal,
                                    z3.Or(
                                        z3.And(no_pipe_or_with_corr),
                                        z3.And(no_pipe_or_no_corr),
                                    ),
                                ))

                            # if normal is J
                            normal = z3.And(
                                z3.Not(self.vars["NodeY"][i][j][k]),
                                z3.Not(self.vars["ExistJ"][i][j][k]),
                            )
                            if j - 1 >= 0:
                                normal = z3.And(
                                    normal,
                                    z3.Not(self.vars["ExistJ"][i][j - 1][k]))
                            no_pipe_or_with_corr = [
                                z3.Or(
                                    z3.Not(self.vars["ExistI"][i][j][k]),
                                    self.vars["CorrIK"][s][i][j][k],
                                ),
                                z3.Or(
                                    z3.Not(self.vars["ExistK"][i][j][k]),
                                    self.vars["CorrKI"][s][i][j][k],
                                ),
                            ]
                            no_pipe_or_no_corr = [
                                z3.Or(
                                    z3.Not(self.vars["ExistI"][i][j][k]),
                                    z3.Not(self.vars["CorrIK"][s][i][j][k]),
                                ),
                                z3.Or(
                                    z3.Not(self.vars["ExistK"][i][j][k]),
                                    z3.Not(self.vars["CorrKI"][s][i][j][k]),
                                ),
                            ]
                            if i - 1 >= 0:
                                no_pipe_or_with_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistI"][i -
                                                                   1][j][k]),
                                        self.vars["CorrIK"][s][i - 1][j][k],
                                    ))
                                no_pipe_or_no_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistI"][i -
                                                                   1][j][k]),
                                        z3.Not(
                                            self.vars["CorrIK"][s][i -
                                                                   1][j][k]),
                                    ))
                            if k - 1 >= 0:
                                no_pipe_or_with_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistK"][i][j][k -
                                                                         1]),
                                        self.vars["CorrKI"][s][i][j][k - 1],
                                    ))
                                no_pipe_or_no_corr.append(
                                    z3.Or(
                                        z3.Not(self.vars["ExistK"][i][j][k -
                                                                         1]),
                                        z3.Not(
                                            self.vars["CorrKI"][s][i][j][k -
                                                                         1]),
                                    ))
                            self.goal.add(
                                z3.Implies(
                                    normal,
                                    z3.Or(
                                        z3.And(no_pipe_or_with_corr),
                                        z3.And(no_pipe_or_no_corr),
                                    ),
                                ))

    def constraint_corr_para(self) -> None:
        """for corr surf parallel to the normal , even number of them exist."""
        for s in range(self.n_s):
            for i in range(self.n_i):
                for j in range(self.n_j):
                    for k in range(self.n_k):
                        if (i, j, k) not in self.port_cubes:
                            # only X or Z spiders, if normal is K
                            normal = z3.And(
                                z3.Not(self.vars["NodeY"][i][j][k]),
                                z3.Not(self.vars["ExistK"][i][j][k]),
                            )
                            if k - 1 >= 0:
                                normal = z3.And(
                                    normal,
                                    z3.Not(self.vars["ExistK"][i][j][k - 1]))

                            # unlike in constraint_corr_perp, we only care
                            # about the cases where the pipe exists and the
                            # correlation surface parallel to K also is present
                            # so we build intermediate expressions as below
                            pipe_with_corr = [
                                z3.And(
                                    self.vars["ExistI"][i][j][k],
                                    self.vars["CorrIK"][s][i][j][k],
                                ),
                                z3.And(
                                    self.vars["ExistJ"][i][j][k],
                                    self.vars["CorrJK"][s][i][j][k],
                                ),
                            ]

                            # add (i-1,j,k)-(i,j,k) to the expression
                            if i - 1 >= 0:
                                pipe_with_corr.append(
                                    z3.And(
                                        self.vars["ExistI"][i - 1][j][k],
                                        self.vars["CorrIK"][s][i - 1][j][k],
                                    ))

                            # add (i,j-1,k)-(i,j,k) to the expression
                            if j - 1 >= 0:
                                pipe_with_corr.append(
                                    z3.And(
                                        self.vars["ExistJ"][i][j - 1][k],
                                        self.vars["CorrJK"][s][i][j - 1][k],
                                    ))

                            # parity of the expressions must be even
                            self.goal.add(
                                z3.Implies(
                                    normal,
                                    cnf_even_parity_upto4(pipe_with_corr)))

                            # if normal is I
                            normal = z3.And(
                                z3.Not(self.vars["NodeY"][i][j][k]),
                                z3.Not(self.vars["ExistI"][i][j][k]),
                            )
                            if i - 1 >= 0:
                                normal = z3.And(
                                    normal,
                                    z3.Not(self.vars["ExistI"][i - 1][j][k]))
                            pipe_with_corr = [
                                z3.And(
                                    self.vars["ExistJ"][i][j][k],
                                    self.vars["CorrJI"][s][i][j][k],
                                ),
                                z3.And(
                                    self.vars["ExistK"][i][j][k],
                                    self.vars["CorrKI"][s][i][j][k],
                                ),
                            ]
                            if j - 1 >= 0:
                                pipe_with_corr.append(
                                    z3.And(
                                        self.vars["ExistJ"][i][j - 1][k],
                                        self.vars["CorrJI"][s][i][j - 1][k],
                                    ))
                            if k - 1 >= 0:
                                pipe_with_corr.append(
                                    z3.And(
                                        self.vars["ExistK"][i][j][k - 1],
                                        self.vars["CorrKI"][s][i][j][k - 1],
                                    ))
                            self.goal.add(
                                z3.Implies(
                                    normal,
                                    cnf_even_parity_upto4(pipe_with_corr)))

                            # if normal is J
                            normal = z3.And(
                                z3.Not(self.vars["NodeY"][i][j][k]),
                                z3.Not(self.vars["ExistJ"][i][j][k]),
                            )
                            if j - 1 >= 0:
                                normal = z3.And(
                                    normal,
                                    z3.Not(self.vars["ExistJ"][i][j - 1][k]))
                            pipe_with_corr = [
                                z3.And(
                                    self.vars["ExistI"][i][j][k],
                                    self.vars["CorrIJ"][s][i][j][k],
                                ),
                                z3.And(
                                    self.vars["ExistK"][i][j][k],
                                    self.vars["CorrKJ"][s][i][j][k],
                                ),
                            ]
                            if i - 1 >= 0:
                                pipe_with_corr.append(
                                    z3.And(
                                        self.vars["ExistI"][i - 1][j][k],
                                        self.vars["CorrIJ"][s][i - 1][j][k],
                                    ))
                            if k - 1 >= 0:
                                pipe_with_corr.append(
                                    z3.And(
                                        self.vars["ExistK"][i][j][k - 1],
                                        self.vars["CorrKJ"][s][i][j][k - 1],
                                    ))
                            self.goal.add(
                                z3.Implies(
                                    normal,
                                    cnf_even_parity_upto4(pipe_with_corr)))

    def check_z3(self, print_progress: bool = True) -> bool:
        """check whether the built goal in self.goal is satisfiable.

        Args:
            print_progress (bool, optional): if print out the progress made.

        Returns:
            bool: True if SAT, False if UNSAT
        """
        if print_progress:
            print("Construct a Z3 SMT model and solve...")
        start_time = time.time()
        self.solver = z3.Solver()
        self.solver.add(self.goal)
        ifsat = self.solver.check()
        elapsed = time.time() - start_time
        if print_progress:
            print("elapsed time: {:2f}s".format(elapsed))
        if ifsat == z3.sat:
            if print_progress:
                print("Z3 SAT")
            return True
        else:
            if print_progress:
                print("Z3 UNSAT")
            return False

    def check_kissat(
        self,
        kissat_dir: str,
        dimacs_file_name: Optional[str] = None,
        sat_log_file_name: Optional[str] = None,
        print_progress: bool = True,
    ) -> bool:
        """check whether there is a solution with Kissat

        Args:
            kissat_dir (str): directory containing an executable named kissat
            dimacs_file_name (str, optional): Defaults to None. Then, the
                dimacs file is in a tmp directory. If specified, the dimacs
                will be saved to that path.
            sat_log_file_name (str, optional): Defaults to None. Then, the
                sat log file is in a tmp directory. If specified, the sat log
                will be saved to that path.
            print_progress (bool, optional): whether print the SAT solving
                process on screen. Defaults to True.

        Raises:
            ValueError: kissat_dir is not a directory
            ValueError: there is no executable kissat in kissat_dir
            ValueError: the return code to kissat is neither SAT nor UNSAT

        Returns:
            bool: True if SAT, False if UNSAT
        """
        if not os.path.isdir(kissat_dir):
            raise ValueError(f"{kissat_dir} is not a directory.")
        if kissat_dir.endswith("/"):
            solver_cmd = kissat_dir + "kissat"
        else:
            solver_cmd = kissat_dir + "/kissat"
        if not os.path.isfile(solver_cmd):
            raise ValueError(f"There is no kissat in {kissat_dir}.")

        if_solved = False
        with tempfile.TemporaryDirectory() as tmp_dir:
            # open a tmp directory as workspace

            # dimacs and sat log are either in the tmp dir, or as user specify
            tmp_dimacs_file_name = (dimacs_file_name + ".dimacs" if dimacs_file_name else
                                    tmp_dir + "/tmp.dimacs")
            tmp_sat_log_file_name = (sat_log_file_name + ".kissat" if sat_log_file_name
                                     else tmp_dir + "/tmp.sat")

            if self.write_cnf(tmp_dimacs_file_name,
                              print_progress=print_progress):
                # continue if the CNF is non-trivial, i.e., write_cnf is True
                kissat_start_time = time.time()

                with open(tmp_sat_log_file_name, "w") as log:
                    # use tmp_sat_log_file_name to record stdout of kissat

                    kissat_return_code = -100
                    # -100 if the return code is not updated later on.

                    import random
                    with subprocess.Popen(
                        [
                            solver_cmd, f'--seed={random.randrange(1000000)}',
                            tmp_dimacs_file_name
                        ],
                            stdout=subprocess.PIPE,
                            bufsize=1,
                            universal_newlines=True,
                    ) as run_kissat:
                        for line in run_kissat.stdout:
                            log.write(line)
                            if print_progress:
                                sys.stdout.write(line)
                        get_return_code = run_kissat.communicate()[0]
                        kissat_return_code = run_kissat.returncode

                if kissat_return_code == 10:
                    # 10 means SAT in Kissat
                    if_solved = True
                    if print_progress:
                        print(
                            f"kissat runtime: {time.time()-kissat_start_time}")
                        print("kissat SAT!")

                    # we read the Kissat solution from the SAT log, then, plug
                    # those into the Z3 model and solved inside Z3 again.
                    # The reason is that Z3 did some simplification of the
                    # problem so not every variable appear in the DIMACS given
                    # to Kissat. We still need to know their value.
                    result = self.read_kissat_result(
                        tmp_dimacs_file_name,
                        tmp_sat_log_file_name,
                    )
                    self.plugin_arrs(result)
                    self.check_z3(print_progress)

                elif kissat_return_code == 20:
                    if print_progress:
                        print(f"{solver_cmd} UNSAT")

                elif kissat_return_code == -100:
                    print("Did not get Kissat return code.")

                else:
                    raise ValueError(
                        f"Kissat return code {kissat_return_code} is neither"
                        " SAT nor UNSAT. Maybe you should add print_process="
                        "True to enable the Kissat printout message to see "
                        "what is going on.")
        # closing the tmp directory, the files and itself are removed

        return if_solved

    def write_cnf(self,
                  output_file_name: str,
                  print_progress: bool = False) -> bool:
        """generate CNF for the problem.

        Args:
            output_file_name (str): file to write CNF.

        Returns:
            bool: False if the CNF is trivial, True otherwise
        """
        cnf_start_time = time.time()
        simplified = z3.Tactic("simplify")(self.goal)[0]
        simplified = z3.Tactic("propagate-values")(simplified)[0]
        cnf = z3.Tactic("tseitin-cnf")(simplified)[0]
        dimacs = cnf.dimacs()
        if print_progress:
            print(f"CNF generation time: {time.time() - cnf_start_time}")

        with open(output_file_name, "w") as output_f:
            output_f.write(cnf.dimacs())
        if dimacs.startswith("p cnf 1 1"):
            print("Generated CNF is trivial meaning z3 concludes the instance"
                  " UNSAT during simplification.")
            return False
        else:
            return True

    def read_kissat_result(self, dimacs_file: str,
                           result_file: str) -> Mapping[str, Any]:
        """read result from external SAT solver

        Args:
            dimacs_file (str):
            result_file (str): log from Kissat containing SAT assignments

        Raises:
            ValueError: in the dimacs file, the last lines are comments that
                records the mapping from SAT variable indices to the variable
                names in Z3. If the coordinates in this name is incorrect.

        Returns:
            Mapping[str, Any]: variable assignment in arrays. All the one with
                a corresponding SAT variable are read off from the SAT log.
                The others are left with -1.
        """
        results = {
            "ExistI": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                       for _ in range(self.n_i)],
            "ExistJ": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                       for _ in range(self.n_i)],
            "ExistK": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                       for _ in range(self.n_i)],
            "ColorI": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                       for _ in range(self.n_i)],
            "ColorJ": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                       for _ in range(self.n_i)],
            "NodeY": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                      for _ in range(self.n_i)],
            "CorrIJ": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
            "CorrIK": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
            "CorrJK": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
            "CorrJI": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
            "CorrKI": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
            "CorrKJ": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
        }

        # in this file, the assigments are lines starting with "v" like
        # v -1 -2 -3 -4 -5 -6 -7 -8 -9 -10 -11 -12 ...
        # the vars starts from 1 and - means it's False; otherwise, it's True
        # we scan through all these lines, and save the assignments to `sat`
        with open(result_file, "r") as f:
            sat_output = f.readlines()
        sat = {}
        for line in sat_output:
            if line.startswith("v"):
                assignments = line[1:].strip().split(" ")
                for assignment in assignments:
                    tmp = int(assignment)
                    if tmp < 0:
                        sat[str(-tmp)] = 0
                    elif tmp > 0:
                        sat[str(tmp)] = 1

        # in the dimacs generated by Z3, there are lines starting with "c" like
        # c 8804 CorrIJ(1,0,3,4)    or    c 60053 k!44404
        # which records the mapping from our variables to variables in dimacs
        # the ones starting with k! are added in the translation, we don"t care
        with open(dimacs_file, "r") as f:
            dimacs = f.readlines()
        for line in dimacs:
            if line.startswith("c"):
                _, index, name = line.strip().split(" ")
                if name.startswith((
                        "NodeY",
                        "ExistI",
                        "ExistJ",
                        "ExistK",
                        "ColorI",
                        "ColorJ",
                        "CorrIJ",
                        "CorrIK",
                        "CorrJI",
                        "CorrJK",
                        "CorrKI",
                        "CorrKJ",
                )):
                    arr, tmp = name[:-1].split("(")
                    coords = [int(num) for num in tmp.split(",")]
                    if len(coords) == 3:
                        results[arr][coords[0]][coords[1]][
                            coords[2]] = sat[index]
                    elif len(coords) == 4:
                        results[arr][coords[0]][coords[1]][coords[2]][
                            coords[3]] = sat[index]
                    else:
                        raise ValueError("number of coord should be 3 or 4!")

        return results

    def get_result(self) -> Mapping[str, Any]:
        """get the variable values.

            Returns:
                Mapping[str, Any]: output in the LaSRe format
        """
        model = self.solver.model()
        data = {
            "n_i":
            self.n_i,
            "n_j":
            self.n_j,
            "n_k":
            self.n_k,
            "n_p":
            self.n_p,
            "n_s":
            self.n_s,
            "ports":
            self.ports,
            "stabs":
            self.stabs,
            "port_cubes":
            self.port_cubes,
            "optional":
            self.optional,
            "ExistI": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                       for _ in range(self.n_i)],
            "ExistJ": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                       for _ in range(self.n_i)],
            "ExistK": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                       for _ in range(self.n_i)],
            "ColorI": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                       for _ in range(self.n_i)],
            "ColorJ": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                       for _ in range(self.n_i)],
            "NodeY": [[[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                      for _ in range(self.n_i)],
            "CorrIJ": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
            "CorrIK": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
            "CorrJK": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
            "CorrJI": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
            "CorrKI": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
            "CorrKJ": [[[[-1 for _ in range(self.n_k)]
                         for _ in range(self.n_j)] for _ in range(self.n_i)]
                       for _ in range(self.n_s)],
        }
        arrs = [
            "NodeY",
            "ExistI",
            "ExistJ",
            "ExistK",
        ]
        if self.color_ij:
            arrs += [
                "ColorI",
                "ColorJ",
            ]
        for s in range(self.n_s):
            for i in range(self.n_i):
                for j in range(self.n_j):
                    for k in range(self.n_k):
                        if s == 0:  # Exist, Node, and Color vars
                            for arr in arrs:
                                data[arr][i][j][k] = (
                                    1 if model[self.vars[arr][i][j][k]] else 0)

                        # Corr vars
                        for arr in [
                                "CorrIJ",
                                "CorrIK",
                                "CorrJI",
                                "CorrJK",
                                "CorrKI",
                                "CorrKJ",
                        ]:
                            data[arr][s][i][j][k] = (
                                1 if model[self.vars[arr][s][i][j][k]] else 0)
        return data
