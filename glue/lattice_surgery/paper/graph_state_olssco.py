import json
import multiprocessing
import sys
from olssco import OptimalLatticeSurgerySubroutineCompiler


def graph_state_olssco(
    edges, seed, layout, wdir, starting_depth, if_print: bool = True
):
  n_q = 8
  # dimensions depending on the qubit layout
  if layout == "1row":
    data = {
        "max_i": n_q,
        "max_j": 2,
    }
  elif layout == "2rows":
    data = {
        "max_i": n_q // 2,
        "max_j": 3,
    }
  else:
    raise ValueError("layout not supported! ")

  # should not be too low, avoids being trivially proved UNSAT by Z3
  depth = starting_depth

  checked_depth = {}
  while True:
    data["max_k"] = depth

    # get the outgoing ports
    if layout == "1row":
      data["ports"] = [
          {
              "location": [i, 0, depth],
              "direction": "-K",
              "z_basis_direction": "I",
          }
          for i in range(n_q)
      ]
    elif layout == "2rows":
      data["ports"] = [
          {
              "location": [i, 0, depth],
              "direction": "-K",
              "z_basis_direction": "I",
          }
          for i in range(n_q // 2)
      ] + [
          {
              "location": [i - (n_q // 2), 2, depth],
              "direction": "-K",
              "z_basis_direction": "I",
          }
          for i in range(n_q // 2, n_q)
      ]

    # stabilizers are X on qubit and Z on all its neighbors
    data["stabilizers"] = []
    for i in range(n_q):
      stab = "." * i + "X" + "." * (n_q - i - 1)
      for edge in edges:
        if edge[0] == i:
          stab = stab[: edge[1]] + "Z" + stab[edge[1] + 1 :]
        if edge[1] == i:
          stab = stab[: edge[0]] + "Z" + stab[edge[0] + 1 :]
      data["stabilizers"].append(stab)

    olssco = OptimalLatticeSurgerySubroutineCompiler(data, color_ij=True)

    if olssco.check_external_solver(
        f"{wdir}graph_n{n_q}_s{seed}", print_progress=if_print
    ):
      checked_depth[str(depth)] = "SAT"
      with open(f"{wdir}graph_n{n_q}_s{seed}.json", "w") as f:
        json.dump(data, f)
      if str(depth - 1) in checked_depth:
        break
      else:
        depth -= 1
    else:
      checked_depth[str(depth)] = "UNSAT"
      if str(depth + 1) in checked_depth:
        break
      else:
        depth += 1

  with open(f"{wdir}graph_n{n_q}_s{seed}.json", "r") as f:
    opt_data = json.load(f)

  with open(wdir + "olsco_stats", "a") as f:
    f.write(f"{n_q}, {seed}, {opt_data['max_k']}\n")


if __name__ == "__main__":
  n_core = int(sys.argv[1])
  layout = sys.argv[2]
  wdir = sys.argv[3]
  starting_depth = int(sys.argv[4])

  def f(s):
    # read the edges from the graph library
    with open("graphs8.json", "r") as f:
      graphs = json.load(f)
    edges = graphs["8"][s]
    return graph_state_olssco(edges, s, "1row", wdir, 3, if_print=False)

  pool = multiprocessing.Pool(n_core)

  outputs = pool.map(f, range(101), chunksize=1)
