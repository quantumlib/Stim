"""A one-line summary of the module or program, terminated by a period.

Leave one blank line.  The rest of this docstring should contain an
overall description of the module or program.  Optionally, it may also
contain a brief description of exported classes and functions and/or usage
examples.

Typical usage example:

  foo = ClassFoo()
  bar = foo.FunctionBar()
"""

import argparse
import json
from typing import Any, Mapping, Sequence

from olssco.translators import ZXGridGraph


class TextLayer:
  pad_i = 1
  pad_j = 1
  sep_i = 4
  sep_j = 4

  def __init__(self, zx_graph: ZXGridGraph, k: int, if_middle: bool) -> None:
    self.n_i, self.n_j, self.n_k = (
        zx_graph.n_i,
        zx_graph.n_j,
        zx_graph.n_k,
    )
    self.chars = [
        [
            " "
            for _ in range(
                2 * TextLayer.pad_i + (self.n_i - 1) * TextLayer.sep_i + 1
            )
        ]
        + ["\n"]
        for _ in range(
            2 * TextLayer.pad_j + (self.n_j - 1) * TextLayer.sep_j + 1
        )
    ]
    if if_middle:
      self.compute_middle(zx_graph, k)
    else:
      self.compute_normal(zx_graph, k)

  def set_char(self, j: int, i: int, character):
    self.chars[j][i] = character

  def compute_middle(self, zx_graph: ZXGridGraph, k: int):
    for i in range(self.n_i):
      for j in range(self.n_j):
        self.set_char(
            TextLayer.pad_j + j * TextLayer.sep_j,
            TextLayer.pad_i + i * TextLayer.sep_i,
            ".",
        )
        spider = zx_graph.spiders[i][j][k]
        color_sum = -1
        if k == self.n_k - 1:
          try:
            for port in zx_graph.lasir["ports"]:
              if (port["i"], port["j"], port["k"]) == (i, j, k):
                color_sum = port["c"] + spider.colors["+K"]
                break
          except ValueError:
            print(f"KPipe({i},{j},{k}) connecting outside but not port.")
        else:
          upper_spider = zx_graph.spiders[i][j][k + 1]
          if spider.exists["+K"] == 1 and upper_spider.exists["-K"] == 1:
            color_sum = spider.colors["+K"] + upper_spider.colors["-K"]
          if spider.exists["+K"] == 0 and upper_spider.exists["-K"] == 1:
            try:
              for port in zx_graph.lasir["ports"]:
                if (port["i"], port["j"], port["k"]) == (i, j, k):
                  color_sum = port["c"] + upper_spider.colors["-K"]
                  break
            except ValueError:
              print(f"KPipe({i},{j},{k})- should be a port.")
          if spider.exists["+K"] == 1 and upper_spider.exists["-K"] == 0:
            try:
              for port in zx_graph.lasir["ports"]:
                if (port["i"], port["j"], port["k"]) == (i, j, k + 1):
                  color_sum = port["c"] + spider.colors["+K"]
                  break
            except ValueError:
              print(f"KPipe({i},{j},{k + 1})- should be a port")

        if color_sum != -1:
          self.set_char(
              TextLayer.pad_j + j * TextLayer.sep_j - 1,
              TextLayer.pad_i + i * TextLayer.sep_i + 1,
              "/",
          )
          self.set_char(
              TextLayer.pad_j + j * TextLayer.sep_j + 1,
              TextLayer.pad_i + i * TextLayer.sep_i - 1,
              "/",
          )
          if color_sum == 1:
            self.set_char(
                TextLayer.pad_j + j * TextLayer.sep_j,
                TextLayer.pad_i + i * TextLayer.sep_i,
                "H",
            )
          else:
            self.set_char(
                TextLayer.pad_j + j * TextLayer.sep_j,
                TextLayer.pad_i + i * TextLayer.sep_i,
                "X",
            )

  def compute_normal(self, zx_graph: ZXGridGraph, k: int):
    for i in range(self.n_i):
      for j in range(self.n_j):
        spider = zx_graph.spiders[i][j][k]

        if spider.type == "N":
          self.set_char(
              TextLayer.pad_j + j * TextLayer.sep_j,
              TextLayer.pad_i + i * TextLayer.sep_i,
              ".",
          )
          continue
        else:
          self.set_char(
              TextLayer.pad_j + j * TextLayer.sep_j,
              TextLayer.pad_i + i * TextLayer.sep_i,
              spider.type,
          )

        # I pipes
        if spider.exists["+I"]:
          for offset in range(1, TextLayer.sep_i):
            self.set_char(
                TextLayer.pad_j + j * TextLayer.sep_j,
                TextLayer.pad_i + i * TextLayer.sep_i + offset,
                "-",
            )

        # J pipes
        if spider.exists["+J"]:
          for offset in range(1, TextLayer.sep_i):
            self.set_char(
                TextLayer.pad_j + j * TextLayer.sep_j + offset,
                TextLayer.pad_i + i * TextLayer.sep_i,
                "|",
            )

        # K pipes
        if spider.exists["+K"]:
          self.set_char(
              TextLayer.pad_j + j * TextLayer.sep_j - 1,
              TextLayer.pad_i + i * TextLayer.sep_i + 1,
              "/",
          )
        if spider.exists["-K"]:
          self.set_char(
              TextLayer.pad_j + j * TextLayer.sep_j + 1,
              TextLayer.pad_i + i * TextLayer.sep_i - 1,
              "/",
          )

  def get_text(self):
    text = ""
    for j in range(2 * TextLayer.pad_j + (self.n_j - 1) * TextLayer.sep_j):
      for i in range(2 * TextLayer.pad_i + (self.n_i - 1) * TextLayer.sep_i):
        text += self.chars[j][i]
      text += "\n"
    return text


def textfig_generator(lasir: dict):
  text = ""
  zx_graph = ZXGridGraph(lasir)
  for k in range(lasir["n_k"] - 1, -1, -1):
    text += TextLayer(zx_graph, k, True).get_text()
    text += "\n======================================\n"
    text += TextLayer(zx_graph, k, False).get_text()
    text += "\n======================================\n"
  return text
