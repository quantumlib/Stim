"""Two wrapper classes, rewrite passes, and translators."""

import functools
import itertools
import json
import time
import multiprocessing
import random
import networkx
from typing import Any, Literal, Mapping, Optional, Sequence
from lassynth.rewrite_passes.attach_fixups import attach_fixups
from lassynth.rewrite_passes.color_z import color_z
from lassynth.rewrite_passes.remove_unconnected import remove_unconnected
from lassynth.sat_synthesis.lattice_surgery_sat import LatticeSurgerySAT
from lassynth.tools.verify_stabilizers import verify_stabilizers
from lassynth.translators.gltf_generator import gltf_generator
from lassynth.translators.textfig_generator import textfig_generator
from lassynth.translators.zx_grid_graph import ZXGridGraph
from lassynth.translators.networkx_generator import networkx_generator


def check_lasre(lasre: Mapping[str, Any]) -> None:
    """check aspects of LaSRe other than SMT constraints, i.e., data layout."""
    if "n_i" not in lasre:
        raise ValueError(
            f"upper bound of I dimension, `n_i`, is missing in lasre.")
    if lasre["n_i"] <= 0:
        raise ValueError("n_i <= 0.")
    if "n_j" not in lasre:
        raise ValueError(
            f"upper bound of J dimension, `n_j`, is missing in lasre.")
    if lasre["n_j"] <= 0:
        raise ValueError("n_j <= 0.")
    if "n_k" not in lasre:
        raise ValueError(
            f"upper bound of K dimension, `n_k`, is missing in lasre.")
    if lasre["n_k"] <= 0:
        raise ValueError("n_k <= 0.")
    if "n_p" not in lasre:
        raise ValueError(f"number of ports, `n_p`, is missing in lasre.")
    if lasre["n_p"] <= 0:
        raise ValueError("n_p <= 0.")
    if "n_s" not in lasre:
        raise ValueError(f"number of stabilizers, `n_s`, is missing in lasre.")
    if lasre["n_s"] < 0:
        raise ValueError("n_s < 0.")
    if lasre["n_s"] == 0:
        print("no stabilizer!")

    if "ports" not in lasre:
        raise ValueError(f"`ports` is missing in lasre.")
    if len(lasre["ports"]) != lasre["n_p"]:
        raise ValueError("number of ports in `ports` is different from `n_p`.")
    for port in lasre["ports"]:
        if "i" not in port:
            raise ValueError(f"location `i` missing from port {port}.")
        if port["i"] not in range(lasre["n_i"]):
            raise ValueError(f"i out of range in port {port}.")
        if "j" not in port:
            raise ValueError(f"location `j` missing from port {port}.")
        if port["j"] not in range(lasre["n_j"]):
            raise ValueError(f"j out of range in port {port}.")
        if "k" not in port:
            raise ValueError(f"location `k` missing from port {port}.")
        if port["k"] not in range(lasre["n_k"]):
            raise ValueError(f"k out of range in port {port}.")
        if "d" not in port:
            raise ValueError(f"direction `d` missing from port {port}.")
        if port["d"] not in ["I", "J", "K"]:
            raise ValueError(f"direction not I, J, or K in port {port}.")
        if "e" not in port:
            raise ValueError(f"open end `e` missing from port {port}.")
        if port["e"] not in ["-", "+"]:
            raise ValueError(f"open end not - or + in port {port}.")
        if "c" not in port:
            raise ValueError(f"color `c` missing from port {port}.")
        if port["c"] not in [0, 1]:
            raise ValueError(f"color not 0 or 1 in port {port}.")

    if "stabs" not in lasre:
        raise ValueError(f"`stabs` is missing in lasre.")
    if len(lasre["stabs"]) != lasre["n_s"]:
        raise ValueError("number of stabs in `stabs` is different from `n_s`.")
    for stab in lasre["stabs"]:
        if len(stab) != lasre["n_p"]:
            raise ValueError("number of boundary corrsurf is not `n_p`.")
        for i, corrsurf in enumerate(stab):
            for (k, v) in corrsurf.items():
                if lasre["ports"][i]["d"] == "I" and k not in ["IJ", "IK"]:
                    raise ValueError(f"stabs[{i}] key invalid {stab}.")
                if lasre["ports"][i]["d"] == "J" and k not in ["JI", "JK"]:
                    raise ValueError(f"stabs[{i}] key invalid {stab}.")
                if lasre["ports"][i]["d"] == "K" and k not in ["KI", "KJ"]:
                    raise ValueError(f"stabs[{i}] key invalid {stab}.")
                if v not in [0, 1]:
                    raise ValueError(f"stabs[{i}] value not 0 or 1 {stab}.")

    port_cubes = []
    for p in lasre["ports"]:
        # if e=-, (i,j,k); otherwise, +1 in the proper direction
        if p["e"] == "-":
            port_cubes.append((p["i"], p["j"], p["k"]))
        elif p["d"] == "I":
            port_cubes.append((p["i"] + 1, p["j"], p["k"]))
        elif p["d"] == "J":
            port_cubes.append((p["i"], p["j"] + 1, p["k"]))
        elif p["d"] == "K":
            port_cubes.append((p["i"], p["j"], p["k"] + 1))
    lasre["port_cubes"] = port_cubes

    if "optional" not in lasre:
        lasre["optional"] = {}

    for key in [
            "NodeY",
            "ExistI",
            "ExistJ",
            "ExistK",
            "ColorI",
            "ColorJ",
    ]:
        if key not in lasre:
            raise ValueError(f"`{key}` missing from lasre.")
        if len(lasre[key]) != lasre["n_i"]:
            raise ValueError(f"dimension of {key} is wrong.")
        for tmp in lasre[key]:
            if len(tmp) != lasre["n_j"]:
                raise ValueError(f"dimension of {key} is wrong.")
            for tmptmp in tmp:
                if len(tmptmp) != lasre["n_k"]:
                    raise ValueError(f"dimension of {key} is wrong.")

    if lasre["n_s"] > 0:
        for key in [
                "CorrIJ",
                "CorrIK",
                "CorrJI",
                "CorrJK",
                "CorrKI",
                "CorrKJ",
        ]:
            if key not in lasre:
                raise ValueError(f"`{key}` missing from lasre.")
            if len(lasre[key]) != lasre["n_s"]:
                raise ValueError(f"dimension of {key} is wrong.")
            for tmp in lasre[key]:
                if len(tmp) != lasre["n_i"]:
                    raise ValueError(f"dimension of {key} is wrong.")
                for tmptmp in tmp:
                    if len(tmptmp) != lasre["n_j"]:
                        raise ValueError(f"dimension of {key} is wrong.")
                    for tmptmptmp in tmptmp:
                        if len(tmptmptmp) != lasre["n_k"]:
                            raise ValueError(f"dimension of {key} is wrong.")


