"""A one-line summary of the module or program, terminated by a period.

Leave one blank line.  The rest of this docstring should contain an
overall description of the module or program.  Optionally, it may also
contain a brief description of exported classes and functions and/or usage
examples.

Typical usage example:

  foo = ClassFoo()
  bar = foo.FunctionBar()
"""

from typing import Sequence


class ZXNode:

  def __init__(self, coord3, n_j):
    self.type = 'N'
    i, j, k = coord3
    self.yTailP = False
    self.yTailM = False

    self.y = -(n_j + 1) * i + j
    self.x = k * (n_j + 2) + j

  def getCoord2(self):
    return (self.x, self.y)

  def computeType(self, exists, colors):
    deg = sum([v for (k, v) in exists.items()])
    if deg == 0:
      self.type = 'N'
      return
    elif deg == 1:
      raise ValueError('There should not be deg-1 Z or X spiders.')
    elif deg == 2:
      self.type = 'I'
    elif deg >= 5:
      raise ValueError('deg > 4: 3D corner exists')
    else:
      if exists['-I'] == 0 and exists['+I'] == 0:
        if exists['-J']:
          if colors['-J'] == 0:
            self.type = 'X'
          else:
            self.type = 'Z'
        else:  # must exist +J
          if colors['+J'] == 0:
            self.type = 'X'
          else:
            self.type = 'Z'

      if exists['-J'] == 0 and exists['+J'] == 0:
        if exists['-I']:
          if colors['-I'] == 0:
            self.type = 'Z'
          else:
            self.type = 'X'
        else:  # must exist +I
          if colors['+I'] == 0:
            self.type = 'Z'
          else:
            self.type = 'X'

      if exists['-K'] == 0 and exists['+K'] == 0:
        if exists['-I']:
          if colors['-I'] == 0:
            self.type = 'X'
          else:
            self.type = 'Z'
        else:  # must exist +I
          if colors['+I'] == 0:
            self.type = 'X'
          else:
            self.type = 'Z'

  def emit(self):
    zigxag = {
        'Z': '@',
        'X': 'O',
        'S': 's',
        'W': 'w',
        'I': 'O',
        'Pi': 'in',
        'Po': 'out',
    }
    # return str(self.x) + ',' + str(self.y) + ',' + str(zigxag[self.type])
    return str(-self.y) + ',' + str(-self.x) + ',' + str(zigxag[self.type])


class ZXEdge:

  def __init__(self, ifH, coord2a, coord2b):
    self.xa, self.ya = coord2a
    self.xb, self.yb = coord2b
    self.type = '-'
    if ifH:
      self.type = 'h'

  def emit(self):
    # return (
    #     str(self.xa)
    #     + ','
    #     + str(self.ya)
    #     + ','
    #     + str(self.xb)
    #     + ','
    #     + str(self.yb)
    #     + ','
    #     + self.type
    # )

    return (
        str(-self.ya)
        + ','
        + str(-self.xa)
        + ','
        + str(-self.yb)
        + ','
        + str(-self.xb)
        + ','
        + self.type
    )


