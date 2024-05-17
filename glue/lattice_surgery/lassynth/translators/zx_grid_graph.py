"""Classes ZXGridEdge, ZXGridSpider, and ZXGridGraph. ZXGridGraph is a graph
where nodes are the cubes in LaS and edges are pipes in LaS.
"""

from typing import Any, Mapping, Sequence, Tuple, Optional


class ZXGridNode:

    def __init__(self, coord3: Tuple[int, int, int],
                 connectivity: Mapping[str, Mapping[str, int]]) -> None:
        """initialize ZXGridNode for a cube in the LaS.

        self.type: type of ZX spider, 'N': no spider, 'X'/'Z': X/Z-spider,
            'S': Y cube, 'I': identity, 'Pi': input port, 'Po': output port. 
        self.i/j/k: 3D corrdinates of the cube in the LaS.
        self.exists is a dictionary with six keys corresponding to whether a
            pipe exist in the six directions to a cube in the LaS.
        self.colors are the colors of these possible pipes.
        self.y_tail_plus: if this node connects a Y on the top.
        self.y_tail_minus: if this node connects a Y on the bottom.
        
        Args:
            coord3 (Tuple[int, int, int]): 3D coordinate of the cube.
            connectivity (Mapping[str, Mapping[str, int]]): contains exists
                and colors of the six possible pipes to a cube
        """
        self.i, self.j, self.k = coord3
        self.y_tail_plus = False
        self.y_tail_minus = False
        self.node_id = -1
        self.exists = connectivity["exists"]
        self.colors = connectivity["colors"]
        self.compute_type()

    def compute_type(self) -> None:
        """decide the type of a ZXGridNoe
        
        Raises:
            ValueError: node has degree=1, which should be forbidden earlier.
            ValueError: node has degree>4, which should be forbidden earlier.
        """
        deg = sum([v for (k, v) in self.exists.items()])
        if deg == 0:
            self.type = "N"
            return
        elif deg == 1:
            raise ValueError("There should not be deg-1 Z or X spiders.")
        elif deg == 2:
            self.type = "I"
        elif deg >= 5:
            raise ValueError("deg > 4: 3D corner exists")
        else:  # degree = 3 or 4
            if self.exists["-I"] == 0 and self.exists["+I"] == 0:
                if self.exists["-J"]:
                    if self.colors["-J"] == 0:
                        self.type = "X"
                    else:
                        self.type = "Z"
                else:  # must exist +J
                    if self.colors["+J"] == 0:
                        self.type = "X"
                    else:
                        self.type = "Z"

            if self.exists["-J"] == 0 and self.exists["+J"] == 0:
                if self.exists["-I"]:
                    if self.colors["-I"] == 0:
                        self.type = "Z"
                    else:
                        self.type = "X"
                else:  # must exist +I
                    if self.colors["+I"] == 0:
                        self.type = "Z"
                    else:
                        self.type = "X"

            if self.exists["-K"] == 0 and self.exists["+K"] == 0:
                if self.exists["-I"]:
                    if self.colors["-I"] == 0:
                        self.type = "X"
                    else:
                        self.type = "Z"
                else:  # must exist +I
                    if self.colors["+I"] == 0:
                        self.type = "X"
                    else:
                        self.type = "Z"

    def zigxag_xy(self, n_j: int) -> Tuple[int, int]:
        return (self.k * (n_j + 2) + self.j, -(n_j + 1) * self.i + self.j)

    def zigxag_str(self, n_j: int) -> str:
        zigxag_type = {
            'Z': '@',
            'X': 'O',
            'S': 's',
            'W': 'w',
            'I': 'O',
            'Pi': 'in',
            'Po': 'out',
        }
        (x, y) = self.zigxag_xy(n_j)
        return str(-y) + ',' + str(-x) + ',' + str(zigxag_type[self.type])


