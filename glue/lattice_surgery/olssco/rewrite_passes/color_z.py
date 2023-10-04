"""A one-line summary of the module or program, terminated by a period.

Leave one blank line.  The rest of this docstring should contain an
overall description of the module or program.  Optionally, it may also
contain a brief description of exported classes and functions and/or usage
examples.

Typical usage example:

  foo = ClassFoo()
  bar = foo.FunctionBar()
"""


def if_uncolorK(n_i, n_j, n_k, ExistK, ColorKP, ColorKM):
  for i in range(n_i):
    for j in range(n_j):
      for k in range(n_k):
        if ExistK[i][j][k] and (
            ColorKP[i][j][k] == -1 or ColorKM[i][j][k] == -1
        ):
          # print((i,j,k))
          return True
  return False


def in_bound(n_i, n_j, n_k, i, j, k):
  if i in range(n_i) and j in range(n_j) and k in range(n_k):
    return True
  return False


def propogate_IJcolor(
    n_i, n_j, n_k, ExistI, ExistJ, ExistK, ColorI, ColorJ, ColorKP, ColorKM
):
  for i in range(n_i):
    for j in range(n_j):
      for k in range(n_k):
        if ExistK[i][j][k]:
          if in_bound(n_i, n_j, n_k, i - 1, j, k) and ExistI[i - 1][j][k]:
            ColorKM[i][j][k] = 1 - ColorI[i - 1][j][k]
          if ExistI[i][j][k]:
            ColorKM[i][j][k] = 1 - ColorI[i][j][k]
          if in_bound(n_i, n_j, n_k, i, j - 1, k) and ExistJ[i][j - 1][k]:
            ColorKM[i][j][k] = 1 - ColorJ[i][j - 1][k]
          if ExistJ[i][j][k]:
            ColorKM[i][j][k] = 1 - ColorJ[i][j][k]

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


def propogate_Kcolor(n_i, n_j, n_k, ExistK, ColorKP, ColorKM, NodeY):
  did_something = False
  for i in range(n_i):
    for j in range(n_j):
      for k in range(n_k):
        if ExistK[i][j][k]:
          if in_bound(n_i, n_j, n_k, i, j, k - 1) and ExistK[i][j][k - 1]:
            if ColorKP[i][j][k - 1] > -1 and ColorKM[i][j][k] == -1:
              ColorKM[i][j][k] = ColorKP[i][j][k - 1]
              did_something = True
          if in_bound(n_i, n_j, n_k, i, j, k + 1) and ExistK[i][j][k + 1]:
            if ColorKM[i][j][k + 1] > -1 and ColorKP[i][j][k] == -1:
              ColorKP[i][j][k] = ColorKM[i][j][k + 1]
              did_something = True
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


def assign_Kcolor(n_i, n_j, n_k, ExistK, ColorKP, ColorKM, NodeY):
  for i in range(n_i):
    for j in range(n_j):
      for k in range(n_k):
        if ExistK[i][j][k]:
          if ColorKM[i][j][k] > -1 and ColorKP[i][j][k] == -1:
            ColorKP[i][j][k] = ColorKM[i][j][k]
            break

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


def color_ports(ports, ColorKP, ColorKM):
  for port in ports:
    if port['d'] == 'K':
      if port['e'] == '+':
        ColorKP[port['i']][port['j']][port['k']] = port['c']
      else:
        ColorKM[port['i']][port['j']][port['k']] = port['c']


def colorZ(
    n_i,
    n_j,
    n_k,
    ExistI,
    ExistJ,
    ExistK,
    ColorI,
    ColorJ,
    ports,
    NodeY,
    noPrune=False,
):
  ColorKP = [[[-1 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]
  ColorKM = [[[-1 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]
  color_ports(ports, ColorKP, ColorKM)
  propogate_IJcolor(
      n_i, n_j, n_k, ExistI, ExistJ, ExistK, ColorI, ColorJ, ColorKP, ColorKM
  )
  while (not noPrune) and if_uncolorK(n_i, n_j, n_k, ExistK, ColorKP, ColorKM):
    if not propogate_Kcolor(n_i, n_j, n_k, ExistK, ColorKP, ColorKM, NodeY):
      assign_Kcolor(n_i, n_j, n_k, ExistK, ColorKP, ColorKM, NodeY)
  return ColorKP, ColorKM


def color_z(lasir: dict):
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

  lasir['ColorKP'], lasir['ColorKM'] = colorZ(
      n_i, n_j, n_k, ExistI, ExistJ, ExistK, ColorI, ColorJ, ports, NodeY
  )
  # lasir['ColorKP'], lasir['ColorKM'] = [[[-1 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)], [[[-1 for _ in range(n_k)] for _ in range(n_j)] for _ in range(n_i)]
  return lasir
