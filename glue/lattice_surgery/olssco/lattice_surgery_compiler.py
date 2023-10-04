"""Two wrapper classes fpr OLSSSol, rewrite passes, and translators."""

import functools
import itertools
import json
import multiprocessing
import random
from typing import Any, Literal, Mapping, Optional, Sequence
from olssco.rewrite_passes.attach_fixups import attach_fixups
from olssco.rewrite_passes.color_z import color_z
from olssco.rewrite_passes.remove_unconnected import remove_unconnected
from olssco.sat_lattice_surgery_solver import OptimalLatticeSurgerySubroutineSolver
from olssco.tools.verify_stabilizers import verify_stabilizers
from olssco.translators.gltf_generator import gltf_generator
from olssco.translators.networkx_generator import networkx_generator
from olssco.translators.textfig_generator import textfig_generator
from olssco.translators.zigxag_generator import zigxag_generator


class LatticeSurgerySolution:
  """A class for the result of compilation lattice surgery subroutine (LSS).

  It internally saves an LaSSIR (Lattice Surgery Subroutine Intermediate
  Representation) and we can apply rewrite passes to it, or use translators to
  derive other formats of the LSS solution
  """

  def __init__(
      self,
      *,
      lassir: Mapping[str, Any],
  ) -> None:
    """initialization for LatticeSurgerySubroutine

    Args:
        lassir (Mapping[str, Any]): LaSSIR
    """
    self.lassir = lassir

  def get_depth(self) -> int:
    """get the depth/height of the LSS in LaSSIR.

    Returns:
        int: depth/height of the LSS in LaSSIR
    """
    return self.lassir["n_k"]

  def after_removing_disconnected_pieces(self):
    """remove_unconnected."""
    return LatticeSurgerySolution(lassir=remove_unconnected(self.lassir))

  def after_color_k_pipes(self):
    """coloring K pipes."""
    return LatticeSurgerySolution(lassir=color_z(self.lassir))

  def after_default_optimizations(self):
    """default optimizations: remove unconnected, and then color K pipes."""
    solution = LatticeSurgerySolution(lassir=remove_unconnected(self.lassir))
    solution = LatticeSurgerySolution(lassir=color_z(solution.lassir))
    return solution

  def after_t_factory_default_optimizations(self):
    """default optimization for T-factories."""
    solution = LatticeSurgerySolution(lassir=remove_unconnected(self.lassir))
    solution = LatticeSurgerySolution(lassir=color_z(solution.lassir))
    solution = LatticeSurgerySolution(lassir=attach_fixups(solution.lassir))
    return solution

  def save_lassir(self, file_name: str) -> None:
    """save the current LaSSIR to a file.

    Args:
        file_name (str): file to save the current LaSSIR
    """
    with open(file_name, "w") as f:
      json.dump(self.lassir, f)

  def to_3d_model_gltf(
      self,
      output_file_name: str,
      stabilizer: int = -1,
      tube_len: float = 2.0,
      no_color_z: bool = False,
  ) -> None:
    """generate gltf file (for 3D modelling).

    Args:
        output_file_name (str):
        stabilizer (int, optional): Defaults to -1 meaning do not draw
          correlation surfaces. If the value is in [0, n_s), the correlation
          surfaces corresponding to that stabilizer are drawn and faces in one
          of the directions are revealed to unveil the correlation surfaces.
        tube_len (float, optional): Length of the pipe comapred to the cube.
          Defaults to 2.0.
        no_color_z (bool, optional): Do not color the K pipes. Defaults to
          False.
    """
    gltf_generator(
        self.lassir,
        output_file=output_file_name,
        stabilizer=stabilizer,
        tube_len=tube_len,
        no_color_z=no_color_z,
    )

  def to_zigxag_url(
      self,
      io_spec: Optional[Sequence[str]] = None,
  ) -> str:
    """generate a link that leads to a ZigXag figure.

    Args:
        io_spec (Optional[Sequence[str]], optional): Specify whether each port
          is an input or an output. Length must be the same with the number of
          ports. Defaults to None, which means all ports are outputs.

    Returns:
        str: the ZigXag link
    """
    return zigxag_generator(self.lassir, io_spec=io_spec)

  def to_text_diagram(
      self,
  ) -> str:
    """generate the text figure of LSS time slices.

    Returns:
        str: text figure of the LSS
    """
    return textfig_generator(self.lassir)

  def generate_networkx(self):
    """generate a annotated networkx.Graph correponding to the LSS.

    Returns:
        networkx.Graph:
    """
    return networkx_generator(self.lassir)

  def verify_stabilizers_stimzx(self, print_stabilizers: bool = False) -> bool:
    """verify the stabilizer of the LSS.

    Use Stim ZX to deduce the stabilizers from the annotated networkx graph.
    Then use Stim to ensure that this set of stabilizers and the set of
    stabilizers specified in the input are equivalent.

    Args:
        print_stabilizers (bool, optional): If True, print the two sets of
          stabilizers. Defaults to False.

    Returns:
        bool: True if the two sets are equivalent; otherwise False.
    """
    paulistrings = [
        paulistring.replace(".", "_")
        for paulistring in self.lassir["specification"]["stabilizers"]
    ]
    return verify_stabilizers(
        paulistrings,
        self.generate_networkx(),
        print_stabilizers=print_stabilizers,
    )


