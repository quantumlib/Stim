import math
from typing import List

import networkx as nx
import stim

from example_zx_graph_stabilizers import zx_graph_stabilizers


def test_cnot():
    graph = nx.Graph()
    graph.add_node(1, type='out')
    graph.add_node(2, type='out')
    graph.add_node(3, type='out')
    graph.add_node(4, type='out')
    graph.add_node(5, type='x')
    graph.add_node(6, type='z')
    graph.add_edge(1, 5)
    graph.add_edge(5, 3)
    graph.add_edge(2, 6)
    graph.add_edge(6, 4)
    graph.add_edge(5, 6)
    assert zx_graph_stabilizers(graph) == external_stabilizers_of_circuit(stim.Circuit("CNOT 1 0"))


def test_cz():
    graph = nx.Graph()
    graph.add_node(1, type='out')
    graph.add_node(2, type='out')
    graph.add_node(3, type='out')
    graph.add_node(4, type='out')
    graph.add_node(5, type='z')
    graph.add_node(6, type='z')
    graph.add_node(7, type='h')
    graph.add_edge(1, 5)
    graph.add_edge(5, 3)
    graph.add_edge(2, 6)
    graph.add_edge(6, 4)
    graph.add_edge(5, 7)
    graph.add_edge(6, 7)
    assert zx_graph_stabilizers(graph) == external_stabilizers_of_circuit(stim.Circuit("CZ 0 1"))


def test_s():
    graph = nx.Graph()
    graph.add_node(1, type='out')
    graph.add_node(2, type='z', angle=math.pi / 2)
    graph.add_node(3, type='out')
    graph.add_edge(1, 2)
    graph.add_edge(2, 3)
    assert zx_graph_stabilizers(graph) == external_stabilizers_of_circuit(stim.Circuit("S 0"))


def test_s_dag():
    graph = nx.Graph()
    graph.add_node(1, type='out')
    graph.add_node(2, type='z', angle=-math.pi / 2)
    graph.add_node(3, type='out')
    graph.add_edge(1, 2)
    graph.add_edge(2, 3)
    assert zx_graph_stabilizers(graph) == external_stabilizers_of_circuit(stim.Circuit("S_DAG 0"))


def test_sqrt_x():
    graph = nx.Graph()
    graph.add_node(1, type='out')
    graph.add_node(2, type='x', angle=math.pi / 2)
    graph.add_node(3, type='out')
    graph.add_edge(1, 2)
    graph.add_edge(2, 3)
    assert zx_graph_stabilizers(graph) == external_stabilizers_of_circuit(stim.Circuit("SQRT_X 0"))


def test_sqrt_x_sqrt_x():
    graph = nx.Graph()
    graph.add_node(1, type='out')
    graph.add_node(2, type='x', angle=math.pi / 2)
    graph.add_node(3, type='x', angle=math.pi / 2)
    graph.add_node(4, type='out')
    graph.add_edge(1, 2)
    graph.add_edge(2, 3)
    graph.add_edge(3, 4)
    assert zx_graph_stabilizers(graph) == external_stabilizers_of_circuit(stim.Circuit("X 0"))


def test_sqrt_z_sqrt_z():
    graph = nx.Graph()
    graph.add_node(1, type='out')
    graph.add_node(2, type='z', angle=math.pi / 2)
    graph.add_node(3, type='z', angle=math.pi / 2)
    graph.add_node(4, type='out')
    graph.add_edge(1, 2)
    graph.add_edge(2, 3)
    graph.add_edge(3, 4)
    assert zx_graph_stabilizers(graph) == external_stabilizers_of_circuit(stim.Circuit("Z 0"))


def test_sqrt_x_dag():
    graph = nx.Graph()
    graph.add_node(1, type='out')
    graph.add_node(2, type='x', angle=-math.pi / 2)
    graph.add_node(3, type='out')
    graph.add_edge(1, 2)
    graph.add_edge(2, 3)
    assert zx_graph_stabilizers(graph) == external_stabilizers_of_circuit(stim.Circuit("SQRT_X_DAG 0"))


def test_x():
    graph = nx.Graph()
    graph.add_node(1, type='out')
    graph.add_node(2, type='x', angle=math.pi)
    graph.add_node(3, type='out')
    graph.add_edge(1, 2)
    graph.add_edge(2, 3)
    assert zx_graph_stabilizers(graph) == external_stabilizers_of_circuit(stim.Circuit("X 0"))


def test_z():
    graph = nx.Graph()
    graph.add_node(1, type='out')
    graph.add_node(2, type='z', angle=math.pi)
    graph.add_node(3, type='out')
    graph.add_edge(1, 2)
    graph.add_edge(2, 3)
    assert zx_graph_stabilizers(graph) == external_stabilizers_of_circuit(stim.Circuit("Z 0"))


def test_id():
    graph = nx.Graph()
    graph.add_node(1, type='out')
    graph.add_node(2, type='x')
    graph.add_node(3, type='z', angle=0)
    graph.add_node(4, type='out')
    graph.add_edge(1, 2)
    graph.add_edge(2, 3)
    graph.add_edge(3, 4)
    assert zx_graph_stabilizers(graph) == external_stabilizers_of_circuit(stim.Circuit("I 0"))


def external_stabilizers_of_circuit(circuit: stim.Circuit) -> List[stim.PauliString]:
    n = circuit.num_qubits
    s = stim.TableauSimulator()
    s.do(circuit)
    t = s.current_inverse_tableau()**-1
    stabilizers = []
    for k in range(n):
        p = [0] * n
        p[k] = 1
        stabilizers.append(stim.PauliString(p) + t.x_output(k))
        p[k] = 3
        stabilizers.append(stim.PauliString(p) + t.z_output(k))
    return stabilizers
