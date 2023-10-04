import json
import random
import sys


def split_map_i(i, lorr, n_col):
  if i == 0:
    return 0
  elif i == n_col - 1:
    return 2 * n_col - 3
  else:
    if lorr == "L":
      return 2 * i
    elif lorr == "R":
      return 2 * i - 1
    else:
      raise ValueError()


def split_map_back_i(i, n_col):
  if i == 0:
    return 0
  elif i == n_col - 1:
    return i // 2
  else:
    return (i + 1) // 2


def split_map_edge(edge, n_col):
  if len(edge) != 2:
    raise ValueError()
  left = min(edge)
  right = max(edge)
  return (split_map_i(left, "L", n_col), split_map_i(right, "R", n_col))


def depth_baseline(edges, n_col, layout):
  # sort according to the smaller col involved (beginning of interval)
  edges.sort(key=lambda x: x[0])

  original_edges = edges
  split_edges = [split_map_edge(edge, n_col) for edge in edges]
  edges = split_edges
  n_col = 2 * (n_col - 1)

  # calculate number of layers
  n_layer = 0
  for c in range(n_col):
    cnt = 0
    for edge in edges:
      if c >= edge[0] and c <= edge[1]:
        cnt += 1
    n_layer = max(n_layer, cnt)

  layer = [-1 for _ in edges]
  for i, edge in enumerate(edges):
    used = []
    for j in range(i):
      if edges[j][1] >= edge[0]:
        used.append(layer[j])
    for l in range(n_layer):
      if l not in used:
        layer[i] = l
        break

  # for l in range(n_layer):
  #   info = f"layer {l}:"
  #   for i, edge in enumerate(edges):
  #     if layer[i] == l:
  #       info += f" {edge}"
  #   print(info)

  # for l in range(n_layer):
  #   info = f"layer {l}:"
  #   for i, edge in enumerate(original_edges):
  #     if layer[i] == l:
  #       info += f" {edge}"
  #   print(info)

  return 2 * n_layer


if __name__ == "__main__":
  n_q = int(sys.argv[1])
  layout = sys.argv[2]
  wdir = sys.argv[3]
  graph_lib = sys.argv[4]

  # read the edges from the graph library
  with open(graph_lib, "r") as f:
    graphs = json.load(f)
  if str(n_q) not in graphs:  # all n_q should be even
    raise ValueError("n_q not found in graph library!")
  with open(wdir + "baseline_stats", "w") as f:
    f.write("n_q, ratio, s, n_layer\n")

  for r in range(1, 5):
    for s in range(len(graphs[str(n_q)][r])):
      edges = graphs[str(n_q)][r][s]
      with open(wdir + "baseline_stats", "a") as f:
        f.write(
            f"{n_q}, {(r+1)/10}, {s}, {2*depth_baseline(edges, n_q, layout)}\n"
        )