class LatticeSurgerySolution:
    """A class for the result of synthesizer lattice surgery subroutine. 

    It internally saves an LaSRe (Lattice Surgery Subroutine Representation)
    and we can apply rewrite passes to it, or use translators to derive
    other formats of the LaS solution
    """

    def __init__(
        self,
        lasre: Mapping[str, Any],
    ) -> None:
        """initialization for LatticeSurgerySubroutine

        Args:
            lasre (Mapping[str, Any]): LaSRe
        """
        check_lasre(lasre)
        self.lasre = lasre

    def get_depth(self) -> int:
        """get the depth/height of the LaS in LaSRe.

        Returns:
            int: depth/height of the LaS in LaSRe
        """
        return self.lasre["n_k"]

    def after_removing_disconnected_pieces(self):
        """remove_unconnected."""
        return LatticeSurgerySolution(lasre=remove_unconnected(self.lasre))

    def after_color_k_pipes(self):
        """coloring K pipes."""
        return LatticeSurgerySolution(lasre=color_z(self.lasre))

    def after_default_optimizations(self):
        """default optimizations: remove unconnected, and then color K pipes."""
        solution = LatticeSurgerySolution(lasre=remove_unconnected(self.lasre))
        solution = LatticeSurgerySolution(lasre=color_z(solution.lasre))
        return solution

    def after_t_factory_default_optimizations(self):
        """default optimization for T-factories."""
        solution = LatticeSurgerySolution(lasre=remove_unconnected(self.lasre))
        solution = LatticeSurgerySolution(lasre=color_z(solution.lasre))
        solution = LatticeSurgerySolution(lasre=attach_fixups(solution.lasre))
        return solution

    def save_lasre(self, file_name: str) -> None:
        """save the current LaSRe to a file.

        Args:
            file_name (str): file name including extension to save the LaSRe
        """
        with open(file_name, "w") as f:
            json.dump(self.lasre, f)

    def to_3d_model_gltf(self,
                         output_file_name: str,
                         stabilizer: int = -1,
                         tube_len: float = 2.0,
                         no_color_z: bool = False,
                         attach_axes: bool = False,
                         rm_dir: Optional[str] = None) -> None:
        """generate gltf file (for 3D modelling).

        Args:
            output_file_name (str): file name including extension to save gltf
            stabilizer (int, optional): Defaults to -1 meaning do not draw
                correlation surfaces. If the value is in [0, n_s),
                the correlation surfaces corresponding to that stabilizer
                are drawn and faces in one of the directions are revealed
                to unveil the correlation surfaces.
            tube_len (float, optional): Length of the pipe comapred to
                the cube. Defaults to 2.0.
            no_color_z (bool, optional): Do not color the K pipes.
                Defaults to False.
            attach_axes (bool, optional): attach IJK axes. Defaults to False.
                If attached, the color coding is I->red, J->green, K->blue.
            rm_dir (str, optional): the (+|-)(I|J|K) faces to remove.
                Intended to reveal correlation surfaces. Default to None.
        """
        gltf = gltf_generator(
            self.lasre,
            stabilizer=stabilizer,
            tube_len=tube_len,
            no_color_z=no_color_z,
            attach_axes=attach_axes,
            rm_dir=rm_dir if rm_dir else (":+J" if stabilizer >= 0 else None),
        )
        with open(output_file_name, "w") as f:
            json.dump(gltf, f)

    def to_zigxag_url(
        self,
        io_spec: Optional[Sequence[str]] = None,
    ) -> str:
        """generate a link that leads to a ZigXag figure.

        Args:
            io_spec (Optional[Sequence[str]], optional): Specify whether
            each port is an input or an output. Length must be the same
            with the number of ports. Defaults to None, which means
            all ports are outputs.

        Returns:
            str: the ZigXag link
        """
        zxgridgraph = ZXGridGraph(self.lasre)
        return zxgridgraph.to_zigxag_url(io_spec=io_spec)

    def to_text_diagram(self) -> str:
        """generate the text figure of LaS time slices.

        Returns:
            str: text figure of the LaS
        """
        return textfig_generator(self.lasre)

    def to_networkx_graph(self) -> networkx.Graph:
        """generate a annotated networkx.Graph correponding to the LaS.

        Returns:
            networkx.Graph:
        """
        return networkx_generator(self.lasre)

    def verify_stabilizers_stimzx(self,
                                  specification: Mapping[str, Any],
                                  print_stabilizers: bool = False) -> bool:
        """verify the stabilizer of the LaS.

        Use StimZX to deduce the stabilizers from the annotated networkx graph.
        Then use Stim to ensure that this set of stabilizers and the set of
        stabilizers specified in the input are equivalent.

        Args:
            specification (Mapping[str, Any]): the LaS specification to verify
                the current solution against.
            print_stabilizers (bool, optional): If True, print the two sets of
                stabilizers. Defaults to False.

        Returns:
            bool: True if the two sets are equivalent; otherwise False.
        """
        paulistrings = [
            paulistring.replace(".", "_")
            for paulistring in specification["stabilizers"]
        ]
        return verify_stabilizers(
            paulistrings,
            self.to_networkx_graph(),
            print_stabilizers=print_stabilizers,
        )