class ZXGridEdge:

    def __init__(self, if_h: bool, node0: ZXGridNode,
                 node1: ZXGridNode) -> None:
        """initialize ZXGridEdge for a pipe in the LaS.
        
        Args:
            if_h (bool): if this edge is a Hadamard edge.
            node0 (ZXGridNode): one end of the edge.
            node1 (ZXGridNode): the other end of the edge.

        Raises:
            ValueError: the two spiders are the same.
            ValueError: the two spiders are not neighbors.
        """

        dist = abs(node0.i - node1.i) + abs(node0.j - node1.j) + abs(node0.k -
                                                                     node1.k)
        if dist == 0:
            raise ValueError(f"{node0} and {node1} are the same.")
        if dist > 1:
            raise ValueError(f"{node0} and {node1} are not neighbors.")
        self.node0, self.node1 = node0, node1
        self.type = "h" if if_h else "-"

    def zigxag_str(self, n_j: int) -> str:
        (xa, ya) = self.node0.zigxag_xy(n_j)
        (xb, yb) = self.node1.zigxag_xy(n_j)
        return (str(-ya) + ',' + str(-xa) + ',' + str(-yb) + ',' + str(-xb) +
                ',' + self.type)


class ZXGridGraph:

    def __init__(self, lasre: Mapping[str, Any]) -> None:
        self.lasre = lasre
        self.n_i, self.n_j, self.n_k = (
            lasre["n_i"],
            lasre["n_j"],
            lasre["n_k"],
        )
        self.nodes = [[[
            ZXGridNode((i, j, k), self.gather_cube_connectivity(i, j, k))
            for k in range(self.n_k + 1)
        ] for j in range(self.n_j + 1)] for i in range(self.n_i + 1)]
        for (i, j, k) in self.lasre["port_cubes"]:
            self.nodes[i][j][k].type = 'Po'
        self.append_y_tails()
        self.edges = []
        self.derive_edges()

    def gather_cube_connectivity(self, i: int, j: int,
                                 k: int) -> Mapping[str, Mapping[str, int]]:
        # exists and colors for no cube
        exists = {"-I": 0, "+I": 0, "-K": 0, "+K": 0, "-J": 0, "+J": 0}
        colors = {
            "-I": -1,
            "+I": -1,
            "-K": -1,
            "+K": -1,
            "-J": -1,
            "+J": -1,
        }
        if i in range(self.n_i) and j in range(self.n_j) and k in range(
                self.n_k) and ((i, j, k) not in self.lasre["port_cubes"]) and (
                    self.lasre["NodeY"][i][j][k] == 0):
            if i > 0 and self.lasre["ExistI"][i - 1][j][k]:
                exists["-I"] = 1
                colors["-I"] = self.lasre["ColorI"][i - 1][j][k]
            if self.lasre["ExistI"][i][j][k]:
                exists["+I"] = 1
                colors["+I"] = self.lasre["ColorI"][i][j][k]
            if j > 0 and self.lasre["ExistJ"][i][j - 1][k]:
                exists["-J"] = 1
                colors["-J"] = self.lasre["ColorJ"][i][j - 1][k]
            if self.lasre["ExistJ"][i][j][k]:
                exists["+J"] = 1
                colors["+J"] = self.lasre["ColorJ"][i][j][k]
            if k > 0 and self.lasre["ExistK"][i][j][k - 1]:
                exists["-K"] = 1
                colors["-K"] = self.lasre["ColorKP"][i][j][k - 1]
            if self.lasre["ExistK"][i][j][k]:
                exists["+K"] = 1
                colors["+K"] = self.lasre["ColorKM"][i][j][k]
        return {"exists": exists, "colors": colors}

    def append_y_tails(self) -> None:
        for i in range(self.n_i):
            for j in range(self.n_j):
                for k in range(self.n_k):
                    if self.lasre["NodeY"][i][j][k]:
                        if (k - 1 >= 0 and self.lasre["ExistK"][i][j][k - 1]
                                and (not self.lasre["NodeY"][i][j][k - 1])):
                            self.nodes[i][j][k - 1].y_tail_plus = True
                        if (k + 1 < self.n_k and self.lasre["ExistK"][i][j][k]
                                and (not self.lasre["NodeY"][i][j][k + 1])):
                            self.nodes[i][j][k + 1].y_tail_minus = True

    def derive_edges(self):
        valid_types = ["Z", "X", "S", "I", "Pi", "Po"]
        for i in range(self.n_i):
            for j in range(self.n_j):
                for k in range(self.n_k):
                    if (self.lasre["ExistI"][i][j][k] == 1
                            and self.nodes[i][j][k].type in valid_types
                            and self.nodes[i + 1][j][k].type in valid_types):
                        self.edges.append(
                            ZXGridEdge(0, self.nodes[i][j][k],
                                       self.nodes[i + 1][j][k]))

                    if (self.lasre["ExistJ"][i][j][k] == 1
                            and self.nodes[i][j][k].type in valid_types
                            and self.nodes[i][j + 1][k].type in valid_types):
                        self.edges.append(
                            ZXGridEdge(0, self.nodes[i][j][k],
                                       self.nodes[i][j + 1][k]))

                    if (self.lasre["ExistK"][i][j][k] == 1
                            and self.nodes[i][j][k].type in valid_types
                            and self.nodes[i][j][k + 1].type in valid_types):
                        self.edges.append(
                            ZXGridEdge(
                                abs(self.lasre["ColorKM"][i][j][k] -
                                    self.lasre["ColorKP"][i][j][k]),
                                self.nodes[i][j][k],
                                self.nodes[i][j][k + 1],
                            ))

    def to_zigxag_url(self, io_spec: Optional[Sequence[str]] = None) -> str:
        """generate a url for ZigXag

        Args:
            io_spec (Sequence[str], optional): specify whether each port is an
                input port or an output port. 

        Raises:
            ValueError: len(io_spec) is not the same with the number of ports.

        Returns:
            str: zigxag url
        """
        if io_spec is not None:
            if len(io_spec) != len(self.lasre["port_cubes"]):
                raise ValueError(
                    f"io_spec has length {len(io_spec)} but there are "
                    f"{len(self.lasre['port_cubes'])} ports.")
            for w, (i, j, k) in enumerate(self.lasre["port_cubes"]):
                self.nodes[i][j][k].type = io_spec[w]

        valid_types = ["Z", "X", "S", "W", "I", "Pi", "Po"]
        nodes_str = ""
        first = True
        for i in range(self.n_i + 1):
            for j in range(self.n_j + 1):
                for k in range(self.n_k + 1):
                    if self.nodes[i][j][k].type in valid_types:
                        if not first:
                            nodes_str += ";"
                        nodes_str += self.nodes[i][j][k].zigxag_str(self.n_j)
                        first = False

        edges_str = ""
        for i, edge in enumerate(self.edges):
            if i > 0:
                edges_str += ";"
            edges_str += edge.zigxag_str(self.n_j)

        # add nodes and edges for Y cubes
        for i in range(self.n_i):
            for j in range(self.n_j):
                for k in range(self.n_k):
                    (x, y) = self.nodes[i][j][k].zigxag_xy(self.n_j)
                    if self.nodes[i][j][k].y_tail_plus:
                        nodes_str += (";" + str(x + self.n_j - j) + "," +
                                      str(y) + ",s")
                        edges_str += (";" + str(x + self.n_j - j) + "," +
                                      str(y) + "," + str(x) + "," + str(y) +
                                      ",-")
                    if self.nodes[i][j][k].y_tail_minus:
                        nodes_str += (";" + str(x - j - 1) + "," + str(y) +
                                      ",s")
                        edges_str += (";" + str(x - j - 1) + "," + str(y) +
                                      "," + str(x) + "," + str(y) + ",-")

        zigxag_str = "https://algassert.com/zigxag#" + nodes_str + ":" + edges_str
        return zigxag_str
