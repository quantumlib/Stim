import argparse
import json
from networkx import Graph
import stimzx


class ZXNode:

  def __init__(self, coord3, n_j):
    self.type = 'N'
    i, j, k = coord3
    self.yTailP = False
    self.yTailM = False
    self.node_id = -1

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
    return str(-self.y) + ',' + str(self.x) + ',' + str(zigxag[self.type])


class ZXEdge:

  def __init__(self, ifH, nodea: ZXNode, nodeb: ZXNode):
    self.type = '-'
    if ifH:
      self.type = 'h'
    self.nodea = nodea
    self.nodeb = nodeb

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
        + str(self.xa)
        + ','
        + str(-self.yb)
        + ','
        + str(self.xb)
        + ','
        + self.type
    )


def networkx_generator(lasir: dict):
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

  for i, j, k in port_cubes:
    nodes[i][j][k].type = 'Po'

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
          edges.append(ZXEdge(0, nodes[i][j][k], nodes[i + 1][j][k]))

        if (
            ExistJ[i][j][k] == 1
            and nodes[i][j][k].type in valid_types
            and nodes[i][j + 1][k].type in valid_types
        ):
          # anc_nodes.append( (nodes[i][j][k].x+1, nodes[i][j][k].y) )
          # edges.append(ZXEdge(0, nodes[i][j][k].getCoord2(), anc_nodes[-1]))
          # edges.append(ZXEdge(0, anc_nodes[-1], nodes[i][j+1][k].getCoord2()))
          edges.append(ZXEdge(0, nodes[i][j][k], nodes[i][j + 1][k]))

        if (
            ExistK[i][j][k] == 1
            and nodes[i][j][k].type in valid_types
            and nodes[i][j][k + 1].type in valid_types
        ):
          edges.append(
              ZXEdge(
                  abs(ColorKM[i][j][k] - ColorKP[i][j][k]),
                  nodes[i][j][k],
                  nodes[i][j][k + 1],
              )
          )

  zx_graph = Graph()
  type_to_str = {'X': 'X', 'Z': 'Z', 'Pi': 'in', 'Po': 'out', 'I': 'X'}
  cnt = 0
  for i, j, k in port_cubes:
    node = nodes[i][j][k]
    zx_graph.add_node(cnt, value=stimzx.ZxType(type_to_str[node.type]))
    node.node_id = cnt
    cnt += 1

  for i in range(n_i + 1):
    for j in range(n_j + 1):
      for k in range(n_k + 1):
        node = nodes[i][j][k]
        if node.type not in ['N', 'Po', 'Pi']:
          zx_graph.add_node(cnt, value=stimzx.ZxType(type_to_str[node.type]))
          node.node_id = cnt
          cnt += 1
        if node.yTailM:
          zx_graph.add_node(cnt, value=stimzx.ZxType('Z', 1))
          zx_graph.add_edge(node.node_id, cnt)
          cnt += 1
        if node.yTailP:
          zx_graph.add_node(cnt, value=stimzx.ZxType('Z', 3))
          zx_graph.add_edge(node.node_id, cnt)
          cnt += 1

  for edge in edges:
    if edge.type != 'h':
      zx_graph.add_edge(edge.nodea.node_id, edge.nodeb.node_id)
    else:
      zx_graph.add_node(cnt, value=stimzx.ZxType('H'))
      zx_graph.add_edge(cnt, edge.nodea.node_id)
      zx_graph.add_edge(cnt, edge.nodeb.node_id)
      cnt += 1

  return zx_graph
