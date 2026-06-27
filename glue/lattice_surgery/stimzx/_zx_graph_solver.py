from typing import Dict, Tuple, List, Any, Union
import stim
import networkx as nx

from ._text_diagram_parsing import text_diagram_to_networkx_graph
from ._external_stabilizer import ExternalStabilizer


class ZxType:
    """Data describing a ZX node."""

    def __init__(self, kind: str, quarter_turns: int = 0):
        self.kind = kind
        self.quarter_turns = quarter_turns

    def __eq__(self, other):
        if not isinstance(other, ZxType):
            return NotImplemented
        return self.kind == other.kind and self.quarter_turns == other.quarter_turns

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash((ZxType, self.kind, self.quarter_turns))

    def __repr__(self):
        return f'ZxType(kind={self.kind!r}, quarter_turns={self.quarter_turns!r})'


ZX_TYPES = {
    "X": ZxType("X"),
    "X(pi/2)": ZxType("X", 1),
    "X(pi)": ZxType("X", 2),
    "X(-pi/2)": ZxType("X", 3),
    "Z": ZxType("Z"),
    "Z(pi/2)": ZxType("Z", 1),
    "Z(pi)": ZxType("Z", 2),
    "Z(-pi/2)": ZxType("Z", 3),
    "H": ZxType("H"),
    "in": ZxType("in"),
    "out": ZxType("out"),
}


def text_diagram_to_zx_graph(text_diagram: str) -> nx.MultiGraph:
    """Converts an ASCII text diagram into a ZX graph (represented as a networkx MultiGraph).

    Supported node types:
        "X": X spider with angle set to 0.
        "Z": Z spider with angle set to 0.
        "X(pi/2)": X spider with angle set to pi/2.
        "X(pi)": X spider with angle set to pi.
        "X(-pi/2)": X spider with angle set to -pi/2.
        "Z(pi/2)": X spider with angle set to pi/2.
        "Z(pi)": X spider with angle set to pi.
        "Z(-pi/2)": X spider with angle set to -pi/2.
        "H": Hadamard node. Must have degree 2.
        "in": Input node. Must have degree 1.
        "out": Output node. Must have degree 1.

    Args:
        text_diagram: A text diagram containing ZX nodes (e.g. "X(pi)") and edges (e.g. "------") connecting them.

    Example:
        >>> import stimzx
        >>> import networkx
        >>> actual: networkx.MultiGraph = stimzx.text_diagram_to_zx_graph(r'''
        ...     in----X------out
        ...           |
        ...     in---Z(pi)---out
        ... ''')
        >>> expected = networkx.MultiGraph()
        >>> expected.add_node(0, value=stimzx.ZxType("in"))
        >>> expected.add_node(1, value=stimzx.ZxType("X"))
        >>> expected.add_node(2, value=stimzx.ZxType("out"))
        >>> expected.add_node(3, value=stimzx.ZxType("in"))
        >>> expected.add_node(4, value=stimzx.ZxType("Z", quarter_turns=2))
        >>> expected.add_node(5, value=stimzx.ZxType("out"))
        >>> _ = expected.add_edge(0, 1)
        >>> _ = expected.add_edge(1, 2)
        >>> _ = expected.add_edge(1, 4)
        >>> _ = expected.add_edge(3, 4)
        >>> _ = expected.add_edge(4, 5)
        >>> networkx.testing.assert_graphs_equal(actual, expected)

    Returns:
        A networkx MultiGraph containing the nodes and edges from the diagram. Nodes are numbered 0, 1, 2, etc in
            reading ordering from the diagram, and have a "value" attribute of type `stimzx.ZxType`.
    """
    return text_diagram_to_networkx_graph(text_diagram, value_func=ZX_TYPES.__getitem__)


def _reduced_zx_graph(graph: Union[nx.Graph, nx.MultiGraph]) -> nx.Graph:
    """Return an equivalent graph without self edges or repeated edges."""
    reduced_graph = nx.Graph()
    odd_parity_edges = set()
    for n1, n2 in graph.edges():
        if n1 == n2:
            continue
        odd_parity_edges ^= {frozenset([n1, n2])}
    for n, value in graph.nodes('value'):
        reduced_graph.add_node(n, value=value)
    for n1, n2 in odd_parity_edges:
        reduced_graph.add_edge(n1, n2)
    return reduced_graph


