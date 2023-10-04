import json
import math
import subprocess
import sys
from olsco_compiler import OLSCo
from rand_graph.rand_graph_baseline_opt import depth_baseline


def solve_sub(
    edges, ratioX10_1, seed, layout, wdir, starting_depth, if_print, note, n_q=8
):
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
              "location": [i, 0, 0],
              "direction": "+K",
              "z_basis_direction": "I",
          }
          for i in range(n_q)
      ] + [
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
    data["stabilizers"] = [
        ("." * i + "Z" + "." * (n_q - i - 1)) * 2 for i in range(n_q)
    ]
    for i in range(n_q):
      stab = ("." * i + "X" + "." * (n_q - i - 1)) * 2
      for edge in edges:
        if edge[0] == i:
          stab = stab[: n_q + edge[1]] + "Z" + stab[n_q + edge[1] + 1 :]
        if edge[1] == i:
          stab = stab[: n_q + edge[0]] + "Z" + stab[n_q + edge[0] + 1 :]
      data["stabilizers"].append(stab)

    olsco = OLSCo(data, cnf=True, color_ij=True)
    olsco.write_cnf(f"{wdir}graph_n{n_q}_r{ratioX10_1}_s{seed}{note}.dimacs")

    subprocess.run(
        f"kissat {wdir}graph_n{n_q}_r{ratioX10_1}_s{seed}{note}.dimacs"
        + (
            f" | tee {wdir}graph_n{n_q}_r{ratioX10_1}_s{seed}{note}.sat"
            if if_print
            else f" > {wdir}graph_n{n_q}_r{ratioX10_1}_s{seed}{note}.sat"
        ),
        capture_output=False,
        check=False,
        shell=True,
    )

    with open(f"{wdir}graph_n{n_q}_r{ratioX10_1}_s{seed}{note}.sat", "r") as f:
      sat_output = f.readlines()

    if "exit 10" in sat_output[-1]:  # found SAT
      checked_depth[str(depth)] = "SAT"
      with open(
          f"{wdir}graph_n{n_q}_r{ratioX10_1}_s{seed}{note}.json", "w"
      ) as f:
        json.dump(data, f)
      if str(depth - 1) in checked_depth or depth <= 2:
        break
      else:
        depth -= 1
    else:
      checked_depth[str(depth)] = "UNSAT"
      if str(depth + 1) in checked_depth:
        break
      else:
        depth += 1

  with open(f"{wdir}graph_n{n_q}_r{ratioX10_1}_s{seed}{note}.json", "r") as f:
    opt_data = json.load(f)

  return opt_data["max_k"]


if __name__ == "__main__":
  n_q = int(sys.argv[1])
  ratioX10_1 = int(sys.argv[2])
  seed = int(sys.argv[3])
  layout = sys.argv[4]
  wdir = sys.argv[5]
  graph_lib = sys.argv[6]
  starting_depth = int(sys.argv[7])
  if_print = int(sys.argv[8])

  max_q = 8

  # read the edges from the graph library
  with open(graph_lib, "r") as f:
    graphs = json.load(f)
  if str(n_q) not in graphs:  # all n_q should be even
    raise ValueError("n_q not found in graph library!")
  if ratioX10_1 not in range(10):
    raise ValueError("invalid ratio x 10 - 1!")
  ratio = (1 + ratioX10_1) / 10
  if seed not in range(10):
    raise ValueError("invalid random seed!")
  edges = graphs[str(n_q)][ratioX10_1][seed]

  total_cycle = 0
  n_stretch = int(math.log(n_q // max_q, 2)) + 1
  for t in range(n_stretch):
    for l in range(2**t):
      n_partition = n_q // (max_q * (2**t))
      max_cycle = 0
      for p in range(n_partition):
        if not edges:
          continue
        subset = []
        others = []
        i_start = max_q * (2**t) * p + l
        i_end = i_start + (2**t) * max_q
        for edge in edges:
          if (edge[0] in range(i_start, i_end, 2**t)) and (
              edge[1] in range(i_start, i_end, 2**t)
          ):
            subset.append([
                (edge[0] - i_start) // (2**t),
                (edge[1] - i_start) // (2**t),
            ])
          else:
            others.append(edge)
        edges = others

        max_cycle = max(
            max_cycle,
            solve_sub(
                subset,
                ratioX10_1,
                seed,
                layout,
                wdir,
                starting_depth,
                if_print,
                f"_t{t}_l{l}_p{p}",
                n_q=max_q,
            ),
        )

      total_cycle += max_cycle

  if edges:
    total_cycle += depth_baseline(edges, n_q, layout)

  by_gate_d = depth_baseline(graphs[str(n_q)][ratioX10_1][seed], n_q, layout)
  # print(f"baseline:{by_gate_d} vs partially-compiled:{total_cycle-1}")
  with open(wdir + "hybrid_stats", "a") as f:
    f.write(f"{n_q}, {ratio}, {seed}, {total_cycle}\n")
