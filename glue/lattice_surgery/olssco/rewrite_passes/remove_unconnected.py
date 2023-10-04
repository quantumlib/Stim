"""A one-line summary of the module or program, terminated by a period.

Leave one blank line.  The rest of this docstring should contain an
overall description of the module or program.  Optionally, it may also
contain a brief description of exported classes and functions and/or usage
examples.

Typical usage example:

  foo = ClassFoo()
  bar = foo.FunctionBar()
"""


def check_connect(n_i, n_j, n_k, ExistI, ExistJ, ExistK, ports, NodeY):
  y_nodes = [
      i * n_j * n_k + j * n_k + k
      for i in range(n_i)
      for j in range(n_j)
      for k in range(n_k)
      if NodeY[i][j][k]
  ]
  Connect = [[[0 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]
  vips = [p['i'] * n_j * n_k + p['j'] * n_k + p['k'] for p in ports]
  mat = [[0 for _ in range(n_i * n_j * n_k)] for _ in range(n_i * n_j * n_k)]
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
  for i in range(n_i):
    for j in range(n_j):
      for k in range(n_k):
        queue = [
            i * n_j * n_k + j * n_k + k,
        ]
        if i * n_j * n_k + j * n_k + k in y_nodes:
          continue
        visited = [0 for _ in range(n_i * n_j * n_k)]
        while len(queue) > 0:
          if queue[0] in vips:
            Connect[i][j][k] = 1
            break
          visited[queue[0]] = 1
          for v in adj[queue[0]]:
            if not visited[v] and v not in y_nodes:
              queue.append(v)
          queue.pop(0)

  return Connect


def array3DAnd(arr0, arr1):
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


def check_edge(n_i, n_j, n_k, ExistI, ExistJ, ExistK, Connect):
  EffectI = [[[0 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]
  EffectJ = [[[0 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]
  EffectK = [[[0 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]
  for i in range(n_i):
    for j in range(n_j):
      for k in range(n_k):
        if ExistI[i][j][k] and (
            Connect[i][j][k] or (i + 1 < n_i and Connect[i + 1][j][k])
        ):
          EffectI[i][j][k] = 1
        if ExistJ[i][j][k] and (
            Connect[i][j][k] or (j + 1 < n_j and Connect[i][j + 1][k])
        ):
          EffectJ[i][j][k] = 1
        if ExistK[i][j][k] and (
            Connect[i][j][k] or (k + 1 < n_k and Connect[i][j][k + 1])
        ):
          EffectK[i][j][k] = 1
  return EffectI, EffectJ, EffectK


def remove_unconnected(lasir: dict):
  n_i, n_j, n_k, ExistK = (
      lasir['n_i'],
      lasir['n_j'],
      lasir['n_k'],
      lasir['ExistK'],
  )
  ExistI, ColorI, ExistJ, ColorJ = (
      lasir['ExistI'],
      lasir['ColorI'],
      lasir['ExistJ'],
      lasir['ColorJ'],
  )
  NodeY = lasir['NodeY']
  ports, port_cubes = lasir['ports'], lasir['port_cubes']
  Connect = check_connect(n_i, n_j, n_k, ExistI, ExistJ, ExistK, ports, NodeY)
  EffectI, EffectJ, EffectK = check_edge(
      n_i, n_j, n_k, ExistI, ExistJ, ExistK, Connect
  )
  ExistI, ExistJ, ExistK = (
      array3DAnd(ExistI, EffectI),
      array3DAnd(ExistJ, EffectJ),
      array3DAnd(ExistK, EffectK),
  )
  lasir['ExistI'], lasir['ExistJ'], lasir['ExistK'] = ExistI, ExistJ, ExistK

  return lasir