def zx_graph_to_external_stabilizers(graph: Union[nx.Graph, nx.MultiGraph]) -> List[ExternalStabilizer]:
    """Computes the external stabilizers of a ZX graph; generators of Paulis that leave it unchanged including sign.

    Args:
        graph: A non-contradictory connected ZX graph with nodes annotated by 'type' and optionally by 'angle'.
            Allowed types are 'x', 'z', 'h', and 'out'.
            Allowed angles are multiples of `math.pi/2`. Only 'x' and 'z' node types can have angles.
            'out' nodes must have degree 1.
            'h' nodes must have degree 2.

    Returns:
        A list of canonicalized external stabilizer generators for the graph.
    """

    graph = _reduced_zx_graph(graph)
    sim = stim.TableauSimulator()

    # Interpret each edge as a cup producing an EPR pair.
    # - The qubits of the EPR pair fly away from the center of the edge, towards their respective nodes.
    # - The qubit keyed by (a, b) is the qubit heading towards b from the edge between a and b.
    qubit_ids: Dict[Tuple[Any, Any], int] = {}
    for n1, n2 in graph.edges:
        qubit_ids[(n1, n2)] = len(qubit_ids)
        qubit_ids[(n2, n1)] = len(qubit_ids)
        sim.h(qubit_ids[(n1, n2)])
        sim.cnot(qubit_ids[(n1, n2)], qubit_ids[(n2, n1)])

    # Interpret each internal node as a family of post-selected parity measurements.
    for n, node_type in graph.nodes('value'):
        if node_type.kind in 'XZ':
            # Surround X type node with Hadamards so it can be handled as if it were Z type.
            if node_type.kind == 'X':
                for neighbor in graph.neighbors(n):
                    sim.h(qubit_ids[(neighbor, n)])
        elif node_type.kind == 'H':
            # Hadamard one input so the H node can be handled as if it were Z type.
            neighbor, _ = graph.neighbors(n)
            sim.h(qubit_ids[(neighbor, n)])
        elif node_type.kind in ['out', 'in']:
            continue  # Don't measure qubits leaving the system.
        else:
            raise ValueError(f"Unknown node type {node_type!r}")

        # Handle Z type node.
        # - Postselects the ZZ observable over each pair of incoming qubits.
        # - Postselects the (S**quarter_turns X S**-quarter_turns)XX..X observable over all incoming qubits.
        neighbors = [n2 for n2 in graph.neighbors(n) if n2 != n]
        center = qubit_ids[(neighbors[0], n)]  # Pick one incoming qubit to be the common control for the others.
        # Handle node angle using a phasing operation.
        [id, sim.s, sim.z, sim.s_dag][node_type.quarter_turns](center)
        # Use multi-target CNOT and Hadamard to transform postselected observables into single-qubit Z observables.
        for n2 in neighbors[1:]:
            sim.cnot(center, qubit_ids[(n2, n)])
        sim.h(center)
        # Postselect the observables.
        for n2 in neighbors:
            _pseudo_postselect(sim, qubit_ids[(n2, n)])

    # Find output qubits.
    in_nodes = sorted(n for n, value in graph.nodes('value') if value.kind == 'in')
    out_nodes = sorted(n for n, value in graph.nodes('value') if value.kind == 'out')
    ext_nodes = in_nodes + out_nodes
    out_qubits = []
    for out in ext_nodes:
        (neighbor,) = graph.neighbors(out)
        out_qubits.append(qubit_ids[(neighbor, out)])

    # Remove qubits corresponding to non-external edges.
    for i, q in enumerate(out_qubits):
        sim.swap(q, len(qubit_ids) + i)
    for i, q in enumerate(out_qubits):
        sim.swap(i, len(qubit_ids) + i)
    sim.set_num_qubits(len(out_qubits))

    # Stabilizers of the simulator state are the external stabilizers of the graph.
    dual_stabilizers = sim.canonical_stabilizers()
    return ExternalStabilizer.canonicals_from_duals(dual_stabilizers, len(in_nodes))


def _pseudo_postselect(sim: stim.TableauSimulator, target: int):
    """Pretend to postselect by using classical feedback to consistently get into the measurement-was-false state."""
    measurement_result, kickback = sim.measure_kickback(target)
    if kickback is not None:
        for qubit, pauli in enumerate(kickback):
            feedback_op = [None, sim.cnot, sim.cy, sim.cz][pauli]
            if feedback_op is not None:
                feedback_op(stim.target_rec(-1), qubit)
    assert kickback is not None or not measurement_result, "Impossible postselection. Graph contained a contradiction."