class LatticeSurgerySynthesizer:
    """A class to synthesize LaS."""

    def __init__(
        self,
        solver: Literal["kissat", "z3"] = "z3",
        kissat_dir: Optional[str] = None,
    ) -> None:
        """initialize.

        Args:
            solver (Literal["kissat", "z3"], optional): the solver to use.
                Defaults to "z3". "kissat" is recommended.
            kissat_dir (Optional[str], optional): directory of the kissat
                executable. Defaults to None.
        """
        self.solver = solver
        self.kissat_dir = kissat_dir

    def solve(
        self,
        specification: Mapping[str, Any],
        given_arrs: Optional[Mapping[str, Any]] = None,
        given_vals: Optional[Sequence[Mapping[str, Any]]] = None,
        print_detail: bool = False,
        dimacs_file_name: Optional[str] = None,
        sat_log_file_name: Optional[str] = None,
    ) -> Optional[LatticeSurgerySolution]:
        """solve an LaS synthesis problem.

        Args:
            specification (Mapping[str, Any]): the LaS specification to solve.
            given_arrs (Optional[Mapping[str, Any]], optional): given array of
                known values to plug in. Defaults to None.
            given_vals (Optional[Sequence[Mapping[str, Any]]], optional): given
                known values to plug in. Defaults to None. Format should be
                a sequence of dicts. Each one contains three fields: "array",
                the name of the array, e.g., "ExistI"; "indices", a sequence of
                the indices, e.g., [0, 0, 0]; and "value", 0 or 1.
            print_detail (bool, optional): whether to print details in
                SAT solving. Defaults to False.
            dimacs_file_name (Optional[str], optional): file to save the
                DIMACS. Defaults to None.
            sat_log_file_name (Optional[str], optional): file to save the
                SAT solver log. Defaults to None.

        Returns:
            Optional[LatticeSurgerySubroutine]: if the problem is
                unsatisfiable, this is None; otherwise, a
                LatticeSurgerySolution initialized by the compiled result.
        """
        start_time = time.time()
        sat_synthesis = LatticeSurgerySAT(
            input_dict=specification,
            given_arrs=given_arrs,
            given_vals=given_vals,
        )
        if print_detail:
            print(f"Adding constraints time: {time.time() - start_time}")

        start_time = time.time()
        if self.solver == "z3":
            if_sat = sat_synthesis.check_z3(print_progress=print_detail)
        else:
            if_sat = sat_synthesis.check_kissat(
                dimacs_file_name=dimacs_file_name,
                sat_log_file_name=sat_log_file_name,
                print_progress=print_detail,
                kissat_dir=self.kissat_dir,
            )
        if print_detail:
            print(f"Total solving time: {time.time() - start_time}")

        if if_sat:
            solver_result = sat_synthesis.get_result()
            return LatticeSurgerySolution(lasre=solver_result)
        else:
            return None

    def optimize_depth(
        self,
        specification: Mapping[str, Any],
        start_depth: Optional[int] = None,
        print_detail: bool = False,
        dimacs_file_name_prefix: Optional[str] = None,
        sat_log_file_name_prefix: Optional[str] = None,
    ) -> LatticeSurgerySolution:
        """find the optimal solution in terms of depth/height of the LaS.

    Args:
        specification (Mapping[str, Any]): the LaS specification to solve.
        start_depth (int, optional): starting depth of the exploration. If not
            provided, use the depth given in the specification
        print_detail (bool, optional): whether to print details in SAT solving.
            Defaults to False.
        dimacs_file_name_prefix (Optional[str], optional): file prefix to save
            the DIMACS. The full file name will contain the specific depth
            after this prefix. Defaults to None.
        sat_log_file_name_prefix (Optional[str], optional): file prefix to save
            the SAT log. The full file name will contain the specific depth
            after this prefix. Defaults to None.
        result_file_name_prefix (Optional[str], optional): file prefix to save
            the variable assignments. The full file name will contain the
            specific depth after this prefix. Defaults to None.
        post_optimization (str, optional): optimization to perform when
            initializing the LatticeSurgerySubroutine object for the result.
            Defaults to "default".

    Raises:
        ValueError: starting depth is too low.

    Returns:
        LatticeSurgerySolution: compiled result with the optimal depth.
    """
        self.specification = dict(specification)
        if start_depth is None:
            depth = self.specification["max_k"]
        else:
            depth = int(start_depth)
        if depth < 2:
            raise ValueError("depth too low.")

        checked_depth = {}
        while True:
            # the ports on the top floor will still be on the top floor when we
            # increase the height. This is an assumption. Adapt to your case.
            for port in self.specification["ports"]:
                if port["location"][2] == self.specification["max_k"]:
                    port["location"][2] = depth
            self.specification["max_k"] = depth

            result = self.solve(
                specification=self.specification,
                print_detail=print_detail,
                dimacs_file_name=dimacs_file_name_prefix +
                f"_d={depth}" if dimacs_file_name_prefix else None,
                sat_log_file_name=sat_log_file_name_prefix +
                f"_d={depth}" if sat_log_file_name_prefix else None,
            )
            if result is None:
                checked_depth[str(depth)] = "UNSAT"
                if str(depth + 1) in checked_depth:
                    # since this depth leads to UNSAT, we need to increase
                    # the depth, but if depth+1 is already checked, we can stop
                    break
                else:
                    depth += 1
            else:
                checked_depth[str(depth)] = "SAT"
                self.sat_result = LatticeSurgerySolution(lasre=result.lasre)
                if str(depth - 1) in checked_depth:
                    # since this depth leads to SAT, we need to try decreasing
                    # the depth, but if depth-1 is already checked, we can stop
                    break
                else:
                    depth -= 1

        return self.sat_result

    def try_one_permutation(
        self,
        perm: Sequence[int],
        specification: Mapping[str, Any],
        print_detail: bool = False,
        dimacs_file_name_prefix: Optional[str] = None,
        sat_log_file_name_prefix: Optional[str] = None,
    ) -> Optional[LatticeSurgerySolution]:
        """check if the problem is satisfiable given a port permutation.

        Args:
            specification (Mapping[str, Any]): the LaS specification to solve.
            perm (Sequence[int]): the given permutation, which is an integer
                tuple of length n (n being the number of ports permuted).
            print_detail (bool, optional): whether to print details in
                SAT solving. Defaults to False.
            dimacs_file_name_prefix (Optional[str], optional): file prefix
                to save the DIMACS. The full file name will contain the
                specific permutation after this prefix. Defaults to None.
            sat_log_file_name_prefix (Optional[str], optional): file prefix
                to save the SAT log. The full file name will contain the
                specific permutation after this prefix. Defaults to None.

        Returns:
            Optional[LatticeSurgerySubroutine]: if the problem is
                unsatisfiable, this is None; otherwise, a
                LatticeSurgerySolution initialized by the compiled result.
        """

        # say `perm` is [0,3,2], then `original` is in order, i.e., [0,2,3]
        # the full permutation is 0,1,2,3 -> 0,1,3,2
        original = sorted(perm)
        this_spec = dict(specification)
        new_ports = []
        for p, port in enumerate(specification["ports"]):
            if p not in perm:
                # the p-th port is not involved in `perm`, e.g., 1 is unchanged
                new_ports.append(port)

            else:
                # after the permutation, the index of the p-th port in
                # specification is the k-th port in `perm` where k is the place
                # of p in `original`. In this example, when p=0 and 1, nothing
                # changed. When p=2, we find `place` to be 1, and perm[place]=3
                # so we attach port_3. When p=3, we end up attach port_2
                place = original.index(p)
                new_ports.append(specification["ports"][perm[place]])
        this_spec["ports"] = new_ports

        result = self.solve(
            specification=this_spec,
            print_detail=print_detail,
            dimacs_file_name=dimacs_file_name_prefix + "_" +
            perm.__repr__().replace(" ", "")
            if dimacs_file_name_prefix else None,
            sat_log_file_name=sat_log_file_name_prefix + "_" +
            perm.__repr__().replace(" ", "")
            if sat_log_file_name_prefix else None,
        )
        print(f"{perm}: {'SAT' if result else 'UNSAT'}")
        return result

    def solve_all_port_permutations(
        self,
        permute_ports: Sequence[int],
        parallelism: int = 1,
        shuffle: bool = True,
        **kwargs,
    ) -> Mapping[str, Sequence[Sequence[int]]]:
        """try all the permutations of given ports, which ones are satisfiable.

        Note that we do not check that the LaS after permuting the ports (we do
        not permute the stabilizers accordingly) is functionally equivalent.
        The user should use this method based on their judgement. Also, the
        number of permutations scales exponentially with the number of ports
        to permute, so this method can easily take an immense amount of time.

        Args:
            permute_ports (Sequence[int]): the indices of ports to permute
            parallelism (int, optional): number of parallel process. Each one
                try one permutation. A New proess starts when an old one
                finishes. Defaults to 1.
            shuffle (bool, optional): whether using a random order to start the
                processes. Defaults to True.
            **kwargs: other arguments to `try_one_permutation`.

        Returns:
            Mapping[str, Sequence[Sequence[int]]]: a dict with two keys.
                "SAT": [.] a list containing all the satisfiable permutations;
                "UNSAT": [.] all the unsatisfiable permutations.
        """
        perms = list(itertools.permutations(permute_ports))
        if shuffle:
            random.shuffle(perms)

        pool = multiprocessing.Pool(parallelism)
        # issue the job one by one (chuck=1)
        results = pool.map(
            functools.partial(
                self.try_one_permutation,
                **kwargs,
            ),
            perms,
            chunksize=1,
        )

        sat_perms = []
        unsat_perms = []
        for p, result in enumerate(results):
            if result is None:
                unsat_perms.append(perms[p])
            else:
                sat_perms.append(perms[p])

        return {
            "SAT": sat_perms,
            "UNSAT": unsat_perms,
        }
