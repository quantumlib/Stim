"""generate a annotated networkx.Graph corresponding to the LaS."""

import networkx
from lassynth.translators import ZXGridGraph
import stimzx
from typing import Mapping, Any


def networkx_generator(lasre: Mapping[str, Any]) -> networkx.Graph:
    n_i, n_j, n_k = lasre["n_i"], lasre["n_j"], lasre["n_k"]
    port_cubes = lasre["port_cubes"]
    zxgridgraph = ZXGridGraph(lasre)
    edges = zxgridgraph.edges
    nodes = zxgridgraph.nodes

    zx_nx_graph = networkx.Graph()
    type_to_str = {"X": "X", "Z": "Z", "Pi": "in", "Po": "out", "I": "X"}
    cnt = 0
    for i, j, k in port_cubes:
        node = nodes[i][j][k]
        zx_nx_graph.add_node(cnt, value=stimzx.ZxType(type_to_str[node.type]))
        node.node_id = cnt
        cnt += 1

    for i in range(n_i + 1):
        for j in range(n_j + 1):
            for k in range(n_k + 1):
                node = nodes[i][j][k]
                if node.type not in ["N", "Po", "Pi"]:
                    zx_nx_graph.add_node(
                        cnt, value=stimzx.ZxType(type_to_str[node.type])
                    )
                    node.node_id = cnt
                    cnt += 1
                if node.y_tail_minus:
                    zx_nx_graph.add_node(cnt, value=stimzx.ZxType("Z", 1))
                    zx_nx_graph.add_edge(node.node_id, cnt)
                    cnt += 1
                if node.y_tail_plus:
                    zx_nx_graph.add_node(cnt, value=stimzx.ZxType("Z", 3))
                    zx_nx_graph.add_edge(node.node_id, cnt)
                    cnt += 1

    for edge in edges:
        if edge.type != "h":
            zx_nx_graph.add_edge(edge.node0.node_id, edge.node1.node_id)
        else:
            zx_nx_graph.add_node(cnt, value=stimzx.ZxType("H"))
            zx_nx_graph.add_edge(cnt, edge.node0.node_id)
            zx_nx_graph.add_edge(cnt, edge.node1.node_id)
            cnt += 1

    return zx_nx_graph
