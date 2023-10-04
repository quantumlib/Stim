"""OLSSSol: Optimal Lattice Surgery Subroutine Solver

It encodes a lattice surgery problem to SAT/SMT and checks whether there is a
solution. In this compilation problem, we are given certain spacetime volume,
certain ports, and certain stabilizers. OLSSSol encodes the constraints on
LaSSIR variables such that the resulting variable assignments consist of a valid
lattice surgery procedure with the correct functionality (satisfies all the
given stabilizers). OLSSSol finds the solution with a SAT/SMT solver.
"""

import os
import subprocess
import sys
import tempfile
import time
from typing import Any, Mapping, Optional, Sequence
import z3


def var_given(
    data: dict[
        str,
        Any,
    ],
    arr: str,
    i: int,
    j: int,
    k: int,
    l: int | None = None,
) -> bool:
  """Check whether data[arr][i][j][k]([l]) is given.

  If the given indices are not found, or the number is neither 0 nor 1, return
  False; otherwise return True.

  Args:
      data (dict[ str, Any, ]): contain arrays
      arr (str): ExistI, etc.
      i (int): first index
      j (int): second index
      k (int): third index
      l (int | None, optional): optional fourth index. Defaults to None.

  Returns:
      bool: whether the variable value is given
  """

  if arr not in data:
    return False
  if i not in range(len(data[arr])):
    return False
  if j not in range(len(data[arr][i])):
    return False
  if k not in range(len(data[arr][i][j])):
    return False
  if l is None:
    if data[arr][i][j][k] != 0 and data[arr][i][j][k] != 1:
      return False
    return True
  # l is not None, then
  if l not in range(len(data[arr][i][j][k])):
    return False
  if data[arr][i][j][k][l] != 0 and data[arr][i][j][k][l] != 1:
    return False
  return True


def port_incident_edges(
    port: Mapping[str, str | int], n_i: int, n_j: int, n_k: int
) -> tuple[Sequence[str], Sequence[tuple[int, int, int]]]:
  """Compute the edges incident to a port.

  A port is an edge with a dangling end. The incident edges of a port are the
  five other edges connecting to that end. However, some of these edges can be
  out of bound, we just want to compute those that are in bound.

  Args:
      port (Mapping[str, str  |  int]): the port to consider
      n_i (int): spatial bound on I direction
      n_j (int): spatial bound on J direction
      n_k (int): spatial bound on K direction

  Returns:
      tuple[Sequence[str], Sequence[tuple[int, int, int]]]:
        Two lists of the same length [0,6): (dirs, coords)
        dirs: the direction of the incident edges, can be 'I', 'J', or 'K'
        coords: the coordinates of the incident edges, each one is (i,j,k)
  """
  coords = []
  dirs = []

  # first, just consider adjancency without caring about out-of-bound
  if port['d'] == 'I':
    adj_dirs = ['I', 'J', 'J', 'K', 'K']
    if port['e'] == '-':  # empty cube is (i,j,k)
      adj_coords = [
          (port['i'] - 1, port['j'], port['k']),  # (i-1,j,k)---(i,j,k)
          (port['i'], port['j'] - 1, port['k']),  # (i,j-1,k)---(i,j,k)
          (port['i'], port['j'], port['k']),  # (i,j,k)---(i+1,j,k)
          (port['i'], port['j'], port['k'] - 1),  # (i,j,k-1)---(i,j,k)
          (port['i'], port['j'], port['k']),  # (i,j,k)---(i,j,k+1)
      ]
    elif port['e'] == '+':  # empty cube is (i+1,j,k)
      adj_coords = [
          (port['i'] + 1, port['j'], port['k']),  # (i+1,j,k)---(i+2,j,k)
          (port['i'] + 1, port['j'] - 1, port['k']),  # (i+1,j-1,k)---(i+1,j,k)
          (port['i'] + 1, port['j'], port['k']),  # (i+1,j,k)---(i+1,j+1,k)
          (port['i'] + 1, port['j'], port['k'] - 1),  # (i+1,j,k-1)---(i+1,j,k)
          (port['i'] + 1, port['j'], port['k']),  # (i+1,j,k)---(i+1,j,k+1)
      ]

  if port['d'] == 'J':
    adj_dirs = ['J', 'K', 'K', 'I', 'I']
    if port['e'] == '-':
      adj_coords = [
          (port['i'], port['j'] - 1, port['k']),
          (port['i'], port['j'], port['k'] - 1),
          (port['i'], port['j'], port['k']),
          (port['i'] - 1, port['j'], port['k']),
          (port['i'], port['j'], port['k']),
      ]
    elif port['e'] == '+':
      adj_coords = [
          (port['i'], port['j'] + 1, port['k']),
          (port['i'], port['j'] + 1, port['k'] - 1),
          (port['i'], port['j'] + 1, port['k']),
          (port['i'] - 1, port['j'] + 1, port['k']),
          (port['i'], port['j'] + 1, port['k']),
      ]

  if port['d'] == 'K':
    adj_dirs = ['K', 'I', 'I', 'J', 'J']
    if port['e'] == '-':
      adj_coords = [
          (port['i'], port['j'], port['k'] - 1),
          (port['i'] - 1, port['j'], port['k']),
          (port['i'], port['j'], port['k']),
          (port['i'], port['j'] - 1, port['k']),
          (port['i'], port['j'], port['k']),
      ]
    elif port['e'] == '+':
      adj_coords = [
          (port['i'], port['j'], port['k'] + 1),
          (port['i'] - 1, port['j'], port['k'] + 1),
          (port['i'], port['j'], port['k'] + 1),
          (port['i'], port['j'] - 1, port['k'] + 1),
          (port['i'], port['j'], port['k'] + 1),
      ]

  # only keep the edges in bound
  for i, coord in enumerate(adj_coords):
    if (
        (coord[0] in range(n_i))
        and (coord[1] in range(n_j))
        and (coord[2] in range(n_k))
    ):
      coords.append(adj_coords[i])
      dirs.append(adj_dirs[i])

  return dirs, coords


def cnf_even_parity_upto4(eles: Sequence[Any]) -> Any:
  """Compute the CNF format of parity of up to four Z3 binary variables.

  Args:
      eles (Sequence[Any]): the binary variables

  Returns:
      (Any) the Z3 constraint meaning the parity of the inputs is even.
  """

  if len(eles) == 1:  # 1 var even parity -> this var is false
    return z3.Not(eles[0])
  if len(eles) == 2:  # 2 vars even pairty -> both True or both False
    return z3.Or(
        z3.And(z3.Not(eles[0]), z3.Not(eles[1])), z3.And(eles[0], eles[1])
    )
  if len(eles) == 3:  # 3 vars even parity -> all False, or 2 True and 1 False
    return z3.Or(
        z3.And(z3.Not(eles[0]), z3.Not(eles[1]), z3.Not(eles[2])),
        z3.And(eles[0], eles[1], z3.Not(eles[2])),
        z3.And(eles[0], z3.Not(eles[1]), eles[2]),
        z3.And(z3.Not(eles[0]), eles[1], eles[2]),
    )
  if len(eles) == 4:  # 4 vars even parity -> 0, 2, or 4 vars are True
    return z3.Or(
        z3.And(
            z3.Not(eles[0]), z3.Not(eles[1]), z3.Not(eles[2]), z3.Not(eles[3])
        ),
        z3.And(z3.Not(eles[0]), z3.Not(eles[1]), eles[2], eles[3]),
        z3.And(z3.Not(eles[0]), eles[1], z3.Not(eles[2]), eles[3]),
        z3.And(z3.Not(eles[0]), eles[1], eles[2], z3.Not(eles[3])),
        z3.And(eles[0], z3.Not(eles[1]), z3.Not(eles[2]), eles[3]),
        z3.And(eles[0], z3.Not(eles[1]), eles[2], z3.Not(eles[3])),
        z3.And(eles[0], eles[1], z3.Not(eles[2]), z3.Not(eles[3])),
        z3.And(eles[0], eles[1], eles[2], eles[3]),
    )