class LatticeSurgeryProblem:
  """A class to provide the input specification of LSS and solve it."""

  def __init__(
      self,
      specification: Optional[Mapping[str, Any]] = None,
      solver: Literal["kissat", "z3"] = "z3",
      kissat_dir: Optional[str] = None,
  ) -> None:
    """initialize LatticeSurgeryCompiler.

    Args:
        specification (Optional[Mapping[str, Any]], optional): the LSS
          specification to solve. Defaults to None.
        solver (Literal["kissat", "z3"], optional): the solver to use. Defaults
          to "z3". "kissat" is recommended.
        kissat_dir (Optional[str], optional): directory of the kissat
          executable. Defaults to None.
    """
    self.specification = specification
    self.result = None
    self.solver = solver
    self.kissat_dir = kissat_dir

  def solve(
      self,
      specification: Optional[Mapping[str, Any]] = None,
      given_arrs: Optional[Mapping[str, Any]] = None,
      given_vals: Optional[Sequence[Mapping[str, Any]]] = None,
      print_detail: bool = False,
      dimacs_file_name: Optional[str] = None,
      sat_log_file_name: Optional[str] = None,
  ) -> Optional[LatticeSurgerySolution]:
    """solve an LSS compilation problem.

    Args:
        specification (Optional[Mapping[str, Any]], optional): the LSS
          specification to solve. Defaults to None.
        given_arrs (Optional[Mapping[str, Any]], optional): given array of known
          values to plug in. Defaults to None.
        given_vals (Optional[Sequence[Mapping[str, Any]]], optional): given
          known values to plug in. Defaults to None. Format should be a sequence
          of dicts. Each one contains three fields: 'array', the name of the
          array, e.g., 'ExistI'; 'indices', a sequence of the indices, e.g., [0,
          0, 0]; and 'value', 0 or 1.
        print_detail (bool, optional): whether to print details in SAT solving.
          Defaults to False.
        dimacs_file_name (Optional[str], optional): file to save the DIMACS.
          Defaults to None.
        sat_log_file_name (Optional[str], optional): file to save the SAT solver
          log. Defaults to None.

    Returns:
        Optional[LatticeSurgerySubroutine]: if the problem is unsatisfiable,
          this is None; otherwise, an object initialized by the compiled result.
    """
    olsssol = OptimalLatticeSurgerySubroutineSolver(
        input_dict=specification if specification else self.specification,
        given_arrs=given_arrs,
        given_vals=given_vals,
    )

    if self.solver == "z3":
      if_sat = olsssol.check_z3(print_progress=print_detail)
    else:
      if_sat = olsssol.check_external_solver(
          dimacs_file_name=dimacs_file_name,
          sat_log_file_name=sat_log_file_name,
          print_progress=print_detail,
          kissat_dir=self.kissat_dir,
      )

    if if_sat:
      solver_result = olsssol.get_result()
      self.result = LatticeSurgerySolution(lassir=solver_result)
      return self.result
    else:
      return None

  def optimize_depth(
      self,
      start_depth: int,
      print_detail: bool = False,
      dimacs_file_name_prefix: Optional[str] = None,
      sat_log_file_name_prefix: Optional[str] = None,
  ) -> LatticeSurgerySolution:
    """find the optimal solution in terms of depth/height of the LSS.

    Args:
        start_depth (int): starting depth of the exploration
        print_detail (bool, optional): whether to print details in SAT solving.
          Defaults to False.
        dimacs_file_name_prefix (Optional[str], optional): file prefix to save
          the DIMACS. The full file name will contain the specific depth after
          this prefix. Defaults to None.
        sat_log_file_name_prefix (Optional[str], optional): file prefix to save
          the SAT log. The full file name will contain the specific depth after
          this prefix. Defaults to None.
        result_file_name_prefix (Optional[str], optional): file prefix to save
          the variable assignments. The full file name will contain the specific
          depth after this prefix. Defaults to None.
        post_optimization (str, optional): optimization to perform when
          initializing the LatticeSurgerySubroutine object for the result.
          Defaults to "default".

    Raises:
        ValueError: starting depth is too low.

    Returns:
        LatticeSurgerySubroutine: compiled result with the optimal depth.
    """
    depth = int(start_depth)
    if depth < 2:
      raise ValueError("depth too low.")

    checked_depth = {}
    while True:
      # the ports on the top floor will still be on the top floor when we
      # increase the height. This is an assumption. Adapt to your use cases.
      for port in self.specification["ports"]:
        if port["location"][2] == self.specification["max_k"]:
          port["location"][2] = depth
      self.specification["max_k"] = depth

      result = self.solve(
          print_detail=print_detail,
          dimacs_file_name=dimacs_file_name_prefix + f"_d={depth}"
          if dimacs_file_name_prefix
          else None,
          sat_log_file_name=sat_log_file_name_prefix + f"_d={depth}"
          if sat_log_file_name_prefix
          else None,
      )
      if result is None:
        checked_depth[str(depth)] = "UNSAT"
        if str(depth + 1) in checked_depth:
          # since this depth leads to UNSAT, we need to increase the depth,
          # but if depth+1 is already checked, we can stop
          break
        else:
          depth += 1
      else:
        checked_depth[str(depth)] = "SAT"
        self.sat_result = LatticeSurgerySolution(lassir=result.lassir)
        if str(depth - 1) in checked_depth:
          # since this depth leads to SAT, we need to try decreasing the depth,
          # but if depth-1 is already checked, we can stop
          break
        else:
          depth -= 1

    return self.sat_result

  def with_permuted_ports(self, perm):
    # say permutation is [0,3,2], then original is in order, i.e., [0,2,3]
    # the full permutation is 0,1,2,3 -> 0,1,3,2
    original = sorted(perm)
    this_input = dict(self.specification)
    new_ports = []
    for p, port in enumerate(self.specification["ports"]):
      if p not in perm:
        # the p-th port is not involved in the permutation, e.g., 1 is unchanged
        new_ports.append(port)

      else:
        # after the permutation, the index of the p-th port is the k-th port in
        # `perm` where k is the place of p in `original`. In this example, when
        # p=0 and 1, nothing changed. When p=2, we find `place` to be 1, and
        # perm[place]=3, so we attach port_3. When p=3, we end up attach port_2
        place = original.index(p)
        new_ports.append(self.specification["ports"][perm[place]])
    this_input["ports"] = new_ports
    return LatticeSurgeryProblem(this_input)

  def try_one_permutation(
      self,
      perm: Sequence[int],
      print_detail: bool = False,
      dimacs_file_name_prefix: Optional[str] = None,
      sat_log_file_name_prefix: Optional[str] = None,
      result_file_name_prefix: Optional[str] = None,
  ) -> Optional[LatticeSurgerySolution]:
    """check after the given permutation of ports, if the problem is satisfiable.

    Args:
        perm (Sequence[int]): the given permutation, which is an integer tuple
          of length n (n being the number of ports permuted).
        print_detail (bool, optional): whether to print details in SAT solving.
          Defaults to False.
        dimacs_file_name_prefix (Optional[str], optional): file prefix to save
          the DIMACS. The full file name will contain the specific permutation
          after this prefix. Defaults to None.
        sat_log_file_name_prefix (Optional[str], optional): file prefix to save
          the SAT log. The full file name will contain the specific permutation
          after this prefix. Defaults to None.
        result_file_name_prefix (Optional[str], optional): file prefix to save
          the variable assignments. The full file name will contain the specific
          permutation after this prefix. Defaults to None.

    Returns:
        Optional[LatticeSurgerySubroutine]: if the problem is unsatisfiable,
          this is None; otherwise, an object initialized by the compiled result.
    """

    # say permutation is [0,3,2], then original is in order, i.e., [0,2,3]
    # the full permutation is 0,1,2,3 -> 0,1,3,2
    original = sorted(perm)
    this_input = dict(self.specification)
    new_ports = []
    for p, port in enumerate(self.specification["ports"]):
      if p not in perm:
        # the p-th port is not involved in the permutation, e.g., 1 is unchanged
        new_ports.append(port)

      else:
        # after the permutation, the index of the p-th port is the k-th port in
        # `perm` where k is the place of p in `original`. In this example, when
        # p=0 and 1, nothing changed. When p=2, we find `place` to be 1, and
        # perm[place]=3, so we attach port_3. When p=3, we end up attach port_2
        place = original.index(p)
        new_ports.append(self.specification["ports"][perm[place]])
    this_input["ports"] = new_ports

    result = self.solve(
        specification=this_input,
        print_detail=print_detail,
        dimacs_file_name=dimacs_file_name_prefix
        + "_"
        + perm.__repr__().replace(" ", "")
        if dimacs_file_name_prefix
        else None,
        sat_log_file_name=sat_log_file_name_prefix
        + "_"
        + perm.__repr__().replace(" ", "")
        if sat_log_file_name_prefix
        else None,
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
    """try all the permutations of given ports to see which ones are satisfiable.

    Notice that we do not check that the LSS after permuting the ports (we do
    not permute the stabilizers accordingly) is functionally equivalent. The
    users should use this method based on their judgement. Also, the number of
    permutations scales exponentially with the number of ports to permute, so
    this method can easily take an immense amount of time.

    Args:
        permute_ports (Sequence[int]): the indices of ports to permite
        parallelism (int, optional): number of parallel process. Each one try
          one permutation. A New proess starts when an old one finishes.
          Defaults to 1.
        shuffle (bool, optional): whether using a random order to start the
          processes. Defaults to True.
        **kwargs:

    Returns:
        Mapping[str, Sequence[Sequence[int]]]: a dict with two keys. "SAT": [.]
          a list containing all the satisfiable permutations; "UNSAT": [.]
          all the unsatisfiable permutations
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