def zigxag_generator(lasir: dict, io_spec: Sequence[str] = None):
  n_i = lasir['n_i']
  n_j = lasir['n_j']
  n_k = lasir['n_k']
  NodeY = lasir['NodeY']
  ExistI = lasir['ExistI']
  ColorI = lasir['ColorI']
  ExistJ = lasir['ExistJ']
  ColorJ = lasir['ColorJ']
  ExistK = lasir['ExistK']
  ColorKP = lasir['ColorKP']
  ColorKM = lasir['ColorKM']
  port_cubes = lasir['port_cubes']

  nodes = [
      [[ZXNode((i, j, k), n_j) for k in range(n_k + 1)] for j in range(n_j + 1)]
      for i in range(n_i + 1)
  ]

  # ports
  for [i, j, k] in port_cubes:
    nodes[i][j][k].type = 'Po'
  if io_spec:
    if len(io_spec) != len(port_cubes):
      raise ValueError('io_spec should be of same length as port_cubes')
    for w, [i, j, k] in enumerate(port_cubes):
      nodes[i][j][k].type = io_spec[w]

  # Y spiders
  for i in range(n_i):
    for j in range(n_j):
      for k in range(n_k):
        if NodeY[i][j][k]:
          if k - 1 >= 0 and ExistK[i][j][k - 1] and (not NodeY[i][j][k - 1]):
            nodes[i][j][k - 1].yTailP = True
          if k + 1 < n_k and ExistK[i][j][k] and (not NodeY[i][j][k + 1]):
            nodes[i][j][k + 1].yTailM = True

  # Z/X Spiders
  for i in range(n_i):
    for j in range(n_j):
      for k in range(n_k):
        if (i, j, k) not in port_cubes and NodeY[i][j][k] == 0:
          exists = {'-I': 0, '+I': 0, '-K': 0, '+K': 0, '-J': 0, '+J': 0}
          colors = {}
          if i > 0 and ExistI[i - 1][j][k]:
            exists['-I'] = 1
            colors['-I'] = ColorI[i - 1][j][k]
          if ExistI[i][j][k]:
            exists['+I'] = 1
            colors['+I'] = ColorI[i][j][k]
          if j > 0 and ExistJ[i][j - 1][k]:
            exists['-J'] = 1
            colors['-J'] = ColorJ[i][j - 1][k]
          if ExistJ[i][j][k]:
            exists['+J'] = 1
            colors['+J'] = ColorJ[i][j][k]
          if k > 0 and ExistK[i][j][k - 1]:
            exists['-K'] = 1
            colors['-K'] = ColorKP[i][j][k - 1]
          if ExistK[i][j][k]:
            exists['+K'] = 1
            colors['+K'] = ColorKM[i][j][k]
          nodes[i][j][k].computeType(exists, colors)

  # edges
  edges = []
  # anc_nodes = []
  valid_types = ['Z', 'X', 'S', 'W', 'I', 'Pi', 'Po']
  for i in range(n_i):
    for j in range(n_j):
      for k in range(n_k):
        if (
            ExistI[i][j][k] == 1
            and nodes[i][j][k].type in valid_types
            and nodes[i + 1][j][k].type in valid_types
        ):
          edges.append(
              ZXEdge(
                  0, nodes[i][j][k].getCoord2(), nodes[i + 1][j][k].getCoord2()
              )
          )

        if (
            ExistJ[i][j][k] == 1
            and nodes[i][j][k].type in valid_types
            and nodes[i][j + 1][k].type in valid_types
        ):
          # anc_nodes.append( (nodes[i][j][k].x+1, nodes[i][j][k].y) )
          # edges.append(ZXEdge(0, nodes[i][j][k].getCoord2(), anc_nodes[-1]))
          # edges.append(ZXEdge(0, anc_nodes[-1], nodes[i][j+1][k].getCoord2()))
          edges.append(
              ZXEdge(
                  0, nodes[i][j][k].getCoord2(), nodes[i][j + 1][k].getCoord2()
              )
          )

        if (
            ExistK[i][j][k] == 1
            and nodes[i][j][k].type in valid_types
            and nodes[i][j][k + 1].type in valid_types
        ):
          edges.append(
              ZXEdge(
                  abs(ColorKM[i][j][k] - ColorKP[i][j][k]),
                  nodes[i][j][k].getCoord2(),
                  nodes[i][j][k + 1].getCoord2(),
              )
          )

  nodes_str = ''
  first = True
  for i in range(n_i + 1):
    for j in range(n_j + 1):
      for k in range(n_k + 1):
        if nodes[i][j][k].type in valid_types:
          if not first:
            nodes_str += ';'
          nodes_str += nodes[i][j][k].emit()
          first = False

  # for (x,y) in anc_nodes:
  #     zigxag_str += ';' + str(x) + ',' + str(y) + '+'

  edges_str = ''
  for i, edge in enumerate(edges):
    if i > 0:
      edges_str += ';'
    edges_str += edge.emit()

  for i in range(n_i):
    for j in range(n_j):
      for k in range(n_k):
        if nodes[i][j][k].yTailP:
          nodes_str += (
              ';'
              + str(nodes[i][j][k].x + n_j - j)
              + ','
              + str(nodes[i][j][k].y)
              + ',s'
          )
          edges_str += (
              ';'
              + str(nodes[i][j][k].x + n_j - j)
              + ','
              + str(nodes[i][j][k].y)
              + ','
              + str(nodes[i][j][k].x)
              + ','
              + str(nodes[i][j][k].y)
              + ',-'
          )
        if nodes[i][j][k].yTailM:
          nodes_str += (
              ';'
              + str(nodes[i][j][k].x - j - 1)
              + ','
              + str(nodes[i][j][k].y)
              + ',s'
          )
          edges_str += (
              ';'
              + str(nodes[i][j][k].x - j - 1)
              + ','
              + str(nodes[i][j][k].y)
              + ','
              + str(nodes[i][j][k].x)
              + ','
              + str(nodes[i][j][k].y)
              + ',-'
          )

  zigxag_str = 'https://algassert.com/zigxag#' + nodes_str + ':' + edges_str
  return zigxag_str
