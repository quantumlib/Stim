from typing import Dict, Tuple, List, Any
import math
import stim
import networkx as nx


def zx_graph_stabilizers(graph: nx.Graph) -> List[stim.PauliString]:
    """Computes the external stabilizers of a ZX graph; generators of Paulis that leave it unchanged including sign.

    Args:
        graph: A non-contradictory connected ZX graph with nodes annotated by 'type' and optionally by 'angle'.
            Allowed types are 'x', 'z', 'h', and 'out'.
            Allowed angles are multiples of `math.pi/2`. Only 'x' and 'z' node types can have angles.
            'out' nodes must have degree 1.
            'h' nodes must have degree 2.
    """
    sim = stim.TableauSimulator()

    # Interpret each edge as a cup producing an EPR pair.
    # - The qubits of the EPR pair fly away from the center of the edge, towards their respective nodes.
    # - The qubit keyed by (a, b) is the qubit heading towards b from the edge between a and b.
    qubit_ids: Dict[Tuple[Any, Any], int] = {}
    for n1, n2 in graph.edges:
        if n1 == n2:
            continue  # Ignore self-edges. In a well formed non-zero graph they have no effect.
        qubit_ids[(n1, n2)] = len(qubit_ids)
        qubit_ids[(n2, n1)] = len(qubit_ids)
        sim.h(qubit_ids[(n1, n2)])
        sim.cnot(qubit_ids[(n1, n2)], qubit_ids[(n2, n1)])

    # Interpret each internal node as a family of post-selected parity measurements.
    for n, attributes in graph.nodes.items():
        node_type = attributes['type']

        if node_type in 'xz':
            # Extract angle.
            quarter_turns = attributes.get('angle', 0) / (math.pi / 2)
            assert int(quarter_turns) == quarter_turns, "Not a stabilizer graph."
            quarter_turns = int(quarter_turns) % 4
            # Surround X type node with Hadamards so it can be handled as if it were Z type.
            if node_type == 'x':
                for neighbor in graph.neighbors(n):
                    sim.h(qubit_ids[(neighbor, n)])
        elif node_type == 'h':
            # Hadamard one input so the H node can be handled as if it were Z type.
            neighbor, _ = graph.neighbors(n)
            sim.h(qubit_ids[(neighbor, n)])
            quarter_turns = 0
        elif node_type == 'out':
            continue  # Don't measure qubits leaving the system.
        else:
            raise ValueError(f"Unknown node type {node_type!r}")

        # Handle Z type node.
        # - Postselects the ZZ observable over each pair of incoming qubits.
        # - Postselects the (S**quarter_turns X S**-quarter_turns)XX..X observable over all incoming qubits.
        neighbors = [n2 for n2 in graph.neighbors(n) if n2 != n]
        center = qubit_ids[(neighbors[0], n)]  # Pick one incoming qubit to be the common control for the others.
        # Handle node angle using a phasing operation.
        [id, sim.s, sim.z, sim.s_dag][quarter_turns](center)
        # Use multi-target CNOT and Hadamard to transform postselected observables into single-qubit Z observables.
        for n2 in neighbors[1:]:
            sim.cnot(center, qubit_ids[(n2, n)])
        sim.h(center)
        # Postselect the observables.
        for n2 in neighbors:
            pseudo_postselect(sim, qubit_ids[(n2, n)])

    # Find output qubits.
    out_nodes = sorted(n for n, attributes in graph.nodes.items() if attributes['type'] == 'out')
    out_qubits = []
    for out in out_nodes:
        (neighbor,) = graph.neighbors(out)
        out_qubits.append(qubit_ids[(neighbor, out)])

    # Remove qubits corresponding to non-external edges.
    for i, q in enumerate(out_qubits):
        sim.swap(q, len(qubit_ids) + i)
    for i, q in enumerate(out_qubits):
        sim.swap(i, len(qubit_ids) + i)
    sim.set_num_qubits(len(out_qubits))

    # Stabilizers of the simulator state are the external stabilizers of the graph.
    return sim.canonical_stabilizers()


def pseudo_postselect(sim: stim.TableauSimulator, target: int):
    """Pretend to postselect by using classical feedback to consistently get into the measurement-was-false state."""
    measurement_result, kickback = sim.measure_kickback(target)
    if kickback is not None:
        for qubit, pauli in enumerate(kickback):
            feedback_op = [None, sim.cnot, sim.cy, sim.cz][pauli]
            if feedback_op is not None:
                feedback_op(stim.target_rec(-1), qubit)
    assert kickback is not None or not measurement_result, "Impossible postselection. Graph contained a contradiction."


def main():
    cnot_graph = nx.Graph()
    cnot_graph.add_node(1, type='out')
    cnot_graph.add_node(2, type='out')
    cnot_graph.add_node(3, type='out')
    cnot_graph.add_node(4, type='out')
    cnot_graph.add_node(5, type='x')
    cnot_graph.add_node(6, type='z')
    cnot_graph.add_edge(1, 5)
    cnot_graph.add_edge(5, 3)
    cnot_graph.add_edge(2, 6)
    cnot_graph.add_edge(6, 4)
    cnot_graph.add_edge(5, 6)

    stabilizers = zx_graph_stabilizers(cnot_graph)
    print(f"External stabilizers of cnot graph:")
    for s in stabilizers:
        print("    ", s)


if __name__ == '__main__':
    main()