class OptimalLatticeSurgerySubroutineSolver:
  """class of compiling LaSSIR using Z3 SMT solver."""

  def __init__(
      self,
      input_dict: Mapping[str, Any],
      given_arrs: Mapping[str, Any] | None = None,
      given_vals: Sequence[Mapping[str, Any]] | None = None,
      color_ij: bool = True,
  ) -> None:
    """initialization of OLSSSol objects.

    Args:
        input_dict (Mapping[str, Any]): specification of LSS
        given_arrs (Mapping[str, Any], optional): Arrays of values to plug in.
          Defaults to None.
        given_vals (Sequence[Mapping[str, Any]], optional): Values to plug in.
          Defaults to None.
        color_ij (bool, optional): if the color matching constraints of I and J
          edges are imposed. Defaults to False.
    """
    self.input_dict = input_dict
    self.color_ij = color_ij
    self.goal = z3.Goal()
    self.process_input(input_dict)
    self.build_smt_model(
        given_arrs=given_arrs, given_vals=given_vals, color_ij=color_ij
    )

  def process_input(self, input_dict: Mapping[str, Any]) -> None:
    """read input file, mainly translating the info of the ports.

    Args:
        input_dict (Mapping[str, Any]):

    Raises:
        ValueError: no input specification given
        ValueError: missing key in input
        ValueError: some spatial bound <= 0
        ValueError: more stabilizers than ports
        ValueError: stabilizer length is not the same as the number of ports
        ValueError: stabilizer contain things other than I, X, Y, or Z
        ValueError: missing key in port
        ValueError: port location is not a 3-tuple
        ValueError: port direction is not 2-string
        ValueError: port sign (which end is dangling) is not - or +
        ValueError: port axis is not I, J, or K
        ValueError: port Z basis direction is not I, J, or K, and not the same
        with the edge
    """
    data = input_dict

    if not input_dict:
      raise ValueError('no input specification given.')

    for key in ['max_i', 'max_j', 'max_k', 'ports', 'stabilizers']:
      if key not in data:
        raise ValueError(f'missing key {key} in input.')

    # load spatial bound, check > 0
    self.n_i = data['max_i']
    self.n_j = data['max_j']
    self.n_k = data['max_k']
    if min([self.n_i, self.n_j, self.n_k]) <= 0:
      raise ValueError('max_i or _j or _k <= 0.')

    self.n_p = len(data['ports'])
    self.n_s = len(data['stabilizers'])
    # there's at most as many stabilizers as ports
    if self.n_s > self.n_p:
      raise ValueError('Too many stabilizers compated to ports.')

    # stabilizers should be paulistrings of length #ports
    self.paulistings = [s.replace('.', 'I') for s in data['stabilizers']]
    for s in self.paulistings:
      if len(s) != self.n_p:
        raise ValueError(f'invalid stabilizer length {s}.')
      for i in range(len(s)):
        if s[i] not in ['I', 'X', 'Y', 'Z']:
          raise ValueError(f'invalid stabilizer pauli {s}.')

    # transform port data
    self.ports = []
    for port in data['ports']:
      my_port = {}

      for key in ['location', 'direction', 'z_basis_direction']:
        if key not in port:
          raise ValueError(f'missing key {key} in port {port}')

      if len(port['location']) != 3:
        raise ValueError(f'port with invalid location {port}.')
      my_port['i'], my_port['j'], my_port['k'] = port['location']

      if len(port['direction']) != 2:
        raise ValueError(f'port with invalid direction {port}.')
      if port['direction'][0] not in ['+', '-']:
        raise ValueError(f'port with invalid sign {port}.')
      if port['direction'][1] not in ['I', 'J', 'K']:
        raise ValueError(f'port with invalid axis {port}.')
      my_port['e'] = '-' if port['direction'][0] == '+' else '+'
      my_port['d'] = port['direction'][1]
      if my_port['e'] == '+':
        # the corresponding coordinate should -1, e.g., if axis is K and
        # it's minus direction, then edge should be (i,j,k-1)-(i,j,k)
        my_port[my_port['d'].lower()] -= 1

      z_dir = port['z_basis_direction']
      if z_dir not in ['I', 'J', 'K'] or z_dir == my_port['d']:
        raise ValueError(f'port with invalid Z basis direction {port}.')
      if my_port['d'] == 'I':
        my_port['c'] = 0 if z_dir == 'J' else 1
      if my_port['d'] == 'J':
        my_port['c'] = 0 if z_dir == 'K' else 1
      if my_port['d'] == 'K':
        my_port['c'] = 0 if z_dir == 'I' else 1

      if 'function' in port:
        my_port['f'] = port['function']

      self.ports.append(my_port)

    # from paulistrings to correlation surfaces
    self.stabs = self.derive_corr_boundary(self.paulistings)

    self.optional = {}
    self.forbidden_cubes = []
    if 'optional' in data:
      self.optional = data['optional']

      if 'forbidden_cubes' in data['optional']:
        for cube in data['optional']['forbidden_cubes']:
          if len(cube) != 3:
            raise ValueError(f'forbid cube invalid {cube}.')
          if (
              cube[0] not in range(self.n_i)
              or cube[1] not in range(self.n_j)
              or cube[2] not in range(self.n_k)
          ):
            continue
          self.forbidden_cubes.append(cube)

    self.get_port_cubes()

  def derive_corr_boundary(
      self, paulistrings: Sequence[str]
  ) -> list[list[Mapping[str, int]]]:
    """derive the boundary correlation surface variable values.

    From the color orientation of the ports and the stabilizers, we can derive
    which correlation surface variables evaluates to True and which to False at
    the ports for each stabilizer.

    Args:
        paulistrings (Sequence[str]): stabilizers as a list of Pauli strings.

    Returns:
        list[list[Mapping[str, int]]]: Outer layer list is the list of
        stabilizers. Inner layer list is the situation at each port for one
        specifeic stabilizer. Each port is specified with a dictionary of 2 bits
        for the 2 correaltion surfaces.
    """
    stabs = []
    for paulistring in paulistrings:
      corr = []
      for p in range(self.n_p):
        if paulistring[p] == 'I':  # I -> no corr surf should be present
          if self.ports[p]['d'] == 'I':
            corr.append({'IJ': 0, 'IK': 0})
          if self.ports[p]['d'] == 'J':
            corr.append({'JI': 0, 'JK': 0})
          if self.ports[p]['d'] == 'K':
            corr.append({'KI': 0, 'KJ': 0})

        if paulistring[p] == 'Y':  # Y -> both corr surf should be present
          if self.ports[p]['d'] == 'I':
            corr.append({'IJ': 1, 'IK': 1})
          if self.ports[p]['d'] == 'J':
            corr.append({'JI': 1, 'JK': 1})
          if self.ports[p]['d'] == 'K':
            corr.append({'KI': 1, 'KJ': 1})

        if paulistring[p] == 'X':  # X -> only corr surf touching red faces
          if self.ports[p]['d'] == 'I':
            if self.ports[p]['c']:
              corr.append({'IJ': 1, 'IK': 0})
            else:
              corr.append({'IJ': 0, 'IK': 1})
          if self.ports[p]['d'] == 'J':
            if self.ports[p]['c']:
              corr.append({'JI': 0, 'JK': 1})
            else:
              corr.append({'JI': 1, 'JK': 0})
          if self.ports[p]['d'] == 'K':
            if self.ports[p]['c']:
              corr.append({'KI': 1, 'KJ': 0})
            else:
              corr.append({'KI': 0, 'KJ': 1})

        if paulistring[p] == 'Z':  # Z -> only corr surf touching blue faces
          if self.ports[p]['d'] == 'I':
            if not self.ports[p]['c']:
              corr.append({'IJ': 1, 'IK': 0})
            else:
              corr.append({'IJ': 0, 'IK': 1})
          if self.ports[p]['d'] == 'J':
            if not self.ports[p]['c']:
              corr.append({'JI': 0, 'JK': 1})
            else:
              corr.append({'JI': 1, 'JK': 0})
          if self.ports[p]['d'] == 'K':
            if not self.ports[p]['c']:
              corr.append({'KI': 1, 'KJ': 0})
            else:
              corr.append({'KI': 0, 'KJ': 1})
      stabs.append(corr)
    return stabs

  def define_vars(self, color_ij: bool = True) -> None:
    """define the variables in Z3 into self.vars.

    Args:
        color_ij (bool, optional): if the color matching constraints of I and J
          edges are imposed. Defaults to True.
    """
    self.vars = {
        'ExistI': [
            [
                [z3.Bool(f'ExistI({i},{j},{k})') for k in range(self.n_k)]
                for j in range(self.n_j)
            ]
            for i in range(self.n_i)
        ],
        'ExistJ': [
            [
                [z3.Bool(f'ExistJ({i},{j},{k})') for k in range(self.n_k)]
                for j in range(self.n_j)
            ]
            for i in range(self.n_i)
        ],
        'ExistK': [
            [
                [z3.Bool(f'ExistK({i},{j},{k})') for k in range(self.n_k)]
                for j in range(self.n_j)
            ]
            for i in range(self.n_i)
        ],
        'NodeY': [
            [
                [z3.Bool(f'NodeY({i},{j},{k})') for k in range(self.n_k)]
                for j in range(self.n_j)
            ]
            for i in range(self.n_i)
        ],
        'CorrIJ': [
            [
                [
                    [
                        z3.Bool(f'CorrIJ({s},{i},{j},{k})')
                        for k in range(self.n_k)
                    ]
                    for j in range(self.n_j)
                ]
                for i in range(self.n_i)
            ]
            for s in range(self.n_s)
        ],
        'CorrIK': [
            [
                [
                    [
                        z3.Bool(f'CorrIK({s},{i},{j},{k})')
                        for k in range(self.n_k)
                    ]
                    for j in range(self.n_j)
                ]
                for i in range(self.n_i)
            ]
            for s in range(self.n_s)
        ],
        'CorrJK': [
            [
                [
                    [
                        z3.Bool(f'CorrJK({s},{i},{j},{k})')
                        for k in range(self.n_k)
                    ]
                    for j in range(self.n_j)
                ]
                for i in range(self.n_i)
            ]
            for s in range(self.n_s)
        ],
        'CorrJI': [
            [
                [
                    [
                        z3.Bool(f'CorrJI({s},{i},{j},{k})')
                        for k in range(self.n_k)
                    ]
                    for j in range(self.n_j)
                ]
                for i in range(self.n_i)
            ]
            for s in range(self.n_s)
        ],
        'CorrKI': [
            [
                [
                    [
                        z3.Bool(f'CorrKI({s},{i},{j},{k})')
                        for k in range(self.n_k)
                    ]
                    for j in range(self.n_j)
                ]
                for i in range(self.n_i)
            ]
            for s in range(self.n_s)
        ],
        'CorrKJ': [
            [
                [
                    [
                        z3.Bool(f'CorrKJ({s},{i},{j},{k})')
                        for k in range(self.n_k)
                    ]
                    for j in range(self.n_j)
                ]
                for i in range(self.n_i)
            ]
            for s in range(self.n_s)
        ],
    }

    if color_ij:
      self.vars['ColorI'] = [
          [
              [z3.Bool(f'ColorI({i},{j},{k})') for k in range(self.n_k)]
              for j in range(self.n_j)
          ]
          for i in range(self.n_i)
      ]
      self.vars['ColorJ'] = [
          [
              [z3.Bool(f'ColorJ({i},{j},{k})') for k in range(self.n_k)]
              for j in range(self.n_j)
          ]
          for i in range(self.n_i)
      ]

  def plugin_arrs(self, data: Mapping[str, Any], color_ij: bool = True) -> None:
    """plug in the given arrays of values.

    Args:
        data (Mapping[str, Any]): contains gieven values
        color_ij (bool, optional): if the color matching constraints of I and J
          edges are imposed. Defaults to True.
    """
    arrs = [
        'NodeY',
        'ExistI',
        'ExistJ',
        'ExistK',
    ]
    if color_ij:
      arrs += ['ColorI', 'ColorJ']

    for s in range(self.n_s):
      for i in range(self.n_i):
        for j in range(self.n_j):
          for k in range(self.n_k):
            if s == 0:  # Exist, Node, and Color vars
              for arr in arrs:
                if var_given(data, arr, i, j, k):
                  self.goal.add(
                      self.vars[arr][i][j][k]
                      if data[arr][i][j][k] == 1
                      else z3.Not(self.vars[arr][i][j][k])
                  )
            # Corr vars
            for arr in [
                'CorrIJ',
                'CorrIK',
                'CorrJI',
                'CorrJK',
                'CorrKI',
                'CorrKJ',
            ]:
              if var_given(data, arr, s, i, j, k):
                self.goal.add(
                    self.vars[arr][s][i][j][k]
                    if data[arr][s][i][j][k]
                    else z3.Not(self.vars[arr][s][i][j][k])
                )

  def plugin_vals(self, data_set: Sequence[Mapping[str, Any]]):
    """plug in the given values

    Args:
        data (Sequence[Mapping[str, Any]]): given values as a sequence of dicts.
          Each one contains three fields: 'array', the name of the array, e.g.,
          'ExistI'; 'indices', a sequence of the indices; and 'value'.
    """
    for data in data_set:
      (arr, idx) = data['array'], data['indices']
      if arr.startswith('Corr'):
        s, i, j, k = idx
        if data['value']:
          self.goal.add(self.vars[arr][s][i][j][k])
        else:
          self.goal.add(z3.Not(self.vars[arr][s][i][j][k]))
      else:
        i, j, k = idx
        if data['value']:
          self.goal.add(self.vars[arr][i][j][k])
        else:
          self.goal.add(z3.Not(self.vars[arr][i][j][k]))

  def get_port_cubes(self) -> None:
    """calculate which cubes are empty serving as ports."""
    self.port_cubes = []
    for p in self.ports:
      # if e=-, (i,j,k); otherwise, +1 in the proper direction
      if p['e'] == '-':
        self.port_cubes.append((p['i'], p['j'], p['k']))
      elif p['d'] == 'I':
        self.port_cubes.append((p['i'] + 1, p['j'], p['k']))
      elif p['d'] == 'J':
        self.port_cubes.append((p['i'], p['j'] + 1, p['k']))
      elif p['d'] == 'K':
        self.port_cubes.append((p['i'], p['j'], p['k'] + 1))

  def constraint_forbid_cube(self) -> None:
    """forbid a list of cubes."""
    for cube in self.forbidden_cubes:
      (i, j, k) = cube[0], cube[1], cube[2]
      if (
          len(cube) != 3
          or i not in range(self.n_i)
          or j not in range(self.n_j)
          or k not in range(self.n_k)
      ):
        continue
      self.goal.add(z3.Not(self.vars['NodeY'][i][j][k]))
      if i > 0:
        self.goal.add(z3.Not(self.vars['ExistI'][i - 1][j][k]))
      self.goal.add(z3.Not(self.vars['ExistI'][i][j][k]))
      if j > 0:
        self.goal.add(z3.Not(self.vars['ExistJ'][i][j - 1][k]))
      self.goal.add(z3.Not(self.vars['ExistJ'][i][j][k]))
      # if k > 0:
      #   self.model.add(z3.Not(self.vars['ExistK'][i][j][k - 1]))
      # self.model.add(z3.Not(self.vars['ExistK'][i][j][k]))

  def constraint_port(self, color_ij: bool = True) -> None:
    """structural constraints at ports: some edges must exist and some must not.

    Args:
        color_ij (bool, optional): if the color matching constraints of I and J
          edges are imposed. Defaults to True.
    """
    for port in self.ports:
      # the edge specified by the port exists
      self.goal.add(
          self.vars[f'Exist{port["d"]}'][port['i']][port['j']][port['k']]
      )
      # if I- or J-edge exist, set the color value too to the given one
      if color_ij:
        if port['d'] != 'K':
          if port['c'] == 1:
            self.goal.add(
                self.vars[f'Color{port["d"]}'][port['i']][port['j']][port['k']]
            )
          else:
            self.goal.add(
                z3.Not(
                    self.vars[f'Color{port["d"]}'][port['i']][port['j']][
                        port['k']
                    ]
                )
            )

      # collect the edges touching the port to forbid them
      dirs, coords = port_incident_edges(port, self.n_i, self.n_j, self.n_k)
      for i, coord in enumerate(coords):
        self.goal.add(
            z3.Not(self.vars[f'Exist{dirs[i]}'][coord[0]][coord[1]][coord[2]])
        )

  def constraint_connect_outside(self) -> None:
    """no edge should cross the spatial bound except for ports."""
    for i in range(self.n_i):
      for j in range(self.n_j):
        # consider K-edges crossing K-bound and not a port
        if (i, j, self.n_k) not in self.port_cubes:
          self.goal.add(z3.Not(self.vars['ExistK'][i][j][self.n_k - 1]))
    for i in range(self.n_i):
      for k in range(self.n_k):
        if (i, self.n_j, k) not in self.port_cubes:
          self.goal.add(z3.Not(self.vars['ExistJ'][i][self.n_j - 1][k]))
    for j in range(self.n_j):
      for k in range(self.n_k):
        if (self.n_i, j, k) not in self.port_cubes:
          self.goal.add(z3.Not(self.vars['ExistI'][self.n_i - 1][j][k]))

  def constraint_timelike_y(self) -> None:
    """Y-spiders must be timelike, i.e., forbid all I- and J- edges to them."""
    for i in range(self.n_i):
      for j in range(self.n_j):
        for k in range(self.n_k):
          if (i, j, k) not in self.port_cubes:
            self.goal.add(
                z3.Implies(
                    self.vars['NodeY'][i][j][k],
                    z3.Not(self.vars['ExistI'][i][j][k]),
                )
            )
            self.goal.add(
                z3.Implies(
                    self.vars['NodeY'][i][j][k],
                    z3.Not(self.vars['ExistJ'][i][j][k]),
                )
            )
            if i - 1 >= 0:
              self.goal.add(
                  z3.Implies(
                      self.vars['NodeY'][i][j][k],
                      z3.Not(self.vars['ExistI'][i - 1][j][k]),
                  )
              )
            if j - 1 >= 0:
              self.goal.add(
                  z3.Implies(
                      self.vars['NodeY'][i][j][k],
                      z3.Not(self.vars['ExistJ'][i][j - 1][k]),
                  )
              )

  def constraint_no_deg1(self) -> None:
    """forbid degree-1 X or Z spiders by considering incident edges."""
    for i in range(self.n_i):
      for j in range(self.n_j):
        for k in range(self.n_k):
          for d in ['I', 'J', 'K']:
            for e in ['-', '+']:
              cube = {'I': i, 'J': j, 'K': k}
              cube[d] += 1 if e == '+' else 0

              # construct fake ports to get incident edges
              p0 = {'i': i, 'j': j, 'k': k, 'd': d, 'e': e, 'c': 0}
              found_p0 = False
              for port in self.ports:
                if (
                    i == port['i']
                    and j == port['j']
                    and k == port['k']
                    and d == port['d']
                ):
                  found_p0 = True

              # only non-port edges need to consider
              if (
                  not found_p0
                  and cube['I'] < self.n_i
                  and cube['J'] < self.n_j
                  and cube['K'] < self.n_k
              ):
                # only cubes inside bound need to consider
                dirs, coords = port_incident_edges(
                    p0, self.n_i, self.n_j, self.n_k
                )
                edges = [
                    self.vars[f'Exist{dirs[l]}'][coord[0]][coord[1]][coord[2]]
                    for l, coord in enumerate(coords)
                ]
                # if the cube is not Y and the edge exist, then
                # at least one of its incident edges exists.
                self.goal.add(
                    z3.Implies(
                        z3.And(
                            z3.Not(
                                self.vars['NodeY'][cube['I']][cube['J']][
                                    cube['K']
                                ]
                            ),
                            self.vars[f'Exist{d}'][i][j][k],
                        ),
                        z3.Or(edges),
                    )
                )

  def constraint_ij_color(self) -> None:
    """color matching for I- and J-edges."""
    for i in range(self.n_i):
      for j in range(self.n_j):
        for k in range(self.n_k):
          if i >= 1 and j >= 1:
            # (i-1,j,k)-(i,j,k) and (i,j-1,k)-(i,j,k)
            self.goal.add(
                z3.Implies(
                    z3.And(
                        self.vars['ExistI'][i - 1][j][k],
                        self.vars['ExistJ'][i][j - 1][k],
                    ),
                    z3.Or(
                        z3.And(
                            self.vars['ColorI'][i - 1][j][k],
                            z3.Not(self.vars['ColorJ'][i][j - 1][k]),
                        ),
                        z3.And(
                            z3.Not(self.vars['ColorI'][i - 1][j][k]),
                            self.vars['ColorJ'][i][j - 1][k],
                        ),
                    ),
                )
            )

          if i >= 1:
            # (i-1,j,k)-(i,j,k) and (i,j,k)-(i,j+1,k)
            self.goal.add(
                z3.Implies(
                    z3.And(
                        self.vars['ExistI'][i - 1][j][k],
                        self.vars['ExistJ'][i][j][k],
                    ),
                    z3.Or(
                        z3.And(
                            self.vars['ColorI'][i - 1][j][k],
                            z3.Not(self.vars['ColorJ'][i][j][k]),
                        ),
                        z3.And(
                            z3.Not(self.vars['ColorI'][i - 1][j][k]),
                            self.vars['ColorJ'][i][j][k],
                        ),
                    ),
                )
            )
            # (i-1,j,k)-(i,j,k) and (i,j,k)-(i+1,j,k)
            self.goal.add(
                z3.Implies(
                    z3.And(
                        self.vars['ExistI'][i - 1][j][k],
                        self.vars['ExistI'][i][j][k],
                    ),
                    z3.Or(
                        z3.And(
                            self.vars['ColorI'][i - 1][j][k],
                            self.vars['ColorI'][i][j][k],
                        ),
                        z3.And(
                            z3.Not(self.vars['ColorI'][i - 1][j][k]),
                            z3.Not(self.vars['ColorI'][i][j][k]),
                        ),
                    ),
                )
            )

          if j >= 1:
            # (i,j,k)-(i+1,j,k) and (i,j-1,k)-(i,j,k)
            self.goal.add(
                z3.Implies(
                    z3.And(
                        self.vars['ExistI'][i][j][k],
                        self.vars['ExistJ'][i][j - 1][k],
                    ),
                    z3.Or(
                        z3.And(
                            self.vars['ColorI'][i][j][k],
                            z3.Not(self.vars['ColorJ'][i][j - 1][k]),
                        ),
                        z3.And(
                            z3.Not(self.vars['ColorI'][i][j][k]),
                            self.vars['ColorJ'][i][j - 1][k],
                        ),
                    ),
                )
            )
            # (i,j-1,k)-(i,j,k) and (i,j,k)-(i,j+1,k)
            self.goal.add(
                z3.Implies(
                    z3.And(
                        self.vars['ExistJ'][i][j - 1][k],
                        self.vars['ExistJ'][i][j][k],
                    ),
                    z3.Or(
                        z3.And(
                            self.vars['ColorJ'][i][j - 1][k],
                            self.vars['ColorJ'][i][j][k],
                        ),
                        z3.And(
                            z3.Not(self.vars['ColorJ'][i][j - 1][k]),
                            z3.Not(self.vars['ColorJ'][i][j][k]),
                        ),
                    ),
                )
            )

          # (i,j,k)-(i+1,j,k) and (i,j,k)-(i,j+1,k)
          self.goal.add(
              z3.Implies(
                  z3.And(
                      self.vars['ExistI'][i][j][k], self.vars['ExistJ'][i][j][k]
                  ),
                  z3.Or(
                      z3.And(
                          self.vars['ColorI'][i][j][k],
                          z3.Not(self.vars['ColorJ'][i][j][k]),
                      ),
                      z3.And(
                          z3.Not(self.vars['ColorI'][i][j][k]),
                          self.vars['ColorJ'][i][j][k],
                      ),
                  ),
              )
          )

  def constraint_3d_corner(self) -> None:
    """forbid 3D corners by saying at least in one direction, both edges nonexist."""
    for i in range(self.n_i):
      for j in range(self.n_j):
        for k in range(self.n_k):
          i_edges = [
              self.vars['ExistI'][i][j][k],
          ]
          if i - 1 >= 0:
            i_edges.append(self.vars['ExistI'][i - 1][j][k])
          j_edges = [
              self.vars['ExistJ'][i][j][k],
          ]
          if j - 1 >= 0:
            j_edges.append(self.vars['ExistJ'][i][j - 1][k])
          k_edges = [
              self.vars['ExistK'][i][j][k],
          ]
          if k - 1 >= 0:
            k_edges.append(self.vars['ExistK'][i][j][k - 1])

          # at least one of the three terms is true. The first term is that
          # both I-edges connecting to (i,j,k) do not exist
          self.goal.add(
              z3.Or(
                  z3.Not(z3.Or(i_edges)),
                  z3.Not(z3.Or(j_edges)),
                  z3.Not(z3.Or(k_edges)),
              )
          )

  def constraint_corr_ports(self) -> None:
    """plug in the correlation surface values at the ports."""
    for s, stab in enumerate(self.stabs):
      for p, corrs in enumerate(stab):
        for k, v in corrs.items():
          if v == 1:
            self.goal.add(
                self.vars[f'Corr{k}'][s][self.ports[p]['i']][
                    self.ports[p]['j']
                ][self.ports[p]['k']]
            )
          else:
            self.goal.add(
                z3.Not(
                    self.vars[f'Corr{k}'][s][self.ports[p]['i']][
                        self.ports[p]['j']
                    ][self.ports[p]['k']]
                )
            )

  def constraint_corr_y(self) -> None:
    """correlation surfaces at Y-spider should both exist or nonexist."""
    for s in range(self.n_s):
      for i in range(self.n_i):
        for j in range(self.n_j):
          for k in range(self.n_k):
            self.goal.add(
                z3.Or(
                    z3.Not(self.vars['NodeY'][i][j][k]),
                    z3.Or(
                        z3.And(
                            self.vars['CorrKI'][s][i][j][k],
                            self.vars['CorrKJ'][s][i][j][k],
                        ),
                        z3.And(
                            z3.Not(self.vars['CorrKI'][s][i][j][k]),
                            z3.Not(self.vars['CorrKJ'][s][i][j][k]),
                        ),
                    ),
                )
            )
            if k - 1 >= 0:
              self.goal.add(
                  z3.Or(
                      z3.Not(self.vars['NodeY'][i][j][k]),
                      z3.Or(
                          z3.And(
                              self.vars['CorrKI'][s][i][j][k - 1],
                              self.vars['CorrKJ'][s][i][j][k - 1],
                          ),
                          z3.And(
                              z3.Not(self.vars['CorrKI'][s][i][j][k - 1]),
                              z3.Not(self.vars['CorrKJ'][s][i][j][k - 1]),
                          ),
                      ),
                  )
              )

  def constraint_corr_perp(self) -> None:
    """for corr surf perpendicular to normal vector, all exist or nonexist."""
    for s in range(self.n_s):
      for i in range(self.n_i):
        for j in range(self.n_j):
          for k in range(self.n_k):
            if (i, j, k) not in self.port_cubes:  # only internal spider
              # only consider X or Z spider
              # if normal is K meaning meaning both (i,j,k)-(i,j,k+1) and
              # (i,j,k)-(i,j,k-1) are out of range, or in range but nonexistent
              normal = z3.And(
                  z3.Not(self.vars['NodeY'][i][j][k]),
                  z3.Not(self.vars['ExistK'][i][j][k]),
              )
              if k - 1 >= 0:
                normal = z3.And(
                    normal, z3.Not(self.vars['ExistK'][i][j][k - 1])
                )

              # for other edges, need to build intermediate expression
              # for (i,j,k)-(i+1,j,k) and (i,j,k)-(i,j+1,k), build expression
              # meaning the edge is nonexistent or exist and has the correlation
              # surface perpendicular to the normal vector in them.
              no_edge_or_with_corr = [
                  z3.Or(
                      z3.Not(self.vars['ExistI'][i][j][k]),
                      self.vars['CorrIJ'][s][i][j][k],
                  ),
                  z3.Or(
                      z3.Not(self.vars['ExistJ'][i][j][k]),
                      self.vars['CorrJI'][s][i][j][k],
                  ),
              ]

              # for (i,j,k)-(i+1,j,k) and (i,j,k)-(i,j+1,k), build expression
              # meaning the edge is nonexistent or exist and does not have the
              # correlation surface perpendicular to the normal vector in them.
              no_edge_or_no_corr = [
                  z3.Or(
                      z3.Not(self.vars['ExistI'][i][j][k]),
                      z3.Not(self.vars['CorrIJ'][s][i][j][k]),
                  ),
                  z3.Or(
                      z3.Not(self.vars['ExistJ'][i][j][k]),
                      z3.Not(self.vars['CorrJI'][s][i][j][k]),
                  ),
              ]

              if i - 1 >= 0:
                # build intermediate expression for (i-1,j,k)-(i,j,k)
                no_edge_or_with_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistI'][i - 1][j][k]),
                        self.vars['CorrIJ'][s][i - 1][j][k],
                    )
                )
                no_edge_or_no_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistI'][i - 1][j][k]),
                        z3.Not(self.vars['CorrIJ'][s][i - 1][j][k]),
                    )
                )

              if j - 1 >= 0:
                # build intermediate expression for (i,j-1,k)-(i,j,k)
                no_edge_or_with_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistJ'][i][j - 1][k]),
                        self.vars['CorrJI'][s][i][j - 1][k],
                    )
                )
                no_edge_or_no_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistJ'][i][j - 1][k]),
                        z3.Not(self.vars['CorrJI'][s][i][j - 1][k]),
                    )
                )

              # if normal vector is K, then in all its incident edges that exist
              # all correlation surface in I-J plane exist or nonexist
              self.goal.add(
                  z3.Implies(
                      normal,
                      z3.Or(
                          z3.And(no_edge_or_with_corr),
                          z3.And(no_edge_or_no_corr),
                      ),
                  )
              )

              # if normal is I
              normal = z3.And(
                  z3.Not(self.vars['NodeY'][i][j][k]),
                  z3.Not(self.vars['ExistI'][i][j][k]),
              )
              if i - 1 >= 0:
                normal = z3.And(
                    normal, z3.Not(self.vars['ExistI'][i - 1][j][k])
                )
              no_edge_or_with_corr = [
                  z3.Or(
                      z3.Not(self.vars['ExistJ'][i][j][k]),
                      self.vars['CorrJK'][s][i][j][k],
                  ),
                  z3.Or(
                      z3.Not(self.vars['ExistK'][i][j][k]),
                      self.vars['CorrKJ'][s][i][j][k],
                  ),
              ]
              no_edge_or_no_corr = [
                  z3.Or(
                      z3.Not(self.vars['ExistJ'][i][j][k]),
                      z3.Not(self.vars['CorrJK'][s][i][j][k]),
                  ),
                  z3.Or(
                      z3.Not(self.vars['ExistK'][i][j][k]),
                      z3.Not(self.vars['CorrKJ'][s][i][j][k]),
                  ),
              ]
              if j - 1 >= 0:
                no_edge_or_with_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistJ'][i][j - 1][k]),
                        self.vars['CorrJK'][s][i][j - 1][k],
                    )
                )
                no_edge_or_no_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistJ'][i][j - 1][k]),
                        z3.Not(self.vars['CorrJK'][s][i][j - 1][k]),
                    )
                )
              if k - 1 >= 0:
                no_edge_or_with_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistK'][i][j][k - 1]),
                        self.vars['CorrKJ'][s][i][j][k - 1],
                    )
                )
                no_edge_or_no_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistK'][i][j][k - 1]),
                        z3.Not(self.vars['CorrKJ'][s][i][j][k - 1]),
                    )
                )
              self.goal.add(
                  z3.Implies(
                      normal,
                      z3.Or(
                          z3.And(no_edge_or_with_corr),
                          z3.And(no_edge_or_no_corr),
                      ),
                  )
              )

              # if normal is J
              normal = z3.And(
                  z3.Not(self.vars['NodeY'][i][j][k]),
                  z3.Not(self.vars['ExistJ'][i][j][k]),
              )
              if j - 1 >= 0:
                normal = z3.And(
                    normal, z3.Not(self.vars['ExistJ'][i][j - 1][k])
                )
              no_edge_or_with_corr = [
                  z3.Or(
                      z3.Not(self.vars['ExistI'][i][j][k]),
                      self.vars['CorrIK'][s][i][j][k],
                  ),
                  z3.Or(
                      z3.Not(self.vars['ExistK'][i][j][k]),
                      self.vars['CorrKI'][s][i][j][k],
                  ),
              ]
              no_edge_or_no_corr = [
                  z3.Or(
                      z3.Not(self.vars['ExistI'][i][j][k]),
                      z3.Not(self.vars['CorrIK'][s][i][j][k]),
                  ),
                  z3.Or(
                      z3.Not(self.vars['ExistK'][i][j][k]),
                      z3.Not(self.vars['CorrKI'][s][i][j][k]),
                  ),
              ]
              if i - 1 >= 0:
                no_edge_or_with_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistI'][i - 1][j][k]),
                        self.vars['CorrIK'][s][i - 1][j][k],
                    )
                )
                no_edge_or_no_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistI'][i - 1][j][k]),
                        z3.Not(self.vars['CorrIK'][s][i - 1][j][k]),
                    )
                )
              if k - 1 >= 0:
                no_edge_or_with_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistK'][i][j][k - 1]),
                        self.vars['CorrKI'][s][i][j][k - 1],
                    )
                )
                no_edge_or_no_corr.append(
                    z3.Or(
                        z3.Not(self.vars['ExistK'][i][j][k - 1]),
                        z3.Not(self.vars['CorrKI'][s][i][j][k - 1]),
                    )
                )
              self.goal.add(
                  z3.Implies(
                      normal,
                      z3.Or(
                          z3.And(no_edge_or_with_corr),
                          z3.And(no_edge_or_no_corr),
                      ),
                  )
              )

  def constraint_corr_para(self) -> None:
    """for corr surf parallel to the normal vector, even number of them exist."""
    for s in range(self.n_s):
      for i in range(self.n_i):
        for j in range(self.n_j):
          for k in range(self.n_k):
            if (i, j, k) not in self.port_cubes:  # only internal spiders
              # only X or Z spiders, if normal is K
              normal = z3.And(
                  z3.Not(self.vars['NodeY'][i][j][k]),
                  z3.Not(self.vars['ExistK'][i][j][k]),
              )
              if k - 1 >= 0:
                normal = z3.And(
                    normal, z3.Not(self.vars['ExistK'][i][j][k - 1])
                )

              # unlike in constraint_corr_perp, we only care about the cases
              # where the edge exists and the correlation surface parallel to
              # K also is present, so we build intermediate expressions as below
              edge_with_corr = [
                  z3.And(
                      self.vars['ExistI'][i][j][k],
                      self.vars['CorrIK'][s][i][j][k],
                  ),
                  z3.And(
                      self.vars['ExistJ'][i][j][k],
                      self.vars['CorrJK'][s][i][j][k],
                  ),
              ]

              # build the intermediate expression for (i-1,j,k)-(i,j,k)
              if i - 1 >= 0:
                edge_with_corr.append(
                    z3.And(
                        self.vars['ExistI'][i - 1][j][k],
                        self.vars['CorrIK'][s][i - 1][j][k],
                    )
                )

              # build the intermediate expression for (i,j-1,k)-(i,j,k)
              if j - 1 >= 0:
                edge_with_corr.append(
                    z3.And(
                        self.vars['ExistJ'][i][j - 1][k],
                        self.vars['CorrJK'][s][i][j - 1][k],
                    )
                )

              # the parity of the intermediate expressions must be even
              self.goal.add(
                  z3.Implies(normal, cnf_even_parity_upto4(edge_with_corr))
              )

              # if normal is I
              normal = z3.And(
                  z3.Not(self.vars['NodeY'][i][j][k]),
                  z3.Not(self.vars['ExistI'][i][j][k]),
              )
              if i - 1 >= 0:
                normal = z3.And(
                    normal, z3.Not(self.vars['ExistI'][i - 1][j][k])
                )
              edge_with_corr = [
                  z3.And(
                      self.vars['ExistJ'][i][j][k],
                      self.vars['CorrJI'][s][i][j][k],
                  ),
                  z3.And(
                      self.vars['ExistK'][i][j][k],
                      self.vars['CorrKI'][s][i][j][k],
                  ),
              ]
              if j - 1 >= 0:
                edge_with_corr.append(
                    z3.And(
                        self.vars['ExistJ'][i][j - 1][k],
                        self.vars['CorrJI'][s][i][j - 1][k],
                    )
                )
              if k - 1 >= 0:
                edge_with_corr.append(
                    z3.And(
                        self.vars['ExistK'][i][j][k - 1],
                        self.vars['CorrKI'][s][i][j][k - 1],
                    )
                )
              self.goal.add(
                  z3.Implies(normal, cnf_even_parity_upto4(edge_with_corr))
              )

              # if normal is J
              normal = z3.And(
                  z3.Not(self.vars['NodeY'][i][j][k]),
                  z3.Not(self.vars['ExistJ'][i][j][k]),
              )
              if j - 1 >= 0:
                normal = z3.And(
                    normal, z3.Not(self.vars['ExistJ'][i][j - 1][k])
                )
              edge_with_corr = [
                  z3.And(
                      self.vars['ExistI'][i][j][k],
                      self.vars['CorrIJ'][s][i][j][k],
                  ),
                  z3.And(
                      self.vars['ExistK'][i][j][k],
                      self.vars['CorrKJ'][s][i][j][k],
                  ),
              ]
              if i - 1 >= 0:
                edge_with_corr.append(
                    z3.And(
                        self.vars['ExistI'][i - 1][j][k],
                        self.vars['CorrIJ'][s][i - 1][j][k],
                    )
                )
              if k - 1 >= 0:
                edge_with_corr.append(
                    z3.And(
                        self.vars['ExistK'][i][j][k - 1],
                        self.vars['CorrKJ'][s][i][j][k - 1],
                    )
                )
              self.goal.add(
                  z3.Implies(normal, cnf_even_parity_upto4(edge_with_corr))
              )

  def build_smt_model(
      self,
      given_arrs: Mapping[str, Any] = None,
      given_vals: Sequence[Mapping[str, Any]] = None,
      color_ij: bool = True,
  ) -> None:
    """build the SMT model with variables and constraints.

    Args:
        given_arrs (Mapping[str, Any], optional): given arrays of values to plug
          in. Defaults to None.
        given_vals (Sequence[Mapping[str, Any]], optional): given values to plug
          in. Defaults to None.
        color_ij (bool, optional): if the color matching constraints of I and J
          edges are imposed. Defaults to True.
    """
    self.define_vars(color_ij=color_ij)
    if given_arrs:
      self.plugin_arrs(given_arrs, color_ij=color_ij)
    if given_vals:
      self.plugin_vals(given_vals)

    # baseline order
    self.constraint_forbid_cube()
    self.constraint_port(color_ij=color_ij)
    self.constraint_connect_outside()
    self.constraint_timelike_y()
    self.constraint_no_deg1()
    if color_ij:
      self.constraint_ij_color()
    self.constraint_3d_corner()

    self.constraint_corr_ports()
    self.constraint_corr_y()
    self.constraint_corr_perp()
    self.constraint_corr_para()

  def check_z3(self, print_progress: bool = True) -> bool:
    """check whether the built goal in self.goal is satisfiable.

    Args:
        print_progress (bool, optional): if print out the progress made.

    Returns:
        bool: True if SAT, False if UNSAT
    """
    if print_progress:
      print('Construct a Z3 SMT model and solve...')
    start_time = time.time()
    self.solver = z3.Solver()
    self.solver.add(self.goal)
    ifsat = self.solver.check()
    elapsed = time.time() - start_time
    if print_progress:
      print('elapsed time: {:2f}s'.format(elapsed))
    if ifsat == z3.sat:
      if print_progress:
        print('Z3 SAT')
      return True
    else:
      if print_progress:
        print('Z3 UNSAT')
      return False

  def get_result(self) -> Mapping[str, Any]:
    """get the variable values.

    Returns:
        Mapping[str, Any]: output in the LaSSIR format
    """
    model = self.solver.model()
    data = {
        'specification': self.input_dict,
        'n_i': self.n_i,
        'n_j': self.n_j,
        'n_k': self.n_k,
        'n_p': self.n_p,
        'n_s': self.n_s,
        'ports': self.ports,
        'stabs': self.stabs,
        'port_cubes': self.port_cubes,
        'optional': self.optional,
        'ExistI': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'ExistJ': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'ExistK': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'ColorI': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'ColorJ': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'NodeY': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'CorrIJ': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
        'CorrIK': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
        'CorrJK': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
        'CorrJI': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
        'CorrKI': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
        'CorrKJ': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
    }
    arrs = [
        'NodeY',
        'ExistI',
        'ExistJ',
        'ExistK',
    ]
    if self.color_ij:
      arrs += [
          'ColorI',
          'ColorJ',
      ]
    for s in range(self.n_s):
      for i in range(self.n_i):
        for j in range(self.n_j):
          for k in range(self.n_k):
            if s == 0:  # Exist, Node, and Color vars
              for arr in arrs:
                data[arr][i][j][k] = 1 if model[self.vars[arr][i][j][k]] else 0

            # Corr vars
            for arr in [
                'CorrIJ',
                'CorrIK',
                'CorrJI',
                'CorrJK',
                'CorrKI',
                'CorrKJ',
            ]:
              data[arr][s][i][j][k] = (
                  1 if model[self.vars[arr][s][i][j][k]] else 0
              )
    return data

  def write_cnf(self, output_file_name: str) -> bool:
    """generate CNF for the problem.

    Args:
        output_file_name (str): file to write CNF.

    Returns:
        bool: False if the CNF is trivial, True otherwise
    """
    simplified = z3.Tactic('simplify')(self.goal)[0]
    simplified = z3.Tactic('propagate-values')(simplified)[0]
    cnf = z3.Tactic('tseitin-cnf')(simplified)[0]
    dimacs = cnf.dimacs()

    with open(output_file_name, 'w') as output_f:
      output_f.write(cnf.dimacs())
    if dimacs.startswith('p cnf 1 1'):
      print(
          'Generated CNF is trivial meaning z3 concludes the instance UNSAT'
          ' during simplification.'
      )
      return False
    else:
      return True

  def read_external_result(
      self, dimacs_file: str, result_file: str
  ) -> Mapping[str, Any]:
    """read result from external SAT solver

    Args:
        dimacs_file (str):
        result_file (str): log, e.g., from Kissat containing SAT assignments

    Raises:
        ValueError: in the dimacs file, the last lines are comments that records
          the mapping from SAT variable indices to the variable names in Z3. If
          the coordinates in this name is incorrect, this error is raised.

    Returns:
        Mapping[str, Any]: variable assignment in arrays. All the one with a
          corresponding SAT variable are read off from the SAT log. The others
          are left with -1.
    """
    results = {
        'ExistI': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'ExistJ': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'ExistK': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'ColorI': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'ColorJ': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'NodeY': [
            [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
            for _ in range(self.n_i)
        ],
        'CorrIJ': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
        'CorrIK': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
        'CorrJK': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
        'CorrJI': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
        'CorrKI': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
        'CorrKJ': [
            [
                [[-1 for _ in range(self.n_k)] for _ in range(self.n_j)]
                for _ in range(self.n_i)
            ]
            for _ in range(self.n_s)
        ],
    }

    # in this file, the assigments are lines starting with 'v' like
    # v -1 -2 -3 -4 -5 -6 -7 -8 -9 -10 -11 -12 ...
    # the vars starts from 1 and - means it's False; otherwise, it's True
    # we scan through all these lines, and save the assignments to `sat`
    with open(result_file, 'r') as f:
      sat_output = f.readlines()
    sat = {}
    for line in sat_output:
      if line.startswith('v'):
        assignments = line[1:].strip().split(' ')
        for assignment in assignments:
          tmp = int(assignment)
          if tmp < 0:
            sat[str(-tmp)] = 0
          elif tmp > 0:
            sat[str(tmp)] = 1

    # in the dimacs generated by Z3, there are lines starting with 'c' like
    # c 8804 CorrIJ(1,0,3,4)    or    c 60053 k!44404
    # which records the mapping from our variables to variables in dimacs
    # the ones starting with k! are added in the translation, we don't care
    with open(dimacs_file, 'r') as f:
      dimacs = f.readlines()
    for line in dimacs:
      if line.startswith('c'):
        _, index, name = line.strip().split(' ')
        if name.startswith((
            'NodeY',
            'ExistI',
            'ExistJ',
            'ExistK',
            'ColorI',
            'ColorJ',
            'CorrIJ',
            'CorrIK',
            'CorrJI',
            'CorrJK',
            'CorrKI',
            'CorrKJ',
        )):
          arr, tmp = name[:-1].split('(')
          coords = [int(num) for num in tmp.split(',')]
          if len(coords) == 3:
            results[arr][coords[0]][coords[1]][coords[2]] = sat[index]
          elif len(coords) == 4:
            results[arr][coords[0]][coords[1]][coords[2]][coords[3]] = sat[
                index
            ]
          else:
            raise ValueError('number of coord should be 3 or 4!')

    return results

  def check_external_solver(
      self,
      kissat_dir: str,
      dimacs_file_name: Optional[str] = None,
      sat_log_file_name: Optional[str] = None,
      print_progress: bool = True,
  ) -> bool:
    """check whether there is a solution with an external solver, e.g., Kissat

    Args:
        kissat_dir (str): directory containing an executable named kissat
        dimacs_file_name (Optional[str], optional): Defaults to None. Then, the
          dimacs file is in a tmp directory. If specified, the dimacs will be
          saved to that path.
        sat_log_file_name (Optional[str], optional): Defaults to None. Then, the
          sat log file is in a tmp directory. If specified, the dimacs will be
          saved to that path.
        print_progress (bool, optional): whether print the SAT solving process
          on screen. Defaults to True.

    Raises:
        ValueError: kissat_dir is not a directory
        ValueError: there is no executable kissat in kissat_dir
        ValueError: the return code to kissat is neither SAT (10) nor UNSAT (20)

    Returns:
        bool: True if SAT, False if UNSAT
    """
    if not os.path.isdir(kissat_dir):
      raise ValueError(f'{kissat_dir} is not a directory.')
    if kissat_dir.endswith('/'):
      solver_cmd = kissat_dir + 'kissat'
    else:
      solver_cmd = kissat_dir + '/kissat'
    if not os.path.isfile(solver_cmd):
      raise ValueError(f'There is no kissat in {kissat_dir}.')

    if_solved = False
    with tempfile.TemporaryDirectory() as tmp_dir:
      # open a tmp directory as workspace

      # dimacs and sat log are either in the tmp directory, or as user specified
      tmp_dimacs_file_name = (
          dimacs_file_name if dimacs_file_name else tmp_dir + '/tmp.dimacs'
      )
      tmp_sat_log_file_name = (
          sat_log_file_name if sat_log_file_name else tmp_dir + '/tmp.sat'
      )

      if self.write_cnf(tmp_dimacs_file_name):
        # continue if the CNF is non-trivial. Otherwise, we conclude it's UNSAT

        with open(tmp_sat_log_file_name, 'w') as log:
          # use tmp_sat_log_file_name to record stdout of kissat

          kissat_return_code = -100
          # -100 if the return code is not updated later on.

          with subprocess.Popen(
              [solver_cmd, tmp_dimacs_file_name],
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
            print('kissat SAT')

          # we read the Kissat solution from the SAT log, then, plug those into
          # the Z3 model and solved inside Z3 again. The reason is that Z3 did
          # some simplification of the problem so not every variable appear in
          # the DIMACS given to Kissat. We still need to know their value.
          result = self.read_external_result(
              tmp_dimacs_file_name,
              tmp_sat_log_file_name,
          )
          self.plugin_arrs(result, self.color_ij)
          self.check_z3(print_progress)

        elif kissat_return_code == 20:
          if print_progress:
            print(f'{solver_cmd} UNSAT')

        elif kissat_return_code == -100:
          print('Did not get Kissat return code.')

        else:
          raise ValueError(
              f'Kissat return code {kissat_return_code} is neither SAT nor'
              ' UNSAT. Maybe you should add print_process=True to enable the'
              ' Kissat printout message to see what is going on.'
          )
    # closing the tmp directory, the files and itself are removed

    return if_solved
