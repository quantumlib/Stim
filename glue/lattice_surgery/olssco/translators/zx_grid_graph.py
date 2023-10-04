"""A one-line summary of the module or program, terminated by a period.

Leave one blank line.  The rest of this docstring should contain an
overall description of the module or program.  Optionally, it may also
contain a brief description of exported classes and functions and/or usage
examples.

Typical usage example:

  foo = ClassFoo()
  bar = foo.FunctionBar()
"""


from typing import Any, Mapping, Sequence


class ZXEdge:

  def __init__(self, if_h: bool, spider0, spider1):
    self.spider0, self.spider1 = spider0, spider1
    self.type = "h" if if_h else "-"


class ZXSpider:

  def __init__(self, coord3):
    self.type = "N"
    self.i, self.j, self.k = coord3
    self.y_tail_plus = False
    self.y_tail_minus = False
    self.exists = {"-I": 0, "+I": 0, "-K": 0, "+K": 0, "-J": 0, "+J": 0}
    self.colors = {"-I": -1, "+I": -1, "-K": -1, "+K": -1, "-J": -1, "+J": -1}

  def compute_type(self, exists: dict, colors: dict):
    self.exists, self.colors = exists, colors
    deg = sum([v for (k, v) in exists.items()])
    if deg == 0:
      self.type = "N"
      return
    elif deg == 1:
      raise ValueError("There should not be deg-1 Z or X spiders.")
    elif deg == 2:
      self.type = "X"
    elif deg >= 5:
      raise ValueError("deg > 4: 3D corner exists")
    else:
      if exists["-I"] == 0 and exists["+I"] == 0:
        if exists["-J"]:
          if colors["-J"] == 0:
            self.type = "X"
          else:
            self.type = "Z"
        else:  # must exist +J
          if colors["+J"] == 0:
            self.type = "X"
          else:
            self.type = "Z"

      if exists["-J"] == 0 and exists["+J"] == 0:
        if exists["-I"]:
          if colors["-I"] == 0:
            self.type = "Z"
          else:
            self.type = "X"
        else:  # must exist +I
          if colors["+I"] == 0:
            self.type = "Z"
          else:
            self.type = "X"

      if exists["-K"] == 0 and exists["+K"] == 0:
        if exists["-I"]:
          if colors["-I"] == 0:
            self.type = "X"
          else:
            self.type = "Z"
        else:  # must exist +I
          if colors["+I"] == 0:
            self.type = "X"
          else:
            self.type = "Z"


class ZXGridGraph:

  def __init__(self, lasir: dict) -> None:
    self.lasir = lasir
    self.n_i, self.n_j, self.n_k = (
        lasir["n_i"],
        lasir["n_j"],
        lasir["n_k"],
    )
    self.spiders = [
        [
            [ZXSpider((i, j, k)) for k in range(self.n_k + 1)]
            for j in range(self.n_j + 1)
        ]
        for i in range(self.n_i + 1)
    ]
    self.edges = []
    self.append_y_tails()
    self.compute_spiders_type()

  def append_y_tails(self):
    for i in range(self.n_i):
      for j in range(self.n_j):
        for k in range(self.n_k):
          if self.lasir["NodeY"][i][j][k]:
            if (
                k - 1 >= 0
                and self.lasir["ExistK"][i][j][k - 1]
                and (not self.lasir["NodeY"][i][j][k - 1])
            ):
              self.spiders[i][j][k - 1].y_tail_plus = True
            if (
                k + 1 < self.n_k
                and self.lasir["ExistK"][i][j][k]
                and (not self.lasir["NodeY"][i][j][k + 1])
            ):
              self.spiders[i][j][k + 1].y_tail_minus = True

  def compute_spiders_type(self):
    for i in range(self.n_i):
      for j in range(self.n_j):
        for k in range(self.n_k):
          if ((i, j, k) not in self.lasir["port_cubes"]) and (
              self.lasir["NodeY"][i][j][k] == 0
          ):
            exists = {"-I": 0, "+I": 0, "-K": 0, "+K": 0, "-J": 0, "+J": 0}
            colors = {
                "-I": -1,
                "+I": -1,
                "-K": -1,
                "+K": -1,
                "-J": -1,
                "+J": -1,
            }
            if i > 0 and self.lasir["ExistI"][i - 1][j][k]:
              exists["-I"] = 1
              colors["-I"] = self.lasir["ColorI"][i - 1][j][k]
            if self.lasir["ExistI"][i][j][k]:
              exists["+I"] = 1
              colors["+I"] = self.lasir["ColorI"][i][j][k]
            if j > 0 and self.lasir["ExistJ"][i][j - 1][k]:
              exists["-J"] = 1
              colors["-J"] = self.lasir["ColorJ"][i][j - 1][k]
            if self.lasir["ExistJ"][i][j][k]:
              exists["+J"] = 1
              colors["+J"] = self.lasir["ColorJ"][i][j][k]
            if k > 0 and self.lasir["ExistK"][i][j][k - 1]:
              exists["-K"] = 1
              colors["-K"] = self.lasir["ColorKP"][i][j][k - 1]
            if self.lasir["ExistK"][i][j][k]:
              exists["+K"] = 1
              colors["+K"] = self.lasir["ColorKM"][i][j][k]
            self.spiders[i][j][k].compute_type(exists, colors)
