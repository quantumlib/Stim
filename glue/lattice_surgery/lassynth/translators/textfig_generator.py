"""Generate text figures of 2D time slices of the LaS."""

from lassynth.translators import ZXGridGraph


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
        self.chars = [[
            " " for _ in range(2 * TextLayer.pad_i +
                               (self.n_i - 1) * TextLayer.sep_i + 1)
        ] + ["\n"] for _ in range(2 * TextLayer.pad_j +
                                  (self.n_j - 1) * TextLayer.sep_j + 1)]
        if if_middle:
            self.compute_middle(zx_graph, k)
        else:
            self.compute_normal(zx_graph, k)

    def set_char(self, j: int, i: int, character):
        self.chars[j][i] = character

    def compute_normal(self, zx_graph: ZXGridGraph, k: int):
        """a normal layer corresponds to a layer of cubes in LaS, e.g., 
             /   /
            X   X 
            |  /  
            |     
            |/    
            Z   . 
           /      
        There are 2x2 tiles of surface codes. The bottom right one is not being
        used, represented by a `.`; the top right one is identity in because
        it has degree 2, but our convention is that these spiders have type `X`
        The top left one is like that, too. The bottom left is a Z-spider with
        three edges, which is non trivial. `-` and `|` I-pipes and J-pipes.
        `/` are K-pipes. The `/` on the bottom left corner of a spider connects
        to the previous moment. The `/` on the top right corner of a spider
        connects to the next moment.

        Args:
            zx_graph (ZXGridGraph):
            k (int): the height of this layer.
        """
        for i in range(self.n_i):
            for j in range(self.n_j):
                spider = zx_graph.nodes[i][j][k]

                if spider.type in ["N", "Pi", "Po"]:
                    self.set_char(
                        TextLayer.pad_j + j * TextLayer.sep_j,
                        TextLayer.pad_i + i * TextLayer.sep_i,
                        ".",
                    )
                    continue
                elif spider.type == "I":
                    self.set_char(
                        TextLayer.pad_j + j * TextLayer.sep_j,
                        TextLayer.pad_i + i * TextLayer.sep_i,
                        "X",
                    )
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

    def compute_middle(self, zx_graph: ZXGridGraph, k: int):
        """a middle layer is either a Hadmard edge or a normal edge, e.g.,
                 /
            .   X 
               /  
                
             /    
            H   . 
           /      
        These layers cannot have `-` or `|`. It only has `/` which are K-pipes.
        The node is either `H` meaning the edge is a Hadamard edge, or `X`
        meaning the edge is a normal edge. We use `X` for identity here.

        Args:
            zx_graph (ZXGridGraph): 
            k (int): the height of this layer. There is a middle layer after a 
                normal layer.
        """
        for i in range(self.n_i):
            for j in range(self.n_j):
                self.set_char(
                    TextLayer.pad_j + j * TextLayer.sep_j,
                    TextLayer.pad_i + i * TextLayer.sep_i,
                    ".",
                )
                spider = zx_graph.nodes[i][j][k]
                color_sum = -1
                if k == self.n_k - 1:
                    try:
                        for port in zx_graph.lasre["ports"]:
                            if (port["i"], port["j"], port["k"]) == (i, j, k):
                                color_sum = port["c"] + spider.colors["+K"]
                                break
                    except ValueError:
                        print(
                            f"KPipe({i},{j},{k}) connect outside but not port."
                        )
                else:
                    upper_spider = zx_graph.nodes[i][j][k + 1]
                    if spider.exists["+K"] == 1 and upper_spider.exists[
                            "-K"] == 1:
                        color_sum = spider.colors["+K"] + upper_spider.colors[
                            "-K"]
                    if spider.exists["+K"] == 0 and upper_spider.exists[
                            "-K"] == 1:
                        try:
                            for port in zx_graph.lasre["ports"]:
                                if (port["i"], port["j"], port["k"]) == (i, j,
                                                                         k):
                                    color_sum = port[
                                        "c"] + upper_spider.colors["-K"]
                                    break
                        except ValueError:
                            print(f"KPipe({i},{j},{k})- should be a port.")
                    if spider.exists["+K"] == 1 and upper_spider.exists[
                            "-K"] == 0:
                        try:
                            for port in zx_graph.lasre["ports"]:
                                if (port["i"], port["j"],
                                        port["k"]) == (i, j, k + 1):
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

    def get_text(self):
        text = ""
        for j in range(2 * TextLayer.pad_j + (self.n_j - 1) * TextLayer.sep_j +
                       1):
            for i in range(2 * TextLayer.pad_i +
                           (self.n_i - 1) * TextLayer.sep_i + 1):
                text += self.chars[j][i]
            text += "\n"
        return text


def textfig_generator(lasre: dict):
    text = "======================================\n"
    zx_graph = ZXGridGraph(lasre)
    for k in range(lasre["n_k"] - 1, -1, -1):
        text += TextLayer(zx_graph, k, True).get_text()
        text += "======================================\n"
        text += TextLayer(zx_graph, k, False).get_text()
        text += "======================================\n"
    return text
